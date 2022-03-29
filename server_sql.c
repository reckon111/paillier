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
#include <gmp.h>
#include<mysql/mysql.h>


#define MYPORT  4000
#define QUEUE   20
#define BUFFER_SIZE 1024
#define BASE 16
#define NAMESIZE 128
#define HEADER_SIZE 9
#define MSG1BYTES_LEN 4 
#define MSG2BYTES_LEN 4
#define TYPE_LEN 1

struct pdata{
    char *name;
    char *data;
};

struct encidata{
    char name[NAMESIZE+1];
    mpz_t data;
};

int selection(char* n1, char* n2, struct pdata *p_data);
void secure_send(int fd, char* send_data);
char* encode_msg(int type, char* msg1);
struct encidata* save_data(char* dmsg1, char* dmsg2);
int isexist(MYSQL* pConn,char* name);

int main()
{  
    struct pdata p_data[2];
    

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
        exit(1);
    }

    ///listen，成功返回0，出错返回-1
    if(listen(server_sockfd,QUEUE) == -1)
    {
        perror("listen");
        exit(1);
    }

    while(1)
    {	printf("等待客户端接入...\n");
        ///客户端套接字
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t length = sizeof(client_addr);

        ///成功返回非负描述字，出错返回-1
        int conn = accept(server_sockfd, (struct sockaddr*)&client_addr, &length);
        if(conn<0)
        {
            perror("connect");
            exit(1);
        }
		printf("客户端接入成功,等待请求...\n");

        char recv_buffer[BUFFER_SIZE];
        char data_buffer[2048];
        memset(recv_buffer,0,sizeof(recv_buffer));
        memset(data_buffer,0,sizeof(data_buffer));
        int len = 0, point = 0;
        while(1)
        {
            len = recv(conn, recv_buffer, sizeof(recv_buffer), 0);
            if( len <= 0){      
                break;
            }
            strncpy(data_buffer+point, recv_buffer, len);
            point += len;
            memset(recv_buffer,0,sizeof(recv_buffer));
            while(1){
                if(strlen(data_buffer) < HEADER_SIZE){
                    break;
                }

                char header[HEADER_SIZE+1];
                int type_size = 1, size1, size2;
                char s_t[TYPE_LEN+1], s_size1[MSG1BYTES_LEN+1], s_size2[MSG2BYTES_LEN+1];
                memset(header, 0, sizeof(header));
                memset(s_t, 0, sizeof(s_t));
                memset(s_size1, 0, sizeof(s_size1));
                memset(s_size2, 0, sizeof(s_size2));

                strncpy(header, data_buffer, HEADER_SIZE);
                strncpy(s_t, header, TYPE_LEN);
                strncpy(s_size1, header+TYPE_LEN,MSG1BYTES_LEN);
                strncpy(s_size2, header+TYPE_LEN+MSG1BYTES_LEN,MSG2BYTES_LEN);
                size1 = atoi(s_size1);
                size2 = atoi(s_size2);   

                if(strlen(data_buffer) < HEADER_SIZE + size1 + size2){
                    break;
                }

                if(strcmp(s_t, "2") == 0)  //type 2 m1 m2 加法
                {
                    char *send_data;

                    mpz_t rel, d1, d2;
                    mpz_init(rel);
                    mpz_init(d1);
                    mpz_init(d2);


                    char dmsg1[size1+1], dmsg2[size2+1];

                    memset(dmsg1, 0, sizeof(dmsg1));
                    memset(dmsg2, 0, sizeof(dmsg2));
                    strncpy(dmsg1, data_buffer+HEADER_SIZE,size1);
                    strncpy(dmsg2, data_buffer+HEADER_SIZE+size1,size2);

                    puts(dmsg1);
                    puts(dmsg2);

                    if(selection(dmsg1, dmsg2, p_data) < 0)
                    {
                        send_data = encode_msg(0, "数据不存在");
                    }
                    else
                    {	
						printf("value1:%s\n", p_data[0].data);
						printf("value2:%s\n", p_data[1].data);						

                        mpz_set_str(d1, p_data[0].data, 16);
                        mpz_set_str(d2, p_data[1].data, 16);

                        if(strcmp(dmsg1,dmsg2)==0){
                            mpz_mul(rel, d1, d1);
                        }
                        else{
                            mpz_mul(rel, d1, d2);
                        }

                        char rel_str[mpz_sizeinbase(rel, BASE)+1];
                        memset(rel_str, 0, sizeof(rel_str));
                        mpz_get_str(rel_str, BASE, rel);
                        
                        send_data = encode_msg(1, rel_str);
                    }
                    
                    memset(data_buffer,0,sizeof(data_buffer));

                    secure_send(conn, send_data);
                    free(send_data);

                    mpz_clear(rel);
                    mpz_clear(d1);
                    mpz_clear(d2);

                }
            }

            if(strlen(data_buffer) == 0)
            {   
                point=0;
            }
        }  

        close(conn);
    }


    close(server_sockfd);
    return 0;
}




