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
#include <time.h>
#include <sys/time.h>
#include "dictionary.h"
#include <mysql/mysql.h>
#include <pthread.h>
#include <sys/sem.h>
#include "paillier.h"
#include "linklist.h"
#include "my_semaphore.h"

#define MYPORT  4000
#define BUFFER_SIZE 1024
#define KEYSIZE 4096
#define BASE 16
#define NAMESIZE 128
#define DATASIZE 2000
#define HEADER_SIZE 5
#define MSG1BYTES_LEN 4 
#define MSG2BYTES_LEN 4
#define TYPE_LEN 1

struct rawdata{
    char name[NAMESIZE];
    unsigned long number;
};

struct encidata{
    char name[NAMESIZE];
    mpz_t encinumber;
};

typedef struct {
    mpz_t *n;
    mpz_t *g;
    mpz_t *hs;
    struct rawdata* rdata;
}MySql_args;

typedef struct {
    mpz_t *n;
    mpz_t *lambda;
    mpz_t *mu;
    mpz_t *npow2;
}Query_args;

typedef struct{
    char* data;
    char* error;
    int type;
}RECVD_MSG;

typedef struct{
    char n[KEYSIZE/4+10];    // 16进制 KEYSIZE/4
    char g[KEYSIZE/4+10];
    char hs[KEYSIZE/4+10];
    char lambda[KEYSIZE/4+10];
    char mu[KEYSIZE/4+10];
}Share_data;

#define ONETIMESIZE 1000

Ndata ndata[ONETIMESIZE];
int sem_consumer;
int sem_producer;
int sem_producerleft;
int sem_consumerleft;
pthread_mutex_t mutex;


void* Query(void* args);
void* Mysql_Send(void *args);
int bits_10baseofnumber(int n);
void my_fgets(char *str, int size);
void secure_send(int fd, char* send_data);
void secure_recv(int sock_cli, RECVD_MSG *recvd_msg);
char* encode_msg(int type, char* msg1, char* msg2);
void share(Share_data* share_data);
void* shmgetdata(void* args);
void* shmgetdataleft(void* args);
int mysql_exist(MYSQL* conn, char* name);




int main()
{   mpz_t pk_n, pk_g, pk_hs, sk_lambda, sk_mu, encipher_msg, rel_mpz, npow2, test_enmsg, test_n;
    
    //mpz_t 初始化
    mpz_init(pk_n);
    mpz_init(pk_g);
    mpz_init(pk_hs);
    mpz_init(sk_lambda);
    mpz_init(sk_mu);
    mpz_init(encipher_msg);
    mpz_init(rel_mpz);
    mpz_init(npow2);
    mpz_init(test_enmsg);

    //信号量 初始化
    pthread_mutex_init(&mutex, NULL);

    sem_consumer = semget(0x6600, 1, 0660|IPC_CREAT);     //  返回信号量标识号；若信号量不存在则创建
    my_sem_init(sem_consumer, 1);                            //  将消费者信号量初始化为可用 
    sem_producer = semget(0x6500, 0, 0);                  //  获取生产者信号量标识
    
    sem_consumerleft = semget(0x6800, 1, 0660|IPC_CREAT);
    my_sem_init(sem_consumerleft, 1);
    sem_producerleft = semget(0x6700, 0, 0);

    //产生G_paillier密钥
    key_generate_G(pk_n, pk_g,pk_hs, sk_lambda, sk_mu, KEYSIZE);
    mpz_pow_ui(npow2, pk_n, 2);
    printf("key generate\n");

    int n_size = mpz_sizeinbase(pk_n, 16);   //16进制下，结果为keysize/4
    printf("n_size: %d\n", n_size);

    //将产生的G_paillier密钥写入共享内存
    Share_data share_data;
    mpz_get_str(share_data.n, BASE, pk_n);
    mpz_get_str(share_data.g, BASE, pk_g);
    mpz_get_str(share_data.hs, BASE, pk_hs);
    mpz_get_str(share_data.lambda, BASE, sk_lambda);
    mpz_get_str(share_data.mu, BASE, sk_mu);
    share(&share_data);
    printf("share key ok\n");


    MySql_args args_sql = {&pk_n, &pk_g, &pk_hs, NULL};

    pthread_t th_shm, th_shmlt;

    pthread_create(&th_shm, NULL, shmgetdata, &args_sql);
    // pthread_create(&th_shmlt, NULL, shmgetdataleft, &args_sql);

    pthread_join(th_shm, NULL);
    // pthread_join(th_shmlt, NULL);


    /*————————————clear mpz————————————*/
    mpz_clear(pk_n);
    mpz_clear(pk_g);
    mpz_clear(pk_hs);
    mpz_clear(sk_lambda);
    mpz_clear(sk_mu);
    mpz_clear(encipher_msg);
    mpz_clear(rel_mpz);
    mpz_clear(npow2);
    mpz_clear(test_enmsg);

    return 0;
}



