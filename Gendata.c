#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>

#include "linklist.h"
#include "my_semaphore.h"


#define ONETIMESIZE 1000	// 一次加密传输的数据量
#define SLEEPTIME 500

pthread_mutex_t mutex;

int flag;           // flag=1数据产生，flag=0数据停止产生
sem_t sem_send;     // 数据发送信号，每采集 ONETIMESIZE 个数据， sem_send资源加1

List list_data;     // 存储数据节点的链表

//用于共享内存，进程间通信
int sem_producer;   // Gendata生产者信号量，数据写入共享内存完毕，sem_producer+1，让消费者取消阻塞，开始消费
int sem_consumer;   //消费者信号量， 共享内存的数据加密完成，sem_consumer+1，让生产者取消阻塞，开始生产

void* gen_contrl(void *args);
void* gendata(void* args);
void* shmsend(void* args);

int main(){
    list_data = (List)malloc(sizeof(struct Lnode));
    pthread_mutex_init(&mutex, NULL);

    sem_init(&sem_send, 0, 0);

    sem_producer = semget(0x6500, 1, 0660|IPC_CREAT);   //  返回信号量标识号；若信号量不存在则创建
    my_sem_init(sem_producer, 0);  						//  将Gendata生产者信号量初始化为不可用，

	sem_consumer = semget(0x6600, 1, 0660|IPC_CREAT);   //  返回信号量标识号；若信号量不存在则创建
    my_sem_init(sem_consumer, 1);                       //  将消费者信号量初始化为可用 

    sem_consumer = semget(0x6600, 0, 0);    			//  获取消费者者信号量标识

	//---------数据产生------------
    pthread_t th_contrl, th_gendata, th_shmsend;

    pthread_create(&th_contrl, NULL, gen_contrl, NULL);
    pthread_create(&th_gendata, NULL, gendata, NULL);
    pthread_create(&th_shmsend, NULL, shmsend, NULL);

    pthread_join(th_contrl,NULL);
    pthread_join(th_gendata, NULL);
    pthread_join(th_shmsend,NULL);

    my_sem_destroy(sem_producer);
    sem_destroy(&sem_send);

    return 0;
}
void* gen_contrl(void *args){
    int a;
    while(1)
    {
        printf("输入1开始产生数据,输入0停止产生数据,输入-1退出程序:");
        scanf("%d", &a);

        if(a == 1)
        {
            flag = 1;
        }

        if(a == 0)
        {   
            flag = 0;
            sem_post(&sem_send);    //  单通道
            // sem_post(&sem_sendleft);     //  增加一个通道，发送末尾<ONETIMESIZE数据
        }
        if(a == -1){
            exit(0);
        }
    }
}

void* gendata(void* args){
    int cnt = 0;
    time_t ti;
    time(&ti);
    srand((unsigned int)ti);

    while(1)
    {	
		// if(cnt==1000)
		// {
		// 	continue;
		// }
        if(flag == 1)
        {   
			cnt++;
            Node* node = (Node*)malloc(sizeof(Node));
            snprintf(node->data.name, sizeof(node->data.name), "m%d", cnt);
            node->data.value = rand()%1000000;
            list_append(list_data, node);
            printf("%s\n",node->data.name);
            printf("%d\n",node->data.value);
            // printf("%d: list_len(list_data)%d\n", cnt, list_len(list_data)); 
            // printf("数据量:%d\n",cnt);
            usleep(SLEEPTIME);
            if( cnt%ONETIMESIZE == 0)
            {
                printf("%d产生完毕\n",cnt/ONETIMESIZE);
                sem_post(&sem_send);	//	sem_post(&sem_send),信号值加1
            }
        }

        if(flag == 0)
        {   
            continue;
        }
    }
}

void* shmsend(void* args){
    while(1)
    {   
        int shmid;

        sem_wait(&sem_send); // sem_send 资源>0,取消阻塞，资源-1

        sem_p(sem_consumer);
        /*将数据存入共享内存*/
        shmid = shmget((key_t)0x5505, 2000*sizeof(Ndata), 0660|IPC_CREAT);
        if(shmid == -1)
        {
            printf("0x5505 已存在\n");
        }

        Ndata *pshm, *ndata;
        Node* p;

        pshm = (Ndata*)shmat(shmid, 0, 0);
        memset(pshm, 0, 2000*sizeof(Ndata));

        for(int i=0; i<ONETIMESIZE; i++){   //将 ONETIMESIZE 个数据写入共享内存 
            p = pop(list_data);
            if(p == NULL)
            {
              printf("\nFIFO空,数据采集完毕\n");
              break;  
            }
            ndata = &p->data;
            memcpy(pshm+i, ndata,sizeof(Ndata));
            free(p);
        }

        sem_v(sem_producer);

        shmdt(pshm);
    }
}
