
#include "../include/raymoncommon.h"
#include "../include/rthread.hpp"

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
