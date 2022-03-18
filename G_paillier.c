#include <stdio.h>
#include <gmp.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>

#define KEYSIZE 3072

void get_nbits_randnumb(mpz_t randnumb, int n);
void get_p(mpz_t p);
char* make_msg(char* fmt,mpz_t msg1);
void get_nbits_randprime(mpz_t randprime, int n);
void get_randr(mpz_t r, mpz_t n);
void key_generate(mpz_t pk_n, mpz_t pk_g, mpz_t hs, mpz_t sk_lambda, mpz_t sk_mu);
void encipher(mpz_t encipher_msg, mpz_t n, mpz_t g, mpz_t hs, unsigned long m);
unsigned long decipher( mpz_t encipher_msg, mpz_t n ,mpz_t lambda, mpz_t mu);

int main(){
    mpz_t pk_n, pk_g, hs, sk_lambda, sk_mu, randprime,encipher_msg1, encipher_msg2, encipher_msg3, npow2;
    unsigned long msg1 = 156748410, msg2 = 548420, msg3;
    mpz_init(pk_n);
    mpz_init(pk_g);
    mpz_init(hs);
    mpz_init(sk_lambda); 
    mpz_init(sk_mu);
    mpz_init(randprime);
    mpz_init(encipher_msg1);
    mpz_init(encipher_msg2);
    mpz_init(encipher_msg3);
    mpz_init(npow2);

    clock_t start, stop;
    
	double s=0;

    struct timeval tpstart,tpend;
    float timeuse;
    // get_nbits_randprime(randprime, KEYSIZE/2);
    // gmp_printf("%Zd\n", randprime);

    // char* str;
    // str = make_msg("%Zd", randprime);
    // printf("%s\n",str);
    // mpz_clear(randprime);
    // free(str);
    key_generate(pk_n, pk_g, hs, sk_lambda, sk_mu);
    // gmp_printf("pk_n:%Zd\n", pk_n);
    // gmp_printf("pk_g:%Zd\n", pk_g);
    // gmp_printf("sk_lambda:%Zd\n", sk_lambda);
    // gmp_printf("sk_mu:%Zd\n", sk_mu);
    printf("key generate\n");
    mpz_pow_ui(npow2, pk_n, 2);

    gettimeofday(&tpstart,NULL);
    encipher(encipher_msg1, pk_n, pk_g, hs, msg1);
    gettimeofday(&tpend,NULL);
    timeuse=100000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    timeuse/=1000;    //sec
    printf("Used Time:%f\n",timeuse);

    encipher(encipher_msg2, pk_n, pk_g, hs, msg2);
    mpz_mul(encipher_msg3, encipher_msg1, encipher_msg1);
    mpz_mod(encipher_msg3, encipher_msg3, npow2);
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
void key_generate(mpz_t pk_n, mpz_t pk_g, mpz_t hs, mpz_t sk_lambda, mpz_t sk_mu){
    mpz_t p, q, gcd_2, p_1, q_1, x, h, npow2;
    mpz_init(gcd_2);
    mpz_init(p);
    mpz_init(q);
    mpz_init(p_1);
    mpz_init(q_1);
    mpz_init(x);
    mpz_init(h);
    mpz_init(npow2);
 
    // get pk_n,sk_lambda and gcd(pk_n,sk_lambda)==1
    
    get_p(p);
    mpz_sub_ui(p_1, p, 1);    // p=p-1
    while(1){
        get_p(q);
        mpz_sub_ui(q_1, q, 1);
        mpz_gcd(gcd_2,p_1, q_1);
        if(mpz_cmp_ui(gcd_2, 2) == 0){
            break;
        }
    }

    
    mpz_mul(pk_n, p, q);  
    mpz_pow_ui(npow2, pk_n, 2);

    get_randr(x, pk_n);
    mpz_pow_ui(x, x, 2);
    mpz_ui_sub(x, 0, x);
    mpz_mod(h, x, pk_n);
    mpz_powm(hs, h, pk_n, npow2);

    
    mpz_mul(sk_lambda, p_1, q_1);
    mpz_fdiv_q_ui(sk_lambda, sk_lambda, 2);

    //get pk_g,sk_mu
    mpz_add_ui(pk_g, pk_n, 1);
    mpz_invert(sk_mu, sk_lambda, pk_n);

    mpz_clear(gcd_2);
    mpz_clear(p);
    mpz_clear(q);
    mpz_clear(p_1);
    mpz_clear(q_1);
    return;
}

void encipher(mpz_t encipher_msg, mpz_t n, mpz_t g, mpz_t hs, unsigned long m){
    mpz_t a;
    mpz_t gpowm;
    mpz_t hspowa;
    mpz_t npow2;
    mpz_t temp;

    mpz_init(a);
    mpz_init(gpowm);
    mpz_init(hspowa);
    mpz_init(npow2);
    mpz_init(temp);

    mpz_pow_ui(npow2, n, 2);
    get_nbits_randnumb(a, KEYSIZE/2);
    
    mpz_mul_ui(gpowm, n, m);      
    mpz_add_ui(gpowm, gpowm, 1);

    mpz_powm(hspowa, hs, a, npow2);
    mpz_mul(temp, gpowm, hspowa);
    mpz_mod(encipher_msg, temp, npow2);

    mpz_clear(a);
    mpz_clear(gpowm);
    mpz_clear(hspowa);
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

void get_p(mpz_t p){
    mpz_t ptemp;

    mpz_init(ptemp);

    while(1){
        get_nbits_randprime(p, KEYSIZE/2);     
        mpz_mod_ui(ptemp, p ,4);
        if(mpz_cmp_ui(ptemp, 3) == 0){
            break;
        }
}
}

void get_nbits_randnumb(mpz_t randnumb, int n){
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
        mpz_urandomm(randnumb, state, tmpmax);
        mpz_add(randnumb, randnumb, min);
        // mpz_nextprime(randprime, randprime);
    }while(mpz_cmp(randnumb, max) >= 0);
    
    mpz_clear(max);
    mpz_clear(min);
    mpz_clear(tmpmax);
    gmp_randclear(state);
    return;
}