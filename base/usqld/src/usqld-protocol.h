#ifndef USQLD_PROTOCOL_H
#define USQLD_PROTOCOL_H

#define PICKLE_CONNECT 0x1
#define PICKLE_DISCONNECT 0x2
#define PICKLE_QUERY 0x3
#define PICKLE_ERROR 0x4
#define PICKLE_OK 0x5
#define PICKLE_STARTROWS 0x6
#define PICKLE_ROW 0x7
#define PICKLE_EOF 0x8
#define PICKLE_MAX 0x9

typedef XDR_tree usqld_packet;

XDR_schema * usqld_get_protocol();
int usqld_recv_packet(int fd,usqld_packet ** packet);
int usqld_send_packet(int fd,usqld_packet* packet);

int usqld_get_packet_type(usqld_packet *packet);



#endif
