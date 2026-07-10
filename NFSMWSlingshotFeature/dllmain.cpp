#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define K4MP_NO_MUTEX
#define K4MP_ENABLE_CORE
#define K4MP_ENABLE_BASIC_ASM

#include <Windows.h>
#include <vector>
#include <limits>
#include <string>

#include "../includes/K4MemPatcher/K4MemPatcher.hpp"
#include "../includes/K4IniReader.hpp"
#include "../includes/NFSVersionManager.hpp"

#include "settings.hpp"
#include "addresses.hpp"
#include "utils.hpp"
#include "vehicles.hpp"

using enum NFSVersionManager::GameKey;

// Constants
constexpr float maxDraftGainRate{ 0.003f };
constexpr float draftDecayRate{ 0.005f };
constexpr float infinity{ std::numeric_limits<float>::infinity() };
constexpr float minVehicleSpeed{ 1.0f };
constexpr FloatBounds slingshotForceRange{ 0.04f, 0.07f };
constexpr float AISpeedWeight{ 0.7f };
constexpr float playerSpeedWeight{ 0.3f };

// Global variables
float MinimumSpeedForMaxDraftGain{};
bool speedDependency{};
float MaxLateralDistance{};
float MaxVerticalDistance{};
bool autoLateral{};
bool autoVertical{};
float slingshotBoost{};

K4MemPatcher::StablePtr ptrToSpeedbreaker{ { 0x589228, 0x84 }, true };

std::vector<VehicleInfo> activeAIVehicles{};

/*
 *  Drafting system update routine.
 *
 *  This function is injected into the game's execution flow via a CALL hook and is executed repeatedly during gameplay.
 *
 *  Responsibilities:
 *  - Reads player vehicle position, velocity, and speed from memory.
 *  - Iterates through the game's vehicle table to find active AI vehicles.
 *  - Attempts to filter out non-vehicle objects and stationary entries.
 *  - Determines whether an AI vehicle is ahead of the player and within the configured longitudinal and lateral drafting ranges.
 *  - Calculates drafting accumulation based on proximity to the closest valid AI vehicle.
 *  - Applies a forward velocity boost ("slingshot") along the player's movement direction.
 *  - Gradually decays the stored boost when the player is not drafting.
 *  - Optionally uses the speedbreaker bar as a visual indicator of the accumulated boost.
 *
 *  The function exits early when:
 *  - The player is not in open world mode.
 *  - The player's speed is too low.
 *  - The game is paused.
 *  - No vehicle was found.
 */

