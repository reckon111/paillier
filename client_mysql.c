#include<stdlib.h>
#include<stdio.h>
#include<mysql/mysql.h>

// MYSQL *conn_ptr;
// unsigned int timeout = 7;	//超时时间7秒
// int main()
// {
// 	int ret = 0;
// 	conn_ptr = mysql_init(NULL);//初始化
// 	if(!conn_ptr)
// 	{
// 		printf("mysql_init failed!\n");
// 		return -1;
// 	}

// 	ret = mysql_options(conn_ptr,MYSQL_OPT_CONNECT_TIMEOUT,(const char*)&timeout);//设置超时选项
// 	if(ret)
// 	{
// 		printf("Options Set ERRO!\n");
// 	}
// 	conn_ptr = mysql_real_connect(conn_ptr,"localhost","root","aabbcc","testdb",0,NULL,0);//连接MySQL testdb数据库
// 	if(conn_ptr)
// 	{
// 		printf("Connection Succeed!\n");
// 		mysql_close(conn_ptr);
// 		printf("Connection closed!\n");
// 	}
// 	else	//错误处理
// 	{
// 		printf("Connection Failed!\n");
// 		if(mysql_errno(conn_ptr))
// 		{
// 			printf("Connect Erro:%d %s\n",mysql_errno(conn_ptr),mysql_error(conn_ptr));//返回错误代码、错误消息
// 		}
// 		return -2;
// 	}

// 	return 0;
// }
int main(){
    MYSQL *pConn = mysql_init(0);
    if(!mysql_real_connect(pConn, "175.24.183.10", "root", "Zs@123nihao", "encipher_data", 0, 0, 0))  //连接数据库，连接失败放回负值
    {
        printf("无法连接数据库: %s\n", mysql_error(pConn));
        exit(-1);
    }
    printf("连接数据库成功\n");

    if(mysql_query(pConn, "insert into data1(name, number) values('m2223', 'afndfjd23453')"))      //执行指令，执行失败放回正值 注意单引号
    {
        printf("插入失败,%s", mysql_error(pConn));
        exit(-1);
    }
    printf("插入成功\n");
    char sql[200];
    char numbername[10]="'m2222'";
    snprintf(sql, sizeof(sql),"select * from data1 where name= %s",numbername);
    if(mysql_query(pConn, sql))
    {
        goto error;
    }
    MYSQL_RES *result = mysql_store_result(pConn);
    MYSQL_ROW row;
    while(row = mysql_fetch_row(result))   //没有数据返回0
    {
        puts(row[1]);
    }
error:
    mysql_close(pConn);
    // char sql[200];
    // char numbername[10]="'m2222'";
    // char s1[50]="select * from data1 where name= ";
    // snprintf(sql, sizeof(sql),"%s%s", s1, numbername);
    // puts(sql);
}