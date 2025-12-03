#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "displayfull.h"


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
        if (header[i] <= '9' && header[i] >= '0') {
            if (header[i-1] == 32) {
                argId += 1;
            }

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

    r = clr<<2;
    r >>= 2;
    out->in2 = opCode | r; // 11RRRRRR

    byte g = clr >> 2;
    out->in3 = opCode | g; // 11GGGGGG

    g = clr << 6;
    g >>= 2;
    out->in4 = opCode | g; // 11GG0000

    byte b = clr >> 4;
    out ->in4 |= b; // 11GGBBBB

    b = clr << 4;
    b >>= 2;
    out->in5 = opCode | b; // 11BBBB00

    byte alpha = 255; // Max default value for alpha
    out->in5 |= alpha >> 6;

    out->in6 = 255; // 11111111
    

    out->setC = 131; //10000011 Colour command

    return out;
}


targetInstr* getTargetInstr(byte x, byte y) {

    targetInstr* out = malloc(sizeof(targetInstr));
    *out = (targetInstr) {0,0,0,0,0,0};

    const byte targetX = 132; //1000 0100
    const byte targetY = 133;  //1000 0101

    out->setX = targetX; 
    out->setY = targetY; 
    byte dataOp = 192; //1100 0000
    
    out->getX1 = dataOp | x >> 6; //1100 00XX
    out->getX2 = dataOp | (x << 2) >> 2; //11XX XXXX

    out->getY1 = dataOp | y >> 6; //1100 00YY
    out->getY2 = dataOp | (y << 2) >> 2; //11YY YYYY
    return out;
}


byte* blockInstr(byte x, byte y, byte colour) {
    /*
    Return an array of bytecodes
    DATA add colours (6 lines)
    COLOUR (1 line)
    DATA add x coordinate (2 lines)
    TARGETX (1 line)
    DATA add y coordinate  (2 lines)
    TARGETY (1 line)
    DY (1 line)
    TOTAL: 14 lines
    */
    byte* instructions = malloc(sizeof(byte)*14 + 1);
    clrInstr* cInstr = getClrInstr(colour);
    targetInstr* tInstr = getTargetInstr(x,y);
    const byte dy = 64; //DY 01000000
    instructions[0] = cInstr->in1;
    instructions[1] = cInstr->in2;
    instructions[2] = cInstr->in3;
    instructions[3] = cInstr->in4;
    instructions[4] = cInstr->in5;
    instructions[5] = cInstr->in6;
    instructions[6] = cInstr->setC;

    instructions[7] = tInstr->getX1;
    instructions[8] = tInstr->getX2;
    instructions[9] = tInstr->setX;
    instructions[10] = tInstr->getY1;
    instructions[11] = tInstr->getY2;
    instructions[12] = tInstr->setY;

    instructions[13] = dy; 
    instructions[14] = '\0';

    free(cInstr);
    free(tInstr);

    return instructions;
}


byte* changeLocInstr(byte x,byte y) {
    /*
    NONE (1 line)
    get X (2 lines)
    TARGETX (1 line)
    get Y (2 lines)
    TARGETY (1 line)
    dy (1 line)
    LINE
    /0
    Total: 10
    */
    byte* instructions = malloc(sizeof(byte)*10);
    targetInstr* tInstr = getTargetInstr(x,y);
    const byte dy = 64; //DY 01000000
    const byte line = 129; //LINE 10000001
    instructions[0] = 128; // NONE 1000 0000
    instructions[1] = tInstr->getX1;
    instructions[2] = tInstr->getX2;
    instructions[3] = tInstr->setX;
    instructions[4] = tInstr->getY1;
    instructions[5] = tInstr->getY2;
    instructions[6] = tInstr->setY;
    instructions[7] = dy; 
    instructions[8] = line; 
    instructions[9] = '\0';

    free(tInstr);
    return instructions;
}


void writeInstr(FILE* out,byte* instr) {
    int i = 0;
    while (instr[i] != '\0') {
        fputc(instr[i],out);
        i++;
    }
}


void parseBytes(FILE* in,FILE* out, pgm* p) {   
    int row = 0;
    byte b = fgetc(in);
    byte currClr = b;
    while (!feof(in)) {
        //set coordinates to start of new row
        if (row > 0) {
            byte* resetLoc = changeLocInstr(0,row);
            writeInstr(out,resetLoc);
            free(resetLoc);
        }
        for (int w = 0; w < p->width; w++) {
            b = fgetc(in);
            if (b != currClr || w == p->width-1) {
                /*
                Add block instructions only if clrCount != 0
                - Set colour
                - Set x to w-1
                - Draw
                */
                byte* blkInstr = blockInstr(w,row,currClr);
                writeInstr(out,blkInstr);
                currClr = b;
                free(blkInstr);
            } 
                 
        }
        row++;
    }

}


