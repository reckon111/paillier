#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linklist.h"

// typedef struct {
//     char name[10];
//     int value;
// }Ndata;

// typedef struct _Node{ 
//     Ndata data;
//     struct _Node* next;
// }Node;

// struct Lnode{  // Lnode包含链表头指针和尾指针
//     Node *head;  // 指向链表的头节点
//     Node *tail;  // 指向链表的尾节点 
// };

// typedef struct Lnode* List; 

// void list_init(List list);
// int list_isempty(List list);
// Node* pop(List list);
// void linklist_append(List list, Node* pnode);

// int main(){

//     List list_data = (struct Lnode*)malloc(sizeof(struct Lnode));
//     list_init(list_data);

//     Node no;
//     snprintf(no.data.name, sizeof(no.data.name), "%s", "djfj");
//     no.data.value = 12;
//     no.next = NULL;

//     Node no1;
//     snprintf(no1.data.name, sizeof(no1.data.name), "%s", "dfj");
//     no1.data.value = 16;
//     no1.next = NULL;

//     linklist_append(list_data, &no);
//     linklist_append(list_data, &no1);

//     int len = list_len(list_data);
//     printf("len:%d\n", len);

//     Node* n1;
//     n1 = pop(list_data);
//     puts(n1->data.name);
//     printf("%d\n",n1->data.value);

//     n1 = pop(list_data);
//     puts(n1->data.name);
//     printf("%d\n",n1->data.value);

//     int r = list_isempty(list_data);
//     printf("r:%d\n", r);
//     len = list_len(list_data);
//     printf("len:%d\n", len);

//     free(list_data);
// }


void list_init(List list){  //
    list->head = NULL;
    list->tail = NULL;
}

void list_append(List list, Node* pnode){   // 添加节点
    if(list->head == NULL){                 //  链表空，新增节点为首节点
        list->head = pnode;
        list->tail = pnode;
        return;
    }
    list->tail->next = pnode;
    list->tail = pnode;
}

int list_isempty(List list){
    if(list->head == NULL){
        return 1;
    }
    else{
        return 0;
    }
}

int list_len(List list){
    Node *p = list->head;
    int cnt = 0;
    if(list->head == NULL)
    {
        return 0;
    }

    cnt = 1;
    while(p!=list->tail)
    {
        p = p->next;
        cnt++;
    }

    return cnt;
}

Node* pop(List list){   //队列式弹出，从链表头弹出; pop 之后可能需free(),释放内存
    Node* p = NULL;

    if(list->head==NULL)    //  队列空
    {
        return p;
    }

    p = list->head;
    if(list->head == list->tail)    //  队列只有一个元素，取出后，队列置为空
    {
        list->head = list->tail = NULL;
    }
    else
    {
        list->head = list->head->next;
    }

    return p;
}