char* encode_msg(int type, char* msg1, char* msg2){ //函数使用结束后需要free
    int header_size ;
    int msg1_size = strlen(msg1);
    int msg2_size = strlen(msg2);
    char header[10];
    header_size = snprintf(header, sizeof(header), "%1d%04d%04d", type, msg1_size, msg2_size);
    char *encode_msg = (char*)malloc((header_size+msg1_size+msg2_size+1)*sizeof(char));
    strcpy(encode_msg, header);
    strcat(encode_msg, msg1);
    strcat(encode_msg, msg2);
    return encode_msg;
}






void secure_recv(int sock_cli, RECVD_MSG *recvd_msg){

    char recv_buffer[BUFFER_SIZE];
    char data_buffer[4096];
    memset(recv_buffer,0,sizeof(recv_buffer));
    memset(data_buffer,0,sizeof(data_buffer));
    int len = 0, point = 0;
    while(1){
        len = recv(sock_cli, recv_buffer, sizeof(recv_buffer), 0);
        if( len == 0)
        {
            break;
        }

        strncpy(data_buffer+point, recv_buffer, len);
        point += len;
        memset(recv_buffer,0,sizeof(recv_buffer));

        while(1)
        {
            if(strlen(data_buffer) < HEADER_SIZE)
            {
                break;
            }

            char header[HEADER_SIZE+1];
            int type_size = 1, size1;
            char s_t[TYPE_LEN+1], s_size1[MSG1BYTES_LEN+1];
            memset(header, 0, sizeof(header));
            memset(s_t, 0, sizeof(s_t));
            memset(s_size1, 0, sizeof(s_size1));

            strncpy(header, data_buffer, HEADER_SIZE);
            strncpy(s_t, header, TYPE_LEN);
            strncpy(s_size1, header+TYPE_LEN,MSG1BYTES_LEN);
            size1 = atoi(s_size1); 

            if(strlen(data_buffer) < HEADER_SIZE + size1)
            {
                break;
            }


            if(strcmp(s_t, "0") == 0)  //服务器回送的一般消息
            {
                char *dmsg = (char*)calloc(size1+1, sizeof(char));
                
                strncpy(dmsg, data_buffer+HEADER_SIZE,size1);
                recvd_msg->error = dmsg;
                recvd_msg->type = 0;
                // printf("type:%s\n",s_t);
                if(strncmp(dmsg, "0", 1) != 0)
                {
                    printf("error\n");
                }
                
                memset(data_buffer,0,sizeof(data_buffer));
            }
            if(strcmp(s_t, "1") == 0)
            {
                char *dmsg = (char*)calloc(size1+1, sizeof(char));
                strncpy(dmsg, data_buffer+HEADER_SIZE,size1);
                recvd_msg->type = 1;
                recvd_msg->data = dmsg;
                // printf("type:%s\n",s_t);
                // printf("%s\n",dmsg1);
                memset(data_buffer,0,sizeof(data_buffer));
            }
        }

        if(strlen(data_buffer) == 0)
        {
            break;
        }
    }
}

