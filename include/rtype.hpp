
#ifndef INCLUDE_RTYPE_HPP
#define INCLUDE_RTYPE_HPP

#include "raymoncommon.h"

class   CListItem;
class   CContentItem;
class   CBufferItem;
/////////////////////////////////////////////////////////////////////////////////////////////////////
// the important struct, for my lazy                                                               //
////////\///////////////////////\///////////////////////////////\//////////////////////////        //
#define ADDR_INT_SELF_OPERATION(op)					\
  void operator op (const INT &one) { this->aLong op (one); };

#define ADDR_PCHAR_SELF_OPERATION(op)					\
  void operator op (CHAR *&one) { this->pChar op (one); };

#define ADDR_PVOID_SELF_OPERATION(op)					\
  void operator op (void *&one) { this->pChar op ((CHAR*)(one)); };

typedef union assignAddress
{
  INT           aLong;
  PCHAR         pChar;
  PVOID         pVoid;
  PINT          pLong;
  CListItem     *pList;
  CContentItem  *pCont;
  CBufferItem   *pBuff;
  assignAddress *pAddr;

  ADDR_INT_SELF_OPERATION(=)
  ADDR_INT_SELF_OPERATION(+=)
  ADDR_INT_SELF_OPERATION(-=)
  ADDR_INT_SELF_OPERATION(&=)
  ADDR_INT_SELF_OPERATION(|=)

  ADDR_PCHAR_SELF_OPERATION(=)
  ADDR_PVOID_SELF_OPERATION(=)

  // assignAddress& operator = (void *&one) { this->pVoid = one; return *this; };
  void operator = (CListItem *&one) { this->pList = one; };
  void operator = (assignAddress *&one) { this->pAddr = one; };
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
  ADDR ret;					       	\
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
  ADDR_INT_OPERATION(*)
  ADDR_INT_OPERATION(/)

#define ADDR_INT_OPERATION2(op)				\
  INT inline operator op (ADDR &one, const INT &two) {	\
    return one.aLong op two; }

#define ADDR_PCHAR_COMPARE(op)					\
  BOOL inline operator op (ADDR &one, const CHAR* &two) {	\
  return (one.pChar op two); }
  ADDR_PCHAR_COMPARE(==)
  ADDR_PCHAR_COMPARE(!=)
  ADDR_PCHAR_COMPARE(>)
  ADDR_PCHAR_COMPARE(>=)
  ADDR_PCHAR_COMPARE(<)
  ADDR_PCHAR_COMPARE(<=)

#define ADDR_PVOID_COMPARE(op)					\
  BOOL inline operator op (ADDR &one, const void* &two){	\
  return (one.pVoid op two); }
  ADDR_PVOID_COMPARE(==)
  ADDR_PVOID_COMPARE(!=)
  ADDR_PVOID_COMPARE(>)
  ADDR_PVOID_COMPARE(>=)
  ADDR_PVOID_COMPARE(<)
  ADDR_PVOID_COMPARE(<=)

#define ADDR_PLONG_COMPARE(op)				\
  BOOL inline operator op (ADDR &one, PINT &two) {	\
  return (one.pLong op two); }
  ADDR_PLONG_COMPARE(==)
  ADDR_PLONG_COMPARE(!=)
  ADDR_PLONG_COMPARE(>)
  ADDR_PLONG_COMPARE(>=)
  ADDR_PLONG_COMPARE(<)
  ADDR_PLONG_COMPARE(<=)

typedef INT (*isThis)(ADDR);                                    // for detect is TRUE?
typedef union SOCKADDR
{
  sockaddr_in saddrin;
  sockaddr saddr;
}SOCKADDR;

typedef struct assignString
{
  PCHAR strStart;
  PCHAR strEnd;

  void operator = (assignString &one) {
    this->strStart = one.strStart;
    this->strEnd = one.strEnd;
  };
  void operator = (const PCHAR pchar) {
    this->strEnd = this->strStart = pchar;
    while (*this->strEnd++);
    this->strEnd --;
  };
  void operator = (const char *pchar) {
    return operator = ((PCHAR)pchar);
  };
}STRING;

SINT strCmp(assignString &one, assignString &two);

#define STR_STR_COMPARE(op)				\
  BOOL inline operator op (STRING one, STRING two)	\
  { return (strCmp(one, two) op 0); }
  STR_STR_COMPARE(==)
  STR_STR_COMPARE(!=)
  STR_STR_COMPARE(>=)
  STR_STR_COMPARE(<=)
  STR_STR_COMPARE(>)
  STR_STR_COMPARE(<)



#endif  // INCLUDE_RTYPE_HPP
