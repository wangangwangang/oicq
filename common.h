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
	char name[128];			//用户名称
	int code;				//用户密码
}USER,*PUSER;

//udp通信数据包结构
typedef struct  mes
{
  char name[1024];		//客户端名称
  char data[1024];		//数据
}MES;

//tcp通信数据包结构
typedef struct pack_head
{
	unsigned int ver;		//数据包类型
	unsigned int type; 		//接口标志
	int len;				//数据长度
	char data[0];			//数据
}HEAD,*PHEAD;

//在线用户列表结构
typedef struct online_node
{
	int fd;                 //客户端通信套接字
	int clock;				//心跳包计时器
	char name[128];			//客户端名称
	char ip[128];			//客户端IP地址
	int port;				//客户端TCP 端口
	int udp_port;			//客户端UDP端口
	struct online_node *next;	//节点链接指针
}NODE,*PNODE;

//作为pthread_create的参数传入线程
typedef struct source
{ 
  char lname[128];			//当前线程的用户
  NODE * head;				//在线用户链表头节点指针
}MAC,*PMAC;
