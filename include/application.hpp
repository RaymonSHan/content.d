#ifndef INCLUDE_APPLICATION_HPP
#define INCLUDE_APPLICATION_HPP

#include "rtype.hpp"

class CApplication
{
public:
  CApplication() {};
  virtual INT DoApplication(ADDR mContent) = 0;
};

#define DefineApplication(app)				\
  class app : public CApplication			\
  {							\
  public:						\
    app();						\
    virtual INT DoApplication(ADDR mContent);		\
  };




DefineApplication(CEchoApplication);
DefineApplication(CWriteApplication);

#endif  // INCLUDE_APPLICATION_HPP
