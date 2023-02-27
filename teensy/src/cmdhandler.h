#pragma once

void h_usb_serial();
void onIncomingInfo(char* s, int size);
int processTheCommand(const char* s, int size = 0);