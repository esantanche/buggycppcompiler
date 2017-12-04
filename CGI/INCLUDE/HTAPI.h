/* Do we need pragma linkage on OE? */
#ifndef HTAPI_H
#define HTAPI_H

/* Return values from user-written functions */
#define HTTP_NOACTION              0     /* Other functions including the server's will be executed */
#define HTTP_OK                  200     /* No other functions of that class will be executed       */
#define HTTP_CREATED             201
#define HTTP_ACCEPTED            202
#define HTTP_NO_CONTENT          204
#define HTTP_MULTIPLE_CHOICES    300
#define HTTP_MOVED_PERMANENTLY   301
#define HTTP_MOVED_TEMPORARILY   302
#define HTTP_NOT_MODIFIED        304
#define HTTP_BAD_REQUEST         400
#define HTTP_UNAUTHORIZED        401
#define HTTP_FORBIDDEN           403
#define HTTP_NOT_FOUND           404
#define HTTP_PROXY_UNAUTHORIZED  407
#define HTTP_SERVER_ERROR        500
#define HTTP_NOT_IMPLEMENTED     501
#define HTTP_BAD_GATEWAY         502
#define HTTP_SERVICE_UNAVAILABLE 503

/* Return values from ICS provided functions */
#define HTTPD_UNSUPPORTED        -1
#define HTTPD_SUCCESS             0
#define HTTPD_FAILURE             1
#define HTTPD_INTERNAL_ERROR      2
#define HTTPD_PARAMETER_ERROR     3
#define HTTPD_STATE_CHECK         4
#define HTTPD_READ_ONLY           5
#define HTTPD_BUFFER_TOO_SMALL    6
#define HTTPD_AUTHENTICATE_FAILED 7
#define HTTPD_EOF                 8

/* prototypes for ICS provided functions */

#if defined(__OS2__)
#define HTTPD_LINKAGE _System
#elif defined(WIN32)
//#define HTTPD_LINKAGE __stdcall  // EMS
#define HTTPD_LINKAGE
#else
#define HTTPD_LINKAGE
#endif

/* Long name to short name mapping */
#define HTTPD_authenticate   HTAUTHEN
#define HTTPD_extract        HTXTRACT
#define HTTPD_set            HTSET
#define HTTPD_file           HTFILE
#define HTTPD_exec           HTEXEC
#define HTTPD_read           HTREAD
#define HTTPD_write          HTWRITE
#define HTTPD_log_error      HTLOGE
#define HTTPD_restart        HTREST

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* authenticate a userid/password                */
/* valid only in PreExit and Authorization steps */
void
HTTPD_LINKAGE
HTTPD_authenticate(
             unsigned char *handle,  /* i; handle (NULL right now) */
             long *return_code);     /* o; return code */

/* extract the value of a variable associated with this request */
/* the available variables are the same as CGI                  */
/* valid in all steps, though not all variables will be         */
void
HTTPD_LINKAGE
HTTPD_extract(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *name,         /* i; name of the value to extract */
    unsigned long *name_length,  /* i; length of the name */
    unsigned char *value,        /* o; buffer in which to place the value */
    unsigned long *value_length, /* i/o; size of buffer/length of value */
             long *return_code); /* o; return code */

/* set the value of a variable associated with this request     */
/* the available variables are the same as CGI                  */
/* - note that one can create variables with this function      */
/* - if created variables are prefixed by "HTTP_", they will    */
/*   be sent as headers in the response, without the "HTTP_"    */
/*   prefix                                                     */
/* valid in all steps, though not all variables are             */
void
HTTPD_LINKAGE
HTTPD_set(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *name,         /* i; name of the value to set */
    unsigned long *name_length,  /* i; length of the name */
    unsigned char *value,        /* o; buffer which contains the value */
    unsigned long *value_length, /* i; length of value */
             long *return_code); /* o; return code */

/* send a file to satisfy this request */
/* valid in PreExit and Service steps  */
void
HTTPD_LINKAGE
HTTPD_file(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *name,         /* i; name of file to send */
    unsigned long *name_length,  /* i; length of the name */
             long *return_code); /* o; return code */

/* execute a script to satisfy this request */
/* valid in PreExit and Service steps       */
void
HTTPD_LINKAGE
HTTPD_exec(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *name,         /* i; name of script to execute */
    unsigned long *name_length,  /* i; length of the name */
             long *return_code); /* o; return code */

/* read the body of the client's request - use set/extract for headers */
/* Keep reading until HTTPD_EOF is returned; 4k is a good buffer size  */
/* valid only in the PreFilter and Service steps                       */
void
HTTPD_LINKAGE
HTTPD_read(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *value,        /* i; buffer in which to place the value */
    unsigned long *value_length, /* i/o; size of buffer/length of header */
             long *return_code); /* o; return code */

/* write the body of the response - use set/extract for headers */
/* valid only in the Sservice and PostFilter steps              */
void
HTTPD_LINKAGE
HTTPD_write(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *value,        /* i; data to send */
    unsigned long *value_length, /* i; length of the data */
             long *return_code); /* o; return code */

/* write a string to the server's error log */
void
HTTPD_LINKAGE
HTTPD_log_error(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *value,        /* i; data to write */
    unsigned long *value_length, /* i; length of the data */
             long *return_code); /* o; return code */

/* restart the server after all active requests have been processed */
void
HTTPD_LINKAGE
HTTPD_restart(
             long *return_code); /* o; return code */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HTAPI_H */
