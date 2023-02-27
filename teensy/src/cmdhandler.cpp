
#include <Arduino.h>
#include "usb_serial.h"

#include "cmdhandler.h"
#include "teetools.h"
#include "ttpins.h"

#include "memsic.h"
//#include "imu_alg.h"
#include "logfile.h"
//#include "logsend.h"
//#include "eth.h"
#include "power.h"

static const int incomingUsbSerialInfoSize = 32;
static char incomingUsbSerialInfo[incomingUsbSerialInfoSize];

static const int infoSize = 88;
static char info[infoSize];
static int infoIndex = 0;

void h_usb_serial() {
	int bs1;
	while ((bs1 = usb_serial_available())) {
		int bs2 = (bs1 <= incomingUsbSerialInfoSize) ? bs1 : incomingUsbSerialInfoSize;
		int bs3 = usb_serial_read(incomingUsbSerialInfo, bs2);
		if (bs3 > 0) {
			//usb_serial_write(incomingUsbSerialInfo, bs3); //  this is for debugging and testing
			onIncomingInfo(incomingUsbSerialInfo, bs3);
		}
	}
}

void onIncomingInfo(char* s, int size) {
	int i;
	for (i = 0; i < size; i++) {
		if (i + infoIndex >= (infoSize-1)) {  //   overflow?
			infoIndex = 0;  //  reset!
		}

		usb_serial_write(s+i, 1); // echo

		if ((s[i] == 0) || (s[i] == '\n') || (s[i] == '\r')) {
			info[infoIndex] = 0;
			processTheCommand(info, infoIndex);

			infoIndex = 0;
		} else {
			info[infoIndex] = s[i];
			infoIndex++;
		}
	}
}

int processTheCommand(const char* s, int size) {
    //xmprintf(0, "got cmd size=%d (%s)", size, s);
	xmprintf(2, "processTheCommand: (%s)\r\n", s);
	if (size == 0) {
		size = strlen(s);
	}
	
	if (strcmp(s, "imu") == 0) {
		memsicPrint();
	}
	//if (strcmp(s, "eth") == 0) {
	//	ethPrint();
	//}
	
	
	if (strncmp(s, "log", 3) == 0) {
		if (strlen(s) > 4) { //   use log file name
			//logSetup(s + 4);
			lfStart(s + 4);
		} else {
			//logSetup("log");
			lfStart();
		}
	}
	if (strcmp(s, "lstop") == 0) {
		lfStop();
	}
	if (strcmp(s, "lprint") == 0) {
		lfPrint();
	}
	//if (strncmp(s, "get ", 4) == 0) {
	//	lfGetFile(s + 4);
	//}
	if (strcmp(s, "batt") == 0) {
		batteryPrint();
	}
	

    return 0;
}

