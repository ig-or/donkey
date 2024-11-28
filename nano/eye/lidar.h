#pragma once

#include <thread>
#include <atomic>
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

/**
 *  the consumer of the lidar info. 
 */
class SLidarConsumer {
	public:
	/// @brief  another scan info. called from lidar thread
	/// @param p 
	/// @param n 
	virtual void slScan(SLPoint* p, int n) {}
	/// @brief  distance to the obstacle. called from lidar thread
	/// @param distance 
	virtual void slFrontObstacle(float distance) {}
};

class SLidar {
public:
	SLidar(SLidarConsumer& lc_) : lc(lc_) {}
	/// start the Lidar and its processing thread
	bool startLidar();
	/// @brief stop the lidar and its processing thread
	/// @return  true if OK
	bool stopLidar();
	virtual ~SLidar();
protected:
	std::atomic<bool> pleaseStop = false;
	SLidarConsumer& lc;
	bool lidarIsWorking = false;
	/// @brief  start the actual lidar hardware
	/// @return 
	virtual bool slStart();
	/// @brief  stop the actual lidar hardware
	/// @return 
	virtual bool slStop();
	/// @brief  get the scan info
	/// @param p all the points
	/// @param n number ofthe points
	/// @return true if have some info in p and 'n'
	virtual bool getScan(SLPoint*& p, int& n);
	virtual void adjustCalibration(SLCalibration& c) {c.rotation = 0.0; };
private:
	
	std::thread st;
	unsigned int rCounter = 0;
	SLCalibration	calibration;
	static const int fdSize = 3;
	XMRoundBuf<double, fdSize> frontDistances;
	/// @brief  the main lidar thread
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