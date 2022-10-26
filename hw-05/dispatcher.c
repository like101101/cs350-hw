#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "hashutil.h"

#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif


int NUM_THREADS = 8;
int NUM_JOBS;
int WORK_PER_THREAD = 0;

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
        if (i == WORK_PER_THREAD){
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
    NUM_JOBS ++;
    while (len > 0){
        strtok(buf, "\n");
        strcpy(head->data, buf);
        head->next = malloc(sizeof(struct node));
        head->next->next = NULL;
        head->result = -1;
        head = head->next;
        NUM_JOBS ++;
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
        unhashes->result = unhash(0, 1000000, unhashes->data);
        unhashes = unhashes->next;
        runs ++;
        if (runs == WORK_PER_THREAD && WORK_PER_THREAD != -1){
            break;
        }
    }

    return NULL;

}


// Main Driver  
int main(int argc, char **argv) {


    FILE *fp;
    int err;
    clock_t start, end;
    char *buf = NULL;
    size_t bufsize = 33;

    if (argc == 1){
        //default reading
        fp = fopen("hashes.txt", "r");


    }else if (argc == 2){
        fp = fopen(argv[1], "r");

    }else{
        fp = fopen(argv[1], "r");
        NUM_THREADS = atoi(argv[2]);
    }
    
    
    if (fp == NULL){
        printf("Error opening file");
        exit(1);
    }


    struct node *unhashes = read_file(fp);
    fclose(fp);
    
    if (NUM_THREADS == 1){
        WORK_PER_THREAD = NUM_JOBS;
    }else{
        if (NUM_JOBS % NUM_THREADS == 0){
            WORK_PER_THREAD = NUM_JOBS / NUM_THREADS;
        }else{
            WORK_PER_THREAD = (NUM_JOBS / NUM_THREADS)+1;
        }   
    }
    
    struct node *assignment[NUM_THREADS];
    pthread_t tid[NUM_THREADS];

    split_work(assignment, unhashes);
    
    //start = clock();
    for (int j = 0; j < NUM_THREADS; j++){
        err = pthread_create(&(tid[j]), NULL, &dowork, assignment[j]);
        if (err != 0)
            printf("can't create thread :[%s]", strerror(err));
    }

    for (int j = 0; j < NUM_THREADS; j++){
        pthread_join(tid[j], NULL);
    }
    print_result(unhashes);
    //end = clock();
    //printf("Time taken: %f\n", ((double) (end - start)) / CLOCKS_PER_SEC);
    pthread_exit(NULL);

    return 0;
}
