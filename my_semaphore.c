#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <stdbool.h>
#include "my_semaphore.h"

// union semun{
//     int val;
//     struct semid_ds *buf;
//     unsigned short *array;
// };

// struct sembuf{
//     short sem_num;
//     short sem_op;
//     short sem_flg;
// };

// void sem_destroy(int sem_id);
// int  sem_init(key_t key);
// void sem_p(int sem_id);
// void sem_v(int sem_id);
// void sem_destroy(int sem_id);

// int main(){
//     int sem_id = sem_init(0x5000);
//     sem_p(sem_id);
//     printf("sem_p\n");
//     sleep(5);
//     printf("sem_v\n");
//     sem_v(sem_id);
    
//     return 0;
// }

int  sem_createANDinit(key_t key, int value){   //  获取信号量标识；若信号量不存在则创建，信号量创建成功后，初始化为可用状态
    int sem_id;
    //获取信号量标识
    if(sem_id=(semget(key, 1, 0660)==-1))
    {   
        //信号量不存在则创建,存在则信号量返回标识符
        if(errno==2)
        {
            if(sem_id=semget(key, 1, 0660|IPC_CREAT)==-1)
            {
                perror("init 1 semget()");
                return -1;
            }
            //  信号量创建成功后，初始化为可用状态(value = 1)
            union semun sem_union;
            sem_union.val = value;
            if(semctl(sem_id, 0, SETVAL, sem_union)==-1){  //   sem——num 信号量下标
                perror("init semctl()");
                return -1;
            }
        }
        else
        {
            perror("init 2 semget()");
            return -1;
        }
    }
    return sem_id;
}

int  my_sem_init(int sem_id, int value){
    union semun sem_union;
    sem_union.val = value;
    if(semctl(sem_id, 0, SETVAL, sem_union)==-1){
        perror("sem_init semctl()");
        return -1;
    }
    return 1;
}
// P操作:
//	若信号量值为1，获取资源并将信号量值-1 
//	若信号量值为0，进程挂起等待
void sem_p(int sem_id){
    // P操作:
    //	若信号量值为1，获取资源并将信号量值-1 
    //	若信号量值为0，进程挂起等待
    struct sembuf sem_b;
    sem_b.sem_num = 0;   // 要操作的信号量下标
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1)==-1)  //   semop()根据 sem_op>0,sem_op<0,sem_op=0 分不同情况处理
    {
        perror("sem_p semop()");
    }
}

// V操作：
//	释放资源并将信号量值+1
//	如果有进程正在挂起等待，则唤醒它们

void sem_v(int sem_id){
    // V操作：
    //	释放资源并将信号量值+1
    //	如果有进程正在挂起等待，则唤醒它们
    struct sembuf sem_b;
    sem_b.sem_num = 0;  // 要操作的信号量下标
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    if(semop(sem_id, &sem_b, 1)==-1){
        perror("sem_v semop()");
    }

}

void my_sem_destroy(int sem_id){
    if(semctl(sem_id, 0, IPC_RMID)==-1){
        perror("sem_destroy semctl()");
    }
}