#include "project.h"

#define HOST "192.168.19.234"
#define PORT 6666
pthread_t TCP_id , UDP_id;

int flag;

int tcp_socket;
int udp_socket;

#define REFLASH 1
#define SEND_MSG 2
#define SEND_FILE 3

NODE head;
//把在线的好友根据客户端的fd发给客户端
void get_friend(int fd)
{
    NODE por = head->next;
    write(fd , "好友列表:\n" , strlen("好友列表:\n"));
    while(por != head)
    {
        char buf[1024];
        sprintf(buf , "%s\n" , por->IP);
        write(fd , buf , strlen(buf));
        por = por->next;
    }
}
//一个客户端给一个指定的客户端发送消息
int send_to(char * client_buf , int fd)
{
    char * buf = client_buf;
    char send_ip[32];
    char * p;
    char send_msg[1024];
    //跳过send to 
    buf+=8;
    //获得需要发送的消息
    p = strpbrk(buf , " ");
    p+=1;
    strcpy(send_msg , p);
    //获得需要发送的ip地址
    strtok(buf, " ");
    strcpy(send_ip , buf);
    printf("send_ip = %s\n" , send_ip);
    printf("send_msg = %s\n" , send_msg);
    NODE por = head->next;
    while(por != head)
    {
        if(strstr(por->IP , send_ip) != NULL)
        {
            write(por->fd , send_msg , strlen(send_msg));
            return 0;
        }
        por = por->next;
    }
    write(fd , "找不到这个好友或者他没有上线,发送reflash刷新好友列表\n" , strlen("找不到这个好友或者他没有上线,发送reflash刷新好友列表\n"));
    return -1  ;
}

int send_file(char * client_buf , struct client_node * send_host)
{
    char * buf = client_buf;
    char send_ip[32];
    char * p;
    char send_path[1024];
    //跳过send to 
    buf+=10;
    //获得需要发送的文件地址
    p = strpbrk(buf , " ");
    p+=1;
    strcpy(send_path , p);
    //获得需要发送的ip地址
    strtok(buf, " ");
    strcpy(send_ip , buf);
    printf("send_ip = %s\n" , send_ip);
    printf("send_msg = %s\n" , send_path);
    NODE por = head->next;
    while(por != head)
    {
        if(strstr(por->IP , send_ip) != NULL)
        {   
            //a给b发送，
            char ret_msg[1024];
            //服务器给b发送询问接收信息
            sprintf(ret_msg , "%s is sending %s to you. Accept?(Y/N)\n" , send_host->IP , send_path);
            write(por->fd , ret_msg , strlen(ret_msg));
            //等待b回复
            bzero(ret_msg , 1024);
            read(por->fd , ret_msg , 1);
            if(strstr(ret_msg , "Y") != NULL || strstr(ret_msg , "y") != NULL)
            {
                bzero(ret_msg , 1024);
                sprintf(ret_msg , "start %s\n" , send_path);
                //告诉a开始发送
                write(send_host->fd , ret_msg , strlen(ret_msg));
                //告诉b开始接收
                bzero(ret_msg , 1024);
                sprintf(ret_msg , "recive %s\n" , send_path);
                write(por->fd , ret_msg , strlen(ret_msg));
                while(1)
                {
                    //设置服务器缓冲区
                    char fllush[4096];
                    bzero(fllush , 4096);
                    int ret_size = read(send_host->fd , fllush , 4096);
                    if(strstr(fllush , "打开文件出错") != NULL)
                    {
                        write(send_host->fd , "打开文件出错" , strlen("打开文件出错\n"));
                        return 0;
                    }
                    if(strstr(fllush , "finish") != NULL)
                    {
                        write(send_host->fd , "发送完毕" , strlen("发送完毕"));
                        write(por->fd , "接收完毕" , strlen("接收完毕"));
                        break;
                    }
                    write(por->fd , fllush , ret_size);
                }  
            }
            else
            {
                //b拒绝接收
                bzero(ret_msg , 1024);
                sprintf(ret_msg , "%s refuse\n" , por->IP);
                write(send_host->fd , ret_msg , strlen(ret_msg));
            }
            return 0;
        }
        por = por->next;
    }
    write(send_host->fd , "找不到这个好友或者他没有上线,发送reflash刷新好友列表\n" , strlen("找不到这个好友或者他没有上线,发送reflash刷新好友列表\n"));
    return -1  ;
}

char * get_whether()
{
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_socket == -1)
    {
        perror("");
        return NULL;
    }
    struct sockaddr_in tcp_addr;
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(80);
    tcp_addr.sin_addr.s_addr = inet_addr("47.107.155.132");
    int ret = connect(tcp_socket , (struct sockaddr *)&tcp_addr , sizeof(tcp_addr));
    if(ret == -1)
    {
        perror("");
        return NULL;
    }
    else
    {
        printf("开始获得天气信息\n");
    }

    //设置请求消息
    //http://api.qingyunke.com/api.php?key=free&appid=0&msg=你好
    char * http_msg = "GET /api.php?key=free&appid=0&msg=湛江天气 HTTP/1.1\r\nHost:api.qingyunke.com\r\n\r\n";
    write(tcp_socket , http_msg , strlen(http_msg));
    char buf[4096];
    read(tcp_socket , buf , 4096);
    //处理回文消息头部;
    char * ret_buf = malloc(1024);
    ret_buf = strstr(buf , "湛江天气");
    close(tcp_socket);
    return ret_buf;
}

