
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include "../include/raymoncommon.h"

void perrorexit(const char* s)
{
  perror(s);
  exit(-1);
}

void __MESSAGE(INT level, const char *_file, const char *_func, INT ret, const char * _Format, ...) 
{
  va_list ap;

  if (!level) return;
  if (_Format) {
    va_start(ap, _Format);
    vprintf(_Format, ap);
    va_end(ap);
  }
  printf(" errno:%d:%s\n  in file:%s, function:%s(), line:%lld\n", \
	 errno, strerror(errno), _file, _func, ret);
}		
