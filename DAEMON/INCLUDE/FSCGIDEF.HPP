
#ifndef FSCGI_GLOBALDEF_HPP
#define FSCGI_GLOBALDEF_HPP

#ifdef OS2_ENVIRONMENT
#define CHARACTER_SLASH      '\\'
#define STRING_SLASH         "\\"
#endif

#ifdef AIX_ENVIRONMENT
#define CHARACTER_SLASH      '/'
#define STRING_SLASH         "/"
#endif

#ifndef _BooleanType
#define _BooleanType
typedef int Boolean;
#endif

#endif

