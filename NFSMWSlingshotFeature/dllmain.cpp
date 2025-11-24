#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <cstdint>
#include <cmath>
#include <vector>
#include <thread>
#include <limits>

#include "../includes/K4MemPatcher.hpp"
#include "../includes/K4IniReader.hpp"

#include "settings.h"

// AI
//constexpr int maxAIOpenWorld = 100;
constexpr DWORD tableStart = 0x009383B0;
//constexpr DWORD tableEnd = tableStart + (4 * maxAIOpenWorld);

// Coordinates offsets
constexpr uint8_t xCoordPositionOffset = 0x10;
constexpr uint8_t yCoordPositionOffset = 0x14;
constexpr uint8_t zCoordPositionOffset = 0x18;

// Velocity offsets
constexpr uint8_t xVelocityOffset = 0x20;
constexpr uint8_t yVelocityOffset = 0x24;
constexpr uint8_t zVelocityOffset = 0x28;

// Player
constexpr DWORD playerVehicleBase = 0x009386C8;

constexpr DWORD playerVehicleXCoordPosition = playerVehicleBase + xCoordPositionOffset;
constexpr DWORD playerVehicleYCoordPosition = playerVehicleBase + yCoordPositionOffset;
constexpr DWORD playerVehicleZCoordPosition = playerVehicleBase + zCoordPositionOffset;

constexpr DWORD playerVehicleXAxisSpeed = playerVehicleBase + xVelocityOffset;
constexpr DWORD playerVehicleYAxisSpeed = playerVehicleBase + yVelocityOffset;
constexpr DWORD playerVehicleZAxisSpeed = playerVehicleBase + zVelocityOffset;

constexpr DWORD playerVehicleSpeedAddress = 0x009142C8; // Game units

// Others
constexpr DWORD openWorldFlagAddress = 0x0092D884;
constexpr DWORD pauseMenuFlagAddress = 0x0091CAE4;
constexpr DWORD numberOfAISpawnedAddress = 0x0092CDE4;

constexpr float minSpeedToClassifyAsVehicle = 1.0f;

constexpr DWORD delayTimeMs = 10;