char* extractFilename(char* filename) {
    char* skFilename = malloc(strlen(filename)+1);
    int i = 0;
    bool isExtension = false;
    while (filename[i] != '\0') {
        char curr = filename[i];
        if (curr == '.') {
            isExtension = true;
            skFilename[i] = '.';
        } else if (isExtension && curr =='p') {
            skFilename[i] = 's';
        } else if (isExtension && curr == 'g') {
            skFilename[i] = 'k';
        } else if (isExtension && curr =='m') {
            skFilename[i] = '\0';
        } else {
            skFilename[i] = filename[i];
        }
        i++;
    }
    return skFilename;
}

bool isValidPGM(char* filename) {
    int len = strlen(filename);
    char* ext = ".pgm";
    int fNameid = len - 4; //Start at extension
    if (len < 5) { // File can't possibly have a .pgm ext
        return false;
    }
    int i = 0;
    while (ext[i] != '\0') {
        if (filename[fNameid] != ext[i]) {
            return false;
        }
        i++;
        fNameid++;
    }
    return true;
}




/*
============== SK TO PGM ==============
*/

byte getByteData(byte in1, byte in2) {
    byte dataOpExtractor = 0x3f; //0011 1111
    byte partition1 = in1 & dataOpExtractor; //0000 00XX
    byte partition2 = in2 & dataOpExtractor; // 00XX XXXX
    byte out = (partition1 << 6) | partition2;
    return out;
}

void parseSK(FILE* in,FILE* out) {
    byte curr_x = 0;
    byte curr_y = 0;
    byte tx = 0;
    byte ty = 0;
    byte in1 = 0;
    byte in2 = 0;
    byte pixelClr = 0;
    int dataCount = 0;
    bool toggleLine = true;

    byte b = fgetc(in);
    while (!feof(in)) {
        const byte opCodeExtractor = 0xc0;
        const byte dataOp = 0xc0;
        const byte targetX = 132; //1000 0100
        const byte targetY = 133;  //1000 0101
        const byte line = 129; //LINE 10000001
        const byte noneCode = 128; //NONE 1000 0000
        const byte dy = 64; //DY 01000000
        byte opCode = b & opCodeExtractor;
        if (opCode == dataOp) {
            printf("|DATA| %02x\n",b & 0x3f);
            switch(dataCount) {
                /*
                If data count is < 2, we record the 2 instructions.
                If it exceeds 2, that means this is a sequence to change colour, so we record pixel colour
                */
                case 0:
                    in1 = b;
                     break;
                case 1:
                    in2 = b;
                    break;
                case 5:
                    printf("|COLOUR|\n");
                    
                    break;
                default:
                    pixelClr = getByteData(in1,in2);
                    break;
            }
            dataCount++;
        } else {
            dataCount = 0;
        }
        
        if (b == targetX) {
            tx = getByteData(in1,in2);
            printf("|TX| %02x\n",tx);
        } else if (b == targetY) {
            ty = getByteData(in1,in2);
            printf("|TY|\n");
        } else if (b == line || b == noneCode) {
            printf("|LINE\\NONE\n");
            toggleLine = (b == line);
        } else if (b == dy && !toggleLine) {
            printf("|DY change loc|\n");
            curr_x = tx;
            curr_y = ty;
        } else if (b == dy && toggleLine) {
            
            int n_pixels = tx - curr_x; //Since line is only used horizontally
            printf("|DY LINE| %d\n",n_pixels);
            for (int i = 0; i < n_pixels; i++) {
                putc(pixelClr,out);
            }
            curr_x = tx-1;
            curr_y = ty;
        }

        b = fgetc(in);
    }
}

void generatePGM(FILE* in, FILE* out) {
    // char* header = "P5 200 200 255\n";
    fprintf(out,"P5 200 200 255 \n");
    parseSK(in,out);
}


bool isValidSK(char* filename) {
    int len = strlen(filename);
    char* ext = ".sk";
    int fNameid = len - strlen(ext); //Start at extension
    if (len < strlen(ext)+1) { // File can't possibly have a .pgm ext
        return false;
    }
    int i = 0;
    while (ext[i] != '\0') {
        if (filename[fNameid] != ext[i]) {
            return false;
        }
        i++;
        fNameid++;
    }
    return true;
}

char* extractFilenameSK(char* filename) {
    char* skFilename = malloc(strlen(filename)+2);
    int i = 0;
    bool isExtension = false;
    while (filename[i] != '\0') {
        char curr = filename[i];
        if (curr == '.') {
            isExtension = true;
            skFilename[i] = '.';
        } else if (isExtension && curr =='s') {
            skFilename[i] = 'p';
        } else if (isExtension && curr == 'k') {
            skFilename[i] = 'g';
        } else if (isExtension && curr =='\0') {
            skFilename[i] = 'm';
        } else {
            skFilename[i] = curr;
        }
        i++;
    }
    skFilename[i] = 'm';
    skFilename[i+1] = '\0';
    return skFilename;
}

