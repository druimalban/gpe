#ifndef XDR_H
#define XDR_H
#include <sys/types.h>
#define XDR_OK 0
#define XDR_PMALLOC 1
#define XDR_SCHEMA_VIOLATION 2
#define XDR_IO_ERROR 3
#define XDR_UNSUPPORTED 4
typedef enum {
  XDR_INT,
  XDR_UINT,
  XDR_ENUM,
  XDR_BOOL,
  XDR_HYPER,
  XDR_UHYPER,
  XDR_FLOAT,
  XDR_DOUBLE,
  XDR_LONGDOUBLE,
  XDR_STRING,
  XDR_FIXEDOPAQUE,
  XDR_VAROPAQUE,
  XDR_FIXEDARRAY,
  XDR_VARARRAY,
  XDR_STRUCT,
  XDR_UNION,
  XDR_VOID
}XDR_type;

#include "xdr-tree.h"
#include "xdr-schema.h"
#include "xdr-io.h"

int  XDR_deserialize_elem(XDR_schema *s,XDR_io *io, XDR_tree **out_t);
int  XDR_serialize_elem(XDR_schema *s, XDR_tree *t,XDR_io * io);
const char * XDR_find_type_name(XDR_type t);

#endif 