int selection(char* n1, char* n2, struct pdata *p_data){
    MYSQL *pConn = mysql_init(0);
    if(!mysql_real_connect(pConn, "localhost", "root", "Zs@123nihao", "encipher_data", 0, 0, 0))  //连接数据库，连接失败放回负值
    {
        printf("无法连接数据库: %s\n", mysql_error(pConn));
        exit(-1);
    }
    printf("连接数据库成功\n");

    if((!isexist(pConn, n1)) || (!isexist(pConn, n2))){  //所查询数据不存在，关闭连接，返回-1
        mysql_close(pConn);
        return -1;
    }

    char sql[4096];
    snprintf(sql, sizeof(sql), "select * from data1 where name='%s' or name='%s'", n1, n2);
    if(mysql_query(pConn, sql))
    {
        printf("插入失败，%s\n", mysql_error(pConn));
        exit(-1);
    }
    
    MYSQL_RES *rel = mysql_store_result(pConn);  //需free mysql_free_result(pConn)
    MYSQL_ROW row;
    int i = 0;
    while(row = mysql_fetch_row(rel)){ // 查询两个相同数据，返回行数为1

        (p_data+i)->name = row[0];
        (p_data+i)->data = row[1];
        i++;
    }
    mysql_free_result(rel);
    mysql_close(pConn);
    return 1;
}

int isexist(MYSQL* pConn,char* name){
    int rows;
    char sql[100];
    snprintf(sql, sizeof(sql), "select 1 from data1 where name='%s' limit 1", name);
    mysql_query(pConn, sql);

    MYSQL_RES *rel = mysql_store_result(pConn);
    rows = mysql_num_rows(rel);
    mysql_free_result(rel);
    
    return rows;
}





char* encode_msg(int type, char* msg1){ //函数使用结束后需要free
    int header_size = 6 ;
    int msg1_size = strlen(msg1);
    char header[header_size+1];
    header_size = snprintf(header, sizeof(header), "%1d%04d", type, msg1_size);
    char *encode_msg = (char*)malloc((header_size+msg1_size+1)*sizeof(char));
    strcpy(encode_msg, header);
    strcat(encode_msg, msg1);
    return encode_msg;
}
void secure_send(int fd, char* send_data){  // 避免数据过长，而数据发送不完整，保证数据能完整传送完
    int len , cnt = 0;
    int size = strlen(send_data);

    while(size>0){
        if(strlen(send_data+cnt) > 1024)
        {
            len = send(fd, send_data+cnt, 1024, 0);
        }
        else
        {
            len = send(fd, send_data+cnt, strlen(send_data+cnt), 0);
        }
        
        if(len == -1)
        {
            // close(fd);
            printf("发送错误\n");
            return;
        }

        cnt += len;
        size -= len;
    }    
}

struct encidata* save_data(char* dmsg1, char* dmsg2){
    struct encidata* new_data = (struct encidata*)calloc(1, sizeof(struct encidata)); //分配内存小了，会报错
    mpz_init(new_data->data);
    
    strncpy(new_data->name, dmsg1, NAMESIZE);
    mpz_set_str(new_data->data, dmsg2, 16);

    return new_data;
}



