#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <gmp.h>
#include <time.h>
#include <sys/time.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <sys/sem.h>
#include <semaphore.h>

#include "paillier.h"
#include "linklist.h"
#include "my_semaphore.h"

#define ONETIMESIZE 1000	// 一次加密传输的数据量
#define MYPORT 4000
#define BUFFER_SIZE 1024
#define KEYSIZE 4096
#define BASE 16
#define NAMESIZE 128
#define DATASIZE 2000
#define HEADER_SIZE 5
#define MSG1BYTES_LEN 4
#define MSG2BYTES_LEN 4
#define TYPE_LEN 1

typedef struct _Encidata
{
	char name[NAMESIZE];
	char value[KEYSIZE / 2 + 10]; // 密文bits是密钥bits两倍，因为 mod npow2
} Encidata;

typedef struct
{
	mpz_t *n;
	mpz_t *g;
	mpz_t *hs;
	mpz_t *lambda;
	mpz_t *mu;
	struct rawdata *rdata;
} MySql_args;

typedef struct
{
	char n[KEYSIZE / 4 + 10]; // 16进制 KEYSIZE/4
	char g[KEYSIZE / 4 + 10];
	char hs[KEYSIZE / 4 + 10];
	char lambda[KEYSIZE / 4 + 10];
	char mu[KEYSIZE / 4 + 10];
} Share_data;

Ndata ndata[ONETIMESIZE];				  // 存储共享内存中的数据
Encidata encidata[ONETIMESIZE + 1];		  // 存储 ndata 加密后中的数据
Encidata encidata_cache[ONETIMESIZE + 1]; // 缓存 encidata，用于发送，第 ONETIMESIZE+1 个结点为空，标识尾节点

// 用于共享内存，进程间通信
int sem_consumer;
int sem_producer;
int sem_producerleft;
int sem_consumerleft;

// 线程通信
sem_t semP_encidata;
sem_t semC_encidata;
sem_t semP_cache;
sem_t semC_cache;

void MYmysql_send(Encidata *encidata, int size);
void MYmysql_send2(Encidata *encidata, int size);
void *Mysql_Send(void *args);
void share(Share_data *share_data);
void *encipher_shmdata(void *args);
void MYmysql_send3(Encidata *encidata);
void *MYmysql_Send(void *args);
void *cache_encidata(void *args);

int main()
{
	mpz_t pk_n, pk_g, pk_hs, sk_lambda, sk_mu, encipher_msg, rel_mpz, npow2, test_enmsg, test_n;

	// mpz_t 初始化
	mpz_init(pk_n);
	mpz_init(pk_g);
	mpz_init(pk_hs);
	mpz_init(sk_lambda);
	mpz_init(sk_mu);
	mpz_init(encipher_msg);
	mpz_init(rel_mpz);
	mpz_init(npow2);
	mpz_init(test_enmsg);

	//信号量 初始化
	sem_consumer = semget(0x6600, 0, 0); //  获取随机数据消费者者信号量标识
	sem_producer = semget(0x6500, 0, 0); //  获取随机数据生产者信号量标识

	sem_init(&semP_encidata, 0, 0);
	sem_init(&semC_encidata, 0, 1);

	sem_init(&semP_cache, 0, 0);
	sem_init(&semC_cache, 0, 1);

	//	产生G_paillier密钥
	key_generate_G(pk_n, pk_g, pk_hs, sk_lambda, sk_mu, KEYSIZE);
	mpz_pow_ui(npow2, pk_n, 2);
	printf("key generate\n");

	int n_size = mpz_sizeinbase(pk_n, 2); // 16进制下，结果为keysize/4
	printf("n_size: %d\n", n_size);

	//	将产生的G_paillier密钥写入共享内存
	Share_data share_data;
	mpz_get_str(share_data.n, BASE, pk_n);
	mpz_get_str(share_data.g, BASE, pk_g);
	mpz_get_str(share_data.hs, BASE, pk_hs);
	mpz_get_str(share_data.lambda, BASE, sk_lambda);
	mpz_get_str(share_data.mu, BASE, sk_mu);
	share(&share_data);
	printf("key shared \n");

	MySql_args args_encipher = {&pk_n, &pk_g, &pk_hs, &sk_lambda, &sk_mu, NULL};

	pthread_t th_encipher, th_CacheEncidata, th_MysqlSend;

	pthread_create(&th_encipher, NULL, *encipher_shmdata, &args_encipher);	   //	加密模块:加密共享内存数据
	pthread_create(&th_CacheEncidata, NULL, cache_encidata, NULL); //	加密数据缓存
	pthread_create(&th_MysqlSend, NULL, MYmysql_Send, NULL);	   //	加密数据上传至数据库

	pthread_join(th_encipher, NULL);
	pthread_join(th_CacheEncidata, NULL);
	pthread_join(th_MysqlSend, NULL);

	/*————————————clear mpz————————————*/
	mpz_clear(pk_n);
	mpz_clear(pk_g);
	mpz_clear(pk_hs);
	mpz_clear(sk_lambda);
	mpz_clear(sk_mu);
	mpz_clear(encipher_msg);
	mpz_clear(rel_mpz);
	mpz_clear(npow2);
	mpz_clear(test_enmsg);

	return 0;
}

