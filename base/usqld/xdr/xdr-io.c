#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h> 
#include <netinet/in.h>
#include <string.h>
#include "xdr.h"

typedef struct XDR_io_membuf{
  XDR_io io;
  char * dat;
  size_t len;
  size_t pos;
  int free_dat;
}XDR_io_membuf;

typedef struct XDR_io_fd{
  XDR_io io;
  int fd;
}XDR_io_fd;



int XDR_io_fd_read(XDR_io * io, char * buf,size_t len){
  XDR_io_fd * iofd = (XDR_io_fd *) io;
  assert(iofd!=NULL);
  return read(iofd->fd,buf,len);
}

int XDR_io_fd_write(XDR_io * io, char * buf,size_t len){
  XDR_io_fd * iofd = (XDR_io_fd *) io;
  assert(iofd!=NULL);
  return write(iofd->fd,buf,len);
}

void XDR_io_fd_free(XDR_io * io){
  XDR_io_fd * iofd = (XDR_io_fd *) io;
  assert(iofd!=NULL);
  free(iofd);
}

int XDR_io_membuf_read(XDR_io * io, char * buf,size_t len){
  XDR_io_membuf * iomb = (XDR_io_membuf *) io;
  assert(iomb!=NULL);

  if(iomb->pos +len > iomb->len)
    return -1;
  else{
    memcpy(buf,iomb->dat +iomb->pos,len);
    iomb->pos +=len;
    return len;
  }
}

int XDR_io_membuf_write(XDR_io * io, char * buf,size_t len){
  XDR_io_membuf * iomb = (XDR_io_membuf *) io;
  assert(iomb!=NULL);

  if(iomb->pos +len > iomb->len)
    return -1;
  else{
    memcpy(iomb->dat +iomb->pos,buf,len);
    iomb->pos +=len;
    return len;
  }
}

void XDR_io_membuf_free(XDR_io * io){
  XDR_io_membuf * iomb = (XDR_io_membuf *) io;
  assert(iomb!=NULL);

  if(iomb->free_dat){
    free(iomb->dat);
    iomb->dat = NULL;
  }
  free(iomb);
}



XDR_io *XDR_io_bind_fd(int fd){
  XDR_io_fd *iofd;
  
  iofd = XDR_malloc(XDR_io_fd);
  assert(iofd!=NULL);
  bzero(iofd,sizeof(XDR_io_fd));
  
  iofd->io.read = XDR_io_fd_read;
  iofd->io.write = XDR_io_fd_write;
  iofd->io.free_io = XDR_io_fd_free;
  iofd->fd = fd;
  return (XDR_io *)  iofd;
}


XDR_io *XDR_io_bind_membuf(char *dat,size_t len,int free_dat){
  XDR_io_membuf * iomb;

  assert(dat!=NULL);
  assert(len>0);

  iomb = XDR_malloc(XDR_io_membuf);
  assert(iomb!=NULL);
  bzero(iomb,sizeof(XDR_io_membuf));
  
  iomb->io.read = XDR_io_membuf_read;
  iomb->io.write = XDR_io_membuf_write;
  iomb->io.free_io = XDR_io_membuf_free;
  iomb->dat = dat;
  iomb->len = len;
  iomb->pos = 0;
  iomb->free_dat = free_dat;
  return (XDR_io*) iomb;
}


int XDR_io_read(XDR_io*io,char *buf,size_t len){
  assert(io!=NULL);
  
  if(io->read)
    return io->read(io,buf,len);
  else
    return -1;
}

int XDR_io_write(XDR_io*io,char *buf,size_t len){
  assert(io!=NULL);
  
  if(io->write)
    return io->write(io,buf,len);
  else
    return -1;
}


void XDR_io_free(XDR_io*io){
  assert(io!=NULL);
 
  if(io->free_io)
    io->free_io(io);
}

