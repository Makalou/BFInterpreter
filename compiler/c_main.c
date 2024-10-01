#include "stdlib.h"
#include <stdlib.h>

extern void bf_main(char*);

int main()
{
    char* tape = (char*)calloc(4096*8, sizeof(char));
    char* pointer = tape + 4096*4; // move pointer to the middle
    bf_main(pointer);
    //free(tape);
    return 0;
}