#pragma once

#include <cstdint>
#include <vector>

#include "utils.hpp"

#include "../includes/nya-common-nfsmw-main/nfsmw.h"

// Extracts position, dimension, forward vector and current speed from an active vehicle.
inline VehicleInfo getIVehicleInfo(IVehicle* const IVehicle) noexcept {
	IRigidBody* const rigidBody{ IVehicle->GetSimable()->GetRigidBody() };

	const UMath::Vector3 pos{ *(rigidBody->GetPosition()) };

	UMath::Vector3 dim{};
	rigidBody->GetDimension(&dim);

	UMath::Vector3 forwardVec{};
	rigidBody->GetForwardVector(&forwardVec);

	UMath::Vector3 rightVec{};
	rigidBody->GetRightVector(&rightVec);

	UMath::Vector3 upVec{};
	rigidBody->GetUpVector(&upVec);

	const float vehicleSpeed{ IVehicle->GetSpeed() };

	return
	{
		{ pos.x, pos.y, pos.z }, // Vec3
		{ dim.x, dim.y, dim.z }, // Size3
		{ forwardVec.x, forwardVec.y, forwardVec.z }, // Vec3
		{ rightVec.x, rightVec.y, rightVec.z }, // Vec3
		{ upVec.x, upVec.y, upVec.z }, // Vec3
		vehicleSpeed
	};
}

// Returns the player's vehicle info.
inline VehicleInfo getPlayerVehicle() noexcept {
	return getIVehicleInfo(VEHICLE_LIST::GetList(VEHICLE_PLAYERS).mBegin[0]); // Get the first player vehicle
}

// Collects all active AI vehicles in the game and extracts their info.
inline void getActiveAIVehicles(std::vector<VehicleInfo>& outList) {
	outList.clear();

	const auto& vehicleListStruct{ VEHICLE_LIST::GetList(VEHICLE_AI) }; // Metadata about vehicle list

	const std::uint32_t vehicleCount{ vehicleListStruct.mSize }; // Current number of vehicles
	IVehicle** const vehicleList{ vehicleListStruct.mBegin }; // Vehicles array

	outList.reserve(vehicleCount);

	// Vehicles and trailers
	for (std::uint32_t i{ 0u }; i < vehicleCount; ++i) {
		IVehicle* const IVehicle{ vehicleList[i] };

		if (!IVehicle->IsActive())
			continue; // Vehicle doesn't exist anymore

		outList.push_back(getIVehicleInfo(IVehicle)); // Add vehicle to vector
	}
}