#ifndef XDR_IO_H
#define XDR_IO_H

/**
   this is a very lax and simple IO abstraction which isn't really
   sufficient for much, it works for what i want.
 */


struct XDR_io;
typedef int (*xdr_io_read)(struct XDR_io* io,char * buf,size_t len);
typedef int (*xdr_io_write)(struct XDR_io* io,char * buf,size_t len);
typedef void (*xdr_io_free)(struct XDR_io* io);


typedef struct XDR_io{
  xdr_io_read read;
  xdr_io_write write;
  xdr_io_free free_io;
}XDR_io;

void XDR_io_free(struct XDR_io*);
int XDR_io_read(struct XDR_io*, char * buf, size_t len);
int XDR_io_write(struct XDR_io*, char * buf, size_t len);
#endif
