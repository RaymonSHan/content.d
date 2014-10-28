
#include   <stdarg.h>

#define MAP_FAIL                -1                              // lazy replace MAP_FAILED
#define NUL                     0                               // lazy replace NULL

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Common const                                                                                    //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define SEG_START_BUFFER        (0x52LL << 40)                  // 'R'
#define SEG_START_STACK         (0x53LL << 40)                  // 'S'

#define SIZE_CACHE              64                              // for struct border
#define SIZE_NORMAL_PAGE        (0x01LL << 12)                  // 4K
#define SIZE_HUGE_PAGE          (0x01LL << 21)                  // 2M
#define SIZE_THREAD_STACK       (0x01LL << 24)                  // 16M

#define NEG_SIZE_THREAD_STACK   (-1*SIZE_THREAD_STACK)
#define SIZE_SMALL_PAD          8

#define INT			long long int                   // basic type
#define BOOL                    bool
#define HANDLE_LOCK             long long int			//


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Error control                                                                                   //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define __RSTRING2(x)           #x
#define __RSTRING(x)            __RSTRING2(x)

#define __JOIN2(x,y)            x ## y
#define __JOIN(x,y)             __JOIN2(x,y)

#define __TO_MARK(x)            _rm_ ## x                       // used for ret_err control
#define _rm_MarkMax             0xffffffff                      // for mark end

#define	__TRY                   INT ret_err = __LINE__;
#define __MARK(x)               static int __TO_MARK(x) = ret_err = __LINE__;
#define __CATCH_BEGIN           return 0; error_stop:
#define __BETWEEN(x,y)          if (ret_err >= __TO_MARK(x) && ret_err <= __TO_MARK(y))
#define __AFTER(x)              if (ret_err >= __TO_MARK(x))
#define __CATCH_END             return ret_err;
#define	__CATCH                 return 0; error_stop: return ret_err;

#define __BREAK                 { goto error_stop; }
#define __BREAK_OK              { ret_err = 0; goto error_stop; }

#define MESSAGE_DEBUG           0x0001
#define MESSAGE_ERROR           0x0002
#define MESSAGE_HALT            0x0004

void    __MESSAGE(INT level, const char *_file, const char *_func, INT ret, const char * _Format, ...);

#define __INFO(level, _Format,...)					\
  { __MESSAGE(level, __FILE__, __FUNCTION__, __LINE__, _Format,##__VA_ARGS__); }
#define __INFOb(level, _Format,...)					\
  { __MESSAGE(level, __FILE__, __FUNCTION__, __LINE__, _Format,##__VA_ARGS__); \
    ret_err = 0; goto error_stop;}

#define __DOc_(func, _Format,...) {					\
    if (func)								\
      __INFO(MESSAGE_DEBUG, _Format,##__VA_ARGS__);			\
}
#define __DOc(func)             __DOc_(func, NULL)

#define __DO_(func, _Format,...) {					\
  if (func) {								\
    __INFO(MESSAGE_DEBUG, _Format,##__VA_ARGS__);			\
    __BREAK								\
  }									\
}
#define __DO(func)              __DO_(func, NULL)

#define __DOb_(func, _Format,...) {					\
  if (func) {								\
    __INFO(MESSAGE_DEBUG, _Format,##__VA_ARGS__);			\
  }									\
  __BREAK								\
}
#define __DOb(func)             __DOb_(func, NULL)

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Synchronous lock                                                                                //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define NOT_IN_PROCESS          (INT)0xa0
#define IN_PROCESS              (INT)0xa1

#define CmpExg                  __sync_bool_compare_and_swap

#define __LOCKp(lock)           while(!CmpExg(lock, NOT_IN_PROCESS, IN_PROCESS));
#define __FREEp(lock)           *lock = NOT_IN_PROCESS;
#define __LOCK(lock)            while(!CmpExg(&lock, NOT_IN_PROCESS, IN_PROCESS));
#define __FREE(lock)            lock = NOT_IN_PROCESS;

/*
#define IPPORT(addr)				inet_ntoa(addr.sin_addr), ntohs(addr.sin_port)
#define	DASZ(name)					name, sizeof(name)
#define	NASZ(name)					name, sizeof(name)-1

void inline SetAF_INET(sockaddr_in &addr, ULONG ip, USHORT port)
{
	addr.sin_family					= AF_INET;
	addr.sin_addr.s_addr			= htonl(ip);
	addr.sin_port					= htons(port);
}
void inline SetAF_INET(sockaddr_in &addr, PCHAR ip, USHORT port)
{
	addr.sin_family					= AF_INET;
	addr.sin_addr.s_addr			= inet_addr(ip);
	addr.sin_port					= htons(port);
}

char* memstr(char* sourstr, long size, char* substr, long subsize);
char* memstr_no(char* sourstr, long size, char* substr, long subsize, char nochar);
*/

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Common                                                                                          //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //

void    perrorexit(const char* s);
