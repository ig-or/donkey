

#pragma once 

extern volatile bool inpExitRequest;
void inputProc(void(cb)(char*));