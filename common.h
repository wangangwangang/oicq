#pragma once 
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <string.h>
#include <pthread.h>
typedef struct user
{
	char name[128];
	int code;
}USER;

typedef struct pack_head
{
	unsigned int ver;
	unsigned int type; 
	int len;
	char data[0];
}HEAD;

typedef struct online_node
{
	int fd;
	int clock;
	char name[128];
	char ip[128];
	int port;
	int udp_port;
	struct online_node *next;
}NODE,*PNODE;

typedef struct source
{ 
  char lname[128];
  NODE * node;
}MAC,*PMAC;

typedef struct  mes
{
  char name[1024];
  char data[1024];
}MES;
