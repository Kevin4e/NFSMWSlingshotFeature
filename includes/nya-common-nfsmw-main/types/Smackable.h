class Smackable {
public:
	static inline auto SimplifySort = (bool(*)(Smackable*, Smackable*))0x6709E0;
	static inline auto Construct = (ISimable*(*)(Sim::Param params))0x6895A0;
};
#define SMACKABLE_LIST UTL::Listable<Smackable, 160, 0x9357B8>