void *encipher_shmdata(void *args)
{
	mpz_t temp;
	mpz_init(temp);
	MySql_args *my_args = (MySql_args *)args;
	struct timeval tpstart, tpend;
	float timeuse;
	Ndata *pshm;

	while (1)
	{
		//	将共享内存数据读入ndata，清空共享内存，释放资源，将消费者信号+1
		int shmid;			 //  共享内存标识号
		sem_p(sem_producer); //	当生产者信号sem_producer值>0时，占用资源（共享内存）
		shmid = shmget((key_t)0x5505, 2000 * sizeof(Ndata), 0660 | IPC_CREAT);
		if (shmid == -1)
		{
			printf("0x5505 已存在\n");
		}
		pshm = (Ndata *)shmat(shmid, 0, 0);
		memset(ndata, 0, ONETIMESIZE * sizeof(Ndata));
		memcpy(ndata, pshm, ONETIMESIZE * sizeof(Ndata));
		memset(pshm, 0, 2000 * sizeof(Ndata));
		shmdt(pshm);		 //	关闭与共享内存的连接
		sem_v(sem_consumer); // 释放资源，通知生产者

		if (strcmp(ndata->name, "\0") == 0)
		{ //	首节点为空，表明共享内存为空，直接进入下一轮等待。
			continue;
		}

		//	当消费者信号semC_encidata值>0时，占用资源（encidata）加密ndata中的数据，并存入encidata
		sem_wait(&semC_encidata);
		printf("加密模块加密数据中...\n");

		memset(encidata, 0, (ONETIMESIZE + 1) * sizeof(Encidata));
		int cnt = 0; //	计数encidata中数据个数

		gettimeofday(&tpstart, NULL);

		for (; cnt < ONETIMESIZE; cnt++)
		{
			if (strcmp((ndata + cnt)->name, "\0") == 0)
			{
				printf("共享内存空,数据加密完毕\n");
				break;
			}
			strcpy((encidata + cnt)->name, (ndata + cnt)->name);
			encipher_G(temp, *(my_args->n), *(my_args->g), *(my_args->hs), (ndata + cnt)->value);
			mpz_get_str(encidata[cnt].value, 16, temp);
		}

		gettimeofday(&tpend, NULL);
		timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
		timeuse /= 1000; // ms
		printf("encipher %d data Used Time(ms):%f\n", cnt, timeuse);

		sem_post(&semP_encidata);
	}

	mpz_clear(temp);
}

void *cache_encidata(void *args)
{
	while (1)
	{
		sem_wait(&semP_encidata);
		sem_wait(&semC_cache);

		memset(encidata_cache, 0, (ONETIMESIZE + 1) * sizeof(Encidata));
		memcpy(encidata_cache, encidata, (ONETIMESIZE + 1) * sizeof(Encidata));

		printf("cached\n");
		sem_post(&semC_encidata);
		sem_post(&semP_cache);
	}
}

