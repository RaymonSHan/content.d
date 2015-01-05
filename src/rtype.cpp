#include "../include/rtype.hpp"

SINT strCmp(assignString &one, assignString &two)
{
  INT onelen, twolen, shortlen, shortlen8, i;
  ADDR oneaddr, twoaddr;

  onelen = one.strEnd - one.strStart;
  twolen = two.strEnd - two.strStart;
  shortlen = onelen < twolen ? onelen : twolen;
  shortlen8 = shortlen & (-1 * sizeof(INT));

  oneaddr = one.strStart;
  twoaddr = two.strStart;
  for (i=0; i<shortlen8; i+=sizeof(INT), oneaddr += 8, twoaddr += 8)
    if (*(oneaddr.pLong) != *(twoaddr.pLong)) break;
  if ((i == shortlen) && (i == shortlen8)) return (onelen - twolen);

  for (; i<shortlen; i++, oneaddr += 1, twoaddr += 1)
    if (*oneaddr.pChar != *twoaddr.pChar)
      return (*oneaddr.pChar - *twoaddr.pChar);

  return (onelen - twolen);
}

