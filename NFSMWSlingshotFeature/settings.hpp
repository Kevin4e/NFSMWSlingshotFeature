#pragma once

#include <string>

// Main
inline bool Enable;

// Slingshot attributes
inline int SlingshotMode;
inline float MaxSlingshotBoost;
inline float ProgressiveBoostMultiplier;
inline float FinalBoostMultiplier;
inline float DecayBoostMultiplier;
inline std::string MinimumSpeedForMaxDraftGainStr;

// Boost ranges
inline float MaxLongitudinalDistance;
inline std::string MaxLateralDistanceStr;
inline std::string MaxVerticalDistanceStr;

// Debug
inline bool UseSpeedbreakerBarAsSlingshotMeter;