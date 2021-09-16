/*
   信号包注释:

   1.注册  2.登录  3.客户端退出
   4.登录退出  5.发送链表   6.更新心跳包

   21.用户不存在
   22.密码不正确
   23.登录成功
   24.用户已登录
 */
#include <stdio.h>
#include "server.h"

int uport=8000;


int main()
{
	//signal()
	signal(SIGPIPE,hangle);
	
	int sockfd,efd;
	int ret,count,i,temp,port,cfd;
	struct epoll_event ev,evs[100];
	struct sockaddr_in cliaddr;
	int clilen=sizeof(struct sockaddr_in);
	char ip[128];
	NODE *head=NULL;
	HEAD *pack=NULL;
	pthread_t pth;

	//pthread_create()
	pthread_create(&pth,NULL,(void *)listen_heart,(void *)&head);
	//pthread_detach()
	pthread_detach(pth);

	//epoll_create()
	efd=epoll_create(100);
	if(efd<0)
	{
		perror("epoll_create");
		return -1;
	}

	//sock_init()
	sockfd=sock_init();
	if(sockfd<0)
	{
		printf("套节字初始化失败!\n");
		return -1;
	}
	
	//epoll_ctl()
	ev.events=EPOLLIN;
	ev.data.fd=sockfd;
	ret=epoll_ctl(efd,EPOLL_CTL_ADD,sockfd,&ev);
	if(ret<0)
	{
		perror("epoll_ctl");
		return -1;
	}

	while(1)
	{
		//epoll_wait()
		printf("epoll_wait....\n");
		count=epoll_wait(efd,evs,100,-1);
		if(count<0)
		{
			perror("epoll_wait");
			return -1;
		}
		printf("epoll_wait over...\n");

		for(i=0;i<count;i++)
		{
			temp=evs[i].data.fd;
			if(temp==sockfd)
			{

				//accept()
				printf("accept....\n");
				cfd=accept(sockfd,(struct sockaddr *)&cliaddr,&clilen);
				if(cfd<0)
				{
					perror("accept");

					continue;
				}
				port=ntohs(cliaddr.sin_port);
				inet_ntop(AF_INET,&cliaddr.sin_addr.s_addr,ip,128);
				printf("已链接客户端[ip:%s|port:%d]",ip,port);
				ev.events=EPOLLIN;
				ev.data.fd=cfd;
				ret=epoll_ctl(efd,EPOLL_CTL_ADD,cfd,&ev);
				if(ret<0)
				{

					perror("epoll_ctl");	
					continue;

				}
				create_link_ser(&head,cfd,ip,port);
				continue;

			}
			else
			{
				pack=malloc(sizeof(HEAD));
				//read()
				ret=read(temp,pack,sizeof(HEAD));
				if(ret<=0)
				{

					if(ret<0)
						printf("read error\n");
					if(ret==0)
						printf("tcp broken!\n");
					ev.events=EPOLLIN;
					ev.data.fd=temp;
					ret=epoll_ctl(efd,EPOLL_CTL_DEL,temp,&ev);
					if(ret<0)
					{
						perror("epoll");




						continue;
					}

					del_link_ser(&head,temp);
					close(temp);
				}
				else
				{
					switch(pack->type)
					{
						case 1:reg(temp);break;
						case 2:enter(temp,&head);break;
						case 3:del_link_ser(&head,temp);break;
						case 4:back_enter(&head,temp);break;
						case 5:send_link_ser(&head,temp);break;
						case 6:jump_heart(&head,temp);
					}
					free(pack);
				}
			}
		}
	}
	close(efd);
	close(sockfd);
}



void *listen_heart(void *head)
{
	NODE *p=NULL;
	p=*((PNODE *)head);
	while(1)
	{
		sleep(1);

		while(p!=NULL)
		{
			if((p->clock)>0)
				(p->clock)--;
			else
			{
				printf("%s已断开!\n",p->ip);
				del_link_ser(head,p->fd);
			}
			p=p->next;
		}
	}
}

void jump_heart(PNODE *head,int temp)
{
	NODE *p=NULL;
	p=*head;

	while(p!=NULL)
	{
		if(p->fd==temp)
			p->clock=9;
		p=p->next;
	}
	return;
}

