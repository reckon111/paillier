#ifndef __LINKLIST_H__
#define __LINKLIST_H__

typedef struct{  // 存储数据的结构体
    char name[10];
    int value;
}Ndata;

typedef struct _Node{  // 链表的结点
    Ndata data;
    struct _Node* next;
}Node;

struct Lnode{  // Lnode包含链表头指针和尾指针
    Node *head;
    Node *tail;
};

typedef struct Lnode* List; 

/*
 ___________       _________       ______________________                   ______________________
|___Lsit____|---->|___head__|---->|___Ndata__|___next__--|---->........---->|___Ndata__|___NULL__|
                  |___tail__|-----------------------------------------------^
*/

void list_init(List list);
int list_isempty(List list);
Node* pop(List list);
void list_append(List list, Node* pnode);
int list_len(List list);



#endif