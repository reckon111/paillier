#include <stdio.h>
#include <pthread.h>

typedef struct {
    int first;
    int last;
    int rel;
}MY_args;


void* my_func(void *args);

int main(){
    pthread_t th1;
    pthread_t th2;
    MY_args args1 = {1,50,0};
    MY_args args2 = {50,101,0};
    
    pthread_create(&th1, NULL, my_func, &args1);
    pthread_create(&th2, NULL, my_func, &args2);

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    printf("%d\n", args1.rel+args2.rel);
}

void* my_func(void *args){
    MY_args *my_args = (MY_args*)args;
    int first = my_args->first;
    int last = my_args->last;
    int rel=0;

    for(int i=first; i<last; i++)
    {
        rel+=i;
    }
    my_args->rel = rel;
    
    return NULL;

}