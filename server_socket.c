#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>

#define MYPORT  8887
#define QUEUE   20
#define BUFFER_SIZE 1024

int main()
{
    ///定义sockfd
    int server_sockfd = socket(AF_INET,SOCK_STREAM, 0);

    ///定义sockaddr_in
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(MYPORT);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    ///bind，成功返回0，出错返回-1
    if(bind(server_sockfd,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr))==-1)
    {
        perror("bind");
         /*在库函数中有个errno变量，每个errno值对应着以字符串表示的错误类型。
         当你调用"某些"函数出错时，该函数已经重新设置了errno的值。
         perror函数只是将你输入的一些信息和errno所对应的错误一起输出。*/
        exit(1);
        /*   exit(1)表示异常退出，在退出前可以给出一些提示信息，或在调试程序中察看出错原因。
             exit(0)表示正常退出。 
             而exit是系统调用级别的，是一个函数，它表示了一个进程的结束。
             exit是在调用处强行退出程序，运行一次程序就结束。
             这个状态标识了应用程序的一些运行信息，这个信息和机器和操作系统有关。*/
    }

    ///listen，成功返回0，出错返回-1
    if(listen(server_sockfd,QUEUE) == -1)
    {
        perror("listen");
        exit(1);
    }

    ///客户端套接字
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);

    ///成功返回非负描述字，出错返回-1
    int conn = accept(server_sockfd, (struct sockaddr*)&client_addr, &length);
    if(conn<0)
    {
        perror("connect");
        exit(1);
    }

    while(1)
    {
        memset(buffer,0,sizeof(buffer));
        int len = recv(conn, buffer, sizeof(buffer),0);
        if(strcmp(buffer,"exit\n")==0)
            break;
        fputs(buffer, stdout);
        send(conn, buffer, len, 0);
    }
    close(conn);
    close(server_sockfd);
    return 0;
}

// 10.recv函数：
// 功能：在已建立连接的套接字上接收数据。
// 格式：int recv(SOCKET s, char *buf, int len, int flags)。
// 参数：s-已建立连接的套接字；buf-存放接收到的数据的缓冲区指针；len-buf的长度；flags-调用方式：
// （1）0：接收的是正常数据，无特殊行为。
// （2）MSG_PEEK：系统缓冲区数据复制到提供的接收缓冲区，但是系统缓冲区内容并没有删除。
// （3）MSG_OOB：表示处理带外数据。
// 返回值：接收成功时返回接收到的数据长度，连接结束时返回0，连接失败时返回SOCKET_ERROR。

// 11.send函数：
// 功能：在已建立连接的套接字上发送数据.
// 格式：int send(SOCKET s, const char *buf, int len, int flags)。
// 参数：参数：s-已建立连接的套接字；buf-存放待发送的数据的指针；len-一次存入发送缓冲区最大字节数；flags-控制数据传输方式：
// （1）0：接收的是正常数据，无特殊行为。
// （2）MSG_DONTROUTE：表示目标主机就在本地网络中，无需路由选择。
// （3）MSG_OOB：表示处理带外数据。
// 成功则返回实际存入发送缓冲区的字节数, 失败返回-1. 错误原因存于error
// 一、send函数

// 函数原型：int send( SOCKET s,char *buf,int len,int flags );

// 功能：不论是客户还是服务器应用程序都用send函数来向TCP连接的另一端发送数据。客户程序一般用send函数向服务器发送请求，而服务器则通常用send函数来向客户程序发送应答。

// 参数一：指定发送端套接字描述符；

// 参数二：存放应用程序要发送数据的缓冲区；

// 参数三：实际要发送的数据的字节数；

// 参数四：一般置为0。


// 同步Socket的send函数的执行流程，当调用该函数时，send先比较待发送数据的长度len和套接字s的发送缓冲的长度（因为待发送数据是要copy到套接字s的发送缓冲区的，注意并不是send把s的发送缓冲中的数据传到连接的另一端的，而是协议传的，send仅仅是把buf中的数据copy到s的发送缓冲区的剩余空间里）：
// 1.如果len大于s的发送缓冲区的长度，该函数返回SOCKET_ERROR；

// 2.如果len小于或者等于s的发送缓冲区的长度，那么send先检查协议是否正在发送s的发送缓冲中的数据，如果是就等待协议把数据发送完，如果协议还没有开始发送s的发送缓冲中的数据或者s的发送缓冲中没有数据，那么 send就比较s的发送缓冲区的剩余空间和len：

//       （i）如果len大于剩余空间大小send就一直等待协议把s的发送缓冲中的数据发送完；

//       （ii）如果len小于剩余空间大小send就仅仅把buf中的数据copy到剩余空间里。

// 3.如果send函数copy数据成功，就返回实际copy的字节数，如果send在copy数据时出现错误，那么send就返回SOCKET_ERROR；如果send在等待协议传送数据时网络断开的话，那么send函数也返回SOCKET_ERROR。

// 注意：send函数把buf中的数据成功copy到s的发送缓冲的剩余空间里后它就返回了，但是此时这些数据并不一定马上被传到连接的另一端。如果协议在后续的传送过程中出现网络错误的话，那么下一个Socket函数就会返回SOCKET_ERROR。(每一个除send外的Socket函数在执行的最开始总要先等待套接字的发送缓冲中的数据被协议传送完毕才能继续，如果在等待时出现网络错误，那么该Socket函数就返回 SOCKET_ERROR）

// 二、recv函数

// 函数原型：int recv( SOCKET s, char *buf, int  len, int flags)

// 功能：不论是客户还是服务器应用程序都用recv函数从TCP连接的另一端接收数据。

// 参数一：指定接收端套接字描述符；

// 参数二：指明一个缓冲区，该缓冲区用来存放recv函数接收到的数据；

// 参数三：指明buf的长度；

// 参数四 ：一般置为0。


// 同步Socket的recv函数的执行流程：当应用程序调用recv函数时，recv先等待s的发送缓冲中的数据被协议传送完毕，

// 如果协议在传送s的发送缓冲中的数据时出现网络错误，那么recv函数返回SOCKET_ERROR；

// 如果s的发送缓冲中没有数据或者数据被协议成功发送完毕后，recv先检查套接字s的接收缓冲区，如果s接收缓冲区中没有数据或者协议正在接收数据，那么recv就一直
// 等待，直到协议把数据接收完毕；

// 当协议把数据接收完毕，recv函数就把s的接收缓冲中的数据copy到buf中（注意协议接收到的数据可能大于buf的长度，所以在这种情况下要调用几次recv函数才能把s的接收缓冲中的数据copy完。recv函数仅仅是copy数据，真正的接收数据是协议来完成的），recv函数返回其实际copy的字节数；

// 如果recv在copy时出错，那么它返回SOCKET_ERROR；如果recv函数在等待协议接收数据时网络中断了，那么它返回0。
