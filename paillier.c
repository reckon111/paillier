#include "paillier.h"

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

void get_p(mpz_t p, int n){
    mpz_t ptemp;

    mpz_init(ptemp);

    while(1){
        get_nbits_randprime(p, n);     
        mpz_mod_ui(ptemp, p ,4);
        if(mpz_cmp_ui(ptemp, 3) == 0){
            break;
        }
    }
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

void key_generate(mpz_t pk_n, mpz_t pk_g, mpz_t sk_lambda, mpz_t sk_mu, int keysize){
    mpz_t p, q, gcd_nlmd;
    mpz_init(gcd_nlmd);
    mpz_init(p);
    mpz_init(q);

    // get pk_n,sk_lambda and gcd(pk_n,sk_lambda)==1
    while(1){
        get_nbits_randprime(p, keysize/2);
        get_nbits_randprime(q, keysize/2);

        while(mpz_cmp(p,q) == 0){
            get_nbits_randprime(q, keysize/2);
        }
        mpz_mul(pk_n, p, q); 

		if(mpz_sizeinbase(pk_n, 2)!=keysize)
		{
			continue;
		} 

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
    mpz_t r;        //随机数 r
    mpz_t gpowm;
    mpz_t rpown;
    mpz_t npow2;
    mpz_t temp;
    
    //mpz_t init
    mpz_init(r);
    mpz_init(gpowm);
    mpz_init(rpown);
    mpz_init(npow2);
    mpz_init(temp);

    //加密数据
    get_randr(r,n);                 //获取随机数 r 
    mpz_pow_ui(npow2, n, 2);        //npow2 = n^2
    mpz_powm_ui(gpowm, g, m, npow2);//gpown = g^m mod n^2
    mpz_powm(rpown, r, n, npow2);   //rpown = r^n mod n^2
    mpz_mul(temp, gpowm, rpown);    
    mpz_mod(encipher_msg, temp, npow2);//get encipher_msg

    //mpz_t clear
    mpz_clear(r);
    mpz_clear(gpowm);
    mpz_clear(rpown);
    mpz_clear(npow2);
    mpz_clear(temp);

    return;
}

unsigned long decipher( mpz_t encipher_msg, mpz_t n ,mpz_t lambda, mpz_t mu){
    unsigned long decipher_msg;     //解密后数据
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

    mpz_clear(temp);
    mpz_clear(npow2);

    return decipher_msg;
}

void mpz_encipher(mpz_t encipher_msg, mpz_t n, mpz_t g, mpz_t m){
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

    get_randr(r, n);
    mpz_pow_ui(npow2, n, 2);
    mpz_powm(gpowm, g, m, npow2);  //改这
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

void mpz_decipher(mpz_t encipher_msg, mpz_t decipher_msg, mpz_t n ,mpz_t lambda, mpz_t mu){
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

    mpz_set(decipher_msg, temp);

    mpz_clear(temp);
    mpz_clear(npow2);
}

void key_generate_G(mpz_t pk_n, mpz_t pk_g, mpz_t hs, mpz_t sk_lambda, mpz_t sk_mu, int keysize){
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
    while(1)
	{
		get_p(p, keysize/2);
		mpz_sub_ui(p_1, p, 1);    // p=p-1
		while(1){
			get_p(q, keysize/2);
			mpz_sub_ui(q_1, q, 1);
			mpz_gcd(gcd_2,p_1, q_1);
			if(mpz_cmp_ui(gcd_2, 2) == 0){
				break;
			}
		}
		mpz_mul(pk_n, p, q); 
		if(mpz_sizeinbase(pk_n, 2) == keysize)
		{
			break;
		} 		
	}

    

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

void encipher_G(mpz_t encipher_msg, mpz_t n, mpz_t g, mpz_t hs, unsigned long m){
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

    get_nbits_randnumb(a, (mpz_sizeinbase(n,2)+1)/2);	//	(mpz_sizeinbase(n,2)+1)/2/2
    // int a_size = mpz_sizeinbase(a, 2);
	// printf("a_size:%d\n",a_size);

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


unsigned long decipher_G( mpz_t encipher_msg, mpz_t n ,mpz_t lambda, mpz_t mu){
    unsigned long decipher_msg;
    mpz_t temp;
    mpz_t npow2;

    mpz_init(temp);
    mpz_init(npow2);

    mpz_pow_ui(npow2, n, 2);
    
    // gmp_printf("n:%Zd", n);
    // gmp_printf("lambda:%Zd", lambda);


    mpz_powm(temp, encipher_msg, lambda, npow2);
    mpz_sub_ui(temp, temp, 1);
    mpz_fdiv_q(temp, temp, n);
    mpz_mul(temp, temp, mu);
    mpz_mod(temp, temp, n);
    decipher_msg = mpz_get_ui(temp);
    // printf("decipher_msg:%ld\n",decipher_msg);
    return decipher_msg;
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

void mpz_encipher_G(mpz_t encipher_msg, mpz_t n, mpz_t g, mpz_t hs, mpz_t m){
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
    get_nbits_randnumb(a, (mpz_sizeinbase(n,2)+1)/2);
    // int a_size = mpz_sizeinbase(a, 2);
	// printf("a_size:%d\n",a_size);

    mpz_mul(gpowm, n, m);       //改这
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

void mpz_decipher_G(mpz_t decipher_msg, mpz_t encipher_msg, mpz_t n ,mpz_t lambda, mpz_t mu){
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

	mpz_set(decipher_msg, temp);

	mpz_clear(temp);
    mpz_clear(npow2);
}