#include <stdio.h>
#include <sys/time.h>
#include "dictionary.h"

#define TEMP_SIZE 128

typedef void (*skill_f)(char *);

struct student
{
    int id;                 // 编号
    int age;                // 年龄
    char name[TEMP_SIZE];   // 姓名
    char addr[TEMP_SIZE];   // 地址
    char skill[TEMP_SIZE];  // 技能名,即函数名
};

// void play_table_tennis(char * name)
// {
//     printf("我是%s, 我会打乒乓球\n", name);
// }

// void eat_food(char * name)
// {
//     printf("我是%s, 我很会吃\n", name);
// }

// void no_skill(char * name)
// {
//     printf("我是%s, 我无能, 我是废物\n", name);
// }

/**定义数据变量, 可以从配置文件中读取, 这里只做简单定义*/
struct student students[3] = {
    {0, 10, "小明", "湖南省长沙市", "play_table_tennis"},
    {1, 10, "小红", "湖南省株洲市", "eat_food"},
    {2, 10, "小华", "江西省南昌市", "no_skill"},
};

void display(dictionary * std_dic, dictionary * skill_dic, char * name)
{
    struct student * p = dictionary_get(std_dic, name, NULL); //从字典中读取
    if (!p)
        return;
    
    printf("id:%d\t 姓名:%s\t 年龄:%d\t 地址:%s\t", p->id, p->name, p->age, p->addr);
    /*根据函数名, 读取函数指针*/
    // skill_f func = (skill_f)(dictionary_get(skill_dic, p->skill, no_skill));
    // func(p->name);
}

int main(int argc, char * argv[])
{
    //初始化30个存储空间，若不够，会自动翻倍，变为60个
    dictionary * std_dic = dictionary_new(30); 
    dictionary * skill_dic = dictionary_new(10);

    /*技能字典, 将函数地址放入字典中, 以后可以通过函数名, 获取函数指针, 并执行函数*/
    // dictionary_set(skill_dic, "play_table_tennis", play_table_tennis);
    // dictionary_set(skill_dic, "eat_food", eat_food);
    // dictionary_set(skill_dic, "no_skill", no_skill);

    /*学生字典, 将结构体指针放入*/
    dictionary_set(std_dic, "小明", &students[0]);
    dictionary_set(std_dic, "小红", &students[1]);
    dictionary_set(std_dic, "小华", &students[2]);

    /*测试*/
    display(std_dic, skill_dic, "小红");// 查找打印

    struct student * p = dictionary_get(std_dic,"小红", NULL); //从字典中读取
    if (!p)
        return;
    
    printf("id:%d\t 姓名:%s\t 年龄:%d\t 地址:%s\t", p->id, p->name, p->age, p->addr);

    /*销毁*/
    dictionary_del(std_dic);
    dictionary_del(skill_dic);
    return 0;
}
