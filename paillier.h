#ifndef __PAILLIER_H__
#define __PAILLIER_H__

// #ifndef __INCLUDE_H__
// #define __INCLUDE_H__
// #endif
#include <gmp.h>
#include <time.h>
#include <stdio.h>

void get_nbits_randprime(mpz_t randprime, int n);
void get_p(mpz_t p, int n); // get prime p = 3 mod 4 by using function: get_nbits_randprime()

void get_randr(mpz_t r, mpz_t n); // get r in naive_paillier

void key_generate(mpz_t pk_n, mpz_t pk_g, mpz_t sk_lambda, mpz_t sk_mu, int keysize);
void encipher(mpz_t encipher_msg, mpz_t n, mpz_t g, unsigned long m);
unsigned long decipher( mpz_t encipher_msg, mpz_t n ,mpz_t lambda, mpz_t mu);

void get_nbits_randnumb(mpz_t randnumb, int n);
void key_generate_G(mpz_t pk_n, mpz_t pk_g, mpz_t hs, mpz_t sk_lambda, mpz_t sk_mu, int keysize);
void encipher_G(mpz_t encipher_msg, mpz_t n, mpz_t g, mpz_t hs, unsigned long m);
unsigned long decipher_G( mpz_t encipher_msg, mpz_t n ,mpz_t lambda, mpz_t mu);

#endif