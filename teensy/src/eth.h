

#pragma once

/**
 * This is a parameter for ethSetInfoHandler
*/
enum GFrequest {
	gfFinish = 0,
	gfPleaseSendNext,
	gfPleaseRepeat,
	gfEmpty
};

/// @brief   function for ethSetInfoHandler
typedef void(*EthInfoHandler)(GFrequest, unsigned int);

/** this is the setup.*/
void ethSetup();
/**
 * call this in every loop (not in interrupt)
*/
void ethLoop(unsigned int now);

/**  printout some statistics*/
void ethPrint();

/**
 * add some info to be send to the other side.
*/
void ethFeed(char* s, int size);

/**
 * setup a callback function.
*/
void ethSetInfoHandler(EthInfoHandler h);