void * TCP_fun(void * arg)
{
    int max_fd = 0;//最大的fd值
    char * wheather = get_whether();//获得天气
    char * announcement = "隔壁王阿姨的女儿生了一个大胖小子出来啦，明天下午两点将在小区篮球场进行小区篮球赛\n";
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_socket == -1)
    {
        perror("");
        return NULL;
    }
    struct sockaddr_in tcp_addr;
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(PORT);
    tcp_addr.sin_addr.s_addr = inet_addr(HOST);
    //设置地址和端口重用
    int on = 1;
    setsockopt(tcp_socket, SOL_SOCKET , SO_REUSEADDR , &on , sizeof(on));
    setsockopt(tcp_socket, SOL_SOCKET , SO_REUSEPORT , &on , sizeof(on));
    int ret = bind(tcp_socket , (struct sockaddr *)&tcp_addr , sizeof(tcp_addr));
    if(ret == -1)
    {
        perror("");
        return NULL;
    }
    else
    {
        printf("服务器IP地址绑定成功\n");
    }
    //监听
    ret = listen(tcp_socket , 300);
    if(ret == -1)
    {
        perror("");
        return NULL;
    }
    else
    {
        printf("正在监听\n");
    }
    //新建头节点
    head = malloc(sizeof(struct client_node));
    head->fd = 0;
    head->port = 0;
    head->next = head;
    head->prev = head; 
    //多路复用，等待客户端进来
    while(1)
    {
        fd_set cilent_set;
        FD_ZERO(&cilent_set);
        FD_SET(tcp_socket , &cilent_set);
        if(max_fd == 0)//第一次进入的时候，链表里面没有任何用户，max_fd设置为tcp_socket
        {
            max_fd = tcp_socket;
        }
        NODE por = head->next;
        //为监听描述符添加加进来的客户端fd
        while(por != head)
        {
            FD_SET(por->fd , &cilent_set);
            if(por->fd > max_fd)
            {
                max_fd = por->fd;
            }
            por = por->next;
        }

        //开始监听
        int sel_ret = select(max_fd+1, &cilent_set, NULL,NULL,NULL);
        if(sel_ret == -1)
        {
            perror("");
            return 0;
        }

        if(FD_ISSET(tcp_socket, &cilent_set))//客户连接上来了，获得其IP，端口号
        {
            struct sockaddr_in new_addr;
            int new_addr_length = sizeof(new_addr);
            int new_ret = accept(tcp_socket , (struct sockaddr *)&new_addr , &new_addr_length);
            if(new_ret == -1)
            {
                perror("");
                return NULL;
            }
            else
            {
                printf("客户端：%s 连接成功，端口号:%d\n" , inet_ntoa(new_addr.sin_addr) , ntohs(new_addr.sin_port));
            }
            //新建节点
            NODE new_node = malloc(sizeof(struct client_node));
            new_node->fd = new_ret;
            strcpy(new_node->IP , inet_ntoa(new_addr.sin_addr));
            new_node->port = ntohs(new_addr.sin_port);
            //尾插到链表中
            por = head;
            por->prev->next = new_node;
            new_node->prev = por->prev;
            new_node->next = por;
            por->prev = new_node;
            //向新用户广播公告和天气
            sprintf(wheather , "%s\n" , wheather);
            write(new_ret , wheather , strlen(wheather));
            write(new_ret , announcement , strlen(announcement));
            //把在线的好友发给客户端
            get_friend(new_ret);

            /***********************/

            //测试好友列表
            por = head->next;
            printf("好友列表:\n");
            while(por != head)
            {
                printf("客户端：%s 端口号:%d 文件描述符：%d\n" , por->IP , por->port , por->fd);
                por = por->next;
            }

            /***********************/
        }
        //监听所有的客户端
        por = head->next;
        while(por != head)
        {
            if(FD_ISSET(por->fd, &cilent_set))
            {
                
                char client_buf[1024];
                bzero(client_buf , 1024);
                int size = read(por->fd , client_buf , 1024);
                if(strstr(client_buf , "reflash") != NULL)
                {
                    get_friend(por->fd);//给需要刷新的客户端发送新的好友列表
                }
                else if(strstr(client_buf , "send to") != NULL)
                {
                    send_to(client_buf , por->fd);
                }
                else if(strstr(client_buf , "send file") != NULL)
                {
                    send_file(client_buf , por);
                }
                else if(strstr(client_buf , "-help") != NULL)
                {
                    write(por->fd , "命令有:\nsend to(发送消息)\nreflash(刷新好友列表)\nsend file(发送文件)" , strlen("命令有:\nsend to(发送消息)\nreflash(刷新好友列表)\nsend file(发送文件)"));
                }
                else
                {
                    write(por->fd , "worng command\n输入：-help以查看所有命令\n" , strlen("worng command\n输入：-help以查看所有命令\n"));

                }
                if(size <= 0)
                {
                    //下线以后删除该节点,防止出现bad file descripter
                    NODE new_por = por;
                    NODE new_new = por->prev;
                    new_por->prev->next = new_por->next;
                    new_por->next->prev = new_por->prev;
                    por->next = NULL;
                    por->prev = NULL;
                    printf("%s下线了\n" , por->IP);
                    close(por->fd);
                    free(por);
                    //回退一个节点
                    por = new_new;
                }
            }
            por = por->next;
        }

    }
}

void * UDP_fun(void * arg)
{
    
}

int main()
{
    pthread_create(&TCP_id , NULL , TCP_fun , NULL);
    pthread_create(&UDP_id , NULL , UDP_fun , NULL);
    while(1);
}