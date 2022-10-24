#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hashutil.h"

int main(int argc, char **argv) {

    FILE *fp1;
    FILE *fp2;

    if (argc != 3) {
        printf("Usage: %s <num_hashes> <start>\n", argv[0]);
        return 1;
    }

    int num_hashes = atoi(argv[1]);
    if (num_hashes <= 0) {
        printf("Invalid number of hashes");
        return 1;
    }
    
    int start = atoi(argv[2]);
    if (start <= 0) {
        printf("Invalid start");
        return 1;
    }

    fp1 = fopen("hashes.txt", "w");
    fp2 = fopen("numbers.txt", "w");
    if (fp1 == NULL || fp2 == NULL) {
        printf("Error opening file");
        return 1;
    }

    char *to_unhash = NULL;
    char *hashed = NULL;
    for (int i = start; i < start+num_hashes; i++) {
        to_unhash = malloc(8);
        sprintf(to_unhash, "%d", i);
        hashed = hash(to_unhash, strlen(to_unhash));
        fprintf(fp1, "%s\n", hashed);
        fprintf(fp2, "%d\n", i);
        
    }

    free(hashed);
    free(to_unhash);
    fclose(fp1);
    fclose(fp2);

    return 0;
}