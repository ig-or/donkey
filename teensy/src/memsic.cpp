

#include "memsic.h"
#include <string.h>
//include <TimeLib.h>
#include <SPI.h>  // include the memsicSPI library:

//#include "xstdef.h"
#include "xmessage.h"
#include "xmatrix2.h"
#include "xmfilter.h"
#include "xmutils.h"
//#include "files.h"
//#include "processor.h"
#include "xmroundbuf.h"
#include "ttpins.h"
#include "teetools.h"
//#include "tinker_wifi.h"
#include "EventResponder.h"
//#ifdef THREAD_LIB
//#include "TeensyThreads.h"
//#endif

//#ifdef ENABLE_PIANO
//#include "piano.h"

//extern Piano piano;
//#endif

#define CLOCK_1MHz    (1000000)
#define CLOCK_2MHz    (2000000)
#define CLOCK_250KHz    (250000)
#define CLOCK_100KHz    (100000)
#define CLOCK_25KHz		(25000)
#define CLOCK_500KHz    (500000)

// FIXME: do the recalibration of the biases and scale factors for every unit
#define G (9.81192f) // for the units that are calibrated in San Jose, the local field is 9.81192

static const int accRange = 8;   ///<  +- 8G TODO: consider using lower range, like 4G
static XMType accScale = G / 4096.0f; ///<  this will change during the setup
static const XMType gyroScale = pii / (200.0f * 180.0f); ///    assume gyro range is 200 degrees/second maximum
static int memsicDataRate = 200;
static int16_t memsicSerialNumber = 0;

volatile int memsicDataready = 0;
volatile int memsicSamplingFlag = 0;
volatile int memsicStatus = -1;  //   not init yet
volatile int memsicOverflowCounter = 0;

int memsicDataReadTime = 0; ///< data read timestamp

long memsicReadCounter = 0;
volatile unsigned long memsicIntCounter = 0;
volatile unsigned long memsicInt1Counter = 0;
volatile unsigned long memsicSamplingCounter = 0;
XMType memsicBoardTempF;
static int testLogCounter = 0;

MemsicBurstInfo	bInfoIn, bInfoOut , bInfoCopy;
static EventResponder memsicResponder;
int memsicEventTime = 0, memsicEventTime1 = 0;
//ImuInfo imuInfo;

struct MemsicInfo {
	unsigned short int status;
	short int	w[3];
	short int	a[3];
	short int	temp;
	int timestamp;
};
static MemsicInfo memsicInfo;
static xqm::ImuData  pInfo;
static XMRoundBuf<MemsicInfo, 30> hInfoRB1;		//    should be OK for 50 ms


// set up the speed, mode and endianness of MEMSIC IMU device
//SPISettings settingsMEMSIC(195000, MSBFIRST, SPI_MODE3);
 SPISettings settingsMEMSIC(CLOCK_250KHz, MSBFIRST, SPI_MODE3);

 void setupMemsicDataRate(int dataRate);
 void parceMemsicInt(const MemsicBurstInfo& mbi);
 

void erFunction(EventResponderRef er) {
	digitalWriteFast(memsicSPISelectPin, HIGH);
	memsicEventTime1 = millis();
	//if ((memsicInt1Counter != 0) && ((memsicEventTime1 - memsicEventTime) > 16)) {
		//xm_printf("memsic ER: dt = %d\n", (memsicEventTime1 - memsicEventTime));
	//}
	memsicEventTime = memsicEventTime1;


	memsicInt1Counter++;
	// memsicSPI.endTransaction();

	//mxat(memsicDataready == 1);
	//if (memsicDataready != 1) {
		//xm_printf("\tmemsicDataready = %d; memsicDataReadyDelay=%d; micros() = %d; imuTimeMks = %d; imuSamplingTimeMks = %d\n",
			//memsicDataready, memsicDataReadyDelay, micros(), imuTimeMks, imuSamplingTimeMks);
	//}
	//memcpy(&bInfoCopy, &bInfoOut, sizeof(MemsicBurstInfo));
	parceMemsicInt(bInfoOut);

	memsicDataready = 2;
}

