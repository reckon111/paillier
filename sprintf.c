#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <string.h>
void make_msg(char* fmt, ...);
int main(){
    // char* str = make-msg("%d%d%d",1,2,3);
    char* fmt = "%d"; //控制格式 1,2,3
    // printf("%s\n", fmt);
    // printf("%ld\n", sizeof(fmt));
    char* s;

    // make_msg(fmt,1,2,3);

    int size = 4, n, m;
    char *p;
    p = (char*)malloc(size*sizeof(char));  //相当于定义一个 char[size] ,最后一位留给终止符，定义在局部，也不会被销毁，直到free
    n = snprintf(p, size, fmt, 35185);
     //(1) 如果格式化后的字符串长度 < size，则将此字符串全部复制到str中，并给其后添加一个字符串结束符('\0')；
     //(2) 如果格式化后的字符串长度 >= size，则只将其中的(size-1)个字符复制到str中，并给其后添加一个字符串结束符('\0')，返回值为欲写入的字符串长度。 [1]
    printf("n:%d\n",n);
    printf("p:%s\n",p);
    printf("sizeof(p):%ld\n",sizeof(p));  // 指针 long unsigned int 8 个字节
    printf("%ld\n",strlen(p));            // 返回不包含终止符长度

    char str[5];                            // 定义在局部，会被销毁。
    m = snprintf(str, sizeof(str), "%d,", 35);
    printf("m:%d\n",m);
    printf("str:%s\n",str);
    printf("sizeof(str):%ld\n",sizeof(str));  // 指针 long unsigned int 8 个字节
    printf("%ld\n",strlen(str));          // 返回不包含终止符长度

    free(p);

    return 0;
}
void make_msg(char* fmt, ...){
    int n, size = 2;
    char *p;
    va_list  ap;
    if((p = (char*)malloc(size*sizeof(char))) == NULL){
        return;
    }
    
    va_start(ap, fmt);
    n = vsnprintf(p, size, fmt, ap); //成功返回实际字符串长度（包括格式内字符），失败返回负值
    va_end(ap);  
    // n = vsnprintf(char* p, size_t size, char* fmt, va_list ap)            size_t size 存入的字节数（包含终止符）；    
    // 执行成功，返回最终生成字符串的长度                                       va_list ap  可变变量，对应format  
    // 若生成字符串的长度大于size，则将字符串的前size个字符复制到str，同时将原串的长度返回（不包含终止符）；
    // 执行失败，返回负值，并置errno.
    printf("%d\n",n);
    printf("%s\n",p);
    free(p);
    return;

}


//(1) 如果格式化后的字符串长度 < size，则将此字符串全部复制到str中，并给其后添加一个字符串结束符('\0')；
//(2) 如果格式化后的字符串长度 >= size，则只将其中的(size-1)个字符复制到str中，并给其后添加一个字符串结束符('\0')，返回值为欲写入的字符串长度。

// #include <stdio.h>
 
// int main () {
//   char a[16];
//   size_t i;
 
//   i = snprintf(a, 13, "%012d", 12345);  // 第 1 种情况
//   printf("i = %lu, a = %s\n", i, a);    // 输出：i = 12, a = 000000012345
 
//   i = snprintf(a, 9, "%012d", 12345);   // 第 2 种情况
//   printf("i = %lu, a = %s\n", i, a);    // 输出：i = 12, a = 00000001
 
//   return 0;
// }