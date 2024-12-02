#pragma once
#include <functional>

enum KbdCode {
    kbdExit

};

/**
 * \param fCode special keys callback
 *  \param fInfo commands callback
 */
void kbdSetup(std::function<void(KbdCode)> fCode, std::function<void(char*)> fInfo);

bool kbdStart();
void kbdStop();
void kbdtest();