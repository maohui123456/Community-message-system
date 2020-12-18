#ifndef PROJECT_H_
#define PROJECT_H_
#include "myhead.h"

typedef struct client_node
{
    int fd;     //  客户端在服务器端的文件描述符
    int port;   //  客户端的端口号
    char IP[32];    //客户端的IP
    struct client_node * next;
    struct client_node * prev;
} * NODE;

char * get_whether();
void get_friend(int fd);
int send_to(char * client_buf , int fd);
int send_file(char * client_buf , NODE send_host);
#endif