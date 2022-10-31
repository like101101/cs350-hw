#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hashutil.h"

#define DEBUG 1

int main(int argc, char **argv) {


    int num_hashes;
    if (DEBUG){
        num_hashes = 1000;
    }else{
        num_hashes= atoi(argv[1]);
    }

    if (num_hashes <= 0) {
        printf("Invalid number of hashes");
        return 1;
    }
    

    FILE *fp1 = fopen("hashes.txt", "w");
    FILE *fp2 = fopen("numbers.txt", "w");
    if (fp1 == NULL || fp2 == NULL) {
        printf("Error opening file");
        return 1;
    }

    char to_unhash[8];
    char *hashed = NULL;
    int start = 1000;
    for (int i = start; i < start+num_hashes; i++) {
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