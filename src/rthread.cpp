// rthread.cpp
#include "../include/rthread.hpp"

// first page for pad
// second page for callnest info, which i fix the place
volatile INT RThreadResource::globalResourceOffset = SIZE_NORMAL_PAGE * 2;

RThreadResource::RThreadResource(INT size)
{
  nowOffset = LockAdd(RThreadResource::globalResourceOffset, PAD_INT(size, 0, 64));
}


