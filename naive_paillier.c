#include <stdio.h>
#include <gmp.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#define KEYSIZE 3072


char* make_msg(char* fmt,mpz_t msg1);
void get_nbits_randprime(mpz_t randprime, int n);
void get_randr(mpz_t r, mpz_t n);
void key_generate(mpz_t pk_n, mpz_t pk_g, mpz_t sk_lambda, mpz_t sk_mu);
void encipher(mpz_t encipher_msg, mpz_t n, mpz_t g, unsigned long m);
unsigned long decipher( mpz_t encipher_msg, mpz_t n ,mpz_t lambda, mpz_t mu);

int main(){
    mpz_t pk_n, pk_g, sk_lambda, sk_mu, randprime,encipher_msg1, encipher_msg2, encipher_msg3;
    unsigned long msg1 = 156748410, msg2 = 548420, msg3;
    mpz_init(pk_n);
    mpz_init(pk_g);
    mpz_init(sk_lambda);
    mpz_init(sk_mu);
    mpz_init(randprime);
    mpz_init(encipher_msg1);
    mpz_init(encipher_msg2);
    mpz_init(encipher_msg3);


    
    struct timeval tpstart,tpend;
    float timeuse;
    // get_nbits_randprime(randprime, KEYSIZE/2);
    // gmp_printf("%Zd\n", randprime);

    // char* str;
    // str = make_msg("%Zd", randprime);
    // printf("%s\n",str);
    // mpz_clear(randprime);
    // free(str);
    key_generate(pk_n, pk_g, sk_lambda, sk_mu);
    // gmp_printf("pk_n:%Zd\n", pk_n);
    // gmp_printf("pk_g:%Zd\n", pk_g);
    // gmp_printf("sk_lambda:%Zd\n", sk_lambda);
    // gmp_printf("sk_mu:%Zd\n", sk_mu);
    printf("key generate\n");

    gettimeofday(&tpstart,NULL);
    encipher(encipher_msg1, pk_n, pk_g, msg1);

    gettimeofday(&tpend,NULL);
    timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    timeuse/=1000;    //sec
    printf("Used Time:%f\n",timeuse);
    encipher(encipher_msg2, pk_n, pk_g, msg2);
    mpz_mul(encipher_msg3, encipher_msg1, encipher_msg2);
    msg3 = decipher(encipher_msg3, pk_n, sk_lambda, sk_mu);
    printf("%ld\n", msg3);
    return 0;
}

void get_nbits_randprime(mpz_t randprime, int n){
    unsigned long seed;
    gmp_randstate_t state;
    mpz_t max, min, tmpmax;
    mpz_init(max);
    mpz_init(min);
    mpz_init(tmpmax);

    mpz_ui_pow_ui(max, 2, n);
    mpz_sub_ui(max, max, 1);
    mpz_ui_pow_ui(min, 2, n-1);
    mpz_sub(tmpmax, max, min);

    gmp_randinit_default(state);
    time(&seed);
    gmp_randseed_ui (state,seed);

    do{
        mpz_urandomm(randprime, state, tmpmax);
        mpz_add(randprime, randprime, min);
        mpz_nextprime(randprime, randprime);
    }while(mpz_cmp(randprime, max) >= 0);
    
    mpz_clear(max);
    mpz_clear(min);
    mpz_clear(tmpmax);
    gmp_randclear(state);
    return;
}
void get_randr(mpz_t r, mpz_t n){
    unsigned long seed;
    gmp_randstate_t state;
    mpz_t gcd_rn;

    mpz_init(gcd_rn);
    gmp_randinit_default(state);
    time(&seed);
    gmp_randseed_ui (state,seed);

    while(1){
        mpz_urandomm(r, state, n);
        mpz_gcd(gcd_rn,r,n);
        if(mpz_cmp_ui(gcd_rn, 1) == 0)
            break;
    }

    mpz_clear(gcd_rn);
    gmp_randclear(state);
    return;
}
void key_generate(mpz_t pk_n, mpz_t pk_g, mpz_t sk_lambda, mpz_t sk_mu){
    mpz_t p, q, gcd_nlmd;
    mpz_init(gcd_nlmd);
    mpz_init(p);
    mpz_init(q);

    // get pk_n,sk_lambda and gcd(pk_n,sk_lambda)==1
    while(1){
        get_nbits_randprime(p, KEYSIZE/2);
        get_nbits_randprime(q, KEYSIZE/2);

        while(mpz_cmp(p,q) == 0){
            get_nbits_randprime(q, KEYSIZE/2);
        }
        mpz_mul(pk_n, p, q);  
        mpz_sub_ui(p, p, 1);    // p=p-1
        mpz_sub_ui(q, q, 1);    // q=q-1
        mpz_mul(sk_lambda, p, q);
        mpz_gcd(gcd_nlmd, pk_n, sk_lambda);
        if(mpz_cmp_ui(gcd_nlmd, 1) == 0){
            break;
        } 
    }

    //get pk_g,sk_mu
    mpz_add_ui(pk_g, pk_n, 1);
    mpz_invert(sk_mu, sk_lambda, pk_n);

    mpz_clear(gcd_nlmd);
    mpz_clear(p);
    mpz_clear(q);
    return;
}

void encipher(mpz_t encipher_msg, mpz_t n, mpz_t g, unsigned long m){
    mpz_t r;
    mpz_t gpowm;
    mpz_t rpown;
    mpz_t npow2;
    mpz_t temp;

    mpz_init(r);
    mpz_init(gpowm);
    mpz_init(rpown);
    mpz_init(npow2);
    mpz_init(temp);

    get_randr(r,n);
    mpz_pow_ui(npow2, n, 2);
    mpz_powm_ui(gpowm, g, m, npow2);
    mpz_powm(rpown, r, n, npow2);
    mpz_mul(temp, gpowm, rpown);
    mpz_mod(encipher_msg, temp, npow2);

    mpz_clear(r);
    mpz_clear(gpowm);
    mpz_clear(rpown);
    mpz_clear(npow2);
    mpz_clear(temp);

    return;
}

unsigned long decipher( mpz_t encipher_msg, mpz_t n ,mpz_t lambda, mpz_t mu){
    unsigned long decipher_msg;
    mpz_t temp;
    mpz_t npow2;

    mpz_init(temp);
    mpz_init(npow2);

    mpz_pow_ui(npow2, n, 2);
    mpz_powm(temp, encipher_msg, lambda, npow2);
    mpz_sub_ui(temp, temp, 1);
    mpz_fdiv_q(temp, temp, n);
    mpz_mul(temp, temp, mu);
    mpz_mod(temp, temp, n);

    decipher_msg = mpz_get_ui(temp);
    return decipher_msg;
}

char* make_msg(char* fmt,mpz_t msg1){
    int n, size = 100;
    char* p;
    p = (char*)malloc(size*sizeof(char));

    while(1)
    { 
        n = gmp_snprintf(p, size, fmt, msg1);
        if(n>-1 && n<size){
            printf("%d\n",n);
            printf("%s\n",p);
            return p;
        }
        size *= 2;
        if ((p = (char *)realloc(p, size*sizeof(char))) == NULL)
            return NULL;
    }
}