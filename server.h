#pragma once

#include "common.h"
#include <mysql/mysql.h>
#include <time.h>

void hangle(int a);

int sock_init();

void create_link_ser(PNODE *head,int cfd,char *ipp,int portt);

void del_link_ser(PNODE *head,int temp);

void mysql_init_connect(MYSQL *mysql);

void mysql_go(MYSQL *mysql,char *sql,MYSQL_RES **resultt);

void change_link_ser(PNODE *head,USER user,int temp);

void send_link_ser(PNODE *head,int temp);


void enter(int temp,PNODE *head);

void reg(int temp);

void show_link_ser(NODE *head);

void back_enter(PNODE *head,int temp);

int  record(char *name,char *stat);

void make_daemon();

void jump_heart(PNODE *head,int temp);

void *listen_heart(void *head);

int  get_ip_port(char *p,int *t);
