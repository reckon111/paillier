#include <stdio.h>
#include "paillier.h"
#include <sys/time.h>
#include <gmp.h>

#define KEYSIZE 4096

int main(){
	mpz_t pk_n, pk_g, hs, sk_lambda, sk_mu, randprime,encipher_msg1, decipher_msg, encipher_msg3, msg, npow2;
    unsigned long msg1 = 156748410, msg2 = 548420, msg3;
    mpz_init(pk_n);
    mpz_init(pk_g);
    mpz_init(hs);
    mpz_init(sk_lambda); 
    mpz_init(sk_mu);
    mpz_init(randprime);
    mpz_init(encipher_msg1);
    mpz_init(decipher_msg);
    mpz_init(encipher_msg3);
    mpz_init(msg);
	mpz_init(npow2);

    struct timeval tpstart,tpend;
    struct timeval tpstart2,tpend2;
    struct timeval tpstart3,tpend3;
    float timeuse,timeuse2,timeuse3;


    gettimeofday(&tpstart2,NULL);

	key_generate(pk_n, pk_g, sk_lambda, sk_mu, KEYSIZE);
    gettimeofday(&tpend2,NULL);
    printf("key generate\n");
    timeuse2=1000000*(tpend2.tv_sec-tpstart2.tv_sec)+tpend2.tv_usec-tpstart2.tv_usec;
    timeuse2/=1000;    //ms
    printf("Genkey Used Time(ms):%f\n",timeuse2);

	int n_size = mpz_sizeinbase(pk_n, 2);   //16进制下，结果为keysize/4
    printf("n_size: %d\n", n_size);

	//-----------保证 msg < n -------------
	while(1)
	{
		get_nbits_randnumb(msg, KEYSIZE);
		if(mpz_cmp(pk_n, msg) > 0){
			break;
		}
	}
	int msg_size = mpz_sizeinbase(msg, 2);
	printf("msg_size:%d\n",msg_size);


    gettimeofday(&tpstart,NULL);
    mpz_encipher(encipher_msg1, pk_n, pk_g, msg);
    gettimeofday(&tpend,NULL);
    timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    timeuse/=1000;    //sec
    printf("Enc Used Time(ms):%f\n",timeuse);

    gettimeofday(&tpstart3,NULL);
    mpz_decipher(encipher_msg1, decipher_msg, pk_n, sk_lambda, sk_mu);
	gettimeofday(&tpend3,NULL);
    timeuse3=1000000*(tpend3.tv_sec-tpstart3.tv_sec)+tpend3.tv_usec-tpstart3.tv_usec;
    timeuse3/=1000;    //sec
    printf("Dec Used Time(ms):%f\n",timeuse3);
    gmp_printf("msg:%Zd\n", msg);
	gmp_printf("decipher_msg:%Zd\n", decipher_msg);



    return 0;
}