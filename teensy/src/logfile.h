/**  a log file
*/
#pragma once

#include "rbuf.h"

extern volatile bool lfWriting; ///< true if writing the log file right now
extern char fileNameCopy[64];
enum LFState {
	lfSInit,
	lfSGood,
	lfSError
};
extern volatile LFState lfState; ///< lfSGood if SD state is good

///  init log file
void lfInit();
/**   start log to file 
 */ 
void lfStart(const char* fileName = nullptr);
/// stop log
void lfStop();

/**  add data to the file.
 *  can be called from the interrupt.
 * */
void lfFeed(void* data, int size);

int lfSendMessage(const unsigned char* data, unsigned char type, unsigned short int size);
template<class T> int lfSendMessage(const T* info) {
	int ret;
	ret = lfSendMessage((unsigned char*)(info), T::type, sizeof(T));
	return ret;
}

/** save info to the log file.
 *  call this not from the interrupt.
 * */
void lfProcess();

void lfPrint();
void lfFiles();
int lfGetFile(char* name);

extern volatile ByteRoundBuf rb;
static const int rbSize = 4096; //8192;
extern unsigned char __attribute__((aligned(32))) rbBuf[rbSize];

//unsigned char* getRBuf() ;
//void resetRB() 



