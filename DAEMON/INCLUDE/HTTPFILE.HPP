#ifndef HTTPFILE_HPP
#define HTTPFILE_HPP

#include <iostream.h>
#include <fstream.h>

#include <istring.hpp>

class HttpFile {

    IString d_fname;
    int     d_size, d_exist, d_bytesSent;

  public:

    HttpFile(IString const& fName);
 
    ostream& coutHeader(ostream& os);
    ostream& coutBody(ostream& os);
    Boolean  exist();

};

#endif

