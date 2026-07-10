// todo this is horrible and inaccurate
class PhysicsObject {
public:
	uintptr_t vtable;
	uint8_t _4[0x28];
	UCOM::Object Object;
};
static_assert(offsetof(PhysicsObject, Object) == 0x2C);

struct BehaviorParams {
	const Sim::Param &fparams;
	PhysicsObject *fowner;
	const UCrc32 &fSig;
	const UCrc32 &fMechanic;
};

class Behavior {
public:
	uint8_t _0[0x2D];
	bool mPaused;
	PhysicsObject* mOwner;
	ISimable* mIOwner;
	const UCrc32 mMechanic;
	const UCrc32 mSignature;
	int mPriority;
	void* mProfile;

	ISimable* GetOwner() {
		return mIOwner;
	}
};
static_assert(offsetof(Behavior, mPaused) == 0x2D);
static_assert(offsetof(Behavior, mOwner) == 0x30);
static_assert(offsetof(Behavior, mPriority) == 0x40);

class VehicleBehavior : public Behavior {
public:
	IVehicle* mVehicle;

	IVehicle* GetVehicle() {
		return mVehicle;
	}
};
static_assert(offsetof(VehicleBehavior, mVehicle) == 0x48);