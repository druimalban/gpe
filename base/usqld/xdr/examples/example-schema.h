#ifndef EXAMPLE_SCHEMA_H
#define EXAMPLE_SCHEMA_H

#define PICKLE_CONNECT 0x0
#define PICKLE_DISCONNECT 0x1
#define PICKLE_QUERY 0x2
#define PICKLE_ERROR 0x3
#define PICKLE_OK 0x4
#define PICKLE_STARTROWS 0x5
#define PICKLE_ROW 0x6
#define PICKLE_EOF 0x7
#define PICKLE_MAX 0x8
XDR_schema * get_schema();
#endif
