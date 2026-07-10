class cFEng {
public:
	static inline auto& mInstance = *(cFEng**)0x91CADC;

	auto IsPackagePushed(const char* pPackageName) { auto f = (bool(__thiscall*)(cFEng*, const char* pPackageName))0x516B50; return f(this, pPackageName); }
};