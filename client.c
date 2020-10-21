/*
   信号包type值注释:

//给服务端发送的指令
1.注册  2.登录  3.客户端退出
4.登录退出  5.发送链表   6.更新心跳包

//从服务端接受的登录反馈信息
21.用户不存在
22.密码不正确
23.登录成功
24.用户已登录
 */


#include <stdio.h>
#include "client.h"

int main()
{

	int i,sockfd,ret,flag=1; 
	pthread_t pth;

	//signal()		定义信号SIGPIPE的触发动作，取代缺省动作
	signal(SIGPIPE,fun);

	//sock_init()	初始化tcp客户端套接字，并建立链接
	sockfd=sock_init(); 
	if(sockfd<0)
	{
		perror("套节字初始化失败!\n");
		return -1;
	}

	//pthread_create()	创建线程，用于发送心跳包
	pthread_create(&pth,NULL,(void *)jump_heart,(void *)&sockfd);
	//pthread_detach()	线程分离
	pthread_detach(pth);

	//主菜单
	while(flag)
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
		scanf("%d",&i);

		switch(i)
		{
			case 1:reg_info(sockfd);break;  
			case 2:enter_info(sockfd);break;
			case 3:back_info(sockfd);flag=0;break;
			default:printf("请重新输入\n");break;
		}
	}

	close(sockfd);//关闭套节字
	return 0;
}


//用户自定义SIGPIPE的触发动作
void fun(int a)
{
	printf("tcp broken!\n");
	exit(0);
}


//初始化并链接socket,建立tcp客户端
int sock_init()
{
	int sockfd,ret;
	struct sockaddr_in seraddr;
	char ip[128];
	int port;

	//过滤配置文件,获取服务端IP port的值
	get_ip_port(ip,&port);

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



//过滤配置文件,提取ip和port值
int get_ip_port(char *p,int *t)
{

	char ip[64];			//用于存放最终处理的IP
	int port;				//用于存放最终处理的端口
	int i;					//循环变量
	char temp[128];			//用于存放每次从文件中读取得一行数据
	char port1[128];		//用于存放初次处理后的port,需要再转换一次
	FILE *fp=NULL;

	fp =fopen("./sys.conf","r");
	if(fp == NULL)
	{
		printf("open error!\n");
		return -1;
	}

	while( fgets(temp,128,fp)!=NULL )
	{
		//提取ip值,fgets()读取每行的字符串时，在末尾有'\n'
		if(0==strncmp(temp,"ip",2))
		{
			//找到第一个是数字的字符，并将之后的所有字符赋值给ip
			for(i=0;i<strlen(temp);i++)
			{


				if(temp[i]>='0'&&temp[i]<='9')
					break;
			}
			strcpy(ip,&temp[i]);

			//将末尾的'\n'换成'\0'
			ip[strlen(ip)-1] = '\0';			
		}

		//提取port值
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

	fclose(fp);

	//将ip port值赋值给参数返回`
	strcpy(p,ip);
	*t=port;

	return 0;
}


//注册函数
void reg_info(int sockfd)
{
	int ret;
	HEAD *phead;			//节点指针
	USER user;
	char buff[128];			//接收服务器返回信息

	printf("请输入姓名:");
	scanf("%s",user.name);
	printf("请输入密码:");
	printf("\033[8m");
	scanf("%d",&user.code);
	printf("\033[0m");

	phead=malloc(sizeof(HEAD)+sizeof(USER));//申请信息包空间
	phead->type=1;
	phead->ver=0;
	phead->len=sizeof(USER);
	memcpy(phead->data,&user,sizeof(USER));

	ret=write(sockfd,phead,sizeof(HEAD)+sizeof(USER));//向服务器发送信息包
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
		printf("注册反馈:%s,按按任意键继续\n",buff);
		setbuf(stdin,NULL);
		getchar();
	}

	free(phead);
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

	//编辑发送内容
	printf("请输入姓名:");
	scanf("%s",user.name);
	printf("请输入密码:");
	printf("\033[8m");
	scanf("%d",&user.code);
	printf("\033[0m");

	//申请信息包空间
	p=malloc(sizeof(HEAD)+sizeof(USER));
	p->ver=0;
	p->type=2;
	p->len=sizeof(USER);
	memcpy(p->data,&user,sizeof(USER));

	//发送信息包
	ret=write(sockfd,p,sizeof(HEAD)+sizeof(USER));
	if(ret<0)
	{
		perror("write");
		return ;
	}

	//获取反馈信息
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
		if(pack.type==23)
		{
			printf("登录反馈:亲爱用户，欢迎登录!按空格继续\n");
			setbuf(stdin,NULL);
			getchar();
		}
		if(pack.type==24)
		{
			printf("登录反馈:用户已登录!按空格继续\n");
			setbuf(stdin,NULL);
			getchar();
			return;
		}
	}

	//接受服务器发来的链表
	create_link_cli(&head,sockfd);
	strcpy(mac.lname,user.name);
	mac.head=head;

	//创建线程,建立udp服务端
	ret=pthread_create(&pth,NULL,resv_info,(void *)&mac);
	if(ret<0)
	{
		perror("pthread_create");
		return;
	}
	//线程分离
	pthread_detach(pth);


	//聊天界面
	while(flag)
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
	HEAD *pnode=NULL;			//节点指针

	//申请信息包空间
	pnode=malloc(sizeof(HEAD));
	
	//编辑发送内容
	pnode->ver=0;
	pnode->type=5;
	pnode->len=0;

	//发送信息包
	ret=write(sockfd,pnode,sizeof(HEAD));
	if(ret<0)
	{
		perror("write");
		return;
	}

	//接受从服务器发来的最新链表
	create_link_cli(head,sockfd);
	show_link_cli(*head);

	printf("按空格返回!\n");
	setbuf(stdin,NULL);
	getchar();

	free(pnode);
	return;
}