//verify if IMU data is ready to be read
extern volatile unsigned long mainCycleCounter;
volatile unsigned int dataRcounter = 0;
void dataR() {
	++dataRcounter;
	//int imuTime1;
	switch (memsicStatus) {
		case -1: 
			break;
		case 0:
			memsicStatus = 1;
			memsicInt1Counter = 0;
			memsicIntCounter = 0;
			break;
		case 1: 
			break;
		case 2: //     normal work mode
			memsicDataReadTime = millis();
			
			//imuTimeMks = micros();
			//mImuTime = imuTime1 

			memsicDataready = 1;
			memsicIntCounter++;
			
			digitalWriteFast(memsicSPISelectPin, LOW);
			memsicSPI.transfer(&bInfoIn, &bInfoOut, sizeof(MemsicBurstInfo), memsicResponder); // ????
		break;
		default:
			break;
	};
}

void memsicDataSampling() {
	memsicDataready = 3;
	//imuSamplingTime = millis();
	//imuSamplingTimeMks = micros();
	memsicSamplingFlag = 1;
	memsicSamplingCounter++;
}

void memsicPrint() {
	xmprintf(0, "MEMSIC: temp = %.1f [deg]; memsicStatus=%d; bStatus = %d; dc = %d; dc1 = %d; \r\nintCounter=%d, memsicInt1Counter=%d; readCounter=%d, memsicSamplingCounter = %d\r\n",
		memsicBoardTempF,
		memsicStatus, bInfoCopy.status,
		memsicIntCounter - memsicReadCounter,
		memsicIntCounter - memsicInt1Counter,
		
		memsicIntCounter, memsicInt1Counter, memsicReadCounter, memsicSamplingCounter);

	xmprintf(0, " \ta=[%f  %f  %f], w=[%f  %f  %f]; memsicOverflowCounter = %d \r\n",
		pInfo.a[0], pInfo.a[1], pInfo.a[2],
		pInfo.w[0], pInfo.w[1], pInfo.w[2],
			  memsicOverflowCounter);
}


void writeMemsicCmd(int16_t cmd) {
	int16_t value;
	delay(1);
	digitalWrite(memsicSPISelectPin, LOW);
	//delay(1);
	value = memsicSPI.transfer16(cmd);
	//delay(1);
	digitalWrite(memsicSPISelectPin, HIGH);
	//delay(1);
}


int16_t memsicReadReg(int16_t reg) {
	int16_t value;
	
	delay(1);
	digitalWrite(memsicSPISelectPin, LOW);
	//delay(1);
	value = memsicSPI.transfer16(reg);
	//delay(1);
	digitalWrite(memsicSPISelectPin, HIGH);
	delay(1);

	digitalWrite(memsicSPISelectPin, LOW);
	//delay(1);
	value = memsicSPI.transfer16(0);
	//delay(1);
	digitalWrite(memsicSPISelectPin, HIGH);
	//delay(1);

	return value;
}

int getMemsicSerialNumber() {
	return memsicSerialNumber;
}

