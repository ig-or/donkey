

#pragma once

#include <unistd.h>
#include <stdarg.h>
#include "xmessage.h"

enum MsgTypes {
	mgFrontObstacle = xqm::message_count,
	mgRearObstacle
};
#pragma pack(1)
struct MsgHeader {

};

struct MFrontObstacle {
	static const MsgTypes type = mgFrontObstacle;
	float distance;
};


#pragma pack()



/**
 * @brief 
 * 
 * @param msg 
 * @param type 
 * @return char*  a pointer to the message body
 */
//char* makeMsgHeader(char* msg, MsgTypes type);
/**
 * @brief 
 * 
 * @param msg 
 * @param type 
 * @return char* 
 */
//char* makeTail(char* msg, MsgTypes type);

int sendMsg(const unsigned char* data, unsigned char type, unsigned short int size);
template<class T> int sendMsg(const T* info/*, unsigned int timestamp*/) {
	int ret;
	ret = sendMsg((unsigned char*)(info), T::type, sizeof(T)/*, timestamp*/);
	return ret;
}