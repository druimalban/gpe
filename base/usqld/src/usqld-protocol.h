#ifndef USQLD_PROTOCOL_H
#define USQLD_PROTOCOL_H

#include <xdr.h>
#include <usqld-network.h>
#define PICKLE_CONNECT 1
#define PICKLE_DISCONNECT 2
#define PICKLE_QUERY 3
#define PICKLE_ERROR 4
#define PICKLE_OK 5
#define PICKLE_STARTROWS 6
#define PICKLE_ROW 7
#define PICKLE_EOF 8
#define PICKLE_MAX 9
#define PICKLE_INTERRUPT 10
#define PICKLE_INTERRUPTED 11
#define PICKLE_ROWID 12
#define PICKLE_REQUEST_ROWID 13

#define USQLD_SERVER_PORT 8322

#define USQLD_OK 0 
#define USQLD_ERRBASE 1024
#define USQLD_ALREADY_OPEN (USQLD_ERRBASE + 1)
#define USQLD_NOT_OPEN (USQLD_ERRBASE + 2)
#define USQLD_VERSION_MISMATCH (USQLD_ERRBASE + 3)
#define USQLD_UNSUPPORTED  (USQLD_ERRBASE + 4)
#define USQLD_PROTOCOL_ERROR  (USQLD_ERRBASE + 5)


XDR_schema * usqld_get_protocol();


usqld_packet * usqld_error_packet(int errcode, const char * str);
usqld_packet * usqld_ok_packet();
usqld_packet * usqld_rowid_packet(int rowid);
int usqld_get_packet_type(usqld_packet *packet);


#endif
