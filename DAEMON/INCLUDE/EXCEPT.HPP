#ifndef ROOT_EXCEPTION
#define ROOT_EXCEPTION

#include <istring.hpp>

class RootException {

  protected:

   int     d_majorReason;
   int     d_minorReason;
   IString d_reasonString;

  public:

   RootException() :   d_majorReason(0), d_minorReason(0)       { }
   RootException(RootException& another)   {
           d_majorReason=another.d_majorReason;
           d_minorReason=another.d_minorReason;
           d_reasonString=another.d_reasonString;
   }
   RootException& operator=(RootException& another)  {
           d_majorReason=another.d_majorReason;
           d_minorReason=another.d_minorReason;
           d_reasonString=another.d_reasonString;
           return *this;
   }

   int      majorReason()                     { return d_majorReason; }
   int      minorReason()                     { return d_minorReason; }
   IString  reasonString()                    { return d_reasonString; }
   void  setReasonCodes(int mj, int min, IString const& rsc)
          { d_majorReason=mj; d_minorReason=min; d_reasonString=rsc; }

};


#endif


