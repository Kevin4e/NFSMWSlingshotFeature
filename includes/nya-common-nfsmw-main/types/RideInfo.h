enum CarRenderUsage : uint32_t {
	CarRenderUsage_Player,
	CarRenderUsage_RemotePlayer,
	CarRenderUsage_AIRacer,
	CarRenderUsage_AICop,
	CarRenderUsage_AITraffic,
	CarRenderUsage_AIHeli,
	carRenderUsage_NISCar,
	CarRenderUsage_Ghost,
	CarRenderUsage_Invalid,
};

class RideInfo {
public:
	uint32_t Type; // +0
	uint8_t _0[0x30C];

	auto Init(int type, CarRenderUsage usage, bool has_dash, bool can_be_vertex_damaged) {
		auto f = (void*(__thiscall*)(RideInfo*, int type, CarRenderUsage usage, bool has_dash, bool can_be_vertex_damaged))0x739A70;
		return f(this, type, usage, has_dash, can_be_vertex_damaged);
	}
	auto SetRandomPaint() { auto f = (void(__thiscall*)(RideInfo*))0x759800; return f(this); }
	auto SetStockParts() { auto f = (void(__thiscall*)(RideInfo*))0x7594A0; return f(this); }
	auto SetRandomParts() { auto f = (void(__thiscall*)(RideInfo*))0x75B220; return f(this); }
};
static_assert(sizeof(RideInfo) == 0x310);