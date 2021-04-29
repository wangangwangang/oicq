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

//设置udp端口
int uport=8000;


int main()
{
	int sockfd；
	int efd;					//epoll文件描述符
	int ret,count,i,temp;
	int port;					//存放服务器端口大小
	int cfd;
	struct epoll_event ev;			//epoll事件队列
	struct epoll_event evs[100];	//epoll响应事件队列
	struct sockaddr_in cliaddr;		//客户端地址结构
	int clilen=sizeof(struct sockaddr_in);	//客户端地址结构大小
	char ip[128];					//存放服务器ip地址
	NODE *head=NULL;			//在线用户链表
	HEAD *pack=NULL;			//
	pthread_t pth;				//线程标识符
	
	//自定义SIGPIPE的信号相应动作
	signal(SIGPIPE,hangle);
		
	//创建线程，每秒遍历一次在线用户链表，检查心跳因子是否为0
	pthread_create(&pth,NULL,(void *)listen_heart,(void *)&head);
	//线程回收
	pthread_detach(pth);

	//初始化socket套接字，建立tcp服务端
	sockfd=sock_init();
	if(sockfd<0)
	{
		printf("套节字初始化失败!\n");
		return -1;
	}
	
	//初始化epoll文件描述符
	efd=epoll_create(100);
	if(efd<0)
	{
		perror("epoll_create");
		return -1;
	}

	//将监听套接字加入队列
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
		//对ev队列中的文件描述符进行监听，将响应队列放入evs
		printf("epoll_wait....\n");
		count=epoll_wait(efd,evs,100,-1);
		if(count<0)
		{
			perror("epoll_wait");
			return -1;
		}
		printf("epoll_wait over...\n");
	    
		//对epoll的有响应的事件队列进行处理
		for(i=0;i<count;i++)
		{
			temp=evs[i].data.fd;
			if(temp==sockfd)
			{

				//accept() 			建立tcp链接
				printf("accept....\n");
				cfd=accept(sockfd,(struct sockaddr *)&cliaddr,&clilen);
				if(cfd<0)
				{
					perror("accept");

					continue;
				}

				//将已链接的客户端信息打印出来
				port=ntohs(cliaddr.sin_port);
				inet_ntop(AF_INET,&cliaddr.sin_addr.s_addr,ip,128);
				printf("已链接客户端[ip:%s|port:%d]",ip,port);
				
				//将cfd加入epoll队列，cfd用来监听各客户端是否有数据传送响应
				ev.events=EPOLLIN;
				ev.data.fd=cfd;
				ret=epoll_ctl(efd,EPOLL_CTL_ADD,cfd,&ev);
				if(ret<0)
				{
					perror("epoll_ctl");	
					continue;

				}

				//将刚链接的tcp客户端节点信息加入在线用户链表
				create_link_ser(&head,cfd,ip,port);
				
				continue;

			}
			else
			{
				//申请接受信息包空间
				pack=malloc(sizeof(HEAD));
				
				//读取包头
				ret=read(temp,pack,sizeof(HEAD));
				if(ret>0)
				{
					//根据包头中类型的信息，对读取的数据做不同的操作
					switch(pack->type)
					{
						case 1:reg(temp);break;							//注册
						case 2:enter(temp,&head);break;					//登陆
						case 3:del_link_ser(&head,temp);break;			//客户端退出
						case 4:back_enter(&head,temp);break;			//退出登录状态
						case 5:send_link_ser(&head,temp);break;			//发送链表
						case 6:jump_heart(&head,temp);					//更新心跳因子
					}

					free(pack);
				}
				else
				{

					if(ret<0)
						printf("read error\n");
					if(ret==0)
						printf("tcp broken!\n");
					
					//将已连接套接字cfd从epoll队列中删除
					ev.events=EPOLLIN;
					ev.data.fd=temp;
					ret=epoll_ctl(efd,EPOLL_CTL_DEL,temp,&ev);
					if(ret<0)
					{
						perror("epoll");
						continue;
					}

					//将已连接客户端节点信息从在线用户链表删除
					del_link_ser(&head,temp);
					
					close(temp);
				}
			}
		}
	}

	close(efd);
	close(sockfd);

	return 0;
}


