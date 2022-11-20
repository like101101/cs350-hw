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
int NUM_COMPLETED = 0;

struct node {
    char data[33];
    struct node *next;
    int result;
};

struct linked_list {
    int val;
    struct linked_list *next;
};

void split_work(struct node* assignment[], struct node* head){
    
    int i = 0, idx = 1;
    
    for (int j = 0; j < NUM_THREADS + 1; j++){
        assignment[j] = NULL;
    }
    
    struct node* current = head;
    assignment[0] = current;
    while ((current != NULL) && (current->data[0] != '\0')){
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

struct linked_list* create_linked_list(struct node * head){
    struct linked_list *list = NULL;
    struct linked_list *current = NULL;
    struct node *current_node = head;
    while ((current_node != NULL) && (current_node->result != -1)){
        if (list == NULL){
            list = malloc(sizeof(struct linked_list));
            list->val = current_node->result;
            list->next = NULL;
            current = list;
        } else {
            current->next = malloc(sizeof(struct linked_list));
            current = current->next;
            current->val = current_node->result;
            current->next = NULL;
        }
        current_node = current_node->next;
    }
    return list;
}


struct node * read_file(FILE *fp){
    char *buf = NULL;
    size_t bufsize = 32;
    ssize_t len = 1;
    NUM_JOBS = 0;
    struct node *head = malloc(sizeof(struct node));
    head->next = NULL;
    head->result = -1;
    struct node *unhashes = head;
    
    len = getline(&buf, &bufsize, fp);
    NUM_JOBS ++;
    while (len > 0){
	char *ptr = strchr(buf, '\n');
	if (ptr){ *ptr='\0';}
        strcpy(head->data, buf);
        len = getline(&buf, &bufsize, fp);
        if (len > 0){
            head->next = malloc(sizeof(struct node));
            head->next->next = NULL;
            head->result = 0;
            head = head->next;
            NUM_JOBS ++;
        }else{
            head->next = NULL;
        }
    }
    
    free(buf);
    return unhashes;
}

void print_result(struct node *head){
    while ((head != NULL) && (head->data[0] != '\0')){
        struct node *current = head;
        if (current->result == -1){
            printf("%s\n", current->data);
        }else{
            printf("%d\n", head->result);
        }
        head = head->next;
        //free(current);
    }
}

void print_list(struct linked_list *list){
    while (list != NULL){
        printf("%d\n", list->val);
        struct linked_list *current = list;
        list = list->next;
        //free(current);
    }
}

void* dowork(struct node *unhashes) {

    while ((unhashes != NULL) && (unhashes->data[0] != '\0') && (unhashes->result == 0)){
        unhashes->result = unhash_timeout(TIMEOUT, unhashes->data);
        unhashes = unhashes->next;
        NUM_COMPLETED++;
    }
    
    LOOP: if(NUM_COMPLETED == NUM_JOBS){
        pthread_exit(NULL);
    }else{
        goto LOOP;
    }
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
        NUM_THREADS = atoi(argv[2]);
        TIMEOUT = atoi(argv[3]);
    }else{
        printf("Usage: %s <hash_file> <num_threads> <timeout>\n", argv[0]);
        return 1;
    }
    
    if (fp == NULL){
        printf("Error opening file");
        exit(1);
    }


    struct node *unhashes = read_file(fp);
    fclose(fp);
    
    if (NUM_THREADS == 1){
        WORK_PER_THREAD = NUM_JOBS;
    }else if (NUM_JOBS % NUM_THREADS == 0){
        WORK_PER_THREAD = NUM_JOBS / NUM_THREADS;
    }else if(NUM_JOBS > NUM_THREADS){
        WORK_PER_THREAD = (NUM_JOBS / NUM_THREADS);
    }else{
        WORK_PER_THREAD = 1;
    }
    
    struct node *assignment[NUM_THREADS+1];
    pthread_t tid[NUM_THREADS+1];

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
    

    //second round
    print_result(unhashes);

    printf("--------\n");
    struct linked_list *list = create_linked_list(unhashes);
    print_list(list);

    return 0;
}