struct Vec3 {
    float x;
    float y;
    float z;

    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

// Takes a value from one numerical range and converts it to the equivalent value in another range
constexpr inline float map(double r1_1, float r1_2, float r2_1, float r2_2, float val) noexcept {
    float perc = (val - r1_1) / (r1_2 - r1_1);
    return (r2_2 - r2_1) * perc + r2_1;
}

// Calculates the magnitude (length) of a 3D vector given its vector components
inline float getMagnitude(float v1, float v2, float v3) noexcept {
    return std::sqrt(v1 * v1 + v2 * v2 + v3 * v3);
}

void DraftingSystemThread() {
    const float MaxLongitudinalDistanceSquared = MaxLongitudinalDistance * MaxLongitudinalDistance;
    const float MaxLateralDistanceSquared = MaxLateralDistance * MaxLateralDistance;

    float slingshotBoost{};
    bool isDrafting{};

    DWORD ptrToSpeedbreaker{};

    std::vector<Vec3>activeAIVehiclesPos;
    activeAIVehiclesPos.reserve(100);

    while (true) {
        const bool openWorldFlag = K4MemPatcher::readMemory<bool>(openWorldFlagAddress);
        const bool pauseMenuFlag = K4MemPatcher::readMemory<bool>(pauseMenuFlagAddress);
        const float currentSpeed = K4MemPatcher::readMemory<float>(playerVehicleSpeedAddress);
        bool skip = !openWorldFlag || pauseMenuFlag;

        if (!openWorldFlag) {
            // Skip
            Sleep(delayTimeMs);
            continue;
        }

        if (UseSpeedbreakerBarAsSlingshotMeter)
            ptrToSpeedbreaker = K4MemPatcher::readMemory<DWORD>(0x989228) + 0x84;

        if (currentSpeed < 1.0f) { // If not in open world or car almost still
            slingshotBoost = 0.0f;

            if (UseSpeedbreakerBarAsSlingshotMeter)
                K4MemPatcher::writeMemory<float>(ptrToSpeedbreaker, 0.0f);

            skip = true;
        }
        if (skip || pauseMenuFlag) { // Skip if reset was required or in pause menu
            // Skip
            Sleep(delayTimeMs);
            continue;
        }

        if (UseSpeedbreakerBarAsSlingshotMeter)
            K4MemPatcher::writeMemory<float>(ptrToSpeedbreaker, slingshotBoost);

        //MessageBoxA(nullptr, "we can get that boost", "debug", NULL);

        activeAIVehiclesPos.clear(); // Make sure to clear the vector before filling it

        DWORD tableEnd = tableStart + (4 * K4MemPatcher::readMemory<uint32_t>(numberOfAISpawnedAddress));

        // Get active AI vehicles' positions (except the ones not moving)
        for (DWORD table = tableStart; table <= tableEnd; table += 4) {

            const DWORD vehicleBase = K4MemPatcher::readMemory<DWORD>(table);

            if (!vehicleBase || vehicleBase == playerVehicleBase)
                continue; // Skip if vehicle base is null or it's the player's

            const float AIVehicleXCoord = K4MemPatcher::readMemory<float>(vehicleBase + xCoordPositionOffset);
            const float AIVehicleYCoord = K4MemPatcher::readMemory<float>(vehicleBase + yCoordPositionOffset);
            const float AIVehicleZCoord = K4MemPatcher::readMemory<float>(vehicleBase + zCoordPositionOffset);

            const float AIVehicleXVel = K4MemPatcher::readMemory<float>(vehicleBase + xVelocityOffset);
            const float AIVehicleYVel = K4MemPatcher::readMemory<float>(vehicleBase + yVelocityOffset);
            const float AIVehicleZVel = K4MemPatcher::readMemory<float>(vehicleBase + zVelocityOffset);

            /*
             *  NOTE ON AI FILTERING:
             *
             *  The game doesn't have a reliable address for the number of active AI vehicles (at least none found during my research)
             *
             *  'numberOfAISpawnedAddress' cannot be trusted:
             *  The game's table starting at 0x9383B0 lists AI cars and world objects in random order,
             *  and it's only possible to know how many AI cars have spawned during the open world session (it never decreases).
             *  So it's easy to miss the last vehicles generated (which are at the end of the table)
             *
             *  The current way to skip non-AI entries is checking the coordinates and velocities, but it's not enough (e.g. a crashed traffic light can have a velocity)
             *
             *  If someone discovers a reliable method to filter AI cars from the world objects, contributions are welcome.
             */
            if
            (
                (std::fabs(AIVehicleXCoord) < 1e-3 && std::fabs(AIVehicleYCoord) < 1e-3 && std::fabs(AIVehicleZCoord) < 1e-3) ||
                (std::fabs(AIVehicleXVel) < minSpeedToClassifyAsVehicle && std::fabs(AIVehicleYVel) < minSpeedToClassifyAsVehicle && std::fabs(AIVehicleZVel) < minSpeedToClassifyAsVehicle)
            )
            {
                tableEnd += 4;
                continue; // Skip this entry if coordinates or velocities are close to 0
            }

            activeAIVehiclesPos.emplace_back
            (
                AIVehicleXCoord,
                AIVehicleYCoord,
                AIVehicleZCoord
            );
        }

        const float coordX = K4MemPatcher::readMemory<float>(playerVehicleXCoordPosition);
        const float coordY = K4MemPatcher::readMemory<float>(playerVehicleYCoordPosition);
        const float coordZ = K4MemPatcher::readMemory<float>(playerVehicleZCoordPosition);

        const float velX = K4MemPatcher::readMemory<float>(playerVehicleXAxisSpeed);
        const float velY = K4MemPatcher::readMemory<float>(playerVehicleYAxisSpeed);
        const float velZ = K4MemPatcher::readMemory<float>(playerVehicleZAxisSpeed);

        const float normVelX = velX / currentSpeed;
        const float normVelY = velY / currentSpeed;
        const float normVelZ = velZ / currentSpeed;

        float minDistance = std::numeric_limits<float>::max();

        for (const auto& aiPos : activeAIVehiclesPos) {
            Vec3 playerToAI
            (
                aiPos.x - coordX,
                aiPos.y - coordY,
                aiPos.z - coordZ
            );

            // Calculate the dot Product to check if AI is ahead
            const float dot = playerToAI.x * normVelX + playerToAI.y * normVelY + playerToAI.z * normVelZ;

            if (dot < 0 || dot > MaxLongitudinalDistance)
                continue; // Skip if AI is behind the player or it's too far

            /*
             *  NOTE ON TOTAL DISTANCE:
             *
             *  The total distance is measured from the vehicle's center point, not from its rear.
             *  Therefore, the player's vehicle must be very close to vehicles with trailers
             */
             // Calculate the total distance
            const float totalDistance = getMagnitude(playerToAI.x, playerToAI.y, playerToAI.z);

            // Get the longitudinal distance
            const float longitudinalDistance = dot;

            // Calculate the lateral distance (using the Pythagorean theorem)
            const float lateralDistanceSquared = totalDistance * totalDistance - longitudinalDistance * longitudinalDistance;

            /*
            // Get the depth distance
            const float depthDistance = std::fabs(playerToAI.y);
            */

            // Apply both checks
            if (longitudinalDistance <= MaxLongitudinalDistanceSquared && lateralDistanceSquared <= MaxLateralDistanceSquared) {
                if (totalDistance < minDistance)
                    minDistance = totalDistance;
            }
        }

        const bool isMinimumDistanceValid = minDistance != std::numeric_limits<float>::max();

        if (isMinimumDistanceValid) { // Nearest AI found within lateral and longitudinal range
            isDrafting = true;

            if (slingshotBoost < MaxSlingshotBoost) {
                // The closer it is, the faster the boost grows
                slingshotBoost += map(0, MaxLongitudinalDistance, 0.002f, 0, minDistance) * ProgressiveBoostMultiplier;

                if (slingshotBoost > MaxSlingshotBoost) slingshotBoost = MaxSlingshotBoost; // Set to maximum boost if it goes beyond
            }
            else
                slingshotBoost = MaxSlingshotBoost; // Cap the boost
        }
        else {
            isDrafting = false;

            if (slingshotBoost > 0.0f) {
                slingshotBoost -= 0.0025f; // Gradually decrease the boost

                if (slingshotBoost < 0.0f) slingshotBoost = 0.0f; // Reset to 0 if it goes below
            }
            else
                slingshotBoost = 0.0f; // Cap the boost
        }

        // Apply the boost correctly along the direction vector
        if (slingshotBoost > 0.0f) {
            if (SlingshotMode != 0 || !isDrafting) { // If post-drafting is chosen, apply boost only after drafting

                /*
                 *  Close to 0.0 -> TOO WEAK
                 *  Close to 1.0 -> TOO STRONG
                 *  Mapping the slingshot boost accumulated to [0.25, 0.55] is a balanced range
                 */
                const float actualBoost = map(0.0f, MaxSlingshotBoost, 0.25f, 0.55f, slingshotBoost);

                const float boostX = ((normVelX * actualBoost) / 17.0f) * FinalBoostMultiplier;
                const float boostY = ((normVelY * actualBoost) / 17.0f) * FinalBoostMultiplier;
                const float boostZ = ((normVelZ * actualBoost) / 17.0f) * FinalBoostMultiplier;

                K4MemPatcher::writeMemory<float>(playerVehicleXAxisSpeed, velX + boostX);
                K4MemPatcher::writeMemory<float>(playerVehicleYAxisSpeed, velY + boostY);
                K4MemPatcher::writeMemory<float>(playerVehicleZAxisSpeed, velZ + boostZ);
            }
        }

        Sleep(delayTimeMs);
    }
}

// Read INI, configure everything, start DraftingSystemThread()
void Setup() {
    K4IniReader iniReader("NFSMWSlingshotFeatureConfig.ini");

    // Main
    {
        Enable = iniReader.read<bool>("Main", "Enable", false);

        if (!Enable)
            return;
    }

    // Slingshot attributes
    {
        SlingshotMode = iniReader.read<int>("SlingshotAttributes", "SlingshotMode", 0);

        if (SlingshotMode != 0 && SlingshotMode != 1)
            return;

        MaxSlingshotBoost = iniReader.read<float>("SlingshotAttributes", "MaxSlingshotBoost", 1.0f);
        ProgressiveBoostMultiplier = iniReader.read<float>("SlingshotAttributes", "ProgressiveBoostMultiplier", 1.0f);
        FinalBoostMultiplier = iniReader.read<float>("SlingshotAttributes", "FinalBoostMultiplier", 1.0f);
    }

    // Boost ranges
    {
        MaxLongitudinalDistance = iniReader.read<float>("BoostRanges", "MaxLongitudinalDistance", 35.0f);
        MaxLateralDistance = iniReader.read<float>("BoostRanges", "MaxLateralDistance", 1.5f);
    }

    // Debug
    {
        UseSpeedbreakerBarAsSlingshotMeter = iniReader.read<bool>("Debug", "UseSpeedbreakerBarAsSlingshotMeter", false);

        /*
         *  NOPping out the writes at the speedbreaker causes flickering wheels
         *
        if (UseSpeedbreakerBarAsSlingshotMeter) {
            // Cancel out every write made at the speedbreaker
            K4MemPatcher::makeNOP(0x6EDE03, 6);
            K4MemPatcher::makeNOP(0x6F8F9F, 3);
            K4MemPatcher::makeNOP(0x6E9B36, 3);
        }
         */
    }

    std::thread(DraftingSystemThread).detach();
}

extern "C" __declspec(dllexport) void InitializeASI() {
    Setup();
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) {
    // Check if .exe file is compatible - Thanks to thelink2012 and MWisBest
    // A few tweaks and simplified condition for clarity; logic unchanged, there were a few redundant operations

    IMAGE_NT_HEADERS* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(0x400108);

    if (nt->OptionalHeader.AddressOfEntryPoint == 0x3C4040)
        return TRUE;
    else {
        MessageBoxA(nullptr, "This .exe is not supported.\nPlease use v1.3 speed.exe (5.75 MB (6.029.312 bytes)).", "NFSMW Slingshot Feature by Kevin4e", MB_ICONERROR);
        return FALSE;
    }
}