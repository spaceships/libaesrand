#include "aesrand_init.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef RANDFILE
#  define RANDFILE "/dev/urandom"
#endif

int
aes_randinit(aes_randstate_t rng)
{
    int file;
    if ((file = open(RANDFILE, O_RDONLY)) == -1) {
        fprintf(stderr, "Error opening %s\n", RANDFILE);
        return 1;
    } else {
        unsigned char seed[16];
        if (read(file, seed, 16) == -1) {
            fprintf(stderr, "Error reading from %s\n", RANDFILE);
            close(file);
            return 1;
        } else {
            aes_randinit_seedn(rng, (char *) seed, 16, NULL, 0);
        }
    }
    if (file != -1)
        close(file);
    return 0;
}

void
aes_randinit_seedn(aes_randstate_t state, char *seed, size_t seed_len,
                   char *additional, size_t additional_len)
{
    SHA256_CTX sha256;

    state->aes_init = 1;
    state->ctr = 0;

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, seed, seed_len);
    SHA256_Update(&sha256, additional, additional_len);
    SHA256_Final(state->key, &sha256);
}

void
aes_randclear(aes_randstate_t state __attribute__ ((unused)))
{
}

int
aes_randstate_fwrite(aes_randstate_t state, FILE *fp)
{
    fwrite(&state->aes_init, sizeof(state->aes_init), 1, fp);
    fwrite(&state->ctr, sizeof(state->ctr), 1, fp);
    fwrite(state->key, sizeof(state->key), 1, fp);
    return 0;
}

int
aes_randstate_fread(aes_randstate_t state, FILE *fp)
{
    fread(&state->aes_init, sizeof(state->aes_init), 1, fp);
    fread(&state->ctr, sizeof(state->ctr), 1, fp);
    fread(state->key, sizeof(state->key), 1, fp);
    return 0;
}

int
aes_randstate_write(aes_randstate_t state, const char *fname)
{
    FILE *f;
    if ((f = fopen(fname, "w")) == NULL) {
        perror(fname);
        return 1;
    }
    (void) aes_randstate_fwrite(state, f);
    fclose(f);
    return 0;
}

int
aes_randstate_read(aes_randstate_t state, const char *fname)
{
    FILE *f;
    if ((f = fopen(fname, "r")) == NULL) {
        perror(fname);
        return 1;
    }
    (void) aes_randstate_fread(state, f);
    fclose(f);
    return 0;
}