void DraftingSystemHook() {
	const bool openWorldFlag{ K4MemPatcher::readMemory<bool>(Game::Flags::OpenWorld, false) };

	if (!openWorldFlag)
		return; // Not in open world

	const VehicleInfo playerVehicle{ getPlayerVehicle() };

	const float playerSpeed{ playerVehicle.currentSpeed };

	if (playerSpeed < minVehicleSpeed) { // Car almost still
		const auto resolvedPtr{ ptrToSpeedbreaker.Resolve() };

		if (UseSpeedbreakerBarAsSlingshotMeter && K4MemPatcher::readMemory<float>(resolvedPtr, false) != 0.0f)
			K4MemPatcher::writeMemory<float>(resolvedPtr, 0.0f, false, false); // Set speedbreaker bar to 0 only if it's not 0

		slingshotBoost = 0.0f;

		return;
	}

	const bool pauseMenuFlag{ K4MemPatcher::readMemory<bool>(Game::Flags::PauseMenu, false) };

	if (pauseMenuFlag)
		return; // In pause menu

	if (UseSpeedbreakerBarAsSlingshotMeter)
		K4MemPatcher::writeMemory<float>(ptrToSpeedbreaker.Resolve(), slingshotBoost, false, false);

	getActiveAIVehicles(activeAIVehicles);

	if (activeAIVehicles.empty())
		return;

	const Size3 playerHalfDim{ playerVehicle.dim * 0.5f };

	const Vec3 playerVelocities
	{
		K4MemPatcher::readMemory<float>(Player::Velocity::X, false),
		K4MemPatcher::readMemory<float>(Player::Velocity::Y, false),
		K4MemPatcher::readMemory<float>(Player::Velocity::Z, false)
	};

	const Vec3 playerNormVelocities{ playerVelocities / playerSpeed };

	float minDistance{ infinity };
	float AISpeed{};

	for (const auto& AIVehicle : activeAIVehicles) {
		const float AICurrentSpeed{ AIVehicle.currentSpeed };

		if (AICurrentSpeed < minVehicleSpeed)
			continue; // AI Vehicle is not moving

		// Get the relative distances from player to AI in longitudinal, lateral, and vertical axes
		const Vec3 relativePos{ playerVehicle.relativePositionTo(AIVehicle) };

		const float longitudinalDistance{ relativePos.x };
		const float lateralDistance{ absolute(relativePos.y) };
		const float verticalDistance{ absolute(relativePos.z) };

		// Skip if AI is behind or too far
		if (longitudinalDistance < 0.0f || longitudinalDistance > MaxLongitudinalDistance)
			continue;

		const Size3& AIDim{ AIVehicle.dim };

		const float lateralDistanceToCompare = autoLateral ?
			AIDim.width + playerHalfDim.width : // AI and player lateral size
			MaxLateralDistance; // From .ini

		const float verticalDistanceToCompare = autoVertical ?
			AIDim.depth + playerHalfDim.depth : // AI and player lateral size
			MaxVerticalDistance; // From .ini

		// Check lateral and vertical bounds
		if (lateralDistance > lateralDistanceToCompare || verticalDistance > verticalDistanceToCompare)
			continue;

		// Keep the nearest AI vehicle
		if (longitudinalDistance < minDistance) {
			minDistance = longitudinalDistance;
			AISpeed = AICurrentSpeed;
		}
	}

	const bool isDrafting{ minDistance != infinity };

	if (isDrafting) { // Nearest AI found within lateral and longitudinal range

		// The closer and the faster (if speed dependency is enabled) it is, the faster the boost grows

		const float boostFromDistance{ mapUnclamped(
			{ 0.0f, MaxLongitudinalDistance },
			{ maxDraftGainRate, 0.0f },
			minDistance
		) };

		float normalizedGain{};

		if (speedDependency) {
			const float boostFromAISpeed{ mapClamped(
				{ 0.0f, MinimumSpeedForMaxDraftGain },
				{ 0.0f, maxDraftGainRate },
				AISpeed
			) };

			const float boostFromPlayerSpeed{ mapClamped(
				{ 0.0f, MinimumSpeedForMaxDraftGain },
				{ 0.0f, maxDraftGainRate },
				playerSpeed
			) };

			const float combinedGain{
				boostFromDistance +
				boostFromAISpeed * AISpeedWeight +
				boostFromPlayerSpeed * playerSpeedWeight
			};

			normalizedGain = combinedGain * 0.5f;
		}
		else
			normalizedGain = boostFromDistance;

		slingshotBoost += normalizedGain * ProgressiveBoostMultiplier;
	}
	else
		slingshotBoost -= draftDecayRate * DecayBoostMultiplier; // Gradually decrease the boost

	clamp(slingshotBoost, { 0.0f, MaxSlingshotBoost }); // Make sure the boost doesn't exceed its bounds

	// Apply the boost correctly along the direction vector
	if (slingshotBoost > 0.0f) {
		if (SlingshotMode != 0 || !isDrafting) { // If post-drafting is chosen, apply boost only after drafting

			/*
			 *  Close to 0.0 -> TOO WEAK
			 *  Higher than 0.1 -> TOO STRONG
			 *  Mapping the slingshot boost accumulated to [0.04, 0.07] is a balanced range
			 */
			const float actualBoost{ mapUnclamped({ 0.0f, MaxSlingshotBoost }, slingshotForceRange, slingshotBoost) };

			const float boostX{ (playerNormVelocities.x * actualBoost) * FinalBoostMultiplier };
			const float boostY{ (playerNormVelocities.y * actualBoost) * FinalBoostMultiplier };
			const float boostZ{ (playerNormVelocities.z * actualBoost) * FinalBoostMultiplier };

			K4MemPatcher::writeMemory<float>(Player::Velocity::X, playerVelocities.x + boostX, false, false);
			K4MemPatcher::writeMemory<float>(Player::Velocity::Y, playerVelocities.y + boostY, false, false);
			K4MemPatcher::writeMemory<float>(Player::Velocity::Z, playerVelocities.z + boostZ, false, false);
		}
	}
}

