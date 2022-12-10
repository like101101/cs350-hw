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

// Global State
int NUM_THREADS = 1;
int TIMEOUT = 1000;
int NUM_BROKEN = 0;
int NUM_HINTS = 0;
int NUM_TOTAL_HINTS = 0;
int NUM_NEXT_HINTS = 0;
int NUM_UNCRACKABLES = 0;


// Global Queue

hint_node_t *HINTS = NULL;
hint_node_t *NEXT_HINTS = NULL;
hint_node_t *TOTAL_HINTS = NULL;
crackable_node_t *CRACKABLES = NULL;
crackable_node_t *CRACKABLES_QUEUE = NULL;
uncrackable_node_t *UNCRACKABLES = NULL;
uncrackable_node_t *UNCRACKABLES_QUEUE = NULL;

pthread_mutex_t next_hint_lock;
pthread_mutex_t broken_lock;
pthread_mutex_t queue_lock;


void create_uncrackbles(){
    //Adjusting work amount

    crackable_node_t* current = CRACKABLES;
    uncrackable_node_t* uncrackable = NULL;
    uncrackable_node_t* uncrackable_head = NULL;
    while (current != NULL){
        if (current->result == -1){
            if (uncrackable == NULL){
                uncrackable = malloc(sizeof(uncrackable_node_t));
                uncrackable_head = uncrackable;
            } else {
                uncrackable->next = malloc(sizeof(uncrackable_node_t));
                uncrackable = uncrackable->next;
            }
            strcpy(uncrackable->data, current->data);
            uncrackable->result = 0;
            uncrackable->next = NULL;
        }
        current = current->next;
    }

    UNCRACKABLES = uncrackable_head;
    UNCRACKABLES_QUEUE = uncrackable_head;

    return;
}

void create_hints(){
    hint_node_t *list = NULL;
    hint_node_t *current = NULL;
    crackable_node_t *current_node = CRACKABLES;
    while ((current_node != NULL)){
        if (current_node->result != -1){
            if (list == NULL){
                list = (hint_node_t*)malloc(sizeof(hint_node_t));
                list->val = current_node->result;
                list->used = 0;
                list->next = NULL;
                current = list;
            }
            else{
                current->next = (hint_node_t*)malloc(sizeof(hint_node_t));
                current = current->next;
                current->val = current_node->result;
                current->used = 0;
                current->next = NULL;
            }
            NUM_HINTS++;
            NUM_TOTAL_HINTS++;
        }
        current_node = current_node->next;
    }
    
    HINTS = list;
    return;
}

void creat_crackables(FILE *fp){
    char *buf = NULL;
    size_t bufsize = 32;
    ssize_t len = 1;

    crackable_node_t *head = malloc(sizeof(crackable_node_t));
    head->next = NULL;
    head->result = -1;
    crackable_node_t *unhashes = head;
    
    len = getline(&buf, &bufsize, fp);
    
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
        }else{
            head->next = NULL;
        }
        NUM_UNCRACKABLES++;
    }
    
    free(buf);
    fclose(fp);
    
    CRACKABLES = unhashes;
    CRACKABLES_QUEUE = unhashes;
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

void print_uncrackables(uncrackable_node_t *head){
    while (head != NULL){
        uncrackable_node_t *current = head;
        head = head->next;
        printf("%s, %d\n", current->data, current->result);
        //free(current);
    }
}

//compare function for qsort
int compare(const void *a, const void *b){
    return (*(int*)a - *(int*)b);
}

void sort_hints(hint_node_t *hints, int num_hints){

    int i = 0;
    hint_node_t *head = hints;

    int *arr = malloc(sizeof(int) * num_hints);
    while (hints != NULL){
        arr[i] = hints->val;
        hints = hints->next;
        i++;
    }
    qsort(arr, num_hints, sizeof(int), compare);
    for (int j = 0; j < num_hints; j++){
        head->val = arr[j];
        head = head->next;
    }
    free(arr);
}

void add_to_total_hints(hint_node_t *hints){
    if (TOTAL_HINTS == NULL){
        TOTAL_HINTS = hints;
    }else{
        hint_node_t *current = TOTAL_HINTS;
        while (current->next != NULL){
            current = current->next;
        }
        current->next = hints;
    }
}

void add_to_next_hints(int val){
    NUM_NEXT_HINTS++;
    NUM_TOTAL_HINTS++;
    if (NEXT_HINTS == NULL){
        NEXT_HINTS = (hint_node_t*)malloc(sizeof(hint_node_t));
        NEXT_HINTS->val = val;
        NEXT_HINTS->next = NULL;
        NEXT_HINTS->used = 0;
    }else{
        hint_node_t *new = (hint_node_t*)malloc(sizeof(hint_node_t));
        new->val = val;
        new->used = 0;
        new->next = NEXT_HINTS;
        NEXT_HINTS = new;
    }
}

void reset_uncrackables(){
    uncrackable_node_t *current = UNCRACKABLES;
    while (current != NULL){
        if (current->result == -1){
            current->result = 0;
        }
        current = current->next;
    }
    UNCRACKABLES_QUEUE = UNCRACKABLES;
}

void* break_crackables() {

    crackable_node_t *current;
    THREAD_BEGIN:
    // Get the first crackable from queue
    pthread_mutex_lock(&queue_lock);
    while ((CRACKABLES_QUEUE != NULL) && (CRACKABLES_QUEUE->result != 0)){
        CRACKABLES_QUEUE = CRACKABLES_QUEUE->next;
    }

    if (CRACKABLES_QUEUE != NULL){
        current = CRACKABLES_QUEUE;
        CRACKABLES_QUEUE->result = -1;
    }else{
        pthread_mutex_unlock(&queue_lock);
        return NULL;
    }
    pthread_mutex_unlock(&queue_lock);

    // Break the crackable

    current->result = unhash_timeout(TIMEOUT, current->data);
    if (current->result != -1){
        pthread_mutex_lock(&broken_lock);
        NUM_BROKEN++;
        pthread_mutex_unlock(&broken_lock);
    }

    goto THREAD_BEGIN;
}

