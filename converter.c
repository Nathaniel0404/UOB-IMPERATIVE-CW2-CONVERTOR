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

typedef struct pgm {
    int height;
    int width;
    int maxGrey;
} pgm;

pgm* parseHeader(char *header) {
    /*
        Indicate which arg we are extracting,
        0 = Magic number; 1 = Height; 2 = Width; 3 = Max Grey
    */
    int argId = 0; 
    
    // Check if this is a pgm file
    if (header[0] != 'P' || header[1] != '5') {
        return NULL;
    }

    pgm *out = malloc(sizeof(pgm));
    *out = (pgm) {0,0,0};

    int i = 2; // Start after the magic number
    const char digit = '0'; // Used to convert char to actual int
    while (i < strlen(header)) {
        // printf("%d\n",header[i]);
        // printf("|i| %d |height| %d, |width| %d, |maxGrey| %d\n",i, out->height,out->width,out->maxGrey);
        if (header[i] <= '9' && header[i] >= '0') {
            // printf("|i-1| %d\n",header[i-1]);
            if (header[i-1] == 32) {
                argId += 1;
            }
            // printf("|ARG| %d\n", argId);
            switch(argId) {
                case 1:
                    out->height *= 10;
                    out->height += header[i] - digit; break;
                case 2:
                    out->width *= 10;
                    out->width += header[i] - digit; break;
                case 3:
                    out->maxGrey *= 10;
                    out->maxGrey += header[i] - digit; break;
                default:
                    free(out);
                    return NULL;

            }
        }
        i++;
    }

    return out;
}


/*
============== TESTS ==============
*/

void assert(char *test, int line, bool b) {
    if (! b) {
        printf("%s FAILED! Test on line %d failed\n",test, line);
        exit(1);
    }
}

void parseHeaderTest(void) {
    char *test = "parseHeaderTest";
    pgm *p = parseHeader("P5 200 200 255");
    assert(test, __LINE__, p != NULL);
    assert(test, __LINE__, p->height == 200);
    assert(test, __LINE__, p->width == 200);
    assert(test, __LINE__, p->maxGrey == 255);
    if (p != NULL) free(p);

    p = parseHeader("P5 250 200 255");
    assert(test, __LINE__, p != NULL);
    assert(test, __LINE__, p->height == 250);
    assert(test, __LINE__, p->width == 200);
    assert(test, __LINE__, p->maxGrey == 255);
    if (p != NULL) free(p);
   
    p = parseHeader("P4 250 200 255");
    assert(test, __LINE__, p == NULL);
    if (p != NULL) free(p);

    p = parseHeader("");
    assert(test, __LINE__, p == NULL);
    if (p != NULL) free(p);

    p = parseHeader("P5 250 200 255 234");
    assert(test, __LINE__, p == NULL);
    if (p != NULL) free(p);
    
    printf("PASSED: %s\n",test);
}


void test(void) {
    parseHeaderTest();
    printf("ALL TESTS PASSED!\n");
}

int main(int n, char *args[]) {
    test();
    return 0;
}
