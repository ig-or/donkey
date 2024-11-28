#pragma once
#include <functional>

void kbdSetup(std::function<void(char)> fc);
bool kbdStart();
void kbdStop();
void kbdtest();