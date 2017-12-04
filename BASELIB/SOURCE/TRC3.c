//----------------------------------------------------------------------------
// TRC3.c
//----------------------------------------------------------------------------
// Questa funzione decodifica gli RC per gli errori su file piu' comuni
// Formato " Rc = ... (ErroreInChiaroSeNoto)"

// Gestione del trace
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3

// EMS001
typedef unsigned long BOOL;

#define MODULO_OS2BASE_DIPENDENTE
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS          // Per i mnemonici degli errori
// EMS002 tolgo define INCL_DOS che non serve e produce
// errori di compilazione sotto Windows
//#define INCL_DOS

//#define DBG   // Per debug : dati file (normalmente OK, si attiva solo ai livelli alti)
//#define DBG2  // Per debug : particolari della ricerca dicotomica ed altro
//#define DBG3  // Per debug : particolari della fase di sort/add del FILE_BS

#include <std.h>

char * _export DecodificaRcOS2(int Rc){
   #define RcEguale(a)           \
case a:         {                \
      static char Zs[] = #a;     \
      Target = Zs;               \
      break;       }
   char * Target = NULL;
   switch (Rc) {
      RcEguale(NO_ERROR                         );
      RcEguale(ERROR_INVALID_FUNCTION           );
      RcEguale(ERROR_FILE_NOT_FOUND             );
      RcEguale(ERROR_PATH_NOT_FOUND             );
      RcEguale(ERROR_TOO_MANY_OPEN_FILES        );
      RcEguale(ERROR_ACCESS_DENIED              );
      RcEguale(ERROR_INVALID_HANDLE             );
      RcEguale(ERROR_ARENA_TRASHED              );
      RcEguale(ERROR_NOT_ENOUGH_MEMORY          );
      RcEguale(ERROR_INVALID_BLOCK              );
      RcEguale(ERROR_BAD_ENVIRONMENT            );
      RcEguale(ERROR_BAD_FORMAT                 );
      RcEguale(ERROR_INVALID_ACCESS             );
      RcEguale(ERROR_INVALID_DATA               );
      RcEguale(ERROR_INVALID_DRIVE              );
      RcEguale(ERROR_CURRENT_DIRECTORY          );
      RcEguale(ERROR_NOT_SAME_DEVICE            );
      RcEguale(ERROR_NO_MORE_FILES              );
      RcEguale(ERROR_WRITE_PROTECT              );
      RcEguale(ERROR_BAD_UNIT                   );
      RcEguale(ERROR_NOT_READY                  );
      RcEguale(ERROR_BAD_COMMAND                );
      RcEguale(ERROR_CRC                        );
      RcEguale(ERROR_BAD_LENGTH                 );
      RcEguale(ERROR_SEEK                       );
      RcEguale(ERROR_NOT_DOS_DISK               );
      RcEguale(ERROR_SECTOR_NOT_FOUND           );
      RcEguale(ERROR_OUT_OF_PAPER               );
      RcEguale(ERROR_WRITE_FAULT                );
      RcEguale(ERROR_READ_FAULT                 );
      RcEguale(ERROR_GEN_FAILURE                );
      RcEguale(ERROR_SHARING_VIOLATION          );
      RcEguale(ERROR_LOCK_VIOLATION             );
      RcEguale(ERROR_WRONG_DISK                 );
      RcEguale(ERROR_FCB_UNAVAILABLE            );
      RcEguale(ERROR_SHARING_BUFFER_EXCEEDED    );
      RcEguale(ERROR_CODE_PAGE_MISMATCHED       );
      RcEguale(ERROR_HANDLE_EOF                 );
      RcEguale(ERROR_HANDLE_DISK_FULL           );
      RcEguale(ERROR_NOT_SUPPORTED              );
      RcEguale(ERROR_REM_NOT_LIST               );
      RcEguale(ERROR_DUP_NAME                   );
      RcEguale(ERROR_BAD_NETPATH                );
      RcEguale(ERROR_NETWORK_BUSY               );
      RcEguale(ERROR_DEV_NOT_EXIST              );
      RcEguale(ERROR_TOO_MANY_CMDS              );
      RcEguale(ERROR_ADAP_HDW_ERR               );
      RcEguale(ERROR_BAD_NET_RESP               );
      RcEguale(ERROR_UNEXP_NET_ERR              );
      RcEguale(ERROR_BAD_REM_ADAP               );
      RcEguale(ERROR_PRINTQ_FULL                );
      RcEguale(ERROR_NO_SPOOL_SPACE             );
      RcEguale(ERROR_PRINT_CANCELLED            );
      RcEguale(ERROR_NETNAME_DELETED            );
      RcEguale(ERROR_NETWORK_ACCESS_DENIED      );
      RcEguale(ERROR_BAD_DEV_TYPE               );
      RcEguale(ERROR_BAD_NET_NAME               );
      RcEguale(ERROR_TOO_MANY_NAMES             );
      RcEguale(ERROR_TOO_MANY_SESS              );
      RcEguale(ERROR_SHARING_PAUSED             );
      RcEguale(ERROR_REQ_NOT_ACCEP              );
      RcEguale(ERROR_REDIR_PAUSED               );
      RcEguale(ERROR_XGA_OUT_MEMORY             );
      RcEguale(ERROR_FILE_EXISTS                );
      RcEguale(ERROR_NET_WRITE_FAULT            );
      RcEguale(ERROR_INTERRUPT                  );
      RcEguale(ERROR_DEVICE_IN_USE              );
      RcEguale(ERROR_TOO_MANY_SEMAPHORES        );
      RcEguale(ERROR_EXCL_SEM_ALREADY_OWNED     );
      RcEguale(ERROR_SEM_IS_SET                 );
      RcEguale(ERROR_TOO_MANY_SEM_REQUESTS      );
      RcEguale(ERROR_SEM_OWNER_DIED             );
      RcEguale(ERROR_SEM_USER_LIMIT             );
      RcEguale(ERROR_DISK_CHANGE                );
      RcEguale(ERROR_DRIVE_LOCKED               );
      RcEguale(ERROR_BROKEN_PIPE                );
      RcEguale(ERROR_OPEN_FAILED                );
      RcEguale(ERROR_BUFFER_OVERFLOW            );
      RcEguale(ERROR_DISK_FULL                  );
      RcEguale(ERROR_SEM_TIMEOUT                );
      RcEguale(ERROR_INSUFFICIENT_BUFFER        );
      RcEguale(ERROR_INVALID_NAME               );
      RcEguale(ERROR_INVALID_LEVEL              );
      RcEguale(ERROR_NO_VOLUME_LABEL            );
      RcEguale(ERROR_SEEK_ON_DEVICE             );
      RcEguale(ERROR_BUSY_DRIVE                 );
      RcEguale(ERROR_BAD_PIPE                   );
      RcEguale(ERROR_PIPE_BUSY                  );
      RcEguale(ERROR_NO_DATA                    );
      RcEguale(ERROR_PIPE_NOT_CONNECTED         );
      RcEguale(ERROR_MORE_DATA                  );
      RcEguale(ERROR_TOO_MANY_HANDLES           );
      RcEguale(ERROR_TOO_MANY_OPENS             );
   };

   static char Out[256];
   if(Target == NULL){
      sprintf(Out," Rc = %u",Rc);
   } else {
      sprintf(Out," Rc = %u (%s)",Rc,Target);
   };
   return Out;
};
