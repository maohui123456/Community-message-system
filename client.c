#include "myhead.h"
#define HOST "192.168.19.235"
#define PORT 6666
int socket_fd;

pthread_t id1 , id2;

void * read_fun(void * arg)
{
    while(1)
    {
        char buf[1024];
        bzero(buf , 1024);
        read(socket_fd , buf ,1024);
        printf("%s\n" , buf);
        if(strstr(buf , "start") != NULL)
        {
            char send_path[1024];
            char * p = strpbrk(buf , " ");
            p+=1;
            strcpy(send_path , p);
            //把\n去掉
            strtok(send_path , "\n");
            printf("send_path:%s\n" , send_path);
            printf("send_path:%d\n" , strlen(send_path));
            int fd = open(send_path , O_RDWR);
            if(fd == -1)
            {
                perror("open:");
                write(socket_fd , "打开文件出错\n" , strlen("打开文件出错\n"));
            }
            while(1)
            {
                char fllush[4096];
                bzero(fllush , 4096);
                int read_size = read(fd , fllush , 4096);
                if(read_size<=0)
                {
                    write(socket_fd , "finish" , strlen("finish\n"));
                    close(fd);
                    break;
                }
                write(socket_fd , fllush , read_size);
            }
        }
        if(strstr(buf , "recive") != NULL)
        {
            char send_path[1024];
            char * p = strpbrk(buf , " ");
            p+=1;
            strcpy(send_path , p);
            //把\n去掉
            strtok(send_path , "\n");
            int fd = open(send_path , O_RDWR|O_CREAT);
            while(1)
            {
                char fllush[4096];
                bzero(fllush , 4096);
                int read_size = read(socket_fd , fllush , 4096);
                if(read_size<=0)
                {
                    write(socket_fd , "finish" , strlen("finish\n"));
                    break;
                }
                write(fd , fllush , read_size);
            }
        }
    }
}

void * write_fun(void * arg)
{
    while(1)
    {
        printf("请输入需要发送的内容:\n");
        char buf[1024];
        bzero(buf , 1024);
        fgets(buf, 1024, stdin);
        write(socket_fd , buf ,strlen(buf));
        printf("发送内容：%s\n" , buf);
    }
}

int main()
{
    socket_fd = socket(AF_INET , SOCK_STREAM , 0);
    if(socket_fd == -1)
    {
        perror("");
    }
    else
    {
        printf("建立网络对象成功\n");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(HOST);
    int ret = connect(socket_fd , (const struct sockaddr *)&addr , sizeof(addr));
    if(ret < 0)
    {
        perror("");
    }
    else
    {
        printf("连接成功\n");
    }
    pthread_create(&id1 , NULL , read_fun , NULL);
    pthread_create(&id2 , NULL , write_fun , NULL);
    while(1);
}