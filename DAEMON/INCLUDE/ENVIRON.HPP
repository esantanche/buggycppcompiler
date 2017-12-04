
#ifndef FSENVIRONMENT_HPP
#define FSENVIRONMENT_HPP

#include <iostream.h>
#include <fstream.h>
#include <istring.hpp>
#include <stdlib.h>

#include <string.h>

#ifdef AIX_ENVIRONMENT
#include <strings.h>
#define  stringCaseCompare(a, b)     (strcasecmp(a, b))
#define HTTPDCONF_DIR       "/etc"
#define HTTPDCONF_FILENAME  "/fscgi.conf"
#endif

#ifdef OS2_ENVIRONMENT
#include <process.h>
#define  stringCaseCompare(a, b)     (strcmpi(a, b))
#define HTTPDCONF_DIR       "c:\\fsconfig"
#define HTTPDCONF_FILENAME  "\\fscgi.cnf"
#endif

#include <iptr.h>
#include <iseq.h>
#include <ieqseq.h>

#include "fscgidef.hpp"
#include "hexcept.hpp"
#include "httpenv.hpp"

// ---> Not used: in case you've gotta change
//                all the 'elementAt' statements prepending a '*'!
//      typedef ISequence<IStringMngPtr>             IStringList;

typedef ISequence<IString>             IStringList;


class FSCGIEnvironment {

  public:

   FSCGIEnvironment();    // Sets defaults for the various fields
                          // and initialize the HttpdEnvironment

   void getConfiguration(char *configFileFullName=NULL);

   friend ostream& operator<<(ostream& os, FSCGIEnvironment& cEnv);

   // The configuration file will be found in this Directory.
   // This variable gets set to the /fsconfig value.
   void setFSConfigDir(IString& valStr)              { d_FSConfigDir=valStr; }

   void setDetailDirectory(IString& cfgLine)         { d_detailDirectory=cfgLine.word(2).strip(); }
   void setDetailErrorFile(IString& cfgLine)         { d_detailErrorFile=cfgLine.word(2).strip(); }
   void setQueryTemplate(IString& cfgLine)           { d_queryTemplate=cfgLine.word(2).strip(); }
   void setQueryAixForm(IString& cfgLine)            { d_queryAixForm=cfgLine.word(2).strip(); }

   void     addQueryUrls(IString& cfgLine);
   IString  randQueryUrl();

   IString  queryTemplate()                          { return d_queryTemplate; }
   IString  detailFile()                             { return d_detailFile; }
   IString  detailErrorFile()                        { return d_detailErrorFile; }

   HttpdEnvironment& httpdEnvironment()              { return d_httpEnv; }

  private:

   IString          d_queryAixForm;
   IString          d_FSConfigDir;

   IString          d_detailDirectory;
   IString          d_detailFile;        // Dedotto da PathInfo!
   IString          d_detailErrorFile;
   IString          d_queryTemplate;
   IStringList      d_queryUrls;

   HttpdEnvironment d_httpEnv;
};

#endif