void secure_send(int fd, char* send_data){  // 避免数据过长，而数据发送不完整，保证数据能完整传送完
    int len , cnt = 0;
    int size = strlen(send_data);

    while(size > 0){
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

        // if(len == 0){
        //     continue;
        // }

        cnt += len;
        size -= len;
    }    
}

void my_fgets(char *str, int size){
    fgets(str, size, stdin);
    char *find = strchr(str, '\n');
    if(find){
        *find = '\0';
    }
}

int bits_10baseofnumber(int n){ 
    /*  返回十进制数位数  */
    int cnt = 0;
    while(n)
    {
        n /= 10;
        cnt++;
    }
    return cnt;
}



void* Mysql_Send(void* args){

    MySql_args* my_args = (MySql_args*)args;

    MYSQL *pConn = mysql_init(0);
    if(!mysql_real_connect(pConn, "175.24.183.10", "root", "Zs@123nihao", "encipher_data", 0, 0, 0))  //连接数据库，连接失败放回负值
    {
        printf("无法连接数据库: %s\n", mysql_error(pConn));
        exit(-1);
    }
    printf("连接数据库成功\n");

    struct timeval tpstart,tpend;
    float timeuse;
    mpz_t temp;
    mpz_init(temp);
    gettimeofday(&tpstart,NULL);

    for(int i=0; i<200; i++){
        encipher_G(temp, *(my_args->n), *(my_args->g), *(my_args->hs), (my_args->rdata)[i].number);

        int msg_size = mpz_sizeinbase(temp, 16);
        char msg[msg_size+1];
        memset(msg, 0, sizeof(msg));
        mpz_get_str(msg, 16, temp);

        char sql[4096];
        // printf("%ld\n",strlen(msg3));
        // printf("%s\n", msg3);
        snprintf(sql, sizeof(sql),"insert into data1(name, number) values('%s', '%s')", (my_args->rdata)[i].name, msg);
        if(mysql_query(pConn,sql))      //执行指令,执行失败放回正值 注意单引号
            {   printf("%d\n", i);
                printf("插入失败,%s", mysql_error(pConn));
                exit(-1);
            }
    }    
    printf("Encrypted message sent\n");

    gettimeofday(&tpend,NULL);
    timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    timeuse/=1000;
    printf("Used Time:%f\n",timeuse);
   
    mysql_close(pConn);
}

void* shmgetdata(void* args){
    mpz_t temp;
    mpz_init(temp);
    while(1)
    {   
        int shmid;      //  共享内存标识号
        sem_p(sem_producer);
        pthread_mutex_lock(&mutex);
        MySql_args* my_args = (MySql_args*)args;

        MYSQL *pConn = mysql_init(0);
        if(!mysql_real_connect(pConn, "175.24.183.10", "root", "Zs@123nihao", "encipher_data", 0, 0, 0))  //连接数据库，连接失败放回负值
        {
            printf("无法连接数据库: %s\n", mysql_error(pConn));
            exit(-1);
        }
        printf("1连接数据库成功\n");
        // sleep(1);

    
        shmid = shmget((key_t)0x5505, 2000*sizeof(Ndata), 0660|IPC_CREAT);
        if(shmid == -1)
        {
            printf("0x5505 已存在\n");
        }

        Ndata *pshm;

        pshm = (Ndata*)shmat(shmid, 0, 0);
        memcpy(ndata, pshm, ONETIMESIZE*sizeof(Ndata));

        for(int i=0; i<ONETIMESIZE; i++){
            if((ndata+i)->value==0)
            {   printf("1共享内存空\n");
                break;
            }

            encipher_G(temp, *(my_args->n), *(my_args->g), *(my_args->hs), (ndata+i)->value);

            int msg_size = mpz_sizeinbase(temp, 16);
            char msg[msg_size+1];
            memset(msg, 0, sizeof(msg));
            mpz_get_str(msg, 16, temp);

            
            char sql[4096];

            if(mysql_exist(pConn, (ndata+i)->name)==0)
            {
                snprintf(sql, sizeof(sql),"insert into data1(name, number) values('%s', '%s')", (ndata+i)->name, msg);
            }
            else
            {
                snprintf(sql, sizeof(sql),"update data1 set number='%s' where name='%s'", msg, (ndata+i)->name);
            }
            // snprintf(sql, sizeof(sql),"insert into data1(name, number) values('%s', '%s')", (ndata+i)->name, msg);
            
            if(mysql_query(pConn,sql))      //执行指令,执行失败放回正值 注意单引号
                {   printf("%d\n", i);
                    printf("插入失败,%s", mysql_error(pConn));
                    exit(-1);
                }
            printf("%s\n", (ndata+i)->name);
            printf("%d\n", (ndata+i)->value);
        }

        // memset(pshm, 0, 2000*sizeof(Ndata));
        mysql_close(pConn);
        printf("1Encrypted message sent\n");

        // sleep(1);
        sem_v(sem_consumer);

        pthread_mutex_unlock(&mutex);
        shmdt(pshm);
    }

    mpz_clear(temp);
}