void* break_uncrackables(){

    uncrackable_node_t *current;
    THREAD_BEGIN:
    // Get the first crackable from queue
    pthread_mutex_lock(&queue_lock);
    while ((UNCRACKABLES_QUEUE != NULL) && (UNCRACKABLES_QUEUE->result != 0)){
        UNCRACKABLES_QUEUE = UNCRACKABLES_QUEUE->next;
    }

    if (UNCRACKABLES_QUEUE != NULL){
        current = UNCRACKABLES_QUEUE;
        UNCRACKABLES_QUEUE->result = -1;
    }else{
        pthread_mutex_unlock(&queue_lock);
        return NULL;
    }
    pthread_mutex_unlock(&queue_lock);

    hint_node_t *head = HINTS;
    hint_node_t *tail = head;

    while ((head != NULL)){
        if (head->used == 0) {
            while ((tail != NULL)){
                if (tail->used == 0){
                    int result = crack_hash(head->val, tail->val, current->data);
                    if (result != 0){
                        current->result = result;
                        head->used = 1;
                        tail->used = 1;
                        pthread_mutex_lock(&broken_lock);
                        NUM_BROKEN++;
                        pthread_mutex_unlock(&broken_lock);
                        pthread_mutex_lock(&next_hint_lock);
                        add_to_next_hints(result);
                        pthread_mutex_unlock(&next_hint_lock);
                        goto THREAD_BEGIN;
                    }
                }
                tail = tail->next;
            }
        }
        head = head->next;
        tail = head;
    }

    goto THREAD_BEGIN;
}

void break_cipher_text(char * cipher_text_loc){
    FILE *fp = fopen(cipher_text_loc, "r");
    char ch;
    int idx = 0;
    hint_node_t *current = TOTAL_HINTS;
    while ((ch = fgetc(fp)) != EOF){
        if (idx == current->val){
            printf("%c", ch);
            current = current->next;
            if (current == NULL){
                break;
            }
        }
        idx++;
    }
    fclose(fp);
    return;
}

void clean_crackables(crackable_node_t *head){
    crackable_node_t *current = head;
    while (current != NULL){
        crackable_node_t *temp = current;
        current = current->next;
        free(temp);
    }
}

void clean_uncrackables(uncrackable_node_t *head){
    uncrackable_node_t *current = head;
    while (current != NULL){
        uncrackable_node_t *temp = current;
        current = current->next;
        free(temp);
    }
}

void clean_hints(hint_node_t *head){
    hint_node_t *current = head;
    while (current != NULL){
        hint_node_t *temp = current;
        current = current->next;
        free(temp);
    }
}

// Main Driver  
int main(int argc, char **argv) {


    FILE *fp;
    int err;
    clock_t start, end;
    char * cipher_loc;

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
    }else if (argc == 5){
        fp = fopen(argv[1], "r");
        NUM_THREADS = 10; //atoi(argv[2]);
        TIMEOUT = atoi(argv[3]);
        cipher_loc = argv[4];
    }else{
        printf("Usage: %s <hash_file> <num_threads> <timeout>\n", argv[0]);
        return 1;
    }
    
    if (fp == NULL){
        printf("Error opening file\n");
        exit(1);
    }

    if (pthread_mutex_init(&next_hint_lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    if (pthread_mutex_init(&broken_lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    if (pthread_mutex_init(&queue_lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    creat_crackables(fp);
    pthread_t tid[NUM_THREADS+1];

    //Break initial crackables
    for (int j = 0; j < NUM_THREADS; j++){
        err = pthread_create(&(tid[j]), NULL, &break_crackables, NULL);
        if (err != 0)
            printf("can't create thread :[%s]", strerror(err));
    }
    
    for (int j = 0; j < NUM_THREADS; j++){
        pthread_join(tid[j], NULL);
    }
    
    // Prepare uncrackables and hints
    create_uncrackbles();
    create_hints();
    clean_crackables(CRACKABLES);
    free_table();
    sort_hints(HINTS, NUM_HINTS);
    TOTAL_HINTS = HINTS;

    // Continously break uncrackables
    while(NUM_BROKEN < NUM_UNCRACKABLES){

        for (int j = 0; j < NUM_THREADS; j++){
            err = pthread_create(&(tid[j]), NULL, &break_uncrackables, NULL);
            if (err != 0)
                printf("can't create thread :[%s]", strerror(err));
        }
        for (int j = 0; j < NUM_THREADS; j++){
            pthread_join(tid[j], NULL);
        }
        add_to_total_hints(NEXT_HINTS);
        HINTS = NEXT_HINTS;
        NUM_HINTS = NUM_NEXT_HINTS;
        NEXT_HINTS = NULL;
        NUM_NEXT_HINTS = 0;
        reset_uncrackables();
        sort_hints(HINTS, NUM_HINTS);
    }
    
    // BREAK THE CIPHER TEXT !!!! LETS GOOOO
    sort_hints(TOTAL_HINTS, NUM_TOTAL_HINTS);
    break_cipher_text(cipher_loc);
    clean_hints(TOTAL_HINTS);


    //destory the lock
    pthread_mutex_destroy(&next_hint_lock);
    pthread_mutex_destroy(&broken_lock);
    pthread_mutex_destroy(&queue_lock);

    return 0;
}
