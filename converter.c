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

typedef unsigned char byte;

typedef struct pgm {
    int height;
    int width;
    int maxGrey;
} pgm;

typedef struct clrInstr {
    byte in1; //110000RR
    byte in2; //11RRRRRR
    byte in3; //11GGGGGG
    byte in4; //11GGBBBB
    byte in5; //11BBBBAA
    byte in6; //11AAAAAA
    byte setC; //10000011
} clrInstr;

typedef struct targetInstr {
    byte getX1; //110000XX
    byte getX2; //11XXXXXX
    byte setX;
    byte getY1; //110000YY
    byte getY2; //11YYYYYY
    byte setY;
} targetInstr;



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

clrInstr* getClrInstr(byte clr) {
    clrInstr *out = malloc(sizeof(clrInstr));
    *out = (clrInstr) {0,0,0,0,0,0};
    byte opCode = 192; // 11000000
    byte r = clr>>6;
    out->in1 = opCode | r; // 110000RR
    // printf("In1: %d\n",out->in1);
    r = clr<<2;
    r >>= 2;
    out->in2 = opCode | r; // 11RRRRRR
    // printf("In2: %d\n",out->in2);
    byte g = clr >> 2;
    out->in3 = opCode | g; // 11GGGGGG
    // printf("In3: %d\n",out->in3);
    g = clr << 6;
    g >>= 2;
    out->in4 = opCode | g; // 11GG0000
    // printf("In4: %d\n",out->in4);
    byte b = clr >> 4;
    out ->in4 |= b; // 11GGBBBB
    // printf("In4: %d\n",out->in4);
    b = clr << 4;
    b >>= 2;
    out->in5 = opCode | b; // 11BBBB00
    // printf("In5: %d\n",out->in5);
    byte alpha = 255; // Max default value
    out->in5 |= alpha >> 6;
    // printf("In5: %d\n",out->in5);
    out->in6 = 255; // 11111111
    // printf("In6: %d\n",out->in6);

    out->setC = 131; //10000011

    return out;
}

targetInstr* getTargetInstr(byte x, byte y) {
    targetInstr* out = malloc(sizeof(targetInstr));
    *out = (targetInstr) {0,0,0,0,0,0};
    out->setX = 133; //1000 0101
    out->setY = 132; //1000 0100
    byte dataOp = 192; //1100 0000
    
    out->getX1 = dataOp | x >> 6; //1100 00XX
    out->getX2 = dataOp | (x << 2) >> 2; //11XX XXXX

    out->getY1 = dataOp | y >> 6; //1100 00YY
    out->getY2 = dataOp | (y << 2) >> 2; //11YY YYYY
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

void getClrInstrTest(void) {
    char *test = "clrInstrTest";
    clrInstr* c = getClrInstr(255); // 1111 1111
    assert(test, __LINE__, c != NULL);
    assert(test, __LINE__, c->in1 == 195); // 1100 0011
    assert(test, __LINE__, c->in2 == 255); // 1111 1111
    assert(test, __LINE__, c->in3 == 255); // 1111 1111
    assert(test, __LINE__, c->in4 == 255); // 1111 1111
    assert(test, __LINE__, c->in5 == 255); // 1111 1111
    assert(test, __LINE__, c->in6 == 255); // 1111 1111
    if (c != NULL) free(c);

    c = getClrInstr(0); // 0000 0000
    assert(test, __LINE__, c != NULL);
    assert(test, __LINE__, c->in1 == 192); // 1100 0000
    assert(test, __LINE__, c->in2 == 192); // 1100 0000
    assert(test, __LINE__, c->in3 == 192); // 1100 0000
    assert(test, __LINE__, c->in4 == 192); // 1100 0000
    assert(test, __LINE__, c->in5 == 195); // 1100 0011
    assert(test, __LINE__, c->in6 == 255); // 1111 1111
    if (c != NULL) free(c);

    c = getClrInstr(200); // 1100 1000
    assert(test, __LINE__, c != NULL);
    assert(test, __LINE__, c->in1 == 195); // 1100 0011
    assert(test, __LINE__, c->in2 == 200); // 1100 1000
    assert(test, __LINE__, c->in3 == 242); // 1111 0010
    assert(test, __LINE__, c->in4 == 204); // 1100 1100
    assert(test, __LINE__, c->in5 == 227); // 1110 0011
    assert(test, __LINE__, c->in6 == 255); // 1111 1111
    if (c != NULL) free(c);

    printf("PASSED: %s\n",test);
}

void getTargetInstrTest(void) {
    char *test = "getTargetInstrTest";
    targetInstr* t = getTargetInstr(0,0);
    assert(test, __LINE__, t != NULL);
    assert(test, __LINE__, t->getX1 == 192); // 1100 0000
    assert(test, __LINE__, t->getX2 == 192); // 1100 0000
    assert(test, __LINE__, t->getY1 == 192); // 1100 0000
    assert(test, __LINE__, t->getY2 == 192); // 1100 0000
    if (t != NULL) free(t);

    t = getTargetInstr(199,199);
    assert(test, __LINE__, t != NULL);
    assert(test, __LINE__, t->getX1 == 195); // 1100 0011
    assert(test, __LINE__, t->getX2 == 199); // 1100 0111
    assert(test, __LINE__, t->getY1 == 195); // 1100 0011
    assert(test, __LINE__, t->getY2 == 199); // 1100 0111
    if (t != NULL) free(t);

    t = getTargetInstr(15,134);
    assert(test, __LINE__, t != NULL);
    assert(test, __LINE__, t->getX1 == 192); // 1100 0000
    assert(test, __LINE__, t->getX2 == 207); // 1100 1111
    assert(test, __LINE__, t->getY1 == 194); // 1100 0010
    assert(test, __LINE__, t->getY2 == 198); // 1100 0110
    if (t != NULL) free(t);

    printf("PASSED: %s\n",test);
}

void test(void) {
    parseHeaderTest();
    getClrInstrTest();
    getTargetInstrTest();
    printf("ALL TESTS PASSED!\n");
}

int main(int n, char *args[]) {
    test();
    return 0;
}
