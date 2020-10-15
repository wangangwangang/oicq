/*
   信号包type值注释:
   
   //指令
   1.注册  2.登录  3.客户端退出
   4.登录退出  5.发送链表   6.更新心跳包

   //反馈信息
   21.用户不存在
   22.密码不正确
   23.登录成功
   24.用户已登录
*/


#include <stdio.h>
#include "client.h"

int main()
{
	
	//signal()
	signal(SIGPIPE,fun); //定义信号SIGPIPE 

	int i,sockfd,ret,flag=1; 
	pthread_t pth;

	//sock_init()
	sockfd=sock_init();  //初始化并链接socket
	if(sockfd<0)
	{
		perror("套节字初始化失败!\n");
		return -1;
	}

	//pthread_create()
	pthread_create(&pth,NULL,(void *)jump_heart,(void *)&sockfd);  //创建线程，用与发送心跳包
	//pthread_detach()
	pthread_detach(pth);//线程分离

	while(flag)
	{
		menu();
		scanf("%d",&i);

		switch(i)
		{
			case 1:reg_info(sockfd);break;  //注册信息函数
			case 2:enter_info(sockfd);break;//登录信息函数
			case 3:back_info(sockfd);flag=0;break;//退出客户端

			default:printf("请重新输入\n");break;
		}


	}

	close(sockfd);//关闭套节字
	return 0;
}


//用与定义SIGPIPE
void fun(int a)
{
	printf("tcp broken!\n");
	exit(0);
}


//初始化并链接socket
int sock_init()
{
	int sockfd,ret;
	struct sockaddr_in seraddr;
	char ip[128];
	int port;

	get_ip_port(ip,&port);//过滤配置文件

	//socket()
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		perror("socket");
		return -1;
	}

	//connect()
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(port);
	inet_pton(AF_INET,ip,&seraddr.sin_addr.s_addr);
	ret=connect(sockfd,(struct sockaddr *)&seraddr,sizeof(struct sockaddr_in));
	if(ret<0)
	{
		perror("connect");
		return -1;
	}

	return sockfd;
}

//发送心跳包,每隔三秒让服务端更新心跳包
void *jump_heart(void *p)
{
	int fd=*((int *)p);
	int ret;
	HEAD *pack=NULL;

	while(1)
	{
		pack=(HEAD *)malloc(sizeof(HEAD));
		pack->type=6;
		sleep(3);
		ret=write(fd,pack,sizeof(HEAD));
		if(ret<0)
		{
			perror("write");
			return NULL;
		}
	}
}



//过滤配置文件
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


//主菜单
void menu()
{
	system("clear");
	printf("**********************************\n");
	printf("*             \033[33m OICQ\033[0m              *\n");
	printf("**********************************\n");
	printf("*            \033[36m1.注册\033[0m              *\n");
	printf("*            \033[36m2.登录\033[0m              *\n");
	printf("*            \033[36m3.退出\033[0m              *\n");
	printf("**********************************\n");
	printf("请选择:");
}


//注册函数
void reg_info(int sockfd)
{
	int ret;
	HEAD *p;
	USER user;
	char buff[128];

	printf("请输入姓名:");
	scanf("%s",user.name);
	printf("请输入密码:");
	printf("\033[8m");
	scanf("%d",&user.code);
	printf("\033[0m");

	p=malloc(sizeof(HEAD)+sizeof(USER));//申请信息包空间
	p->ver=0;
	p->type=1;
	p->len=sizeof(USER);
	memcpy(p->data,&user,sizeof(USER));

	ret=write(sockfd,p,sizeof(HEAD)+sizeof(USER));//向服务器发送信息包
	if(ret<0)
	{
		perror("write");
		return ;
	}

	ret=read(sockfd,buff,sizeof(buff));//读取注册反馈信息
	if(ret<0)
	{
		perror("read");
		return ;
	}
	else
	{
		buff[ret]='\0';
		printf("注册反馈:%s,按空格继续\n",buff);
		setbuf(stdin,NULL);
		getchar();
	}

	free(p);
	return;
}


