class ISteeringWheel : public UCOM::IUnknown {
public:
	//static inline uint32_t IHandle = 0x6E9970;

	enum SteeringType {
		kGamePad = 0,
		kWheelSpeedSensitive = 1,
		kWheelSpeedInsensitive = 2,
	};

	virtual void UpdateForces(IPlayer*);
	virtual void ReadInput(float*);
	virtual bool IsConnected();
	virtual SteeringType GetSteeringType();
};