void* shmgetdataleft(void* args){
    mpz_t temp;
    mpz_init(temp);

    while(1)
    {
        int shmid;
        sem_p(sem_producerleft);
        pthread_mutex_lock(&mutex);

        MySql_args* my_args = (MySql_args*)args;

        MYSQL *pConn = mysql_init(0);
        if(!mysql_real_connect(pConn, "175.24.183.10", "root", "Zs@123nihao", "encipher_data", 0, 0, 0))  //连接数据库，连接失败放回负值
        {
            printf("无法连接数据库: %s\n", mysql_error(pConn));
            exit(-1);
        }
        printf("2连接数据库成功\n");
        // sleep(1);
        
        shmid = shmget((key_t)0x5506, 2000*sizeof(Ndata), 0660|IPC_CREAT);
        if(shmid == -1)
        {
            printf("0x5506 已存在\n");
        }

        Ndata *pshm;

        pshm = (Ndata*)shmat(shmid, 0, 0);
        memcpy(ndata, pshm, ONETIMESIZE*sizeof(Ndata));

        for(int i=0; i<ONETIMESIZE; i++){
            if((ndata+i)->value==0)
            {   printf("2共享内存空\n");
                break;
            }
            encipher_G(temp, *(my_args->n), *(my_args->g), *(my_args->hs), (ndata+i)->value);

            int msg_size = mpz_sizeinbase(temp, 16);
            char msg[msg_size+1];
            memset(msg, 0, sizeof(msg));
            mpz_get_str(msg, 16, temp);

            char sql[4096];

            if(mysql_exist(pConn, (ndata+i)->name)==0)
            {
                snprintf(sql, sizeof(sql),"insert into data1(name, number) values('%s', '%s')", (ndata+i)->name, msg);
            }
            else
            {
                snprintf(sql, sizeof(sql),"update data1 set number='%s' where name='%s'", msg, (ndata+i)->name);
            }

            // snprintf(sql, sizeof(sql),"insert into data1(name, number) values('%s', '%s')", (ndata+i)->name, msg);
            if(mysql_query(pConn,sql))      //执行指令,执行失败放回正值 注意单引号
                {   printf("%d\n", i);
                    printf("插入失败,%s", mysql_error(pConn));
                    exit(-1);
                }

            printf("%s\n", (ndata+i)->name);
            printf("%d\n", (ndata+i)->value);
        }
        mysql_close(pConn);
        printf("2Encrypted message sent\n");

        // sleep(1);
        // memset(pshm, 0, 2000*sizeof(Ndata));

        pthread_mutex_unlock(&mutex);
        sem_v(sem_consumerleft);

        shmdt(pshm);
    }
    
    mpz_clear(temp);

}