//退出聊天菜单，让服务端将在线用户链表节点中的名字该成"未登录!"
void back_talk(int sockfd)
{
	int ret;
	HEAD *pnode=NULL;		//节点指针

	//申请信息包空间
	pnode=malloc(sizeof(HEAD));

	//编辑发送内容
	pnode->ver=0;
	pnode->type=4;
	pnode->len=0;


	//发送信息包
	ret=write(sockfd,pnode,sizeof(HEAD));
	if(ret<0)
	{
		perror("write");
		return;
	}

	sleep(2);
	free(pnode);
	return;
}


//单聊函数，建立udp客户端链接
int  one_one(char *user_name,NODE *head)
{
	int a;
	NODE *pn=head;				//移动指针
	char talk[128];
	MES *mes;
	int sockfd,ret;
	char buff[128];
	struct sockaddr_in seraddr;


	//打印当前接受的最新在线用户列表
	show_link_cli(head);				

	//输入聊天对象
	printf("[cli]:请输入你想聊天的对象:");
	setbuf(stdin,NULL);
	scanf("%s",talk);

	//判断用户是否在线,并找到聊天对象对应的节点信息
	while(pn!=NULL)
	{
		if(strcmp(pn->name,talk)==0)
		{
			break;
		}
		pn=pn->next;
	}
	if(pn==NULL)
	{
		printf("[cli];%s不在线,按任意键继续\n",talk);
		setbuf(stdin,NULL);
		getchar();
		return 0;
	}


	//socket()
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd<0)
	{
		perror("socket");
		return -1;
	}

	//编辑upd服务端地址信息
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(pn->udp_port);
	inet_pton(AF_INET,pn->ip,&seraddr.sin_addr.s_addr);

	//发送信息操作
	do
	{
		//申请信息包空间
		mes=malloc(sizeof(MES));

		//编辑发送内容
		printf("[cli]:please input massage:");
		setbuf(stdin,NULL);
		fgets(buff,128,stdin);
		strcpy(mes->name,user_name);
		strcpy(mes->data,buff);

		//sendto()     向udp服务端发送信息
		ret=sendto(sockfd,mes,sizeof(MES),0,(struct sockaddr *)&seraddr,sizeof(struct sockaddr_in));//发送信息包（包含发送方用户名及信息）
		if(ret>=0)
		{
			printf("[cli]:发送成功，按任意键继续\n");
			setbuf(stdin,NULL);
			getchar();

			printf("[cli]:是否继续发送(1/0):");
			setbuf(stdin,NULL);
			scanf("%d",&a);
		}
		else
		{
			printf("[cli]:发送失败，按任意键继续\n");
			setbuf(stdin,NULL);
			getchar();

			break;
		}

	}while(a==1);

	free(mes);
	close(sockfd);

	return 0;
}