void processFile(char* filename, bool isConvertingPGM) {
    FILE* in = openFile(filename,"rb");
    
    printf("Files opened.\n");
    if (isConvertingPGM) {
        char* skFile = extractFilename(filename);
        FILE* sk = openFile(skFile,"wb");
        char header[1024];
        fgets(header,sizeof(header),in);
    

        pgm* p = parseHeader(header);
        if (p == NULL) {
            fflush(stderr);
            perror("Incorrect file format");
            exit(1);
        }
        parseBytes(in,sk,p);
        free(skFile);
        fclose(sk);
    } else {
        char* pgmFile = extractFilenameSK(filename);
        FILE* pgmF = openFile(pgmFile,"wb");
        generatePGM(in,pgmF);
        free(pgmFile);
        fclose(pgmF);
    }
    
    fclose(in);
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


void blockInstrTest(void) {
    char *test = "blockInstrTest";
    byte* b = blockInstr(199,199,255);
    assert(test, __LINE__, b[0] == 195); // 1100 0011
    assert(test, __LINE__, b[1] == 255); // 1111 1111
    assert(test, __LINE__, b[2] == 255); // 1111 1111
    assert(test, __LINE__, b[3] == 255); // 1111 1111
    assert(test, __LINE__, b[4] == 255); // 1111 1111
    assert(test, __LINE__, b[5] == 255); // 1111 1111
    assert(test, __LINE__, b[6] == 131);
    assert(test, __LINE__, b[7] == 195); // 1100 0011
    assert(test, __LINE__, b[8] == 199);
    assert(test, __LINE__, b[9] == 132); // 1100 0111
    assert(test, __LINE__, b[10] == 195); // 1100 0011
    assert(test, __LINE__, b[11]== 199);
    assert(test, __LINE__, b[12]== 133);
    assert(test, __LINE__, b[13]== 64);
     // 1100 0111
    free(b);
    printf("PASSED: %s\n",test);
}


void extractFilenameTest(void) {
    char *test = "extractFilenameTest";
    char* s = extractFilename("myimage.pgm");
    assert(test, __LINE__, strcmp(s,"myimage.sk") == 0);
    free(s);

    s = extractFilename(".pgm");
    assert(test, __LINE__, strcmp(s,".sk") == 0);
    free(s);

    printf("PASSED: %s\n",test);
}

void isValidPgmTest(void) {
    char *test = "isValidPgmTest";
    assert(test, __LINE__, isValidPGM("x.pgm") == true);
    assert(test, __LINE__, isValidPGM("fractal.pgm") == true);
    assert(test, __LINE__, isValidPGM(".pgm") == false);
    assert(test, __LINE__, isValidPGM("h.c") == false);
    assert(test, __LINE__, isValidPGM("display.c") == false);
    assert(test, __LINE__, isValidPGM("display.pgn") == false);
    assert(test, __LINE__, isValidPGM("displaypgm.sk") == false);
    assert(test, __LINE__, isValidPGM("displaypgm.pgmm") == false);
    printf("PASSED: %s\n",test);
}

/*
============== SK TO PGM TESTS ==============
*/

void getByteDataTest(void) {
    char *test = "getByteDataTest";
    assert(test, __LINE__, getByteData(0xc0,0xc0) == 0);
    assert(test, __LINE__, getByteData(0xc3,0xff) == 255);
    assert(test, __LINE__, getByteData(0xc1,0xf8) == 120);
    printf("PASSED: %s\n",test);
}

void isValidSKTest(void) {
    char *test = "isValidSKTest";
    assert(test, __LINE__, isValidSK("x.sk") == true);
    assert(test, __LINE__, isValidSK("fractal.sk") == true);
    assert(test, __LINE__, isValidSK(".sk") == false);
    assert(test, __LINE__, isValidSK("h.c") == false);
    assert(test, __LINE__, isValidSK("display.c") == false);
    assert(test, __LINE__, isValidSK("display.pgn") == false);
    assert(test, __LINE__, isValidSK("displaypgm.pgm") == false);
    assert(test, __LINE__, isValidSK("displaypgm.pgmm") == false);
    printf("PASSED: %s\n",test);
}

void extractFilenameSKTest(void) {
    char *test = "extractFilenameSKTest";
    char* s = extractFilenameSK("myimage.sk");
    assert(test, __LINE__, strcmp(s,"myimage.pgm") == 0);
    free(s);

    s = extractFilenameSK("fractal-test1.sk");
    assert(test, __LINE__, strcmp(s,"fractal-test1.pgm") == 0);
    free(s);

    printf("PASSED: %s\n",test);
}


void test(void) {
    parseHeaderTest();
    getClrInstrTest();
    getTargetInstrTest();
    blockInstrTest();
    extractFilenameTest();
    isValidPgmTest();
    getByteDataTest();
    isValidSKTest();
    extractFilenameSKTest();
    printf("ALL TESTS PASSED!\n");
}


int main(int n, char *args[]) {
    if (n == 1) {
        test();
    } else if (n != 2 || (!isValidPGM(args[1]) && !isValidSK(args[1]))) {
        printf("Incorrect input format!\n");
    } else if (isValidPGM(args[1])){
        processFile(args[1],true);
    } else {
        processFile(args[1],false);
    }
    return 0;
}
