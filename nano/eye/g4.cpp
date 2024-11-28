#include "g4.h"

#include <cstdio>
#include "src/CYdLidar.h"
#include <core/base/timer.h>
#include <core/common/ydlidar_help.h>

int xmprintf(int q, const char * s, ...);

static CYdLidar laser;
static LaserScan scan;

bool G4Lidar::slStart() {
	xmprintf(0, "starting g4 ..    ... \n");
	///lidar port
	std::string str_optvalue("/dev/ttyUSB0");
	laser.setlidaropt(LidarPropSerialPort, str_optvalue.c_str(), str_optvalue.size());

	///ignore array
	str_optvalue = std::string("");
	laser.setlidaropt(LidarPropIgnoreArray, str_optvalue.c_str(), str_optvalue.size());

	///frameid
	std::string frame_id = std::string("laser_frame");

	/// lidar baudrate
	int optval = 230400;
	laser.setlidaropt(LidarPropSerialBaudrate, &optval, sizeof(int));
	/// tof lidar
	optval = static_cast<int>(TYPE_TRIANGLE);
	laser.setlidaropt(LidarPropLidarType, &optval, sizeof(int));
	/// device type
	optval = static_cast<int>(YDLIDAR_TYPE_SERIAL);
	laser.setlidaropt(LidarPropDeviceType, &optval, sizeof(int));
	/// sample rate
	optval = 9;;
	laser.setlidaropt(LidarPropSampleRate, &optval, sizeof(int));;
	/// abnormal count
	optval = 4;
	laser.setlidaropt(LidarPropAbnormalCheckCount, &optval, sizeof(int));

	/// Intenstiy bit count
	optval = 8;
	laser.setlidaropt(LidarPropIntenstiyBit, &optval, sizeof(int));
		
	/// fixed angle resolution
	bool b_optvalue =  true;
	laser.setlidaropt(LidarPropFixedResolution, &b_optvalue, sizeof(bool));
	/// rotate 180
	b_optvalue = true;
	laser.setlidaropt(LidarPropReversion, &b_optvalue, sizeof(bool));
	/// Counterclockwise
	b_optvalue = true;
	laser.setlidaropt(LidarPropInverted, &b_optvalue, sizeof(bool));
	/// Auto reconnect
	b_optvalue = true;
	laser.setlidaropt(LidarPropAutoReconnect, &b_optvalue, sizeof(bool));
	/// one-way communication
	b_optvalue = false;
	laser.setlidaropt(LidarPropSingleChannel, &b_optvalue, sizeof(bool));
	/// intensity
	b_optvalue = false;
	laser.setlidaropt(LidarPropIntenstiy, &b_optvalue, sizeof(bool));
	/// Motor DTR
	b_optvalue = false;
	laser.setlidaropt(LidarPropSupportMotorDtrCtrl, &b_optvalue, sizeof(bool));

	//////////////////////float property/////////////////
	/// unit: °
	float f_optvalue = 180.0f;
	laser.setlidaropt(LidarPropMaxAngle, &f_optvalue, sizeof(float));
	f_optvalue =  -180.0f;
	laser.setlidaropt(LidarPropMinAngle, &f_optvalue, sizeof(float));
	/// unit: m
	f_optvalue = 64.f;
	laser.setlidaropt(LidarPropMaxRange, &f_optvalue, sizeof(float));
	f_optvalue = 0.01f;
	laser.setlidaropt(LidarPropMinRange, &f_optvalue, sizeof(float));
	/// unit: Hz
	f_optvalue = 10.f;
	laser.setlidaropt(LidarPropScanFrequency, &f_optvalue, sizeof(float));

	bool ret = laser.initialize(); //  try USB0
	if (!ret) {
		str_optvalue = "/dev/ttyUSB1";
		laser.setlidaropt(LidarPropSerialPort, str_optvalue.c_str(), str_optvalue.size());
		ret = laser.initialize();
	}
	if (ret) {
		ret = laser.turnOn();
		if (ret) {
			xmprintf(0, "g4 started\n");
		} else {
			xmprintf(0, "cannot start g4 \n");
			return false;
		}
	} else {
		xmprintf(0, "startG4() cannot start the laser; %s\n", laser.DescribeError());
		return false;
	}
	lidarIsWorking = true;
	return true;
}

bool G4Lidar::slStop() {
	xmprintf(8, "G4Lidar::slStop() starting .. \n");
	//return true;
	if (lidarIsWorking) {
		xmprintf(8, "stopping g4 .. \n");
		bool test = laser.turnOff();
		if (!test) {
			xmprintf(0, "G4Lidar::slStop() can not stop the motor \n");
		}
		laser.disconnecting();
		lidarIsWorking = false;
		xmprintf(8, " .. g4 stopped\n");
	} else {
		xmprintf(8, "G4Lidar::slStop() lidarIsWorking false \n");	
	}
	xmprintf(8, "G4Lidar::slStop() finished \n");
	return true;
}

G4Lidar::~G4Lidar() {
	xmprintf(8, "G4Lidar::~G4Lidar() starting\n");
	if (points != 0) {
		delete[] points;
		points = 0;
		pointsSize = 0;
	}
	xmprintf(8, "G4Lidar::~G4Lidar() completed\n");
}

bool G4Lidar::getScan(SLPoint*& p, int& n) {
	//printf("!");
	if (!ydlidar::os_isOk()) {
		return false;
	}
	
	//putchar('&');
	if (pleaseStop) {
		return false;
	}
	bool result = laser.doProcessSimple(scan);  // call to YD sdk
	if (!result) {								// no new info ?
		
		//putchar('%');
		return false;
	}
	//putchar('*');
	// adjust points array
	int np = scan.points.size();
	if (points == 0) {
		pointsSize = np << 1;
		points = new SLPoint[pointsSize];
	} else {
		if (pointsSize < np) {
			delete[] points;
			pointsSize = np << 1;
			points = new SLPoint[pointsSize];
		}
	}
	pointsCount = np;
	int i;
	for (i = 0; i < np; i++) {
		points[i].angle = scan.points[i].angle;
		points[i].intensity = scan.points[i].intensity;
		points[i].range = scan.points[i].range;
	}
	n = np;
	p = points;
	return true;
}

void G4Lidar::adjustCalibration(SLCalibration& c) {
	c.rotation = -42.0 * pii / 180.0;
	xmprintf(5, "G4 calibration adjusted \n");
}
