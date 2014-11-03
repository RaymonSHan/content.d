
#include "../include/rmemory.hpp"
#include "../include/epollpool.hpp"

typedef void SigHandle(int, siginfo_t *, void *);
void SIGSEGV_Handle(int sig, siginfo_t *info, void *secret);
void SetupSIG(int num, SigHandle func);

int main(int, char**);

/////////////////////////////////////////////////////////////////////////////////////////////////////
// This is thread test main program from. From                                                     //
// request : raymoncommon.cpp rthread.cpp                                                          //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define TEST_THREAD
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#ifdef  TEST_THREAD
RpollGlobalApp* GetApplication();
#endif  // TEST_THREAD

/////////////////////////////////////////////////////////////////////////////////////////////////////
// This is rmemory test main program from. From Oct. 22 - 29 2014.                                 //
// total program time about 50 hour, base on memoryiocp                                            //
// request : raymoncommon.cpp rmemory.cpp                                                          //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
// #define TEST_RMEMORY                                                                            //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#ifdef  TEST_MEMORY
CMemoryAlloc* GetContextList();
#endif // TEST_MEMORY