//登录函数
void enter_info(int sockfd)
{
	pthread_t pth;
	int ret,i;
	int flag=1;
	HEAD *p=NULL;
	USER user;
	NODE *q=NULL;
	NODE *head=NULL;
	MAC mac;
	char buff[1024];
	HEAD pack;

	printf("请输入姓名:");
	scanf("%s",user.name);
	printf("请输入密码:");
	printf("\033[8m");
	scanf("%d",&user.code);
	printf("\033[0m");

	p=malloc(sizeof(HEAD)+sizeof(USER));//申请信息包空间
	p->ver=0;
	p->type=2;
	p->len=sizeof(USER);
	memcpy(p->data,&user,sizeof(USER));

	ret=write(sockfd,p,sizeof(HEAD)+sizeof(USER));//发送信息包
	if(ret<0)
	{
		perror("write");
		return ;
	}

	ret=read(sockfd,&pack,sizeof(HEAD));
	if(ret<0)
	{
		perror("read");
		return;
	}
	else
	{
		if(pack.type==21)
		{
			printf("登录反馈:用户不存在!按空格继续\n");
			setbuf(stdin,NULL);
			getchar();
			return;
		}
		if(pack.type==22)
		{
			printf("登录反馈:用户密码不正确!按空格继续\n");
			setbuf(stdin,NULL);
			getchar();
			return;
		}
		if(pack.type==24)
		{
			printf("登录反馈:用户已登录!按空格继续\n");
			setbuf(stdin,NULL);
			getchar();
			return;
		}
		if(pack.type==23)
		{
			printf("登录反馈:亲爱用户，欢迎登录!按空格继续\n");
			setbuf(stdin,NULL);
			getchar();
		}
	}

	create_link_cli(&head,sockfd);//接受服务器发来的链表
	//	show_link_cli(head);//打印链表
	/*
	   q=head;
	   while(q!=NULL)
	   {
	   if(strcmp(user.name,q->name)==0)
	   {
	   break;
	   }
	   q=q->next;
	   }
	 */
	strcpy(mac.lname,user.name);
	mac.node=head;

	ret=pthread_create(&pth,NULL,resv_info,(void *)&mac);//创建线程接受其他客户端发来的信息
	if(ret<0)
	{
		perror("pthread_create");
		return;
	}
	pthread_detach(pth);//线程分离
	while(flag)
	{
		menu_talk();//聊天菜单
		scanf("%d",&i);
		switch(i)
		{
			case 1:one_one(user.name,head);break;//单聊函数
			case 2:one_more(user.name,head);break;//群聊函数
			case 3:back_talk(sockfd);flag=0;break;//退出聊天函数
			case 4:accept_link(&head,sockfd);break;//更新服务器发来的在线用户列表
		}
	}
	free(p);
	return ;
}


//更新服务器发来的在线用户列表
void accept_link(PNODE *head,int sockfd)
{
	int ret;
	HEAD *p=NULL;
	p=malloc(sizeof(HEAD));//申请包空间

	p->ver=0;
	p->type=5;
	p->len=0;
	ret=write(sockfd,p,sizeof(HEAD));//发送信息包
	if(ret<0)
	{
		perror("write");
		return;
	}


	create_link_cli(head,sockfd);//接受从服务器发来的最新列表
	show_link_cli(*head);
	printf("按空格返回!\n");
	setbuf(stdin,NULL);
	getchar();

	free(p);
	return;
}



//退出聊天菜单
void back_talk(int sockfd)
{
	int ret;
	HEAD *p=NULL;
	p=malloc(sizeof(HEAD));//申请包空间

	p->ver=0;
	p->type=4;
	p->len=0;
	ret=write(sockfd,p,sizeof(HEAD));//发送信息包
	if(ret<0)
	{
		perror("write");
		return;
	}

	sleep(2);
	free(p);
	return;
}


//单聊函数
int  one_one(char *user_name,NODE *head)
{
	NODE *p=head;
	char talk[128];
	show_link_cli(head);//打印当前接受的最新在线用户列表
	MES *mes;
	printf("请输入你想聊天的对象:");
	setbuf(stdin,NULL);
	scanf("%s",talk);

	while(p!=NULL)
	{
		if(strcmp(p->name,talk)==0)
		{
			break;
		}
		p=p->next;
	}

	int sockfd,ret;
	char buff[128];
	struct sockaddr_in seraddr;

	sockfd=socket(AF_INET,SOCK_DGRAM,0);//建立UDP连接，初始化套节字
	if(sockfd<0)
	{
		perror("socket");
		return -1;
	}
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(p->udp_port);
	inet_pton(AF_INET,p->ip,&seraddr.sin_addr.s_addr);
	int a;

	//发送信息操作
	do
	{
		mes=malloc(sizeof(MES));//申请信息包
		setbuf(stdin,NULL);
		printf("please input massage:");
		fgets(buff,128,stdin);
		strcpy(mes->name,user_name);
		strcpy(mes->data,buff);
		ret=sendto(sockfd,mes,sizeof(MES),0,(struct sockaddr *)&seraddr,sizeof(struct sockaddr_in));//发送信息包（包含发送方用户名及信息）
		printf("send over...\n");
		setbuf(stdin,NULL);
		sleep(1);
		free(mes);
		printf("是否继续发送(1/0):");
		scanf("%d",&a);
	}while(a==1);

	close(sockfd);
	return 0;
}



