// rthread.cpp
#include "../include/rthread.hpp"

// first page for pad
// second page for callnest info, which i fix the place
volatile INT RThreadResource::globalResourceOffset = PAD_TRACE_INFO + sizeof(threadTraceInfo);

RThreadResource::RThreadResource(INT size)
{
  SetResourceOffset(size);
}

INT RThreadResource::SetResourceOffset(INT size)
{
  nowOffset = LockAdd(RThreadResource::globalResourceOffset, PAD_INT(size, 0, 64));
  return nowOffset;
}
