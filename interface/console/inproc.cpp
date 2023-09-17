


#include "inproc.h"

#ifdef WIN32
	#include <SDKDDKVer.h>
	#include <conio.h>
#else
	#define USE_NCURSES
	//#include <termios.h>
	#ifdef USE_NCURSES
	#include <ncurses.h>
	#endif
	#include <unistd.h>
#endif

#include <chrono>
#include <thread>
#include <iostream>
#include <mutex>
#include <stdarg.h>
#include "xmroundbuf.h"

volatile bool inpExitRequest = false;
int xmprintf(const char * s, ...);

#ifdef WIN32
int getch_1(){
	if (_kbhit()) {
		return _getch();
	} else {
		return -1;
	}
}
#else
/*
static struct termios old, current;

void initTermios(int echo) 
{
  tcgetattr(0, &old); // grab old terminal i/o settings 
  current = old; // make new settings same as old settings 
  cfmakeraw(&current);
  current.c_lflag &= ~ICANON; // disable buffered i/o 

  //current.c_cc[VMIN] = 0;
  //current.c_cc[VTIME] = 10;

  if (echo) {
      current.c_lflag |= ECHO; // set echo mode 
  } else {
      current.c_lflag &= ~ECHO; // set no echo mode 
  }
  tcsetattr(0, TCSANOW, &current); //use these new terminal i/o settings now 
}

// Restore old terminal i/o settings 
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

// Read 1 character - echo defines echo mode 
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

//Read 1 character without echo 
char getch(void) 
{
  return getch_(0);
}

// Read 1 character with echo 
char getche(void) 
{
  return getch_(1);
}

int kbhit()
{
    // timeout structure passed into select
    struct timeval tv;
    // fd_set passed into select
    fd_set fds;
    // Set up the timeout.  here we can wait for 1 second
    tv.tv_sec = 0;
    tv.tv_usec = 200000;

    // Zero out the fd_set - make sure it's pristine
    FD_ZERO(&fds);
    // Set the FD that we want to read
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    // select takes the last file descriptor value + 1 in the fdset to check,
    // the fdset for reads, writes, and errors.  We are only passing in reads.
    // the last parameter is the timeout.  select will return if an FD is ready or 
    // the timeout has occurred
    int test = select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	if ((test == 0) || (test == -1)) {
		return 0;
	}
	return 1;
    // return 0 if STDIN is not ready to be read.
    return FD_ISSET(STDIN_FILENO, &fds);
}

*/
	//WINDOW *local_win;
#endif

void inputProc(void(cb)(char*)) {
	int ch;
	using namespace std::chrono_literals;
	const int cmdSize = 256;
	char cmd[cmdSize];
	XMRoundBuf<std::string, 10> cmdList;
	int cmdListIndex = 0;
	int cmdIndex = 0;

#ifdef WIN32

#else
	#ifdef USE_NCURSES
	int row, col;
	initscr();  /* Start curses mode 		*/
	//start_color();			/* Start color 			*/
	//raw();    /* Line buffering disabled	*/
	cbreak();   
	keypad(stdscr, TRUE);	 /* We get F1, F2 etc..		*/
	timeout(200);
	//init_pair(1, COLOR_BLACK, COLOR_WHITE);
	//attron(COLOR_PAIR(1));
	//getmaxyx(stdscr, row, col);		/* find the boundaries of the screeen */
	//local_win = newwin(row, col, 0, 0);
	//noecho();
	#endif
#endif

	while (!inpExitRequest) {
        //xmprintf(".");
		//if (kbhit() == 0) {
			//std::this_thread::sleep_for(100ms);
			//continue;
		//}
		//xmprintf("*\r\n");
		//ch = getche();
#ifdef WIN32
		if (_kbhit()) {
			ch = _getch();
		}
		else {
			std::this_thread::sleep_for(100ms);
			continue;
		}
#else
	#ifdef USE_NCURSES
		ch = getch();

		if (ch == ERR) {
			refresh();
			continue;
		}
	#else


	#endif
#endif
		//xmprintf(" ch1=%d \r\n", ch);
		
#ifdef WIN32
		xmprintf("%c", ch);
#else
	#ifdef USE_NCURSES
		refresh();
	#else

	#endif
#endif

		if (cmdIndex > cmdSize - 3) {
			cmdIndex = 0;
		}
		
		cmd[cmdIndex] = ch;
		cmdIndex++;

		//xmprintf("%c", char(ch & 0x00FF));
		//ch = toupper(ch);
		//if ((ch == 'Q') || (inpExitRequest)) {
		//	//inpExitRequest = true;
		//	break;
		//}
		if (ch == 276) { //  F12
			inpExitRequest = true;
#ifdef WIN32
			ch = _getch();

#else		
#ifdef USE_NCURSES
			timeout(-1);
			ch = getch();
#endif
#endif
			break;
		}

		if ((ch == '\n') || (ch == '\r')) {
            if (cmdIndex < 1) {
                continue;
            }
			cmd[cmdIndex-1] = 0;
            //xmprintf("inp: got {%s} \n", cmd);
			
			//cmd[cmdIndex - 1] = 0;
			cmdListIndex = 0;
			std::cout << std::endl;
			if (strncmp(cmd, "cls", 3) == 0) {
				//clrscr();
				//system("cls");
			} else {
				//xmprintf("sending {%s}\r\n", cmd);
				cb(cmd);
				cmdList.put(std::string(cmd));
				cmdListIndex = cmdList.num - 1;
			}

			//std::cout << " sending " << cmd << std::endl;
			//std::cout << "ret = " << ret << std::endl;
			cmdIndex = 0;
		}

	}
	int abc = 0;
    inpExitRequest = true;
#ifndef WIN32
#ifdef USE_NCURSES
	endwin();  /* End curses mode		  */
#endif
#endif
	//resetTermios();
    //xmprintf("exiting inp thread \n");

}

std::mutex xmpMutex;
static const int sbSize = 1024;
char sbuf[sbSize];

int xmprintf(const char * s, ...) {
	std::lock_guard<std::mutex> lk(xmpMutex);
	va_list args;
	va_start(args, s);
	int ok;


	int bs = 0;

	ok = vsnprintf(sbuf + bs, sbSize - 1 - bs, s, args);
	if ((ok <= 0) || (ok >= (sbSize - bs))) {
		//strcpy(sbuf, " errror 2\n");
		ok = snprintf(sbuf, sbSize - 1, "errror #2, ok = %d \n", ok);
	}


	int eos = strlen(sbuf);
	sbuf[sbSize - 1] = 0;


	//usb_serial_write((void*)(sbuf), eos);
	#ifdef WIN32
		printf("%s", sbuf);
	#else
		//#ifdef USE_NCURSES
		//	printw("%s", sbuf);
		//	refresh();
		//#else
			printf("%s", sbuf);
		//#endif
	#endif

	va_end(args);
	return 0;
}

