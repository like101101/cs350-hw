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


int NUM_THREADS = 1;
int NUM_JOBS;
int WORK_PER_THREAD = 0;
int TIMEOUT = 1000;
int NUM_COMPLETED;
int NUM_HINTS = 0;
hint_node_t *HINTS = NULL;
pthread_mutex_t lock;


void split_crackables(crackable_node_t* assignment[], crackable_node_t* head){

        //Adjusting work amount

    if (NUM_THREADS == 1){
        WORK_PER_THREAD = NUM_JOBS;
    }else if (NUM_JOBS % NUM_THREADS == 0){
        WORK_PER_THREAD = NUM_JOBS / NUM_THREADS;
    }else if(NUM_JOBS > NUM_THREADS){
        WORK_PER_THREAD = (NUM_JOBS / NUM_THREADS) + 1;
    }else{
        WORK_PER_THREAD = 1;
    }

    int i = 0, idx = 0;
    
    for (int j = 0; j < NUM_THREADS + 1; j++){
        assignment[j] = NULL;
    }
    
    crackable_node_t* current = head;
    while ((current != NULL) && (current->data[0] != '\0')){
        if (i % WORK_PER_THREAD == 0){
            assignment[idx] = current;
            if (idx == NUM_THREADS){
                return;
            }
            idx++;
        }
        i++;
        current = current->next;
    }
    return;
}

void split_uncrackables(uncrackable_node_t* assignment[], uncrackable_node_t* head){

    if (NUM_THREADS == 1){
        WORK_PER_THREAD = NUM_JOBS;
    }else if (NUM_JOBS % NUM_THREADS == 0){
        WORK_PER_THREAD = NUM_JOBS / NUM_THREADS;
    }else if(NUM_JOBS > NUM_THREADS){
        WORK_PER_THREAD = (NUM_JOBS / NUM_THREADS) + 1;
    }else{
        WORK_PER_THREAD = 1;
    }
    
    int i = 0, idx = 1;
    
    for (int j = 0; j < NUM_THREADS + 1; j++){
        assignment[j] = NULL;
    }
    
    uncrackable_node_t* current = head;
    assignment[0] = current;
    while ((current != NULL)){
        current = current->next;
        i++;
        if (i == WORK_PER_THREAD){
            assignment[idx] = current;
            i = 0;
            if (idx == NUM_THREADS){
                return;
            }
            idx++;
        }
    }
    return;
}

uncrackable_node_t * create_uncrackbles(crackable_node_t* head){
    //Adjusting work amount


    NUM_COMPLETED = 0;
    NUM_JOBS = 0;
    crackable_node_t* current = head;
    crackable_node_t* uncrackable = NULL;
    crackable_node_t* uncrackable_head = NULL;
    while (current != NULL){
        if (current->result == -1){
            if (uncrackable == NULL){
                uncrackable = malloc(sizeof(crackable_node_t));
                uncrackable_head = uncrackable;
            } else {
                uncrackable->next = malloc(sizeof(crackable_node_t));
                uncrackable = uncrackable->next;
            }
            strcpy(uncrackable->data, current->data);
            uncrackable->result = NULL;
            uncrackable->next = NULL;
            NUM_JOBS ++;
        }
        current = current->next;
    }
    return uncrackable_head;
}

hint_node_t* create_hints(crackable_node_t* head){
    hint_node_t *list = NULL;
    hint_node_t *current = NULL;
    crackable_node_t *current_node = head;
    while ((current_node != NULL)){
        if (current_node->result != -1){
            if (list == NULL){
                list = (hint_node_t*)malloc(sizeof(hint_node_t));
                list->val = current_node->result;
                list->next = NULL;
                current = list;
            }
            else{
                current->next = (hint_node_t*)malloc(sizeof(hint_node_t));
                current = current->next;
                current->val = current_node->result;
                current->next = NULL;
            }
            NUM_HINTS++;
        }
        current_node = current_node->next;
    }
    return list;
}

crackable_node_t * creat_crackables(FILE *fp){
    char *buf = NULL;
    size_t bufsize = 32;
    ssize_t len = 1;

    NUM_COMPLETED = 0;
    NUM_JOBS = 0;
    crackable_node_t *head = malloc(sizeof(crackable_node_t));
    head->next = NULL;
    head->result = -1;
    crackable_node_t *unhashes = head;
    
    len = getline(&buf, &bufsize, fp);
    NUM_JOBS ++;
    while (len > 0){
	char *ptr = strchr(buf, '\n');
	if (ptr){ *ptr='\0';}
        strcpy(head->data, buf);
        len = getline(&buf, &bufsize, fp);
        if (len > 0){
            head->next = malloc(sizeof(crackable_node_t));
            head->next->next = NULL;
            head->result = 0;
            head = head->next;
            NUM_JOBS ++;
        }else{
            head->next = NULL;
        }
    }
    
    free(buf);
    fclose(fp);
    return unhashes;
}

void print_result(crackable_node_t *head){
    while ((head != NULL) && (head->data[0] != '\0')){
        crackable_node_t *current = head;
        if (current->result == -1){
            printf("%s\n", current->data);
        }else{
            printf("%d\n", head->result);
        }
        head = head->next;
        //free(current);
    }
}

void print_hints(hint_node_t *hints){

    int i = 1;
    while (hints != NULL){
        printf("%d\n", hints->val);
        hint_node_t *current = hints;
        hints = hints->next;
        i++;
        //free(current);
    }
}