// Read INI, configure everything, start DraftingSystemHook()
void Setup() {
	const K4IniReader iniReader{ "NFSMWSlingshotFeatureConfig.ini" };

	if (!iniReader) {
		MessageBoxA(
			nullptr,
			"Couldn't open the configuration file for reading.\n"
			"Verify the file exists in the directory the .asi is in.\n\n"
			"File not found: \"NFSMWSlingshotFeatureConfig.ini\".",
			"NFSMW Slingshot Feature by Kevin4e",
			MB_ICONERROR
		);

		return;
	}

	// Main
	{
		Enable = iniReader.read<bool>("Main", "Enable", false);

		if (!Enable)
			return; // Disable script
	}

	// Slingshot attributes
	{
		SlingshotMode = iniReader.read<int>("SlingshotAttributes", "SlingshotMode", 0);

		if (SlingshotMode != 0 && SlingshotMode != 1)
			return; // Invalid slingshot mode

		MaxSlingshotBoost = iniReader.read<float>("SlingshotAttributes", "MaxSlingshotBoost", 1.0f);
		ProgressiveBoostMultiplier = iniReader.read<float>("SlingshotAttributes", "ProgressiveBoostMultiplier", 1.0f);
		FinalBoostMultiplier = iniReader.read<float>("SlingshotAttributes", "FinalBoostMultiplier", 1.0f);
		DecayBoostMultiplier = iniReader.read<float>("SlingshotAttributes", "DecayBoostMultiplier", 1.0f);
		MinimumSpeedForMaxDraftGainStr = iniReader.read<std::string>("SlingshotAttributes", "MinimumSpeedForMaxDraftGain", "300.0");
	}

	// Boost ranges
	{
		MaxLongitudinalDistance = iniReader.read<float>("BoostRanges", "MaxLongitudinalDistance", 45.0f);
		MaxLateralDistanceStr = iniReader.read<std::string>("BoostRanges", "MaxLateralDistance", "AUTO");
		MaxVerticalDistanceStr = iniReader.read<std::string>("BoostRanges", "MaxVerticalDistance", "AUTO");
	}

	// Debug
	{
		UseSpeedbreakerBarAsSlingshotMeter = iniReader.read<bool>("Debug", "UseSpeedbreakerBarAsSlingshotMeter", false);
	}

	speedDependency = MinimumSpeedForMaxDraftGainStr != "OFF";

	if (speedDependency) MinimumSpeedForMaxDraftGain = iniReader.read<float>("SlingshotAttributes", "MinimumSpeedForMaxDraftGain", 300.0f);

	autoLateral = MaxLateralDistanceStr == "AUTO";
	autoVertical = MaxVerticalDistanceStr == "AUTO";

	if (!autoLateral) MaxLateralDistance = iniReader.read<float>("BoostRanges", "MaxLateralDistance", 1.5f);
	if (!autoVertical) MaxVerticalDistance = iniReader.read<float>("BoostRanges", "MaxVerticalDistance", 0.5f);

	// Hook into the game's execution flow
	K4MemPatcher::makeCALL(Game::PerFrameUpdate, DraftingSystemHook, false);
}

// ASI entry point (called by the ASI loader)
extern "C" __declspec(dllexport) void InitializeASI() {
	Setup();
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE, DWORD ul_reason_for_call, LPVOID)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) // If the DLL is being loaded
	{
		if (!NFSVersionManager::is(MostWanted)) // If the DLL has not been injected into Most Wanted v1.3
		{
			MessageBoxA(
				nullptr,
				"This .exe is potentially incompatible.\n"
				"Use Most Wanted v1.3 executable for reliable use.",
				"NFSMW Slingshot Feature by Kevin4e",
				MB_ICONERROR
			);
			
			return FALSE; // Detach DLL
		}
	}

	return TRUE; // Keep DLL attached
}