void* Query(void* args){
    Query_args* myargs = (Query_args*)args;

    mpz_t rel_mpz;
    RECVD_MSG recvd_msg={NULL, NULL, 0};   //服务器回送的消息，数据
    unsigned long rel;  //存放询问结果
    mpz_init(rel_mpz);

    // char sendbuf[BUFFER_SIZE];
    // char recvbuf[BUFFER_SIZE];
    char *send_data;

    //定义sockfd
    int sock_cli = socket(AF_INET,SOCK_STREAM, 0);

    ///定义sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MYPORT);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr("175.24.183.10");  ///服务器ip

    ///连接服务器，成功返回0，错误返回-1
    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
        exit(1);
    }


    while(1)
    {   char elemt1[bits_10baseofnumber(DATASIZE)+2], elemt2[bits_10baseofnumber(DATASIZE)+2], operation[10];
        printf("input element1:(exit)\n");
        my_fgets(elemt1, DATASIZE+2);
        if(strcmp(elemt1,"exit") == 0)
        {
            break;
        }   
        printf("input element2:(exit)\n");
        my_fgets(elemt2, DATASIZE+2);
        if(strcmp(elemt2,"exit") == 0)
        {
            break;
        }  
        printf("input operation:(exit)");
        my_fgets(operation,10);
        printf("input element1:(exit)\n");
        if(strcmp(operation,"exit") == 0)
        {
            break;
        }  
        
        if(strcmp(operation,"+") == 0)
        {
            send_data = encode_msg(2, elemt1, elemt2);
            printf("询问发送中...\n");
            secure_send(sock_cli, send_data);
            printf("询问发送成功\n");
            printf("询问:");
            puts(send_data);

            secure_recv(sock_cli,&recvd_msg);

            if(recvd_msg.type == 0)
            {
                puts(recvd_msg.error);

                free(recvd_msg.error);
            }

            if(recvd_msg.type == 1)
            {   puts(recvd_msg.data);
                mpz_set_str(rel_mpz, recvd_msg.data, BASE);
                mpz_mod(rel_mpz, rel_mpz, *(myargs->npow2));
                printf("o56k\n");
                rel = decipher_G(rel_mpz, *(myargs->n), *(myargs->lambda), *(myargs->mu));
                printf("rel:%ld\n", rel);
                free(recvd_msg.data);
            }

            printf("okkk\n");
            
        }


    }

    mpz_clear(rel_mpz);
    close(sock_cli);
}

void share(Share_data* share_data){
    /*将产生的G_paillier密钥写入共享内存*/

    int shmid;

    shmid = shmget((key_t)0x5005, 12000, 0660|IPC_CREAT);
    if(shmid == -1)
    {
        printf("0x5005 已存在\n");
    }

    Share_data* pshm;
    pshm = (Share_data*)shmat(shmid, 0, 0);

    memcpy(pshm, share_data, sizeof(Share_data));
    // gmp_printf("pk_n:%Zd\n", pshm->n);
    // gmp_printf("pk_g:%Zd\n", pshm->g);
    // printf("%d\n", pshm->a);
    // printf("%d\n", pshm->b);
    // printf("pshm->n:%s\n",pshm->n);
    // printf("pshm->n:%s\n",pshm->g);
    // printf("pshm->n:%s\n",pshm->hs);
    // printf("pshm->n:%s\n",pshm->lambda);
    // printf("pshm->n:%s\n",pshm->mu);
    
    shmdt(pshm);
}

int mysql_exist(MYSQL* conn, char* name){
    /*判断元素在数据表是否存在*/

    int rows;
    char sql[100];
    snprintf(sql, sizeof(sql), "select 1 from data1 where name='%s' limit 1", name);
    mysql_query(conn, sql);
    
    MYSQL_RES *rel = mysql_store_result(conn);
    rows = mysql_num_rows(rel);
    mysql_free_result(rel);

    return rows;
}