void memsicTest(const char* name) {
	int16_t value;
	//digitalWrite(memsicSPISelectPin, LOW);
	xmprintf(0, "====memsicTest==== %s \r\n", (name == 0) ? " " : name);

	int oldMemsicStatus = memsicStatus;
	memsicStatus = -1;
	int counter = 250;
	while ((counter != 0) && (memsicDataready == 1)) { // wait spi sending to complete
		delay(1);
		counter--;
	}
	setupMemsicDataRate(0); delay(20);
	setupMemsicDataRate(0); delay(20);
	setupMemsicDataRate(0); delay(20);
	setupMemsicDataRate(0); delay(20);

	memsicSerialNumber = memsicReadReg(0x5800);
	int16_t hw_sw_version = memsicReadReg(0x7E00);
	int16_t productID = memsicReadReg(0x5600);
	int16_t unit_code = memsicReadReg(0x5400);
	int16_t manuf_code = memsicReadReg(0x5200);


	unsigned short int outputDataRateReg = memsicReadReg(0x3600);

	//   self test:
	writeMemsicCmd(0xB504);  //    initiate the self test
	uint32_t selfTestTime = micros(), stTime, stCounter = 0, stCounter2 = 0;
	bool stComplete = false;
	uint32_t t1, t2 = 0;

	//   whait while self test bit goes high:
	do {
		int16_t memsicSTReg = memsicReadReg(0x3400);
		if (memsicSTReg & 0x400) { // Unit self-test bit (bit reset upon completion of self-test)
			stTime = micros();
			stComplete = true;
			break;
		}
		t1 = micros();
		while ((micros() - t1) > 2) { t2++; } //  small delay
		stCounter2++;
		stTime = micros();
	} while ((stTime - selfTestTime) < 250000);
	if (!stComplete) {
		xmprintf(0,"WARNING: memsic self test bit high timeout\n");
		return;
	}

	//   whait while self test bit goes low
	stComplete = false;
	do {
		//threads.delay(1);
		t1 = micros();
		while ((micros() - t1) > 2) { t2++; } //  small delay
		int16_t memsicSTReg = memsicReadReg(0x3400);
		if ((memsicSTReg & 0x400) == 0) { // Unit self-test bit (bit reset upon completion of self-test)
			stTime = micros();
			stComplete = true;
			break;
		}
		stCounter++;
		stTime = micros();
	} while ((stTime - selfTestTime) < 500000);
	if (stComplete) {
		xmprintf(0,"\tmemsic self test complete in %d mks; stCounter = %d stCounter2 = %d \r\n", stTime - selfTestTime, stCounter, stCounter2);
	} else {
		xmprintf(0,"\tmemsic self test timeout \n");
	}

	 //    now lets read out memsicStatusReg
	int16_t memsicStatusReg = memsicReadReg(0x3C00);

	//digitalWrite(memsicSPISelectPin, HIGH);
	//memsicSPI.endTransaction();

	xmprintf(0,"\tproductID = %d;  memsicSerialNumber = %d; memsicStatusReg = %d; Hardware Version =%d; Software Version = %d\r\n",
		productID, memsicSerialNumber, memsicStatusReg, (hw_sw_version >> 8), (hw_sw_version & 0xFF));
	xmprintf(0,"\tunit_code = %d; manuf_code = %d \r\n", unit_code, manuf_code);


	unsigned short int odr = (outputDataRateReg & 0xF00) >> 8;
	int dataRate[11] = { 0, 200, 100, 50, 25, 20, 10, 5, 4, 2, 1 };
	if (odr < 11) {
		char s[64];
		xmprintf(0,"\toutput data rate: %d Hz (reg = %d)\n", dataRate[odr], outputDataRateReg);
	} else {
		xmprintf(0,"\tstrange output data rate: %d\n", odr);
	}

	memsicStatus = oldMemsicStatus;
	setupMemsicDataRate(memsicDataRate);
}

void setupMemsic_1() {
	memsicStatus = -1;
	memsicDataready = 0;
	hInfoRB1.clear();
	memsicResponder.attachInterrupt(erFunction); // attachImmediate // attachInterrupt
	memsicInfo.status = 100;  //  not valid

	pinMode( memsicDataReadyPin, OUTPUT );  
	pinMode( memsicSPISelectPin, OUTPUT );  
	//pinMode(memsicResetPin, OUTPUT);
	//pinMode(memsicSyncInputPin, OUTPUT);   //   memsic memsicSPI input freq generation
	//pinMode(memsicSamplingPin, INPUT);

	// set the memsic data ready as an output, and set high so that MEMSIC configures for memsicSPI at power on
	digitalWrite(memsicDataReadyPin, HIGH);

	// set the slaveSelectPin as an output and make sure is NOT enabled
	digitalWrite(memsicSPISelectPin, HIGH);

	//digitalWrite(memsicResetPin, LOW); // Hold the external reset line (pin 8) low while applying power to the unit
}

