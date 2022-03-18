#include <stdio.h>
#include <sys/time.h>
#include "dictionary.h"

#define NAME_SIZE 128

struct rawdata{
    char name[NAME_SIZE];   //数据名
    unsigned long number;   //数据值
};

int main(){
    struct rawdata rd[4] = {
        {"m1", 32145},
        {"m2", 54648},
        {"m3", 385},
        {"m4", 841},
    };

    dictionary *rawdata_dic = dictionary_new(20); //初始化30个存储空间，若不够，会自动翻倍，变为60个
    dictionary_set(rawdata_dic, "m1", &rd[0]);    //字符串为键，指针为值
    dictionary_set(rawdata_dic, "m2", &rd[1]);

    struct rawdata *pm1 = dictionary_get(rawdata_dic, "m1", NULL);

    printf("%ld\n", pm1->number);
    dictionary_del(rawdata_dic);
    
    return 0;
}