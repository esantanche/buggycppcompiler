#ifndef HTTPD_ENVIRONMENT
#define HTTPD_ENVIRONMENT

#include <iostream.h>
#include <istring.hpp>

// Enum Definitions ----------------------------------------------
enum HttpdServer  { IBM_ICS };
enum HttpdMethods { HTTP_UNKNOWN, HTTP_GET, HTTP_POST, HTTP_HEAD };

#include "httparg.hpp"

// Class HttpdEnvironment --------------------------------------
class HttpdEnvironment {

 public:

   HttpdEnvironment(HttpdServer server);
   IString formFieldValue(IString const& fieldName);
   ostream& coutObject(ostream& os, Boolean noHeader=true);

   IString& pathInfo()        { return d_pathInfo; }

   // Friend class for outputting the values of the enviroment!
   friend ostream& operator<<(ostream& os, HttpdEnvironment &httpObj);

 private:

   HttpdMethods    d_method;

   IString         d_remoteName;
   IString         d_userAgent;
   IString         d_documentRoot;
   IString         d_scriptName;

   IString         d_pathInfo;

   IString         d_queryString;
   HttpFieldList   d_formFields;

   static HttpdMethods methodLookUp(IString& met);
};

#endif