//群聊函数,建立udp客户端链接
int  one_more(char *user_name,NODE *head)
{
	int flag=1;
	NODE *pn=head;								//移动指针
	char talk[128];
	MES *mes;
	int sockfd,ret;
	char buff[128];
	struct sockaddr_in seraddr;

	//打印当前接受的最新在线用户列表
	show_link_cli(head);

	//socket()
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd<0)
	{
		perror("socket");
		return -1;
	}

	while(flag)
	{ 
		//移动指针指向头节点
		pn=head;

		//申请信息包空间
		mes=malloc(sizeof(MES));

		//编辑发送信息
		printf("please input massage:");
		setbuf(stdin,NULL); 
		fgets(buff,128,stdin);
		strcpy(mes->name,user_name);
		strcpy(mes->data,buff);

		//循环遍历链表，将信息依次发给链表中登陆状态的用户
		while(pn!=NULL)
		{

			if(strcmp(pn->name,"未登录!")!=0)
			{
				//编辑当前pn指向的udp服务器的地址信息
				seraddr.sin_family=AF_INET;
				seraddr.sin_port=htons(pn->udp_port);
				inet_pton(AF_INET,pn->ip,&seraddr.sin_addr.s_addr);

				//向当前pn指向的服务器发送信息包
				ret=sendto(sockfd,mes,sizeof(MES),0,(struct sockaddr *)&seraddr,sizeof(struct sockaddr_in));
			}

			pn=pn->next;
		}

		printf("是否继续(1/0):");
		setbuf(stdin,NULL);
		scanf("%d",&flag);
	}

	free(mes);
	close(sockfd);

	return 0;
}


//打印在线用户列表
void show_link_cli(NODE *head)
{
	NODE *pn=NULL;			//移动指针

	pn=head;

	//循环便利链表，打印链表各节点信息
	printf("name:\tfd:\tip:\tport:\tudp_port\t\n\n");
	while(pn!=NULL)
	{
		printf("%s\t%d\t%s\t%d\t%d\t\n",pn->name,pn->fd,pn->ip,pn->port,pn->udp_port);
		pn=pn->next;
	}

}

//接受来自服务端的在线用户链表,注意新生成的链表节点与原链表顺序完全相反,采用的前叉法
void create_link_cli(PNODE *head,int sockfd)
{
	int ret;
	NODE *pnode=NULL;		//节点指针
	*head=NULL;				//头指针

	while(1)
	{
		//申请节点空间
		pnode=malloc(sizeof(NODE));

		//一个节点一个节点从socket管道中读取链表的内容
		ret=read(sockfd,pnode,sizeof(NODE));
		if(ret<0)
		{
			perror("read");
			return ;
		}

		//头节点前叉法，将节点组成链表
		if(pnode->next==NULL)
		{
			pnode->next=*head;
			*head=pnode;
			break;
		}
		else
		{
			pnode->next=*head;
			*head=pnode;
		}
	}

	return;
}

//创建线程,建立udp服务端
void *resv_info(void *q)
{
	int udp_sockfd;									
	int ret;
	char buff[128];
	MES *mes;										//udp信息包结构
	struct sockaddr_in udp_seraddr,udp_cliaddr;
	int udp_clilen=sizeof(struct sockaddr_in);
	char ip[128];
	MAC mac=*((MAC *)q);
	NODE *pn=mac.head;								//在线用户链表移动指针

	//找到当前登陆用户的节点
	while(pn!=NULL)
	{
		if(strcmp(pn->name,mac.lname)==0)
			break;
		pn=pn->next;
	}

	//socket()  udp链接
	udp_sockfd=socket(AF_INET,SOCK_DGRAM,0);
	if(udp_sockfd<0)
	{
		perror("socket");
		return NULL;
	}

	//bind()
	udp_seraddr.sin_family=AF_INET;
	udp_seraddr.sin_port=htons(pn->udp_port);
	udp_seraddr.sin_addr.s_addr=INADDR_ANY;
	ret=bind(udp_sockfd,(struct sockaddr *)&udp_seraddr,sizeof(struct sockaddr_in));
	if(ret<0)
	{
		perror("bind");
		return NULL;
	}


	while(1)
	{
		//申请接受信息包空间
		mes=malloc(sizeof(MES));

		//recvfrom()
		printf("[ser]:recvfrom...");
		ret=recvfrom(udp_sockfd,mes,sizeof(MES),0,(struct sockaddr *)&udp_cliaddr,&udp_clilen);
		if(ret>0)
		{
			printf("[ser]:recvfrom succeed!!!:[%s]:%s",mes->name,mes->data);
		}

	}
}


//客户端tcp断开链接,让服务器将对应节点从在线链表中删除
void back_info(int sockfd)
{
	HEAD *pnode;						//节点指针
	int ret;

	//申请发送信息包空间
	pnode=malloc(sizeof(HEAD));

	//编辑发送信息
	pnode->ver=0;
	pnode->type=3;
	pnode->len=0;

	//向tcp服务端发送信息
	ret=write(sockfd,pnode,sizeof(HEAD));
	if(ret<0)
	{
		perror("write");
		return ;
	}

	free(pnode);
}