void *MYmysql_Send(void *args)
{
	char sql[3 * KEYSIZE / 2 + 500 * 3];
	struct timeval tpstart, tpend;
	float timeuse;
	int cnt;

	while (1)
	{
		MYSQL *pConn = mysql_init(0); //	初始化数据库

		sem_wait(&semP_cache);

		gettimeofday(&tpstart, NULL);

		if (!mysql_real_connect(pConn, "175.24.183.10", "root", "Zs@123nihao", "encipher_data", 0, 0, 0)) //连接数据库，连接失败放回负值
		{
			printf("无法连接数据库: %s\n", mysql_error(pConn));
			exit(-1);
		}
		printf("连接数据库成功，数据上传中...\n");

		snprintf(sql, sizeof(sql), "%s", "set autocommit=0");
		if (mysql_query(pConn, sql))
		{
			printf("关闭自动提交模式失败,%s\n", mysql_error(pConn));
			exit(-1);
		}

		snprintf(sql, sizeof(sql), "%s", "START TRANSACTION"); //	start transaction
		if (mysql_query(pConn, sql))
		{
			printf("开启事务失败,%s\n", mysql_error(pConn));
			exit(-1);
		}

		cnt = 0;
		while (strncmp((encidata_cache + cnt)->value, "\0", 1) != 0)
		{
			if (strncmp((encidata_cache + cnt + 1)->value, "\0", 1) == 0)
			{
				snprintf(sql, sizeof(sql), "replace into data1(name, number) values('%s', '%s')",
						 (encidata_cache + cnt)->name, (encidata_cache + cnt)->value);

				cnt += 1;
			}
			else if (strncmp((encidata_cache + cnt + 2)->value, "\0", 1) == 0)
			{ // c
				snprintf(sql, sizeof(sql), "replace into data1(name, number) values('%s', '%s'),('%s', '%s')",
						 (encidata_cache + cnt)->name, (encidata_cache + cnt)->value, (encidata_cache + (cnt + 1))->name, (encidata_cache + (cnt + 1))->value);

				cnt += 2;
			}
			else
			{
				snprintf(sql, sizeof(sql), "replace into data1(name, number) values('%s', '%s'),('%s', '%s'),('%s', '%s')",
						 (encidata_cache + cnt)->name, (encidata_cache + cnt)->value, (encidata_cache + (cnt + 1))->name, (encidata_cache + (cnt + 1))->value,
						 (encidata_cache + (cnt + 2))->name, (encidata_cache + (cnt + 2))->value);

				cnt += 3; //一次插入三条数据
			}

			if (mysql_query(pConn, sql)) //执行指令,执行失败返回正值 注意单引号
			{
				printf("%d\n", cnt);
				printf("插入失败,%s", mysql_error(pConn));
				exit(-1);
			}
		}

		snprintf(sql, sizeof(sql), "%s", "commit");
		if (mysql_query(pConn, sql))
		{
			printf("提交事务失败,%s\n", mysql_error(pConn));
		}

		mysql_close(pConn);

		gettimeofday(&tpend, NULL);
		timeuse = (1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec) / 1000;
		printf("send %d data Used Time(ms):%f\n", cnt, timeuse);

		sem_post(&semC_cache);
	}
}

void share(Share_data *share_data)
{
	/*将产生的G_paillier密钥写入共享内存*/

	int shmid;

	shmid = shmget((key_t)0x5005, 12000, 0660 | IPC_CREAT);
	if (shmid == -1)
	{
		printf("0x5005 已存在\n");
	}

	Share_data *pshm;
	pshm = (Share_data *)shmat(shmid, 0, 0);

	memcpy(pshm, share_data, sizeof(Share_data));

	shmdt(pshm);
}