//compare function for qsort
int compare(const void *a, const void *b){
    return (*(int*)a - *(int*)b);
}

void sort_hints(hint_node_t *hints){

    int i = 0;
    hint_node_t *head = hints;
    int *arr = malloc(sizeof(int) * NUM_HINTS);
    while (hints != NULL){
        arr[i] = hints->val;
        hints = hints->next;
        i++;
    }
    qsort(arr, NUM_HINTS, sizeof(int), compare);
    for (int j = 0; j < NUM_HINTS; j++){
        head->val = arr[j];
        head = head->next;
    }
    free(arr);
}

void print_uncrackables(uncrackable_node_t *head){
    while (head != NULL){
        uncrackable_node_t *current = head;
        head = head->next;
        //free(current);
    }
}

void* break_crackables(crackable_node_t *unhashes) {
    int i = 0;
    while ((unhashes != NULL) && (unhashes->data[0] != '\0') && (unhashes->result == 0)){
        unhashes->result = unhash_timeout(TIMEOUT, unhashes->data);
        unhashes = unhashes->next;
        NUM_COMPLETED++;
        i++;
        if (i == WORK_PER_THREAD){
            goto LOOP;
        }
    }
    
    LOOP: if(NUM_COMPLETED == NUM_JOBS){
        pthread_exit(NULL);
    }else{
        goto LOOP;
    }
}

void* break_uncrackables(uncrackable_node_t *unhashes){

    int i = 0;
    while ((unhashes != NULL) && (unhashes->data[0] != '\0')&& (unhashes->result == NULL)){
        hint_node_t *head = HINTS;
        hint_node_t *tail = head;

        //printf("Thread %d is working on %s\n", (int)pthread_self(), unhashes->data);
        while ((head != NULL)){
            if (head->val != -1) {
                while ((tail != NULL)){
                    if (tail->val != -1) {
                        char *result = crack_hash(head->val, tail->val, unhashes->data);
                        //printf("checking: %d %d %s\n", head->val, tail->val, unhashes->data);
                        if (result != NULL){
                            unhashes->result = result;
                            printf("%s\n", result);
                            head->val = -1;
                            tail->val = -1;
                            goto CURRENT_COMPLETE;
                        }
                    }
                    tail = tail->next;
                }
            }
            head = head->next;
            tail = head;
        }

        unhashes->result = "-1";
        printf("%s\n", unhashes->data);
        CURRENT_COMPLETE: 
        i++;
        NUM_COMPLETED++;
        if (i == WORK_PER_THREAD){
            goto LOOP;
        }
        unhashes = unhashes->next;
    }
    
    LOOP: if(NUM_COMPLETED == NUM_JOBS){
        pthread_exit(NULL);
    }else{
        goto LOOP;
    }
}

void clean_up(crackable_node_t *unhashes, uncrackable_node_t *unhashables, hint_node_t *hints){
    crackable_node_t *current = unhashes;
    crackable_node_t *next;
    while (current != NULL){
        next = current->next;
        free(current);
        current = next;
    }
    uncrackable_node_t *current2 = unhashables;
    uncrackable_node_t *next2;
    while (current2 != NULL){
        next2 = current2->next;
        free(current2);
        current2 = next2;
    }
    hint_node_t *current3 = hints;
    hint_node_t *next3;
    while (current3 != NULL){
        next3 = current3->next;
        free(current3);
        current3 = next3;
    }
    return;
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

    }else if (argc == 3){
        fp = fopen(argv[1], "r");
        NUM_THREADS = atoi(argv[2]);
    }else if (argc == 4){
        fp = fopen(argv[1], "r");
        NUM_THREADS = 10; //atoi(argv[2]);
        TIMEOUT = atoi(argv[3]);
    }else{
        printf("Usage: %s <hash_file> <num_threads> <timeout>\n", argv[0]);
        return 1;
    }
    
    if (fp == NULL){
        printf("Error opening file\n");
        exit(1);
    }

    pthread_mutex_init(&lock, NULL);

    crackable_node_t *unhashes = creat_crackables(fp);
    
    crackable_node_t *crackable_assignment[NUM_THREADS+1];
    uncrackable_node_t *uncrackable_assignment[NUM_THREADS+1];
    pthread_t tid[NUM_THREADS+1];

    //First Run
    split_crackables(crackable_assignment, unhashes);

 
    for (int j = 0; j < NUM_THREADS; j++){
        err = pthread_create(&(tid[j]), NULL, &break_crackables, crackable_assignment[j]);
        if (err != 0)
            printf("can't create thread :[%s]", strerror(err));
    }
    
    for (int j = 0; j < NUM_THREADS; j++){
        pthread_join(tid[j], NULL);
    }
    

    // Parsing some results
    HINTS = create_hints(unhashes);
    uncrackable_node_t *uncrackables = create_uncrackbles(unhashes);


    // Split work for second run and sort hints
    split_uncrackables(uncrackable_assignment, uncrackables);
    print_hints(HINTS);
    sort_hints(HINTS);
    
    // Second Run
    
    for (int j = 0; j < NUM_THREADS; j++){
        err = pthread_create(&(tid[j]), NULL, &break_uncrackables, uncrackable_assignment[j]);
        if (err != 0)
            printf("can't create thread :[%s]", strerror(err));
    }
    
    for (int j = 0; j < NUM_THREADS; j++){
        pthread_join(tid[j], NULL);
    }
    
    
    clean_up(unhashes, uncrackables, HINTS);

    //destory the lock
    pthread_mutex_destroy(&lock);

    return 0;
}
