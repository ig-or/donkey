#include "g4.h"

#include "src/CYdLidar.h"

int xmprintf(const char * s, ...);

CYdLidar laser;
bool g4IsWorking = false;

bool startG4()  {
	xmprintf("starting g4 ..    ... \n");
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
			xmprintf("g4 started\n");
		} else {
			xmprintf("cannot start g4 \n");
			return false;
		}
	} else {
		xmprintf("startG4() cannot start the laser; %s\n", laser.DescribeError());
		return false;
	}
	g4IsWorking = true;
	return true;
}

bool stopG4() {
	if (g4IsWorking) {
		xmprintf("stopping g4 .. \n");
		laser.turnOff();
		laser.disconnecting();
		g4IsWorking = false;
		xmprintf(" .. g4 stopped\n");
	}
	return true;
}