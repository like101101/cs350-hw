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
#include "hashutil.h"
#include <pthread.h>

int NUM_THREADS = 10;

void* unhash_one(void *arguments){
    struct unhash_args *args = arguments;
    int result = unhash(args->start, args->count, args->hash);
    if (result != -1){
        printf("%d\n", result);
        return NULL;
    }
    
    free(args);
    return NULL;

}

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage: %s <hash>\n", argv[0]);
        return 1;
    }

    pthread_t tid[NUM_THREADS];
    struct unhash_args *threads[NUM_THREADS];

    for (int j = 0; j < NUM_THREADS; j++){
        struct unhash_args *args = malloc(sizeof(struct unhash_args));
        args->start = j * 100000;
        args->count = 100000;
        args->hash = argv[1];
        int err = pthread_create(&(tid[j]), NULL, &unhash_one, args);
        if (err != 0)
            printf("can't create thread :[%s]", strerror(err));
    }

    for (int j = 0; j < NUM_THREADS; j++){
        pthread_join(tid[j], NULL);
    }

    pthread_exit(NULL);
    
    return 0;
}