void MYmysql_send3(Encidata *encidata)
{
	MYSQL *pConn = mysql_init(0); //	初始化数据库
	char sql[3 * KEYSIZE / 2 + 500 * 3];

	if (!mysql_real_connect(pConn, "175.24.183.10", "root", "Zs@123nihao", "encipher_data", 0, 0, 0)) //连接数据库，连接失败放回负值
	{
		printf("无法连接数据库: %s\n", mysql_error(pConn));
		exit(-1);
	}
	printf("连接数据库成功\n");

	int cnt = 0;
	while (strncmp((encidata + cnt)->value, "\0", 1) != 0)
	{
		if (strncmp((encidata + cnt + 1)->value, "\0", 1) == 0)
		{
			snprintf(sql, sizeof(sql), "replace into data1(name, number) values('%s', '%s')",
					 (encidata + cnt)->name, (encidata + cnt)->value);
			break;
		}
		else if (strncmp((encidata + cnt + 2)->value, "\0", 1) == 0)
		{
			snprintf(sql, sizeof(sql), "replace into data1(name, number) values('%s', '%s'),('%s', '%s')",
					 (encidata + cnt)->name, (encidata + cnt)->value, (encidata + (cnt + 1))->name, (encidata + (cnt + 1))->value);
			break;
		}
		else
		{
			snprintf(sql, sizeof(sql), "replace into data1(name, number) values('%s', '%s'),('%s', '%s'),('%s', '%s')",
					 (encidata + cnt)->name, (encidata + cnt)->value, (encidata + (cnt + 1))->name, (encidata + (cnt + 1))->value,
					 (encidata + (cnt + 2))->name, (encidata + (cnt + 2))->value);
		}

		if (mysql_query(pConn, sql)) //执行指令,执行失败返回正值 注意单引号
		{
			printf("%d\n", cnt);
			printf("插入失败,%s", mysql_error(pConn));
			exit(-1);
		}

		cnt += 3; //一次插入三条数据
	}

	mysql_close(pConn);
}

void MYmysql_send2(Encidata *encidata, int size)
{
	MYSQL *pConn = mysql_init(0);
	char sql[3 * KEYSIZE / 2 + 500 * 3];

	if (!mysql_real_connect(pConn, "175.24.183.10", "root", "Zs@123nihao", "encipher_data", 0, 0, 0)) //连接数据库，连接失败放回负值
	{
		printf("无法连接数据库: %s\n", mysql_error(pConn));
		exit(-1);
	}
	printf("2连接数据库成功\n");

	snprintf(sql, sizeof(sql), "%s", "set autocommit=0");
	if (mysql_query(pConn, sql))
	{
		printf("关闭自动提交模式失败,%s\n", mysql_error(pConn));
		exit(-1);
	}

	snprintf(sql, sizeof(sql), "%s", "START TRANSACTION"); //	start transaction
	if (mysql_query(pConn, sql))
	{
		printf("开启事务失败,%s\n", mysql_error(pConn));
		exit(-1);
	}

	for (int i = 0; i < size; i += 3)
	{
		if (size - i > 2)
		{
			snprintf(sql, sizeof(sql), "replace into data1(name, number) values('%s', '%s'),('%s', '%s'),('%s', '%s')",
					 (encidata + i)->name, (encidata + i)->value, (encidata + (i + 1))->name, (encidata + (i + 1))->value, (encidata + (i + 2))->name, (encidata + (i + 2))->value);
		}
		else if (size - i == 2)
		{
			snprintf(sql, sizeof(sql), "replace into data1(name, number) values('%s', '%s'),('%s', '%s')",
					 (encidata + i)->name, (encidata + i)->value, (encidata + (i + 1))->name, (encidata + (i + 1))->value);
		}
		else
		{
			snprintf(sql, sizeof(sql), "replace into data1(name, number) values('%s', '%s')",
					 (encidata + i)->name, (encidata + i)->value);
		}

		if (mysql_query(pConn, sql)) //执行指令,执行失败返回正值 注意单引号
		{
			printf("%d\n", i);
			printf("插入失败,%s", mysql_error(pConn));
			exit(-1);
		}
	}

	snprintf(sql, sizeof(sql), "%s", "commit");
	if (mysql_query(pConn, sql))
	{
		printf("提交事务失败,%s\n", mysql_error(pConn));
	}

	mysql_close(pConn);
}