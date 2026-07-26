#pragma once

#include <cstdint>

namespace Game {
	constexpr std::uintptr_t Base{ 0x00400000 };

	namespace Flags {
		constexpr std::uintptr_t OpenWorld{ 0x0092D884 };
		constexpr std::uintptr_t PauseMenu{ 0x0091CAE4 };
		constexpr std::uintptr_t MomentCamera{ 0x009A3A70 };
	}

	constexpr std::uintptr_t PerFrameUpdate{ 0x00663EE3 };
}