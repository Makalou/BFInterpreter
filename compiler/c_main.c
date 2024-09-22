#include "stdlib.h"
#include <stdlib.h>

extern void bf_main(char*);

int main()
{
    char* tape = (char*)calloc(4096, sizeof(char));
    bf_main(tape);
    return 0;
}