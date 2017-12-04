
#ifndef HTTP_EXCEPTION
#define HTTP_EXCEPTION

#include "except.hpp"

class HttpdException : public RootException {

  public:

   enum majorErrorCode  { documentFollows=200, found=302,        notModified=304,
                          badRequest=400,      unauthorized=401, forbidden=403,
                          notFound=404,        serverError=500,  notImplemented=501 };

   // Basic Output like: Status: 500 Server Error\012\012
   virtual void coutBody()  {
         IString temp("500 Server Error");

         cout << "Status: " << temp << '\012';
         cout << "Content-type: text/html" << '\012' << '\012';

         cout << "<HEAD><TITLE>" << temp  << "</TITLE></HEAD>" << '\012';
         cout << "<BODY><H1>" << temp << "</H1>" << '\012';
         cout << "Major: " << d_majorReason << "; Minor: " << d_minorReason << "; " << d_reasonString;
         cout << "</BODY>" << '\012';
   }

};

#endif

