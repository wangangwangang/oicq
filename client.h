#pragma once
#include "common.h"

void fun(int a);

int sock_init();

void reg_info(int sockfd);

void enter_info(int sockfd);

int  one_one(char *user_name,NODE *head);
int  one_more(char *user_name,NODE *head);

void show_link_cli(NODE *head);


void menu_talk();

void create_link_cli(PNODE *head,int sockfd);


void *resv_info(void *p);

void back_talk(int sockfd);

void back_info(int sockfd);

void accept_link(PNODE *head,int socfd);

void *jump_heart(void *p);

int get_ip_port(char *p,int *t);
