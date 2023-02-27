#pragma once

#include "xmessage.h"
#include "xmroundbuf.h"

extern volatile int memsicDataready;
extern volatile int memsicStatus;
extern volatile int memsicSamplingFlag;
extern volatile int memsicDataReadDelay;
extern volatile tstype imuTimeMks;

void setupMemsic();
void setupMemsic_1(); ///< during POWER OFF
void setupMemsic_2(); ///<  after POWER ON

/**
	@param[in] dataRate could be 100 or 200
*/
void setupMemsic_3(int dataRate); ///<  main setup

//void readMemsic();
//void parceMemsic(bool burstMode);
void memsicPrint();
void memsicTest(const char* name = 0);
void memsicStop();

/**
 * call this function periodically in low level priority,
 * to process all the IMU measurements.
 * \param cb callback function, will be called for every IMU measurement
 * */
void processMemsicData(void (*cb)(const xqm::ImuData& imu) = nullptr);
int getMemsicSerialNumber();

//void testLogModeStart();
//void testLogModeStop();

#pragma pack(1)
struct MemsicBurstInfo {
	unsigned short int bm;
	unsigned short int status;
	short int	w[3];
	short int	a[3];
	short int	temp;
};

#pragma pack()

/*
class ImuInfo {
public:
	xqm::ImuData data;
	ImuInfo();
	void setMode(int mode_);
	void feed(const xqm::ImuData& data_);
private:
	int mode;
	XMRoundBuf<xqm::ImuData, 128> info;
};

extern ImuInfo imuInfo;
*/


