
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include "../include/raymoncommon.h"
#include "../include/rthread.hpp"

void perrorexit(const char* s)
{
  perror(s);
  exit(-1);
}

void __MESSAGE(INT level, const char * _Format, ...) 
{
  va_list ap;
  threadTraceInfo *info;

  if (!level) return;
  if (_Format) {
    va_start(ap, _Format);
    vprintf(_Format, ap);
    va_end(ap);
    printf("\n");
    displayTraceInfo(info);
  }
}		
