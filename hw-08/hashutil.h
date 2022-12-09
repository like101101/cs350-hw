#ifndef HASHUTIL_H_  /* Include guard */
#define HASHUTIL_H_

typedef struct crackable_node {
    char data[33];
    struct crackable_node *next;
    int result;
}crackable_node_t;

typedef struct uncrackable_node {
    char data[33];
    struct uncrackable_node *next;
    int result;
}uncrackable_node_t;

typedef struct hint_node {
    int val;
    int used;
    struct hint_node *next;
}hint_node_t;

char *hash(const char *str, int length);
int unhash(int start, int count, const char *str);
char * crack_hash(int start, int end, const char *str);
void free_table();

#endif // HASHUTIL