//群聊函数
int  one_more(char *user_name,NODE *head)
{
	NODE *p=head;
	char talk[128];
	MES *mes;
	show_link_cli(head);//打印当前接受的最新在线用户列表
	int sockfd,ret;
	char buff[128];
	struct sockaddr_in seraddr;
	sockfd=socket(AF_INET,SOCK_DGRAM,0);//建立UDP连接，初始化套节字
	if(sockfd<0)
	{
		perror("socket");
		return -1;
	}
	int flag=1,a;
	while(flag)
	{ 
		p=head;
		mes=malloc(sizeof(MES));
		setbuf(stdin,NULL); 
		printf("please input massage:");
		fgets(buff,128,stdin);
		strcpy(mes->name,user_name);
		strcpy(mes->data,buff);
		while(p!=NULL)
		{

			if(strcmp(p->name,"未登录!")!=0)

			{
				seraddr.sin_family=AF_INET;
				seraddr.sin_port=htons(p->udp_port);
				inet_pton(AF_INET,p->ip,&seraddr.sin_addr.s_addr);

				ret=sendto(sockfd,mes,sizeof(MES),0,(struct sockaddr *)&seraddr,sizeof(struct sockaddr_in));
			}
			p=p->next;
		}

		printf("是否继续(1/0):");
		setbuf(stdin,NULL);
		scanf("%d",&a);
		flag=a;
		free(mes);
	}
	close(sockfd);
	return 0;
}


//打印在线用户列表
void show_link_cli(NODE *head)
{
	NODE *p=NULL;
	p=head;

	printf("name:\tfd:\tip:\tport:\tudp_port\t\n\n");
	while(p!=NULL)
	{
		printf("%s\t%d\t%s\t%d\t%d\t\n",p->name,p->fd,p->ip,p->port,p->udp_port);
		p=p->next;
	}

	return;
}


//聊天菜单
void menu_talk()
{
	system("clear");
	printf("*******************************\n");
	printf("*         \033[33m聊天界面\033[0m            *\n");
	printf("*******************************\n");
	printf("*         \033[36m1.单聊\033[0m              *\n");
	printf("*         \033[36m2.群聊\033[0m              *\n");
	printf("*         \033[36m3.退出\033[0m              *\n");
	printf("*         \033[36m4.刷新好友列表\033[0m      *\n");
	printf("*******************************\n");
	printf("请选择:");
}


//接受
void create_link_cli(PNODE *head,int sockfd)
{
	int ret;
	NODE *pnew=NULL;
	*head=NULL;

	while(1)
	{
		pnew=malloc(sizeof(NODE));
		ret=read(sockfd,pnew,sizeof(NODE));
		if(ret<0)
		{
			perror("read");
			return ;
		}

		if(pnew->next==NULL)
		{
			pnew->next=*head;
			*head=pnew;
			break;
		}

		pnew->next=*head;
		*head=pnew;
	}

	return;
}


void *resv_info(void *q)
{
	int udp_sockfd,ret;
	char buff[128];
	MES *mes;
	MAC ptr=*((MAC *)q);
	NODE *p=ptr.node;

	while(p!=NULL)
	{
		if(strcmp(p->name,ptr.lname)==0)
			break;
		p=p->next;
	}

	struct sockaddr_in udp_seraddr,udp_cliaddr;
	int udp_clilen=sizeof(struct sockaddr_in);
	char ip[128];

	udp_sockfd=socket(AF_INET,SOCK_DGRAM,0);
	if(udp_sockfd<0)
	{
		perror("socket");
		return NULL;
	}

	udp_seraddr.sin_family=AF_INET;
	udp_seraddr.sin_port=htons(p->udp_port);
	udp_seraddr.sin_addr.s_addr=INADDR_ANY;

	ret=bind(udp_sockfd,(struct sockaddr *)&udp_seraddr,sizeof(struct sockaddr_in));
	if(ret<0)
	{
		perror("bind");
		return NULL;
	}
	while(1)
	{  
		mes=malloc(sizeof(MES));
		//	printf("recv...\n");
		ret=recvfrom(udp_sockfd,mes,sizeof(MES),0,(struct sockaddr *)&udp_cliaddr,&udp_clilen);
		//	printf("recv over...\n");
		printf("\n[%s]:%s\n",mes->name,mes->data);

	}
	return NULL;
}

void back_info(int sockfd)
{
	HEAD *p;
	int ret;

	p=malloc(sizeof(HEAD));
	p->ver=0;
	p->type=3;
	p->len=0;

	ret=write(sockfd,p,sizeof(HEAD));
	if(ret<0)
	{
		perror("write");
		return ;
	}
	free(p);
	return ;
}


