

#include "../include/rmemory.hpp"

CMemoryAlloc* GetContextList();

int main(int, char**);


/////////////////////////////////////////////////////////////////////////////////////////////////////
// This is thread test main program from. From                                                     //
// request : raymoncommon.cpp rthread.cpp                                                          //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
//#define TEST_THREAD
////////\///////////////////////\///////////////////////////////\//////////////////////////        //

#ifdef  TEST_THREAD

#endif // TEST_THREAD

/////////////////////////////////////////////////////////////////////////////////////////////////////
// This is rmemory test main program from. From Oct. 22 - 29 2014.                                 //
// total program time about 50 hour, base on memoryiocp                                            //
// request : raymoncommon.cpp rmemory.cpp                                                          //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define TEST_RMEMORY                                                                            //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //

void SIGSEGV_Handle(int sig, siginfo_t *info, void *secret);