//创建线程，每秒遍历一遍在线用户链表检查心跳包是否为0
void *listen_heart(void *head)
{
	NODE *pn=NULL;			//移动指针

	pn=*((PNODE *)head);

	while(1)
	{
		sleep(1);
		
		//如果clock不为0，每秒减1，如果为0，说明客户端断裂了
		while(pn!=NULL)
		{
			if((pn->clock)>0)
				(pn->clock)--;
			else
			{
				printf("%s已断开!\n",pn->ip);
				del_link_ser(head,pn->fd);
			}

			pn=pn->next;
		}
	}
}

//更新心跳因子，将clock元素赋值为9
void jump_heart(PNODE *head,int temp)
{
	NODE *pn=NULL;				//移动指针
	
	pn=*head;
	
	//找到temp文件描述符相应的节点，修改clock为9
	while(pn!=NULL)
	{
		if(pn->fd==temp)
			pn->clock=9;
		pn=pn->next;
	}
	return;
}

//设置后台运行
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

//退出登录状态，将节点中姓名设置为"未登录!",并设置udp port为0
void back_enter(PNODE *head,int temp)
{
	NODE *pn=NULL;				//移动指针
	
	pn=*head;

	//根据文件描述符temp找到对应节点
	while(pn!=NULL)
	{
		if(pn->fd==temp)
			break;
		pn=pn->next;
	}
	
	record(pn->name,"用户退出登录!");
	
	//更改未登录状态
	strcpy(pn->name,"未登录!");
	pn->udp_port=0;
}

//自定义SIGPIPE信号的响应操作
void hangle(int a)
{
	printf("tcp broken!\n");
	return;
}

//初始化socket套接字，建立tcp服务器
int sock_init()
{
	int sockfd;						//监听套接字
	int ret;
	struct sockaddr_in seraddr;		//服务端地址组结构
	char ip[128];					//服务端IP
	int port;						//服务端的port
	
	//从配置文件过滤ip port信息
	get_ip_port(ip,&port);

	//建立网络套接字
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		perror("socket");
		return -1;
	}

	//构建服务端地址组结构
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(port);           			//主机字节序转换为网络字节序后赋值
	inet_pton(AF_INET,ip,&seraddr.sin_addr.s_addr);		//主机字节序转换为网络字节序后赋值

	//绑定服务端地址
	ret=bind(sockfd,(struct sockaddr *)&seraddr,sizeof(struct sockaddr_in));
	if(ret<0)
	{
		perror("bind");
		return -1;
	}
	
	ret=listen(sockfd,5);
	if(ret<0)
	{
		perror("listen");
		return -1;
	}

	make_daemon();

	return sockfd;
}

