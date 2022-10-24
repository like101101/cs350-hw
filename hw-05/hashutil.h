#ifndef HASHUTIL_H_  /* Include guard */
#define HASHUTIL_H_

int foo(int x);  /* An example function declaration */

char *hash(const char *str, int length);
int unhash(int start, const char *str);

#endif // HASHUTIL