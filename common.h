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

//用户信息结构
typedef struct user
{
	char name[128];
	int code;
}USER,*PUSER;

//udp链接信息包结构
typedef struct  mes
{
  char name[1024];
  char data[1024];
}MES;

//tcp链接信息包结构
typedef struct pack_head
{
	unsigned int ver;
	unsigned int type; 
	int len;
	char data[0];
}HEAD,*PHEAD;

//在线用户列表结构
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

//作为pthread_create的参数传入线程
typedef struct source
{ 
  char lname[128];			//当前线程的用户
  NODE * head;				//在线用户链表头节点指针
}MAC,*PMAC;
