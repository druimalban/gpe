#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>

#include <sys/ioctl.h>

static int 
uart_speed(int s)
{
  switch (s) {
  case 9600:
    return B9600;
  case 19200:
    return B19200;
  case 38400:
    return B38400;
  case 57600:
    return B57600;
  case 115200:
    return B115200;
#ifdef B230400
  case 230400:
    return B230400;
#endif
#ifdef B460800
  case 460800:
    return B460800;
#endif
#ifdef B921600
  case 921600:
    return B921600;
#endif
#ifdef B1000000
  case 1000000:
    return B1000000;
#endif
  default:
    return B57600;
  }
}

static int 
set_speed (int fd, struct termios *ti, int speed)
{
  cfsetospeed (ti, uart_speed(speed));
  return tcsetattr (fd, TCSANOW, ti);
}

int 
init_uart (char *dev, int speed)
{	
  struct termios ti;	
  int fd;

  fd = open (dev, O_RDWR | O_NOCTTY);
  if (fd < 0) 
    {
      perror("Can't open serial port");
      return -1;
    }

  tcflush (fd, TCIOFLUSH);
  
  if (tcgetattr (fd, &ti) < 0) 
    {
      perror ("Can't get port settings");
      return -1;
    }

  cfmakeraw(&ti);

  ti.c_cflag |= CLOCAL;
  
  if (tcsetattr (fd, TCSANOW, &ti) < 0) 
    {
      perror ("Can't set port settings");
      return -1;
    }
  
  tcflush (fd, TCIOFLUSH);
  
  /* Set actual baudrate */
  if (set_speed (fd, &ti, speed) < 0) 
    {
      perror("Can't set baud rate");
      return -1;
    }

  return fd;
}

int
main (int argc, char *argv[])
{
  struct timeval tv;
  int fd;
  fd_set readfds;

  if (argc < 3)
    {
      fprintf (stderr, "usage: %s <device> <speed>\n", argv[0]);
      exit (1);
    }

  fd = init_uart (argv[1], atoi (argv[2]));

  if (fd < 0)
    exit (1);

  tv.tv_sec = 3;
  tv.tv_usec = 0;

  FD_ZERO (&readfds);
  FD_SET (fd, &readfds);

  if (select (fd + 1, &readfds, NULL, NULL, &tv) > 0)
    exit (0);

  exit (2);
}
