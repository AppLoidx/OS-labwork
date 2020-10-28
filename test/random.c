#include <stdio.h>

int main() {
    
    FILE* urandom = fopen("/dev/urandom", "r");

    unsigned long int val;

    fread(&val, sizeof(unsigned long), 1, urandom);

    printf("value : %lu", val);


    return 0;
}
