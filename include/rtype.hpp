
#ifndef INCLUDE_RTYPE_HPP
#define INCLUDE_RTYPE_HPP

#include "raymoncommon.h"

class   CListItem;
/////////////////////////////////////////////////////////////////////////////////////////////////////
// the important struct, for my lazy                                                               //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define ADDR_INT_SELF_OPERATION(op)			\
  assignAddress& operator op (const INT &one) { this->aLong op one; return *this; };

#define ADDR_PCHAR_SELF_OPERATION(op)					\
  assignAddress& operator op (char *&one) { this->pChar op one; return *this; };

#define ADDR_PVOID_SELF_OPERATION(op)					\
  assignAddress& operator op (void *&one) { this->pVoid op one; return *this; };

typedef union assignAddress
{
  INT   aLong;
  char  *pChar;
  void  *pVoid;
  INT   *pLong;
  CListItem *pList;
  assignAddress *pAddr;

  ADDR_INT_SELF_OPERATION(=)
  ADDR_INT_SELF_OPERATION(+=)
  ADDR_INT_SELF_OPERATION(-=)
  ADDR_INT_SELF_OPERATION(&=)
  ADDR_INT_SELF_OPERATION(|=)

  ADDR_PCHAR_SELF_OPERATION(=)
  // One question i have not solved is: WHAT WRONG with the following line.
//ADDR_PVOID_SELF_OPERATION(=)

  assignAddress& operator = (void *&one) { this->pVoid = one; return *this; };
  assignAddress& operator = (CListItem *&one) { this->pList = one; return *this; };
  assignAddress& operator = (assignAddress *&one) { this->pAddr = one; return *this; };
}ADDR;

#define ADDR_ADDR_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, ADDR &two) {	\
  return (one.aLong op two.aLong); }
  ADDR_ADDR_COMPARE(==)
  ADDR_ADDR_COMPARE(!=)
  ADDR_ADDR_COMPARE(>)
  ADDR_ADDR_COMPARE(>=)
  ADDR_ADDR_COMPARE(<)
  ADDR_ADDR_COMPARE(<=)

#define ADDR_ADDR_OPERATION(op)				\
  ADDR inline operator op (ADDR &one, ADDR &two) {	\
  ADDR ret;						\
  ret.aLong = one.aLong op two.aLong;  return ret; }
  ADDR_ADDR_OPERATION(+)
  ADDR_ADDR_OPERATION(-)

#define ADDR_INT_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, const INT &two) {	\
  return (one.aLong op two); }
  ADDR_INT_COMPARE(==)
  ADDR_INT_COMPARE(!=)
  ADDR_INT_COMPARE(>)
  ADDR_INT_COMPARE(>=)
  ADDR_INT_COMPARE(<)
  ADDR_INT_COMPARE(<=)

#define ADDR_INT_OPERATION(op)				\
  ADDR inline operator op (ADDR &one, const INT &two) {	\
  ADDR ret;						\
  ret.aLong = one.aLong op two;  return ret; }
  ADDR_INT_OPERATION(+)
  ADDR_INT_OPERATION(-)
  ADDR_INT_OPERATION(&)
  ADDR_INT_OPERATION(|)

#define ADDR_INT_OPERATION2(op)				\
  INT inline operator op (ADDR &one, const INT &two) {	\
    return one.aLong op two; }
  ADDR_INT_OPERATION2(*)
  ADDR_INT_OPERATION2(/)

#define ADDR_PCHAR_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, char* &two) {	\
  return (one.pChar op two); }
  ADDR_PCHAR_COMPARE(==)
  ADDR_PCHAR_COMPARE(!=)
  ADDR_PCHAR_COMPARE(>)
  ADDR_PCHAR_COMPARE(>=)
  ADDR_PCHAR_COMPARE(<)
  ADDR_PCHAR_COMPARE(<=)

#define ADDR_PVOID_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, const void* &two){\
  return (one.pVoid op two); }
  ADDR_PVOID_COMPARE(==)
  ADDR_PVOID_COMPARE(!=)
  ADDR_PVOID_COMPARE(>)
  ADDR_PVOID_COMPARE(>=)
  ADDR_PVOID_COMPARE(<)
  ADDR_PVOID_COMPARE(<=)

#define ADDR_PLONG_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, INT* &two) {	\
  return (one.pLong op two); }
  ADDR_PLONG_COMPARE(==)
  ADDR_PLONG_COMPARE(!=)
  ADDR_PLONG_COMPARE(>)
  ADDR_PLONG_COMPARE(>=)
  ADDR_PLONG_COMPARE(<)
  ADDR_PLONG_COMPARE(<=)

/////////////////////////////////////////////////////////////////////////////////////////////////////
// pad size for border, stack must at 2^n border                                                   //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define PAD_SIZE(p, pad, bord)				\
  ((sizeof(p) + pad - 1) & (-1 * bord)) + bord
#define PAD_INT(p, pad, bord)				\
  ((p + pad - 1) & (-1 * bord)) + bord
#define PAD_ADDR(p, pad, bord)				\
  ((p.aLong + pad - 1) & (-1 * bord)) + bord

#endif  // INCLUDE_RTYPE_HPP