void setupMemsic_2() {
	memsicStatus = 0;
	//digitalWrite(memsicResetPin, HIGH); // When ready to configure the unit and receive data, set the reset line high to release the unit from hold
}

void memsicStop() {
	memsicStatus = -1;
	memsicResponder.detach();
	detachInterrupt(digitalPinToInterrupt(memsicDataReadyPin));
	
	//digitalWrite(memsicResetPin, LOW);
	digitalWrite(imuPwrPin, HIGH);
	digitalWriteFast(memsicSPISelectPin, HIGH);
	memsicDataready = 0;
	hInfoRB1.clear();
}

void setupMemsicDataRate(int dataRate) {
	switch (dataRate) {
		case 0:
			writeMemsicCmd(0xB700);    //  disable data output
			break;
		case 25:
			writeMemsicCmd(0xB704);    //  set data output rate to 25 Hz:
			break;
		case 100:
			writeMemsicCmd(0xB702);  //  set data output rate to 100 Hz:
			break;
		case 200:
		default:
			writeMemsicCmd(0xB701);  //  set data output rate to 200 Hz:
			break;
	};
}

void setupMemsic_3(int dataRate) {
	int16_t value;
	memsicDataRate = dataRate;
	memsicOverflowCounter = 0;
	xmprintf(0, "Starting Up MEMSIC IMU....  ");
	pinMode( memsicDataReadyPin, INPUT); 
	//bInfoIn.bm = 0x3E00;
	bInfoIn.bm = 0x003E; //  because MSB first not working in the burst mode
	//setup interrupt on pin 7 to indicate data is ready
	attachInterrupt(digitalPinToInterrupt(memsicDataReadyPin), dataR, RISING);

	delay(10);

	//  wait for 'data ready':
	tstype t11 = millis(), dt = 99999;
	while ((memsicStatus == 0) && ((dt = (millis() - t11)) < 1850)) {
		delay(1);
		yield();
	}
	if (memsicStatus == 0) {
		xmprintf(0, " .. \tMEMSIC IMU setup timeout\n");
		return;
	}

	xmprintf(4, " [%d]   .. started! (in %d ms); dataRcounter=%u; memsicStatus=%d\r\n", 
		memsicStatus, dt, dataRcounter, memsicStatus);
   
	// initialize memsicSPI:
	memsicSPI.begin();
	//memsicSPI.usingInterrupt(digitalPinToInterrupt(memsicDataReadyPin));
	//memsicSPI.setCS(memsicSPISelectPin);
	  
	memsicSPI.beginTransaction(settingsMEMSIC);

	//memsicTest("(before config)");

	// take the SS pin low to select the chip:
   // digitalWrite( memsicSPISelectPin, LOW );

	setupMemsicDataRate(dataRate);
	
	//
	//

	//   set angle rate params:
	writeMemsicCmd(0xB902);  // 0x02 (2): +/-125.0°/sec (default) - 200 LSB/( °/sec )


	writeMemsicCmd(0xB830); // 0x30 (48): 50 Hz Butterworth
	//value = memsicSPI.transfer16(0xB850); // 0x50 (80): 10 Hz Butterworth
	//value = memsicSPI.transfer16(0xB806); // 0x06 (6): 5 Hz Bartlett (default)
	//writeMemsicCmd(0xB805); // 0x05 (5): 10 Hz Bartlett
	//value = memsicSPI.transfer16(0xB840); //  0x40 (64): 20 Hz Butterworth
	//value = memsicSPI.transfer16(0xB800); // 0x00 (0): Unfiltered
	
	//  accelerometers scale:
	switch (accRange) {
		case 2:
			writeMemsicCmd(0xF010); // Accelerometer Scaling/Dynamic Range Selector  0x1: +/-2.0 [g] 16384 LSB/[g] 	+/ -1.96[g]  	+ / -1.8[g]
			accScale = G / 16384.0;
			break;
		case 4:
			writeMemsicCmd(0xF020); // Accelerometer Scaling/Dynamic Range Selector  0x2: +/-4.0 [g]; 8192 LSB/[g]
			accScale = G / 8192.0;
			break;
		case 8:
		default:
			writeMemsicCmd(0xF040); // Accelerometer Scaling/Dynamic Range Selector  0x4: +/-8.0 [g];   +/-8.0 [g]	4096 LSB / [g]	+ / -7.84[g]+ / -7.2[g]
			accScale = G / 4096.0f;
			break;
	};
	//
	//
	

	writeMemsicCmd(0xB406); // To enable data-ready with a high signal
	
	//digitalWrite ( memsicSPISelectPin, HIGH );
	//memsicSPI.endTransaction();
	
	for (int i = 0; i < 3; i++) {
		pInfo.a[i] = 0.0f; //lastInfo1.a[i] = 0.0f; lastInfo2.a[i] = 0.0f; 
		pInfo.w[i] = 0.0f; //lastInfo1.w[i] = 0.0f; lastInfo2.w[i] = 0.0f; 
	}
	pInfo.status = 1; pInfo.timestamp = 0;

	memsicTest("(after config)");
	
	//attachInterrupt(digitalPinToInterrupt(memsicSamplingPin), memsicDataSampling, FALLING);

	//FrequencyTimer2::setPeriod(1000);
	//FrequencyTimer2::enable();
	hInfoRB1.clear();
	memsicStatus = 2; // ?
	
	//xm_printf("Memsic setup finished\n");
}

