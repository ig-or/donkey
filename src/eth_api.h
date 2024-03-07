

#pragma once

#include <unistd.h>
#include <stdarg.h>
#include "xmessage.h"

enum MsgTypes {
	mgFrontObstacle,
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
 * @brief make a message with several 'float' numbers
 * 
 * @param type message type
 * @param buf where to put everything
 * @param  bufSize size of 'buf'
 * @param ...  all the float numbers
 * @return message size in bytes
 */
int makeFmessage(MsgTypes type, char* buf, int bufSize, ...);

/**
 * @brief 
 * 
 * @param msg 
 * @param type 
 * @return char*  a pointer to the message body
 */
char* makeMsgHeader(char* msg, MsgTypes type);
/**
 * @brief 
 * 
 * @param msg 
 * @param type 
 * @return char* 
 */
char* makeTail(char* msg, MsgTypes type);
