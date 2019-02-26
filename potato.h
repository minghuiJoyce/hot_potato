#ifndef __POTATO_H__
#define __POTATO_H__
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>


struct _potato{
  int hops;
  int id;
  int trace[512];
};
typedef struct _potato potato;


#endif