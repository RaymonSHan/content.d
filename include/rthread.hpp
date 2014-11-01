
// rthread.hpp
#ifndef INCLUDE_RTHREAD_HPP
#define INCLUDE_RTHREAD_HPP

#include "rtype.hpp"

class RThreadResource
{
protected:
  INT  nowOffset;
public:
  static volatile INT globalResourceOffset;
public:
  RThreadResource(INT size);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// per thread info, saved at the bottom of stack                                                   //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define PAD_TRACE_INFO          SIZE_NORMAL_PAGE

#define getThreadInfo(info, off)				\
  asm volatile ("movq %%rsp, %0;"				\
		"andq %2, %0;"					\
		"addq %1, %0;"					\
		: "=r" (info)					\
		: "m" (off), "i"(NEG_SIZE_THREAD_STACK));

#define beginCall()						\
  asm volatile ("movq %%rsp, %%rcx;"				\
		"andq %3, %%rcx;"				\
		"addq %4, %%rcx;"				\
		"addq %5, (%%rcx);"				\
		"addq (%%rcx), %%rcx;"				\
		"movq %0, (%%rcx);"				\
		"movq %1, 8(%%rcx);"				\
		"movq %2, 16(%%rcx);"				\
		:						\
		: "i" (__FILE__), "i"(__PRETTY_FUNCTION__),	\
		  "i" (__LINE__),				\
		  "i" (NEG_SIZE_THREAD_STACK),			\
		  "i" (PAD_TRACE_INFO),				\
		  "i" (sizeof(perTraceInfo))			\
		: "%rcx");

#define endCall()						\
  asm volatile ("movq %%rsp, %%rcx;"				\
		"andq %0, %%rcx;"				\
		"addq %1, %%rcx;"				\
		"movq %%rcx, %%rdx;"				\
		"addq (%%rcx), %%rdx;"				\
		"movq $0, (%%rdx);"				\
		"movq $0, 8(%%rdx);"				\
		"movq $0, 16(%%rdx);"				\
		"subq %2, (%%rcx);"				\
		:						\
		: "i" (NEG_SIZE_THREAD_STACK),			\
		  "i" (PAD_TRACE_INFO),				\
		  "i" (sizeof(perTraceInfo))			\
		: "%rcx", "%rdx");

#define setLine()						\
  asm volatile ("movq %%rsp, %%rcx;"				\
		"andq %1, %%rcx;"				\
		"addq %2, %%rcx;"				\
		"addq (%%rcx), %%rcx;"				\
		"movq %0, 16(%%rcx);"				\
		:						\
		: "i" (__LINE__),				\
		  "i" (NEG_SIZE_THREAD_STACK),			\
		  "i" (PAD_TRACE_INFO)				\
		: "%rcx");

#define getTraceInfo(info)					\
  asm volatile ("movq %%rsp, %0;"				\
		"andq %1, %0;"					\
		"addq $4096, %0;"				\
		: "=r" (info)					\
		: "i"(NEG_SIZE_THREAD_STACK));


#define displayTraceInfo(info)					\
  getTraceInfo(info);						\
  for (int i=0; i<info->nowLevel/sizeof(perTraceInfo); i++)	\
    printf("  %d, in file:%14s, line:%4lld, function:%s\n",	\
	   i,							\
	   info->calledInfo[i].fileInfo,			\
	   info->calledInfo[i].lineInfo,			\
	   info->calledInfo[i].funcInfo);


#define MAX_NEST_LOOP           1023                             // size of func call nest

typedef struct perTraceInfo {
  char  *fileInfo;
  char  *funcInfo;
  INT   lineInfo;
  INT   pad;
}perTraceInfo;

// MUST be multi of NORMAL_PAGE_SIZE
typedef struct threadTraceInfo {
  INT   nowLevel;
  INT   pad[3];
  perTraceInfo calledInfo[MAX_NEST_LOOP];
}threadTraceInfo;

#define DISPLAY					\
  threadTraceInfo *_pt_debugtestinfo;		\
  displayTraceInfo(_pt_debugtestinfo);

#endif  // INCLUDE_RTHREAD_HPP