/*
void testLogModeStart() {
	//int16_t value;
	//value = memsicSPI.transfer16(0xB800); // 0x00 (0): Unfiltered
	testLogCounter = 500;
	sdLogStart();
#ifdef ENABLE_PIANO
	piano.play(Piano::F);
#endif
}

void testLogModeStop() {
	int16_t value;
	digitalWriteFast(memsicSPISelectPin, LOW);
	value = memsicSPI.transfer16(0xB805); // 0x05 (5): 10 Hz Bartlett
	digitalWriteFast(memsicSPISelectPin, HIGH);
	testLogCounter = 1000;
#ifdef ENABLE_PIANO
	piano.play(Piano::F);
#endif
}
*/

void parceMemsicInt(const MemsicBurstInfo& mbi) {
	int i;

	//memcpy(&lastInfo2, &lastInfo1, sizeof(xqm::ImuData));
	//memcpy(&lastInfo1, &memsicImuInfo, sizeof(xqm::ImuData));

	if (hInfoRB1.full()) {
		++memsicOverflowCounter;
	} else {
		for (i = 0; i < 3; i++) {
			tswap(mbi.a[i]);
			tswap(mbi.w[i]);

			memsicInfo.a[i] = mbi.a[i];
			memsicInfo.w[i] = mbi.w[i];

			//memsicImuInfo.a[i] = ((XMType)(mbi.a[i])) * accScale;
			//memsicImuInfo.w[i] = ((XMType)(mbi.w[i])) * gyroScale;
		}
		tswap(mbi.temp);
		tswap(mbi.status);

		memsicInfo.temp = mbi.temp;
		memsicInfo.status = mbi.status;
		memsicInfo.timestamp = memsicDataReadTime;


		//memsicImuInfo.status = (unsigned char)(mbi.status);
		//memsicBoardTempF = (((XMType)(mbi.temp)) * 0.07311f) + 31.0f;
		//memsicImuInfo.timestamp = memsicDataReadTime;
		memsicReadCounter++;

		// median filtering:
		/*
		for (int i = 0; i < 3; i++) {
		filteredInfo.a[i] = med3(memsicImuInfo.a[i], lastInfo1.a[i], lastInfo2.a[i]);
		filteredInfo.w[i] = med3(memsicImuInfo.w[i], lastInfo1.w[i], lastInfo2.w[i]);
		}
		*/
		hInfoRB1.put(memsicInfo);
	}
}


