#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Global TABLE
#define TABLE_SIZE 10000000
char *TABLE[TABLE_SIZE] = { 0 };

char *hash(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    //free(digest);
    return out;
}

int unhash(int start, int count, const char *str){
    char *to_unhash = NULL;
    to_unhash = malloc(8);
    for (int i = start; i < start+count; i++){
        sprintf(to_unhash, "%d", i);
        char *hashed = hash(to_unhash, strlen(to_unhash));
        if (strcmp(hashed, str) == 0){
            free(to_unhash);
            return i;
        }
    }
    free(to_unhash);
    return -1;
}

int unhash_timeout(int timeout, const char *str){
    
    int i = 0;
    clock_t end = clock() + ((timeout / 1000) * CLOCKS_PER_SEC * 3);
    while (clock() < end){
        if (TABLE[i] != NULL){
            if (strcmp(TABLE[i], str) == 0){
                return i;
            }
        }else{
            char *to_unhash = NULL;
            to_unhash = malloc(8);
            sprintf(to_unhash, "%d", i);
            char *hashed = hash(to_unhash, strlen(to_unhash));
            TABLE[i] = hashed;
            if (strcmp(hashed, str) == 0){
                free(to_unhash);
                return i;
            }
            free(to_unhash);
        }
        i++;
    }
    return -1;
}

int crack_hash(int start, int end, const char *str){
    char *to_unhash = NULL;
    to_unhash = malloc(24);
    for (int i = start; i < end; i++){
        sprintf(to_unhash, "%d;%d;%d", start, i, end);
        char *hashed = hash(to_unhash, strlen(to_unhash));
        if (strcmp(hashed, str) == 0){
            free(hashed);
            free(to_unhash);
            return i;
        }
        free(hashed);
    }
    free(to_unhash);
    return 0;
}

void free_table(){
    for (int i = 0; i < TABLE_SIZE; i++){
        if (TABLE[i] != NULL){
            free(TABLE[i]);
        }
    }
}