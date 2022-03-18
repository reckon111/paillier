#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <sys/ipc.h>
#include <sys/sem.h>

union semun{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};


int  sem_createANDinit(key_t key, int value);
int  my_sem_init(int sem_id, int value);
void sem_p(int sem_id);
void sem_v(int sem_id);
void my_sem_destroy(int sem_id);


#endif