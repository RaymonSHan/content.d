
#ifndef INCLUDE_RAYMONCOMMON_H
#define INCLUDE_RAYMONCOMMON_H

#include <errno.h>
#include <fcntl.h> 
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/wait.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Common const                                                                                    //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
typedef long long int           SINT;
typedef char                    SCHAR;
typedef unsigned long long int  INT;                            // basic type
typedef unsigned char           CHAR;                           // basic type
typedef unsigned long long int* PINT;
typedef unsigned char*          PCHAR;
typedef void*                   PVOID;

#define SEG_START_BUFFER        (0x52LL << 40)                  // 'R'
#define SEG_START_STACK         (0x53LL << 40)                  // 'S'

#define CHAR_SMALL              32
#define CHAR_NORMAL             512
#define CHAR_LARGE              4096

#define SIZE_CACHE              64                              // for struct border
#define SIZE_NORMAL_PAGE        (0x01LL << 12)                  // 4K
#define SIZE_HUGE_PAGE          (0x01LL << 21)                  // 2M
#define SIZE_THREAD_STACK       (0x01LL << 24)                  // 16M
#define SIZE_DATA_BUFFER        ((0x01LL << 22) - SIZE_NORMAL_PAGE) // 4M

#define NEG_SIZE_THREAD_STACK   (-1*SIZE_THREAD_STACK)
#define SIZE_SMALL_PAD          8


#define BOOL                    bool
#define HANDLE_LOCK             volatile long long int		//

#define MAP_FAIL                -1                              // lazy replace MAP_FAILED
#define NUL                     0                               // lazy replace NULL

#define TIMEOUT_TCP             20
#define TIMEOUT_INFINITE        0xffffffff
#define TIMEOUT_QUIT            2

///////////////////////////////////////////////////////////////////////////////////////////
// pad size for border, stack must at 2^n border                                         //
////////\///////////////////////\///////////////////////////////\////////////////        //
#define PAD_SIZE(p, pad, bord)				\
  ((sizeof(p) + pad - 1) & (-1 * bord)) + bord
#define PAD_INT(p, pad, bord)				\
  ((p + pad - 1) & (-1 * bord)) + bord
#define PAD_ADDR(p, pad, bord)				\
  ((p.aLong + pad - 1) & (-1 * bord)) + bord

///////////////////////////////////////////////////////////////////////////////////////////
// Error control                                                                         //
////////\///////////////////////\///////////////////////////////\////////////////        //
#define __RSTRING2(x)           #x
#define __RSTRING(x)            __RSTRING2(x)

#define __JOIN2(x,y)            x ## y
#define __JOIN(x,y)             __JOIN2(x,y)

#define __TO_MARK(x)            _rm_ ## x                       // used for ret_err control
#define _rm_MarkMax             0xffffffff                      // for mark end

#define	__TRY                   INT ret_err = __LINE__; beginCall();
#define __MARK(x)               static int __TO_MARK(x) = ret_err = __LINE__; setLine();
#define __CATCH_BEGIN           endCall(); return 0; error_stop:
#define __BETWEEN(x,y)          if (ret_err >= __TO_MARK(x) && ret_err <= __TO_MARK(y))
#define __AFTER(x)              if (ret_err >= __TO_MARK(x))
#define __CATCH_END             endCall(); return ret_err;
#define	__CATCH                 endCall(); return 0; error_stop: endCall(); return ret_err;
#define __TRY__                 beginCall();
#define __CATCH__               endCall(); return 0;
#define __BREAK                 { goto error_stop; }
#define __BREAK_OK              { ret_err = 0; goto error_stop; }

#define MESSAGE_DEBUG           0x0001
#define MESSAGE_ERROR           0x0002
#define MESSAGE_HALT            0x0004

void    __MESSAGE(INT level, const char * _Format, ...);

#define __INFO(level, _Format,...)					\
  { __MESSAGE(level,  _Format,##__VA_ARGS__); }
#define __INFOb(level, _Format,...)					\
  { __MESSAGE(level, _Format,##__VA_ARGS__); __BREAK_OK }

#define __DO1c_(val, func, _Format,...) {				\
    setLine();								\
    val = func;								\
    if (val == -1) 							\
      __INFO(MESSAGE_DEBUG, _Format,##__VA_ARGS__);			\
  }
#define __DO1c(val, func)						\
  { setLine(); val = func; }

#define __DO1_(val, func, _Format,...) {				\
    setLine();								\
    val = func;								\
    if (val == -1) {							\
      __INFO(MESSAGE_DEBUG, _Format,##__VA_ARGS__);			\
      __BREAK								\
    }									\
  }
#define __DO1(val, func)						\
  { setLine(); val = func; if (val == -1) __BREAK; }

#define __DOc_(func, _Format,...) {					\
    setLine();								\
    if (func)								\
      __INFO(MESSAGE_DEBUG, _Format,##__VA_ARGS__);			\
  }
#define __DOc(func)							\
  { setLine(); func; }

#define __DO_(func, _Format,...) {					\
    setLine();								\
    if (func) {								\
      __INFO(MESSAGE_DEBUG, _Format,##__VA_ARGS__);			\
      __BREAK								\
    }									\
  }
#define __DO(func)							\
  { setLine(); if (func) __BREAK }

#define __DOb_(func, _Format,...) {					\
    setLine();								\
    if (func) {								\
      __INFO(MESSAGE_DEBUG, _Format,##__VA_ARGS__);			\
    }									\
  __BREAK								\
  }
#define __DOb(func)							\
  { setLine(); func; __BREAK }

///////////////////////////////////////////////////////////////////////////////////////////
// Synchronous lock                                                                      //
////////\///////////////////////\///////////////////////////////\////////////////        //
#define NOT_IN_PROCESS          (INT)0xa0
#define IN_PROCESS              (INT)0xa1

#define CmpExg                  __sync_bool_compare_and_swap
#define LockAdd(p,s)            __sync_fetch_and_add(&p, s)
#define LockInc(p)              __sync_fetch_and_add(&p, 1)
#define LockDec(p)              __sync_fetch_and_add(&p, -1)

#define __LOCKp(lock)           while(!CmpExg(lock, NOT_IN_PROCESS, IN_PROCESS));
#define __FREEp(lock)           *lock = NOT_IN_PROCESS;
#define __LOCK(lock)            while(!CmpExg(&lock, NOT_IN_PROCESS, IN_PROCESS));
#define __LOCK__TRY(lock)       !CmpExg(&lock, NOT_IN_PROCESS, IN_PROCESS)
#define __FREE(lock)            lock = NOT_IN_PROCESS;

///////////////////////////////////////////////////////////////////////////////////////////
// get __CLASS__ for thread                                                              //
////////\///////////////////////\///////////////////////////////\////////////////        //
#define __class(name)           class name {					\
                                  protected:			     		\
				  virtual const char* getClassName(void) {    	\
				    return #name; };		\
                                  private:

#define __class_(name, base)    class name : public base {			\
                                  protected:			     		\
				  virtual const char* getClassName(void) {      \
				    return #name; };		\
                                  private:

#endif  // INCLUDE_RAYMONCOMMON_H
