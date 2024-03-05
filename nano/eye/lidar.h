#pragma once

#include <thread>
#include "xmatrix2.h"
#include "xmroundbuf.h"

#pragma pack(1)

/**
 * @brief The Laser Point struct
 * @note angle unit: rad.\n
 * range unit: meter.\n
 */
struct SLPoint {
  /// lidar angle. unit(rad)
  float angle;
  /// lidar range. unit(m)
  float range;
  /// lidar intensity
  float intensity;
};

#pragma pack()

struct SLCalibration {
	V3 v2lOffset;		/// [meters]  offset from vehicle to the lidar (expr. in vehicle frame)
	DCM l2vRotation; 	/// rotation from lidar frame into vehicle frame
	float rotation = 0.0f; 	/// [radians] what to add to the angle values, to make 'zero' looking forward (along vehicle X axis)
};

class SLidarConsumer {
	public:
	virtual void slScan(SLPoint* p, int n) {}
	virtual void slFrontObstacle(float distance) {}
};

class SLidar {
public:
	SLidar(SLidarConsumer& lc_) : lc(lc_) {}
	bool startLidar();
	bool stopLidar();
	virtual ~SLidar();
protected:
	SLidarConsumer& lc;
	bool lidarIsWorking = false;
	virtual bool slStart();
	virtual bool slStop();
	virtual bool getScan(SLPoint*& p, int& n);
	virtual void adjustCalibration(SLCalibration& c) {c.rotation = 0.0; };
private:
	bool pleaseStop = false;
	std::thread st;
	unsigned int rCounter = 0;
	SLCalibration	calibration;
	static const int fdSize = 5;
	XMRoundBuf<double, fdSize> frontDistances;
	void slRun();
	double updateFrontDistance(double d);
	
};

 class SimpleLC : public SLidarConsumer {
	public:
	void slScan(SLPoint* p, int n);
	void slFrontObstacle(float distance);
	private:
	unsigned int counter = 0;
 };