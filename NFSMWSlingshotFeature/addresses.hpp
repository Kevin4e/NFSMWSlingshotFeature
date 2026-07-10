#pragma once

#include <cstdint>

namespace Game {
	constexpr std::uintptr_t Base{ 0x00400000 };

	namespace Flags {
		constexpr std::uintptr_t OpenWorld{ 0x0092D884 };
		constexpr std::uintptr_t PauseMenu{ 0x0091CAE4 };
	}

	constexpr std::uintptr_t PerFrameUpdate{ 0x00663EE3 };
}

namespace Offsets {
	// Velocity
	constexpr std::uint8_t VelX{ 0x20 };
	constexpr std::uint8_t VelY{ 0x24 };
	constexpr std::uint8_t VelZ{ 0x28 };
}

namespace Player {
	constexpr std::uintptr_t Base{ 0x009386C8 };

	namespace Velocity {
		constexpr std::uintptr_t X{ Base + Offsets::VelX };
		constexpr std::uintptr_t Y{ Base + Offsets::VelY };
		constexpr std::uintptr_t Z{ Base + Offsets::VelZ };
	}
}