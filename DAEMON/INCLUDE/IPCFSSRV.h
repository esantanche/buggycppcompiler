



#define  IPCFS_OK                             0
#define  IPCFS_ERROR                          1
#define  IPCFS_SERVERNOTSTARTED               2
#define  IPCFS_CLIENTNOTSTARTED               3





/*
 * this function initializes the IPC.
 *
 *
 */
ULONG ipcServerBegin(void );



/*
 * this function writes into an IPC memory buffer
 *
 * pcBuf points to the buffers to copy from
 * ulBufLen is the length (number of bytes) to copy
 *
 */
ULONG ipcServerWrite( PCHAR pcBuf, ULONG ulBufLen );


/*
 * this function writes a first block into an IPC memory buffer
 *
 * pcBuf points to the buffer to copy from
 * ulBufLen is the length (number of bytes) to copy
 *
 */
ULONG ipcServerFirstWrite( PCHAR pcBuf, ULONG ulBufLen );


/*
 * this function appends another block to an IPC memory buffer
 *
 * pcBuf points to the buffer to copy from
 * ulBufLen is the length (number of bytes) to copy
 *
 */
ULONG ipcServerNextWrite( PCHAR pcBuf, ULONG ulBufLen );


/*
 * this function appends the last block to an IPC memory buffer
 *
 * pcBuf points to the buffer to copy from
 * ulBufLen is the length (number of bytes) to copy
 *
 */
ULONG ipcServerLastWrite( PCHAR pcBuf, ULONG ulBufLen );



/*
 * this function reads bytes from the IPC memory buffer
 *
 * pcBuf points to the buffers to copy to
 * pulBufLen points to the length (number of bytes) of the buffer copied
 *
 */
ULONG ipcServerRead( PCHAR pcBuf, PULONG pulBufLen );


/*
 * this function returns the number of bytes from the IPC memory buffer
 *
 * pcBuf points to the buffers to copy to
 * pulBufLen points to the length (number of bytes) of the buffer copied
 *
 */
ULONG ipcServerRead( PCHAR pcBuf, PULONG pulBufLen );



/*
 * this function ends the IPC.
 *
 *
 */
ULONG ipcServerEnd( void);



