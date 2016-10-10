#include "aesrand_buf.h"

#include <string.h>
#include <openssl/evp.h>

unsigned char *
random_aes(aes_randstate_t state, size_t *n)
{
    const size_t nbytes = *n / 8 + 1;
    unsigned char *output, *buf;
    size_t outlen = 0;
    EVP_CIPHER_CTX *ctx;

    ctx = EVP_CIPHER_CTX_new();
    output = malloc(2 * (nbytes + EVP_MAX_IV_LENGTH));

    {
        unsigned char *in;
        unsigned char *iv;
        const int in_size = nbytes;
        int final_len = 0;

        in = calloc(in_size, sizeof(unsigned char));
        iv = calloc(EVP_CIPHER_iv_length(AES_ALGORITHM), sizeof(unsigned char));

#pragma omp critical
        {
            memcpy(iv, &state->ctr, sizeof(state->ctr));
            state->ctr++;
        }

        /* Compute E_K(0 || 0 || ...) */
        EVP_EncryptInit_ex(ctx, AES_ALGORITHM, NULL, state->key, iv);
        while (outlen < nbytes) {
            int buflen = 0;
            EVP_EncryptUpdate(ctx, output + outlen, &buflen, in, in_size);
            outlen += buflen;
        }

        EVP_EncryptFinal(ctx, output + outlen, &final_len);
        outlen += final_len;

        if (outlen > nbytes) {
            outlen = nbytes; // we will only use nbytes bytes
        }
        free(in);
        free(iv);
    }

    /* Convert to format amenable by mpz_inp_raw */
    {
        const size_t true_len = outlen + 4;
        size_t bytelen = outlen;

        buf = calloc(true_len, sizeof(unsigned char));
        memcpy(buf + 4, output, outlen);
        buf[4] >>= ((outlen * 8) - (unsigned int) *n);

        for (int i = 3; i >= 0; i--) {
            buf[i] = (unsigned char) (bytelen % (1 << 8));
            bytelen /= (1 << 8);
        }

        free(output);
        *n = true_len;
    }

    EVP_CIPHER_CTX_cleanup(ctx);
    free(ctx);

    return buf;
}