void make_daemon()
{
	pid_t pid;
	if((pid=fork())!=0)
		exit(0);
	setsid();//(设置对话)
	signal(SIGHUP,SIG_IGN);
	// signal(SIGCHLD,sig_routine);
	if((pid=fork())!=0)
		exit(0);
	// daemon_flag=1;
	//  chdir("/");
	umask(0);
	//	fclose(stdout);
}

void back_enter(PNODE *head,int temp)
{
	NODE *p=NULL;
	p=*head;

	while(p!=NULL)
	{
		if(p->fd==temp)
			break;
		p=p->next;
	}
	record(p->name,"用户退出登录!");
	strcpy(p->name,"未登录!");
	p->udp_port=0;

	return;
}


void hangle(int a)
{
	printf("tcp broken!\n");
	return;
}

int sock_init()
{
	int sockfd,ret;
	struct sockaddr_in seraddr;
	char ip[128];
	int port;

	get_ip_port(ip,&port);

	//socket()
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		perror("socket");
		return -1;
	}

	//bind()
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(port);
	inet_pton(AF_INET,ip,&seraddr.sin_addr.s_addr);
	ret=bind(sockfd,(struct sockaddr *)&seraddr,sizeof(struct sockaddr_in));
	if(ret<0)
	{
		perror("bind");
		return -1;
	}

	//listen()
	ret=listen(sockfd,5);
	if(ret<0)
	{
		perror("listen");
		return -1;
	}

	make_daemon();

	return sockfd;
}

int get_ip_port(char *p,int *t)
{
	char ip[64];
	int port,i;
	char temp[128];
	char port1[128];

	FILE *fp=NULL;
	fp =fopen("./sys.conf","r");
	if(fp == NULL)
	{

		printf("open error!\n");
		return -1;
	}

	while(NULL != fgets(temp,128,fp))
	{

		if(0==strncmp(temp,"ip",2))
		{

			for(i=0;i<strlen(temp);i++)
			{

				if(temp[i]>='0'&&temp[i]<='9')
					break;
			}
			strcpy(ip,&temp[i]);
			ip[strlen(ip)-1] = '\0';
		}
		else if(0==strncmp(temp,"port",4))
		{

			for(i=0;i<strlen(temp);i++)
			{
				if(temp[i]>='0'&&temp[i]<='9')
					break;
			}
			strcpy(port1,&temp[i]);
			port1[strlen(port1)-1] = '\0';
			port = atoi(port1);
		}
	}
	fclose(fp);
	strcpy(p,ip);
	*t=port;
	return 0;
}

void create_link_ser(PNODE *head,int cfd,char *ipp,int portt)
{

	PNODE pnew=NULL;
	pnew=malloc(sizeof(NODE));
	pnew->fd=cfd;
	pnew->clock=9;
	strcpy(pnew->name,"未登录");
	strcpy(pnew->ip,ipp);
	pnew->port=portt;
	pnew->udp_port=0;
	pnew->next=*head;
	*head=pnew;

	printf("ip:%s\n",(*head)->ip);
}

void del_link_ser(PNODE *head,int temp)
{
	NODE *p=NULL;
	p=*head;
	NODE *ptr=NULL;
	ptr=malloc(sizeof(NODE));
	while(p!=NULL)
	{
		if(p->fd==temp)
		{
			ptr=p;
			if(p==*head)
			{
				*head=p->next;
				free(ptr);
				break;
			}
			else
			{
				NODE *q=*head;
				while(q->next!=p)
					q=q->next;
				q->next=p->next;
				free(ptr);
				break;
			}
		}
		p=p->next;
	}

}

void mysql_init_connect(MYSQL *mysql)
{
	if(NULL==mysql_init(mysql))
	{
		printf("%s\n",mysql_error(mysql));
		return;
	}

	if(NULL==mysql_real_connect(mysql,"localhost","root","1qaz2wsx","user",0,NULL,0))
	{
		printf("连接数据库失败!\n");
		return ;
	}

	mysql_set_character_set(mysql,"utf8");

	return;
}

void mysql_go(MYSQL *mysql,char *sql,MYSQL_RES **resultt)
{
	int ret;
	MYSQL_RES *result;

	ret=mysql_query(mysql,sql);
	if(ret<0)
	{
		printf("执行查询命令失败!\n");
		return ;
	}

	result=mysql_store_result(mysql);
	if(result!=NULL)
		*resultt=result;
	return ;
}

