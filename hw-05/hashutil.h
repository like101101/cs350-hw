#ifndef HASHUTIL_H_  /* Include guard */
#define HASHUTIL_H_

struct unhash_args {
    int start;
    int end;
    char *hash;
};

char *hash(const char *str, int length);
int unhash(int start, int count, const char *str);

#endif // HASHUTIL