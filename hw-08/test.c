#include "hashutil.h";
#include <stdlib.h>


int main(int argc, char **argv) {
    printf("%s\n", crack_hash(123, 345, "37cba5957ccb647a5fb310923ac8794a"));
}