void change_link_ser(PNODE *head,USER user,int temp)
{
	NODE *p=*head;

	while(p!=NULL)
	{
		if(p->fd==temp)
		{
			strcpy(p->name,user.name);
			p->udp_port=++uport;
			break;
		}
		p=p->next;
	}

	return;
}

void send_link_ser(PNODE *head,int temp)
{
	NODE *p=*head;
	int ret;

	while(p!=NULL)
	{
		ret=write(temp,p,sizeof(NODE));
		if(ret<0)
		{
			perror("write");
			return;
		}
		p=p->next;
	}

	return;
}



void enter(int temp,PNODE *head)
{
	USER user;
	int ret,num_row;
	char sql[128];
	MYSQL mysql;
	HEAD pack;
	MYSQL_RES *result;


	ret=read(temp,&user,sizeof(USER));
	if(ret<0)
	{
		perror("read");
		return ;
	}

	sprintf(sql,"select * from  user_info   where name='%s'",user.name);

	mysql_init_connect(&mysql); 
	mysql_go(&mysql,sql,&result);
	num_row=mysql_num_rows(result);
	if(num_row==0)
	{
		pack.type=21;
		ret=write(temp,&pack,sizeof(HEAD));
		if(ret<0)
		{
			perror("write");
			return ;
		}

		return ;
	}
	else
	{
		sprintf(sql,"select * from user_info where name='%s'&&code=%d",user.name,user.code);
		mysql_go(&mysql,sql,&result);
		num_row=mysql_num_rows(result);
		if(num_row==0)
		{
			pack.type=22;
			ret=write(temp,&pack,sizeof(HEAD));
			if(ret<0)
			{
				perror("write");

				return ;
			}

			return ;
		}
		else
		{
			NODE *ps=*head;
			while(ps!=NULL)
			{
				if(0==strcmp(ps->name,user.name))
				{
					pack.type=24;
					ret=write(temp,&pack,sizeof(HEAD));
					if(ret<0)
					{
						perror("write");
						return ;
					}
					return;

				}
				ps=ps->next;
			}
			pack.type=23;
			ret=write(temp,&pack,sizeof(HEAD));
			if(ret<0)
			{
				perror("write");
				return ;
			}
			record(user.name,"用户登录!");
			change_link_ser(head,user,temp);
			send_link_ser(head,temp);
		}
	}
	mysql_close(&mysql);
	return;
}


void reg(int temp)
{
	USER user;
	int ret,row_num;
	MYSQL mysql;
	char sql[128];
	MYSQL_RES *result;

	ret=read(temp,&user,sizeof(USER));
	if(ret<0)
	{
		perror("read");
		return ;
	}
	sprintf(sql,"select * from user_info where name='%s'",user.name);
	mysql_init_connect(&mysql);
	mysql_go(&mysql,sql,&result);
	row_num=mysql_num_rows(result);
	if(row_num!=0)
	{
		write(temp,"用户已被注册!",sizeof("用户已被注册!"));
		return;
	}
	else
	{
		sprintf(sql,"insert into user_info values ('%s',%d)",user.name,user.code);
		mysql_go(&mysql,sql,NULL);
		ret=write(temp,"用户注册成功!",sizeof("用户注册成功!"));
		if(ret<0)
		{
			perror("write");
			return ;
		}
	}
	mysql_close(&mysql);
}


void show_link_ser(NODE *head)
{
	NODE *p=head;
	printf("name:\tfd:\tip:\tport:\tudp_port\t\n\n");
	while(p!=NULL)
	{
		printf("%s\t%d\t%s\t%d\t%d\t\n",p->name,p->fd,p->ip,p->port,p->udp_port);
		p=p->next;
	}
	return;
}

int  record(char *name,char *stat)
{
	time_t t;
	struct tm *lt;
	time(&t);
	lt=localtime(&t);
	FILE *fp=NULL;
	fp=fopen("record.txt","a");
	if(fp==NULL)
	{
		printf("open file error!");
		return -1;
	}
	fprintf(fp,"%d-%d-%d %d:%d:%d    %s    %s\n",lt->tm_year+1900,
			lt->tm_mon+1,
			lt->tm_mday,
			lt->tm_hour,
			lt->tm_min,
			lt->tm_sec,name,stat
		   );
	fclose(fp);
	return 0;
}
