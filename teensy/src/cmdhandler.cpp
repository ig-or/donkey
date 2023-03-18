
#include <Arduino.h>
#include "usb_serial.h"

#include "cmdhandler.h"
#include "teetools.h"
#include "ttpins.h"

#include "memsic.h"
#include "sr04.h"
#include "motors.h"
#include "controller.h"
#include "receiver.h"
//#include "imu_alg.h"
#include "logfile.h"
//#include "logsend.h"
#include "eth.h"
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
			////usb_serial_write(incomingUsbSerialInfo, bs3); //  this is for debugging and testing
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
			if (infoIndex != 0) {
				info[infoIndex] = 0;
				processTheCommand(info, infoIndex);

				infoIndex = 0;
			}
		} else {
			info[infoIndex] = s[i];
			infoIndex++;
		}
	}
}

int processTheCommand(const char* s, int size) {
    //xmprintf(1, "got cmd size=%d (%s) \r\n", size, s);
	xmprintf(3, "\r\nprocessTheCommand: (%s)\r\n", s);
	
	if (size == 0) {
		size = strlen(s);
	}

	if (strcmp(s, "us") == 0 ) {
		usPrint();
	} else 	if (strcmp(s, "imu") == 0) {
		memsicPrint();
	} else 	if (strcmp(s, "eth") == 0) {
		ethPrint();
	} else 	if (strncmp(s, "log", 3) == 0) {
		if (size > 4) { //   use log file name
			lfStart(s + 4);
		} else {
			lfStart();
		}
	} else 	if (strcmp(s, "lstop") == 0) {
		lfStop();
	} else  if (strcmp(s, "lprint") == 0) {
		lfPrint();
	} else 	if (strcmp(s, "batt") == 0) {
		batteryPrint();
	} else if (strncmp(s, "mstart", 6) == 0) {	
		int m;
		int k = sscanf(s + 6, "%d", &m);
		if (k == 1) {
			enableMotor(m);
			xmprintf(3, "motor (%d) enabled \r\n", m);
		} else {
			xmprintf(3, "motor : k = %d \r\n", k);
		}
	} else if (strcmp(s, "mstop") == 0) {
		enableMotor(0);
	} else if (strcmp(s, "cprint") == 0) {
		controlPrint();
	} 


	//return 0;
	//if (strncmp(s, "get ", 4) == 0) {
	//	lfGetFile(s + 4);
	//}	

    return 0;
}

