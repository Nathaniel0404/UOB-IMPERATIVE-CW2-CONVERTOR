#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "displayfull.h"

FILE* openFile(char* fileName, char* purpose) {
    FILE* p = fopen(fileName, purpose);
    if (p != NULL) {
        return p;
    } 
    fprintf(stderr,"Can't open %s: ", fileName);
    fflush(stderr);
    perror("");
    exit(1);
}

int main(int n, char *args[]) {
    return 0;
}
