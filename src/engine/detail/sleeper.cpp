#include "sleeper.hpp"

#include <iostream>
#include <boost/predef.h>

#if defined(BOOST_OS_WINDOWS)
#include <Windows.h>
#endif


void asm_volatile_pause(){
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
  ::_mm_pause();
#endif
}

int nanosleep(std::time_t tv_sec, long tv_nsec){
	HANDLE timer = NULL;
	LARGE_INTEGER sleepTime;

    sleepTime.QuadPart = -(tv_sec * 10000000 + tv_nsec / 100);

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	if(timer == NULL){
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, (LPTSTR) &buffer, 0, NULL);

        std::cerr << "nanosleep: CreateWaitableTimer failed: (" << GetLastError() << ") " << (LPTSTR)buffer << std::endl;
		LocalFree (buffer);
		return -1;
	}

	if(!SetWaitableTimer(timer, &sleepTime, 0, NULL, NULL, 0)){
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, (LPTSTR) &buffer, 0, NULL);

        std::cerr << "nanosleep: SetWaitableTimer failed: (" << GetLastError() << ") " << (LPTSTR)buffer << std::endl;
		LocalFree (buffer);
		return -1;
	}

	if(WaitForSingleObject (timer, INFINITE) != WAIT_OBJECT_0){
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, (LPTSTR) &buffer, 0, NULL);

        std::cerr << "nanosleep: WaitForSingleObject failed: (" << GetLastError() << ") " << (LPTSTR)buffer << std::endl;
		LocalFree (buffer);
		return -1;
	}

	return 0;
}

namespace akva{
namespace engine{

// +++ Sleeper +++

Sleeper::Sleeper()
: spin_count_(0)
{ }

void Sleeper::wait(){
    if(spin_count_ < kMaxActiveSpin){
        ++spin_count_;
        asm_volatile_pause();
    }
    else{
        nanosleep(0, 1000000); // Always sleep 1 ms
    }
}

} // engine
} // akva
