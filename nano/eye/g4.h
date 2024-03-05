

# pragma once
#include "lidar.h"

class G4Lidar : public SLidar {
public:
	G4Lidar(SLidarConsumer& lc_) : SLidar(lc_) {}
	~G4Lidar();
protected:
	virtual bool slStart();
	virtual bool slStop();
	virtual bool getScan(SLPoint*& p, int& n);
	void adjustCalibration(SLCalibration& c);
private:
	SLPoint* points = 0; /// the points from the lidar
	int pointsCount = 0;  /// real number of points in 'pointrs'
	int pointsSize = 0; /// size of 'points'
};
