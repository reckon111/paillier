#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct{
    char s1[100];
    char s2[100];
}Share_data;

int main(){
    char a[23] ="fgjgalga";
    int shmid;

    shmid = shmget((key_t)0x5005, sizeof(Share_data), 0660|IPC_CREAT);
    if(shmid == -1)
    {
        printf("0x5005 已存在\n");
    }
    printf("%ld\n", strlen(a));

    Share_data share_data={"fjajf","fagd"};
    Share_data* pshm;
    pshm = (Share_data*)shmat(shmid, 0, 0);
    snprintf(pshm->s1, strlen(share_data.s1)+1, "%s", share_data.s1);
    snprintf(pshm->s2, strlen(share_data.s2)+1, "%s", share_data.s2);
    
    shmdt(pshm);

    // //删除共享内存
    // if (shmctl(shmid, IPC_RMID, 0) == -1)
    // { printf("shmctl(0x5005) failed\n"); return -1;}
}

// /*
//  * 程序名：book258.cpp，此程序用于演示共享内存的用法
//  * 作者：C语言技术网(www.freecplus.net) 日期：20190525
// */
// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/ipc.h>
// #include <sys/shm.h> 

// int main()
// {
//   int shmid; // 共享内存标识符
 
//   // 创建共享内存，键值为0x5005，共1024字节。
//   if ((shmid = shmget((key_t)0x5005, 1024, 0660|IPC_CREAT)) == -1)
//   { printf("shmat(0x5005) failed\n"); }

//   printf("%d\n",shmid);
   
//   char *ptext=0;   // 用于指向共享内存的指针
 
//   // 将共享内存连接到当前进程的地址空间，由ptext指针指向它
//   ptext = (char *)shmat(shmid, 0, 0);
 
//   // 操作本程序的ptext指针，就是操作共享内存
//   printf("写入前：%s\n",ptext);
//   sprintf(ptext,"本程序的进程号是：%d",getpid());
//   printf("写入后：%s\n",ptext);
 
//   // 把共享内存从当前进程中分离
//   shmdt(ptext);
   
//   删除共享内存
//   if (shmctl(shmid, IPC_RMID, 0) == -1)
//   { printf("shmctl(0x5005) failed\n"); return -1; }
// }