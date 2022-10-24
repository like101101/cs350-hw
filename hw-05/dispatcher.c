#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "hashutil.h"

#define NUM_THREADS 1
#define NUM_JOBS 192
#define WORK_PER_THEARD -1

#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif

pthread_t tid[NUM_THREADS];
pthread_mutex_t lock;

struct node {
    char data[33];
    struct node *next;
    int result;
};


void split_work(struct node* assignment[], struct node* head){
    int i = 0, idx = 0;
    struct node* current = head;
    assignment[0] = current;
    while (current->data[0] != '\0'){
        current = current->next;
        i++;
        if (i == WORK_PER_THEARD){
            idx++;
            assignment[idx] = current;
            i = 0;
        }
    }
}


struct node * read_file(FILE *fp){
    char *buf = NULL;
    size_t bufsize = 32;
    ssize_t len = 0;

    struct node *head = malloc(sizeof(struct node));
    head->next = NULL;
    head->result = -1;
    struct node *unhashes = head;

    len = getline(&buf, &bufsize, fp);
    while (len > 0){
        strtok(buf, "\n");
        strcpy(head->data, buf);
        head->next = malloc(sizeof(struct node));
        head->next->next = NULL;
        head->result = -1;
        head = head->next;
        len = getline(&buf, &bufsize, fp);
    }
    
    free(buf);
    return unhashes;
}

void print_result(struct node *head){
    while (head->data[0] != '\0'){
        struct node *current = head;
        printf("%d\n", head->result);
        head = head->next;
        free(current);
    }
}

void* dowork(struct node *unhashes) {

    int runs = 0;
    while (unhashes->data[0] != '\0'){
        unhashes->result = unhash(0, unhashes->data);
        unhashes = unhashes->next;
        runs ++;
        if (runs == WORK_PER_THEARD && WORK_PER_THEARD != -1){
            break;
        }
    }

    return NULL;

}


// Main Driver  
int main(int argc, char **argv) {


    FILE *fp;
    int i = 0;
    int err;
    int len = 0;
    clock_t start, end;
    char *buf = NULL;
    size_t bufsize = 33;

    int DEBUG = 0;

    struct node *assignment[NUM_THREADS];
    
    if (DEBUG){
        fp = fopen("hashes.txt", "r");
    }else{
        fp = fopen(argv[1], "r");
    }
    
    if (fp == NULL){
        printf("Error opening file");
        exit(1);
    }

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }


    struct node *unhashes = read_file(fp);
    fclose(fp);

    split_work(assignment, unhashes);
    
    start = clock();
    for (int j = 0; j < NUM_THREADS; j++){
        err = pthread_create(&(tid[j]), NULL, &dowork, assignment[j]);
        if (err != 0)
            printf("can't create thread :[%s]", strerror(err));
    }

    for (int j = 0; j < NUM_THREADS; j++){
        pthread_join(tid[j], NULL);
    }
    print_result(unhashes);
    end = clock();
    //printf("Time taken: %f\n", ((double) (end - start)) / CLOCKS_PER_SEC);
    pthread_exit(NULL);

    return 0;
}
