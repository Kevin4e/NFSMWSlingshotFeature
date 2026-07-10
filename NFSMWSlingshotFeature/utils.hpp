#pragma once

inline float absolute(float value) noexcept {
	return value < 0.0f ? -value : value;
}

// Represents a 3D vector with x, y, z fields
struct Vec3 {
	const float x{};
	const float y{};
	const float z{};

	Vec3 operator-(const Vec3& other) const noexcept {
		return
		{
			x - other.x,
			y - other.y,
			z - other.z
		};
	}

	Vec3 operator/(float divisor) const noexcept {
		const float inv{ 1.0f / divisor };

		return
		{
			x * inv,
			y * inv,
			z * inv
		};
	}

	float dot(const Vec3& other) const noexcept {
		return x * other.x + y * other.y + z * other.z;
	}
};

struct Size3 {
	const float width{};
	const float length{};
	const float depth{};

	Size3 operator*(float multiplier) const noexcept {
		return
		{
			width  * multiplier,
			length * multiplier,
			depth  * multiplier
		};
	}
};

struct VehicleInfo {
	const Vec3 pos{};
	const Size3 dim{};
	const Vec3 forwardVector{ 0.0f, 0.0f, 1.0f };
	const Vec3 rightVector{ 1.0f, 0.0f, 0.0f };
	const Vec3 upVector{ 0.0f, 1.0f, 0.0f };
	const float currentSpeed{};

	Vec3 relativePositionTo(const VehicleInfo& vi) const noexcept {
		const Vec3 delta{ vi.pos - pos };

		return
		{
			delta.dot(forwardVector), // Longitudinal
			delta.dot(rightVector),   // Lateral
			delta.dot(upVector)       // Vertical
		};
	}
};

// Float range struct used for mapping and clamping
struct FloatBounds {
	const float start{};
	const float end{};
};

// Takes a value from one numerical range and converts it to the equivalent value in another range
inline float mapUnclamped(FloatBounds from, FloatBounds to, float val) noexcept {
	const float denom{ from.end - from.start };
	if (denom == 0.0f) return to.start; // Division by zero check

	const float perc{ (val - from.start) / denom };
	return (to.end - to.start) * perc + to.start;
}

// Takes a value from one numerical range and converts it to the equivalent value in another range
// Clamps the value if it exceeds the limit
inline float mapClamped(FloatBounds from, FloatBounds to, float val) noexcept {
	if (val <= from.start) return to.start;
	if (val >= from.end) return to.end;

	return mapUnclamped(from, to, val);
}

// Clamps a value to a specified range
inline void clamp(float& value, FloatBounds r) noexcept {
	if (value < r.start) value = r.start;
	else if (value > r.end) value = r.end;
}