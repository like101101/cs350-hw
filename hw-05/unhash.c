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

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage: %s <hash>\n", argv[0]);
        return 1;
    }

    int result = unhash(0, argv[1]);
    if (result == -1) {
        printf("No match found\n");
    } else {
        printf("%d\n", result);
    }

    return 0;
}