//从配置文件过滤ip port信息
int get_ip_port(char *p,int *t)
{
	char ip[64];							//存放最终处理完的ip地址
	int port;								//存放最终处理完的port整型变量
	int i;									//循环变量
	char temp[128];							//存放fgets()每次从文件中读取的一行数据
	char port1[128];						//存放初次处理的port字符串
	FILE *fp=NULL;
	
	//fopen()    打开文件
	fp =fopen("./sys.conf","r");
	if(fp == NULL)
	{
		printf("open error!\n");
		return -1;
	}
	
	//fgets()   按行读取文件
	while( fgets(temp,128,fp)!=NULL )
	{
		//过滤ip 
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
		
		//过滤port
		if(0==strncmp(temp,"port",4))
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

	//fclose() 		关闭文件
	fclose(fp);
	
	//将ip port值赋给参数返回
	strcpy(p,ip);
	*t=port;

	return 0;
}

//链表增加节点
void create_link_ser(PNODE *head,int cfd,char *ipp,int portt)
{

	PNODE pnew=NULL;		//节点指针
	
	//申请节点内存
	pnew=malloc(sizeof(NODE));
	
	//初始化节点中各参数值
	pnew->fd=cfd;
	pnew->clock=9;
	strcpy(pnew->name,"未登录");
	strcpy(pnew->ip,ipp);
	pnew->port=portt;
	pnew->udp_port=0;
	
	//前叉法将节点插入链表的头部
	pnew->next=*head;
	*head=pnew;

	printf("%s已建立tcp链接\n",(*head)->ip);
}

//链表删除节点
void del_link_ser(PNODE *head,int temp)
{
	NODE *pn=*head;				//移动指针
	NODE *pnode=NULL;			//节点指针
	
	//申请节点内存
	pnode=malloc(sizeof(NODE));
	
	while(pn!=NULL)
	{
		//找到需要删除的节点
		if(pn->fd==temp)
		{
			//将节点空间备份,便于之后的空间释放
			pnode=pn;
			
			if(pn==*head)					//如果是头节点，则直接将头指针后移一个节点
			{
				*head=pn->next;
				free(pnode);
				break;
			}
			else							//如果不是头节点，则需要找到被删除节点的上一个节点
			{
				pn=*head;
				
				while(pn->next!=pnode)
					pn=pn->next;
				pn->next=pnode->next;
				
				free(pnode);
				break;
			}
		}
		
		pn=pn->next;
	}
}

//数据库初始化并建立链接
void mysql_init_connect(MYSQL *mysql)
{
	
	//mysql_init()		初始化
	if(NULL==mysql_init(mysql))
	{
		printf("%s\n",mysql_error(mysql));
		return;
	}

	//mysql_real_connect()		建立链接
	if(NULL==mysql_real_connect(mysql,"localhost","root","1qaz2wsx","user",0,NULL,0))
	{
		printf("连接数据库失败!\n");
		return ;
	}
	
	//mysql_set_character_set()		设置字符集识别中文
	mysql_set_character_set(mysql,"utf8");

	return;
}

//执行sql语句，如果是select查询语句，则将查询结果集返回
void mysql_go(MYSQL *mysql,char *sql,MYSQL_RES **resultt)
{
	int ret;
	MYSQL_RES *result;				//用于保存select的查询结果

	//mysql_query()			执行sql语句
	ret=mysql_query(mysql,sql);
	if(ret<0)
	{
		printf("执行查询命令失败!\n");
		return ;
	}

	//mysql_store_result()		获取select查询结果,如果执行的不是select desc等查询相关命令，则返回NULL
	result=mysql_store_result(mysql);
	if(result!=NULL)
		*resultt=result;
	
	return ;
}

//将用户的状态改为已登陆
void change_link_ser(PNODE *head,USER user,int temp)
{
	NODE *pn=*head;

	while(pn!=NULL)
	{
		if(pn->fd==temp)
		{
			//已登录状态的标志，有具体的姓名，udp_port不为0
			strcpy(pn->name,user.name);
			pn->udp_port=++uport;
			break;
		}
		else
		{
			pn=pn->next;
		}
	}

	return;
}

//将在线用户链表发送给客户端
void send_link_ser(PNODE *head,int temp)
{
	NODE *pn=*head;
	int ret;
	
	//将链表的节点依次发送给客户端
	while(pn!=NULL)
	{
		//write()
		ret=write(temp,pn,sizeof(NODE));
		if(ret<0)
		{
			perror("write");
			return;
		}
		
		pn=pn->next;
	}

	return;
}


//用户登陆函数
void enter(int temp,PNODE *head)
{
	USER user;
	NODE *pn=*head;
	int ret;
	int num_row;	
	char sql[128];
	MYSQL mysql;
	HEAD pack;
	MYSQL_RES *result;

	//read()		读取信息包的数据部分
	ret=read(temp,&user,sizeof(USER));
	if(ret<0)
	{
		perror("read");
		return ;
	}

	//初始化链接数据库
	mysql_init_connect(&mysql);

	//(1)判断数据库中是否有这个名字，也就是说是否注册
	sprintf(sql,"select * from  user_info   where name='%s'",user.name);
	mysql_go(&mysql,sql,&result);
	
	//获取查询结果的行数
	num_row=mysql_num_rows(result);
	if(num_row==0)
	{
		//将用户不存在信息返回
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
		
		//(2)判断密码是否正确
		sprintf(sql,"select * from user_info where name='%s'&&code=%d",user.name,user.code);
		mysql_go(&mysql,sql,&result);
		
		//获取查询结果的行数
		num_row=mysql_num_rows(result);
		if(num_row==0)
		{
			//将密码不正确信息返回
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
			//(3)查看是否已登录
			while(pn!=NULL)
			{
				if(0==strcmp(pn->name,user.name))
				{
					//将用户已登录的信息返回给客户端
					pack.type=24;
					ret=write(temp,&pack,sizeof(HEAD));
					if(ret<0)
					{
						perror("write");
						return ;
					}
					
					return;

				}
				
				pn=pn->next;
			}
			
			//更改登陆信息
			change_link_ser(head,user,temp);
			
			//将用户登录成功的信息发送给客户端
			pack.type=23;
			ret=write(temp,&pack,sizeof(HEAD));
			if(ret<0)
			{
				perror("write");
				return ;
			}
			
			//将在线用户链表发送给客户端
			send_link_ser(head,temp);
			
			//将登陆历史记录在文件中
			record(user.name,"用户登录!");
		}
	}
	
	//mysql_close()		关闭数据库
	mysql_close(&mysql);
	
	return;
}

//用户注册
void reg(int temp)
{
	USER user;   				//客户端用户信息         
	int ret;
	int row_num;				//
	MYSQL mysql;
	char sql[128];
	MYSQL_RES *result;

	//读取信息包数据部分
	ret=read(temp,&user,sizeof(USER));
	if(ret<0)
	{
		perror("read");
		return ;
	}
	
	//初始化并链接数据库
	mysql_init_connect(&mysql);
	
	//判断数据库中是否已有这个名字，也就是说有没有被注册
	sprintf(sql,"select * from user_info where name='%s'",user.name);
	mysql_go(&mysql,sql,&result);
	row_num=mysql_num_rows(result);
	if(row_num!=0)
	{
		//将用户已被注册返回给客户端
		write(temp,"用户已被注册!",sizeof("用户已被注册!"));
		return;
	}
	else
	{
		//将用户信息写入数据库
		sprintf(sql,"insert into user_info values ('%s',%d)",user.name,user.code);
		mysql_go(&mysql,sql,NULL);
		
		//将注册成功信息返回给客户端
		ret=write(temp,"用户注册成功!",sizeof("用户注册成功!"));
		if(ret<0)
		{
			perror("write");
			return ;
		}
	}
	
	//关闭数据库
	mysql_close(&mysql);
}

//打印在线用户链表
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

//将客户端登录，退出历史记录到文件中
int  record(char *name,char *stat)
{
	time_t t;
	struct tm *lt;
	FILE *fp=NULL;
	
	//获取当前时间,具体查询time() localtime()的使用方法
	time(&t);
	lt=localtime(&t);
	
	//打开文件
	fp=fopen("record.txt","a");
	if(fp==NULL)
	{
		printf("open file error!");
		return -1;
	}
	
	//将时间按照格式写入文件
	fprintf(fp,"%d-%d-%d %d:%d:%d    %s    %s\n",lt->tm_year+1900,
			lt->tm_mon+1,
			lt->tm_mday,
			lt->tm_hour,
			lt->tm_min,
			lt->tm_sec,name,stat
		   );
	
	//关闭文件
	fclose(fp);
	
	return 0;
}
