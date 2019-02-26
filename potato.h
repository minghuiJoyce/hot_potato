#ifndef __POTATO_H__
#define __POTATO_H__
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct _potato {
  int hops;
  int id;
  int trace[512];
};
typedef struct _potato potato;

#endif