#if 0
void parceMemsic(bool burstMode) {
	int i;

	//memcpy(&lastInfo2, &lastInfo1, sizeof(xqm::ImuData));
	//memcpy(&lastInfo1, &memsicImuInfo, sizeof(xqm::ImuData));
	for (i = 0; i < 3; i++) {
		if (burstMode) {
			tswap(bInfoCopy.a[i]);
			tswap(bInfoCopy.w[i]);
		}

		memsicImuInfo.a[i] = ((XMType)(bInfoCopy.a[i])) * accScale;
		memsicImuInfo.w[i] = ((XMType)(bInfoCopy.w[i])) * gyroScale;
	}
	if (burstMode) {
		tswap(bInfoCopy.temp);
		tswap(bInfoCopy.status);
	}

	memsicImuInfo.status = (unsigned char)(bInfoCopy.status);
	memsicBoardTempF = (((XMType)(bInfoCopy.temp)) * 0.07311f) + 31.0f;
	memsicImuInfo.timestamp = memsicDataReadTime;
	memsicReadCounter++;

	// median filtering:
	/*
	for (int i = 0; i < 3; i++) {
	filteredInfo.a[i] = med3(memsicImuInfo.a[i], lastInfo1.a[i], lastInfo2.a[i]);
	filteredInfo.w[i] = med3(memsicImuInfo.w[i], lastInfo1.w[i], lastInfo2.w[i]);
	}
	*/

	tp.feedImuInfo(memsicImuInfo); // 
	//tp.feedImuInfo(V3(filteredInfo.a[0], filteredInfo.a[1], filteredInfo.a[2]), V3(filteredInfo.w[0], filteredInfo.w[1], filteredInfo.w[2])); // 

	//Serial.print(mImuTime);
	//Serial.print(",");
	if (loggingSensors) {
		if (memsicImuInfo.status != 0) {
			memsicImuInfo.status = 128 + (memsicImuInfo.status & 0x0F);
		}
		memsicImuInfo.status |= 0x10; //  memsic IMU info
		logImu(memsicImuInfo);
	}

	if (testLogCounter != 0) {
		testLogCounter--;

		if (testLogCounter == 0) {  // ok, switch to the unfiltered data
			short int value;
			digitalWriteFast(memsicSPISelectPin, LOW);
			value = memsicSPI.transfer16(0xB800); // 0x00 (0): Unfiltered
			digitalWriteFast(memsicSPISelectPin, HIGH);
#ifdef ENABLE_PIANO
			piano.play(Piano::A);
#endif
		}

		if (testLogCounter == 501) {
			sdLogStop();
			testLogCounter = 0;
#ifdef ENABLE_PIANO
			piano.play(Piano::C1);
#endif
		}
	}
}
#endif

void processMemsicData(void (*cb)(const xqm::ImuData& imu)) {
	bool check = true;
	int pc = 0, i;
	
	MemsicInfo memsicInfo1;

	while (check) {
		//__disable_irq();
		bool irq = disableInterrupts();
		if (hInfoRB1.get(&memsicInfo1)) { //  in case we have some new info
			//__enable_irq();
			enableInterrupts(irq);
			pc++;

			pInfo.timestamp = memsicInfo1.timestamp;
			for (i = 0; i < 3; i++) {
				pInfo.a[i] = ((XMType)(memsicInfo1.a[i])) * accScale;
				pInfo.w[i] = ((XMType)(memsicInfo1.w[i])) * gyroScale;
			}
			pInfo.status = memsicInfo1.status;

			if (cb != nullptr) {
				cb(pInfo);
			}
/*
			if (USE_IMU_INDEX == 1) {
				tp.feedImuInfo(pInfo);

			}
			if (loggingSensors) {
				if (pInfo.status != 0) {
					pInfo.status = 128 + (pInfo.status & 0x0F);
				}
				pInfo.status |= 0x10; //  memsic IMU info
				logImu(pInfo);
			}
*/
		} else {
			enableInterrupts(irq);
			//__enable_irq();
			check = false;
		}
	}
}

void setupMemsic() {
	setupMemsic_1();
	//digitalWriteFast(led1_pin, HIGH);
	delay(50);
	setupMemsic_2();
	digitalWriteFast(imuPwrPin, HIGH); // turn on IMU
	delay(500);
	setupMemsic_3(100);
}











