
#ifdef G4LIDAR
#include "g4.h"
#endif
#include "xmfilter.h"

#include "lidar.h"

int xmprintf(int q, const char * s, ...);

/**
 * \return the minimum distance to the front obstacle
 * TODO: make it nice. search only a range of points.  but, points are not sorted..
*/
float checkFrontDistance(SLPoint* p, int n, const SLCalibration& c) {
	int i;
	int k = 0;
	float minDist = 100.0f; // meters
	float a, r;
	constexpr float ang = 8.0;
	constexpr float minAng = -ang * Deg2Rad;
	constexpr float maxAng = ang * Deg2Rad;
	for (i = 0; i < n; i++) {
		a = p[i].angle + c.rotation;
		if (a < maxAng && a > minAng) {
			r = p[i].range;
			if ((r > 0.05) && (r < minDist)) {
				minDist = p[i].range;
				k = i;
			}
		}
	}
	return minDist;
}

SLidar::~SLidar() {

}

bool SLidar::slStart() {
	return true;
}

bool SLidar::slStop() {
	return true;
}

bool SLidar::getScan(SLPoint*& p, int& n){
	n = 0;
	p = 0;
	return false;
}

bool  SLidar::startLidar() {
	if (st.joinable()) { //  working already?
		return false;
	}
	pleaseStop = false;
	
	st = std::thread(&SLidar::slRun, this);
	return true;
}

bool SLidar::stopLidar() {
	xmprintf(8, "SLidar::stopLidar() starting \n");
	if (!st.joinable()) { //  already stopped?
		return false;
	}
	pleaseStop = true;
	st.join();
	xmprintf(8, "SLidar::stopLidar() finished \n");
	return true;
}

double SLidar::updateFrontDistance(double d) {
	int i;
	frontDistances.put(d);
	//float a = d;
	//for (i = 0; i < fdSize-1; i++) {
	//	if (frontDistances[i] < a) {
	//		a = frontDistances[i];
	//	}
	//}
	double tmp[fdSize];
	memcpy(tmp, frontDistances.buf, fdSize * sizeof(double));
	double  a = opt_med5(tmp);
	return a;
}

void SLidar::slRun() {
	xmprintf(5, "SLidar::slRun() is starting \n");
	int i;
	bool test = slStart();
	if (!test) {
		xmprintf(5, "ERROR: SLidar::slRun(): cannot start the lidar; \n");
		return;
	}
	adjustCalibration(calibration);

	// prepare frontDistances
	frontDistances.clear();
	float d = 100.0, d1;
	for (i = 0; i < fdSize; i++ ) {frontDistances.put(d);}
	
	SLPoint* 	p;
	int 		np;

	while (!pleaseStop) {
		std::this_thread::yield();
		test = getScan(p, np);
		if (!test) {
			continue;
		}
		//xmprintf(12, "SLidar::slRun() got %d points   \n", np);
		lc.slScan(p, np);
		d = checkFrontDistance(p, np, calibration);
		d1 = updateFrontDistance(d);
		lc.slFrontObstacle(d1);

		rCounter += 1;
	}
	xmprintf(5, "SLidar::slRun()    stoppint the lidar \n");
	test = slStop();


	xmprintf(5, "SLidar::slRun() finished \n");
}

void SimpleLC::slScan(SLPoint* p, int n){
	if (counter % 150 == 0) {
		//xmprintf(6, "slFrontObstacle got %d points; min ang = %.3f; max ang = %.3f\n", n, p[0].angle, p[n-1].angle);

		/*
		xmprintf(6, "slScan got %d points; measurements (ang, range): \n", n);
		int i;
		for (i = 0; i < n; i+=8) {
			printf("(%.3f %.3f)\t ", p[i].angle * 180.0 / pii, p[i].range);
			if (i % 5 == 0) {
				printf("\n");
			}
		}
		printf("\n");
		*/
	}
}
void SimpleLC::slFrontObstacle(float distance) {
	if (counter % 5 == 0) {
		xmprintf(6, "slFrontObstacle %.3f;\n", distance);
	}
	counter += 1;
}