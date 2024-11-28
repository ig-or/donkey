

#include "kbd.h"
#include <unistd.h>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>

std::function<void(char)> f_c = nullptr;
std::atomic<bool> pstop = false;
std::thread kbdThread;

int xmprintf(int q, const char * s, ...);

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);

  struct termios raw = orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  //raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void kbdSetup(std::function<void(char)> fc) {
    f_c = fc;

}

bool kbdStart() {
	pstop = false;
	kbdThread = std::thread(kbdtest);
	return true;
}

void kbdStop() {
    pstop = true;
	//using namespace std::chrono_literals;
	if (!kbdThread.joinable()) { //  already stopped?
		return;
	}
	kbdThread.join();
    xmprintf(8, "kbd thread finished\n");
	return;
}

void kbdtest() {
    enableRawMode();
    size_t ok;
    using namespace std::chrono_literals;
    char c;
    while (!pstop) {
        //std::this_thread::yield();
        std::this_thread::sleep_for(50ms);
        c = '\0';
        ok = read(STDIN_FILENO, &c, 1);
        if (ok == 0) {
            continue;
        }
        if (f_c != nullptr) {
            f_c(c);
        }
        if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
        //if (c == 'q') break;
    }
}

