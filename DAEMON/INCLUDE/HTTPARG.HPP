#ifndef HTTP_ARGS
#define HTTP_ARGS

#include <istring.hpp>

#include <iptr.h>
#include <iseq.h>
#include <ieqseq.h>

// Class HttpField -------------------------------------------
class HttpField {

  public:

    HttpField()  { }

    HttpField(IString const& n, IString const& v) : d_name(n), d_value(v)
    { }

    void initFromEncStr(IString const& enc);

    HttpField(HttpField& another) :
         d_name(another.d_name), d_value(another.d_value)
    { }

    HttpField& operator=(HttpField& another)  {
         d_name=another.d_name; d_value=another.d_value;
         return *this;
    }

    Boolean operator==(HttpField const& another) const {
         return d_name==another.d_name;
    }

    void     setName(IString const& nm)  { d_name=nm; }
    void     setValue(IString const& vl) { d_value=vl; }
    
    IString& name()      { return d_name;  }
    IString& value()     { return d_value; }

    IString  asEncodedString();

  private:

    IString d_name, d_value;

    static  const IString  d_encodeGroupChars;

    IString encript(IString const& s, IString const& inGroup, Boolean SpaceToPlus);
    IString decript(IString const& s, Boolean SpaceToPlus);
};

typedef IMngElemPointer<HttpField>       HttpFieldPtr;
typedef IEqualitySequence<HttpFieldPtr>  HttpFieldList;

#endif

