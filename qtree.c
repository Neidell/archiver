#include "qtree.h"

static bool _insertToQueue(qtree*, qtreeNode*, qtreeNode*, bool);
static void _traverseTree(qtreeNode*, void (*)(qtreeNode*, uint8_t, uint32_t, codeInfo[]), 
                            uint8_t, uint32_t, codeInfo[]);
static void _visitNode(qtreeNode*, uint8_t, uint32_t, codeInfo[]);
static bool _add(qtree*, uint8_t, uint32_t);
static bool _rebuildTree(qtree*, FILE*);
static bool _buildTree(qtree*);
static bool _buildQueue(qtree*, const char*);
static bool _generateCodeTable(qtree*);
static bool _decodeFile(qtree*, FILE*, FILE*);
static uint32_t _reverse_bits(uint32_t, uint32_t);
static bool _writeDataToFile(qtree*, const char*, const char*);
static bool _writeCodesToFile(qtree*, const char*);
static bool _writeArchiveInfo(qtree*, const char*);
static bool _readArchiveInfo(qtree*, FILE*);

static bool _insertToQueue(qtree* self, qtreeNode* dst, qtreeNode* src,  bool rightInsert) {
    if (self->head == NULL) {
        /*queue doesn't contain any elements*/
        self->head = self->tail = src;
    } else if (dst == self->head) {
        if (rightInsert) {
            /*right insertion*/
            if (self->head == self->tail) {
                /*queue contains only one element*/
                self->tail = src;
                self->head->nextNode = self->tail;
            } else {
                /*queue contains more than one element*/
                src->nextNode = self->head->nextNode;
                self->head->nextNode = src;
            }
        } else {
            /*left insertion*/
            src->nextNode = self->head;
            self->head = src;
        }
    } else if (dst == self->tail) {
        self->tail->nextNode = src;
        src->nextNode = NULL;
        self->tail = src;
    } else {
        /*insertion inside the queue*/
        src->nextNode = dst->nextNode;
        dst->nextNode = src;
    }

    return true;
}

static bool _add(qtree* self, uint8_t symb, uint32_t value) {
    qtreeNode* newNode = initQTreeNode();
    qtreeNode *cPtr = self->head;
    
    newNode->symb = symb;
    newNode->weight = value;

    if(cPtr != NULL)  {
        while (cPtr->nextNode != NULL) {
            if (cPtr->nextNode->weight > value) {
                break;
            }

            cPtr = cPtr->nextNode;
        }

        _insertToQueue(self, cPtr, newNode, (bool)(cPtr->weight < value));
    } else {
        _insertToQueue(self, cPtr, newNode, true);
    }    

    return true;
}

static bool _buildQueue(qtree* self, const char* srcFileName) {
    uint32_t symbols[256] = {0};
    uint8_t buff[BUFFER_SIZE] = {0};
    uint32_t readedChars;
    int i;

    FILE *text = fopen(srcFileName, "r");
    
    while((bool)(readedChars = fread(buff, sizeof(uint8_t), BUFFER_SIZE, text))) {
        //printf("readedChars - %d\n", readedChars);
        for(i = 0; i < readedChars; i++) {
            //printf("%c - %d\n", buff[i], buff[i]);
            symbols[buff[i]]++;    
        }
    }

    for(i = 0; i < 256; i++) {
        if(symbols[i] > 0) {
            //printf("[from _buildQueue] symbols[%d] - %c, weight - %d\n", i, i, symbols[i]);
            _add(self, i, symbols[i]);
        }
    }

    fclose(text);
    return true;
}

static bool _buildTree(qtree* self) {
    qtreeNode *firstNode, *secondNode, *newNode, *cPtr;

    if (self->head == self->tail) {
        self->root = initQTreeNode();
        self->root->lchild = self->head;

        return true;
    }

    while (self->head != self->tail) {     
        //printf("HEAD - %d, TAIL - %d\n", self->head->weight, self->tail->weight);   
        firstNode = self->head;
        secondNode = firstNode->nextNode;
        newNode = initQTreeNode();

        newNode->weight = firstNode->weight + secondNode->weight;
        newNode->lchild = firstNode /*minWeightNode(firstNode, secondNode)*/;
        newNode->rchild = secondNode /*maxWeightNode(firstNode, secondNode)*/;
        newNode->nextNode = secondNode->nextNode;

        if(self->head->nextNode != self->tail) {
            if(newNode->nextNode->weight < newNode->weight) {
                self->head = newNode->nextNode;
                cPtr = self->head;

                while (cPtr->nextNode && (cPtr->nextNode->weight < newNode->weight)) {
                    cPtr = cPtr->nextNode;
                }

                _insertToQueue(self, cPtr, newNode, (bool)(cPtr->weight < newNode->weight));
            } else {
                self->head = newNode;
            }
        } else {
            newNode->nextNode = NULL;
            self->head = self->tail = newNode;
            self->root = newNode;
        }
    }

    //printf("HEAD - %d, TAIL - %d\n", self->head->weight, self->tail->weight);

    return true;
}

//assumes little endian
void printBits(uint32_t const size, void const * const ptr) {
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = b[i] & (1<<j);
            byte >>= j;
            printf("%u", byte);
        }
    }
}

static uint32_t _reverse_bits(uint32_t v, uint32_t codeLength) {
    uint32_t r = v; // r will be reversed bits of v; first get LSB of v
    uint32_t s = sizeof(v) * 8 - 1; // extra shift needed at end

    for (v >>= 1; v; v >>= 1) {   
      r <<= 1;
      r |= v & 1;
      s--;
    }

    r <<= s; // shift when v's highest bits are zero
    r >>= sizeof(v) * 8 - codeLength;

    return r;
}

static void _visitNode(qtreeNode* root, uint8_t depth, 
                        uint32_t code, codeInfo codes[]) {

    uint32_t reversedCode;
    //printf("depth - %d\n", depth);
    if (root->symb) {
        reversedCode = _reverse_bits(code, depth);
        codes[root->symb] = (codeInfo){root->symb, depth, reversedCode};
        //printf("Char - %c\n", root->symb);
    }
}

static void _traverseTree(qtreeNode* root, void (*visitNode)(qtreeNode*, uint8_t, uint32_t, codeInfo[]), 
                            uint8_t depth, uint32_t code, codeInfo codes[]) {
    if (root) {
        visitNode(root, depth, code, codes);
        
        code = code << 1;
        _traverseTree(root->lchild, visitNode, depth + 1, code | 0, codes);
        _traverseTree(root->rchild, visitNode, depth + 1, code | 1, codes);
    }
}

static bool _generateCodeTable(qtree* self) {
    _traverseTree(self->root, _visitNode, (uint8_t)0, (uint32_t)0, self->codes);

    for (int i = 0; i < 256; ++i) {
        if((self->codes[i].length) > 0) {
            /*printf("Char - %c[%d], code - ", self->codes[i].character, self->codes[i].character);
            printBits(sizeof(uint32_t), &(self->codes[i].code));
            printf(", code length - %d\n", self->codes[i].length);*/
            (self->numberOfCodes)++;
        }
    }

    return 0;
}

static bool _writeCodesToFile(qtree* self, const char* dstFileName) {
    codeInfo codes[self->numberOfCodes];

    FILE *dstFile = fopen(dstFileName, "w+");

    for (int i = 0, j = 0; i < 256; ++i) {
        if((self->codes[i].length) > 0) {
            codes[j++] = self->codes[i];
        }
    }

    self->archInfo.tableLength = self->numberOfCodes;
    fseek(dstFile, sizeof(archiveInfo), SEEK_SET);    
    //fwrite(&(self->numberOfCodes), sizeof(uint8_t), 1, dstFile);
    fwrite(codes, sizeof(codeInfo), self->numberOfCodes, dstFile);

    fclose(dstFile);

    return true;
}

static bool _writeDataToFile(qtree* self, const char* dstFileName, const char* srcFileName) {

    FILE *dstFile = fopen(dstFileName, "a+");
    FILE *srcFile = fopen(srcFileName, "r");

    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);

    uint8_t readBuff[BUFFER_SIZE];
    uint32_t writeBuff[BUFFER_SIZE] = {0};
    uint32_t processedBytes = 0;
    uint32_t readedChars;
    uint32_t writedBlocks = 0;
    int32_t freeBlocks = BUFFER_SIZE, freeBits = 32, tempCode_1 = 0, tempCode_2 = 0;
    int32_t availableBits = 0;

    while((freeBlocks) > 0) {
        if((bool)(readedChars = fread(readBuff, sizeof(uint8_t), BUFFER_SIZE, srcFile))) {
            //printf("readedChars - %d\n", readedChars);
            for(int i = 0; i < readedChars; i++) {
                availableBits = freeBits - self->codes[readBuff[i]].length;
                //printf("available bits - %d\n", availableBits);

                if(availableBits > 0) {
                    tempCode_1 = self->codes[readBuff[i]].code;
                    tempCode_1 <<= (32 - freeBits);
                    //write code of another readed character
                    writeBuff[BUFFER_SIZE - freeBlocks] |= tempCode_1;

                    freeBits -= self->codes[readBuff[i]].length;
                } else {
/*                    printf("free bits: %d\n", freeBits);
                    printf("available bits: %d\n", availableBits);*/

                    tempCode_1 = (self->codes[readBuff[i]].code << (32 - self->codes[readBuff[i]].length + abs(availableBits)));
                    tempCode_2 = (self->codes[readBuff[i]].code >> freeBits);
                    freeBits = 32 + availableBits;
                    writeBuff[BUFFER_SIZE - freeBlocks] |= tempCode_1;
                    freeBlocks--;
/*
                    printf("freeBits - %d\n", freeBits);
                    printBits(sizeof(uint32_t), &(writeBuff[1023]));
                    printf("\n");
                    printBits(sizeof(uint32_t), &(self->codes[readBuff[i]].code)); 
                    printf("\nlast element length - %d\n", self->codes[readBuff[i]].length);*/
                    
                    if (freeBlocks == 0) {
                        // printf("freeBitsBlocks - %d\n", freeBlocks);
                        // printf("freeBits - %d\n", freeBits);
                        processedBytes += BUFFER_SIZE * sizeof(uint32_t);
                        //printf("%d\n", processedBytes);
                        //printf("Wrighting... Last code length - %d, Remaining bits - %d\n", self->codes[readBuff[i]].length, availableBits);
                        //printBits(sizeof(uint32_t), &(writeBuff[BUFFER_SIZE - 1]));
                        //printf("\n");
                        fwrite(writeBuff, sizeof(uint32_t), BUFFER_SIZE, dstFile);
                        writedBlocks++;
                        memset(writeBuff, 0, sizeof(writeBuff));
                        
                        freeBlocks = BUFFER_SIZE;
                        freeBits = 32;
                
                        if (i >= readedChars) break;
                        
                        i--;
                    } else {
                        writeBuff[BUFFER_SIZE - freeBlocks] |= tempCode_2;
                    }
                }
            }
        } else {
            //printf("to write - %d\n", BUFFER_SIZE - freeBlocks + 1);
            //printBits(sizeof(uint32_t), &(writeBuff[0]));
            //printf("remainingBits - %d\n", remainingBits);
            if (freeBlocks - 1 < BUFFER_SIZE) {
                //printf("Wrighting... Remaining bits - %d\n", freeBits);
                fwrite(writeBuff, sizeof(uint32_t), BUFFER_SIZE - freeBlocks + 1, dstFile);
                writedBlocks++;
            }

            self->archInfo.remainingBits = (sizeof(writeBuff) * 8) - (((freeBlocks - 1) * sizeof(uint32_t) * 8) + freeBits);
            self->archInfo.numberOfBlocks = writedBlocks;
            break;
        }
    }

    fclose(srcFile);
    fclose(dstFile);

    return true;
}

static bool _writeArchiveInfo(qtree* self, const char* dstFileName) {
    FILE* dstFile = fopen(dstFileName, "r+");
    //printf("tableLength - %d, numberOfBlocks - %d, remainingBits - %d\n", self->archInfo.tableLength, self->archInfo.numberOfBlocks, self->archInfo.remainingBits);

    fseekResult = fseek(dstFile, 0, SEEK_SET);
    fwrite(&(self->archInfo), sizeof(archiveInfo), 1, dstFile);

    fclose(dstFile);

    return true;
}

bool compress(qtree* self, const char* dstFileName, const char* srcFileName) {
    _buildQueue(self, srcFileName);
    _buildTree(self);
    _generateCodeTable(self);
    _writeCodesToFile(self, dstFileName);
    _writeDataToFile(self, dstFileName, srcFileName);
    _writeArchiveInfo(self, dstFileName);
    return true;
}

bool decompress(qtree* self, const char* dstFileName, const char* srcFileName) {
    FILE *dstFile = fopen(dstFileName, "w+");
    FILE *srcFile = fopen(srcFileName, "r");

    _readArchiveInfo(self, srcFile);
    _rebuildTree(self, srcFile);
    _decodeFile(self, dstFile, srcFile);
    
    fclose(dstFile);
    fclose(srcFile);

    return true;
}

static bool _readArchiveInfo(qtree* self, FILE* srcFile) {
    if(fread(&(self->archInfo), sizeof(archiveInfo), 1, srcFile)) {
        //printf("tableLength - %d, numberOfBlocks - %d, remainingBits - %d\n", self->archInfo.tableLength, self->archInfo.numberOfBlocks, self->archInfo.remainingBits);

        return true;
    } else {
        return false;
    }
}

static bool _decodeFile(qtree* self, FILE* dstFile, FILE* srcFile) {
    uint32_t currentBitMask = 1;
    uint32_t currentBlock = 0;
    uint32_t readedBlocksNumber = 0;
    uint32_t totalReadedBlocks = 0;
    int32_t remainingSignificantBits = self->archInfo.remainingBits;
    uint32_t readBuff[BUFFER_SIZE] = {0};
    uint8_t writeBuff[BUFFER_SIZE] = {0};
    uint32_t currentWriteBuffByte = 0;
    bool isLastBlock = false;

    qtreeNode* cNodePtr = self->root;

    while((bool)(readedBlocksNumber = fread(readBuff, sizeof(uint32_t), BUFFER_SIZE, srcFile))) {
/*        printf("\n----totalReadedBlocks---- - %d\n", totalReadedBlocks);
        printf("\n----numberOfBlocks----- - %d", self->archInfo.numberOfBlocks);*/

        /*printf("\nreadBuff[0]:\n");
        printBits(sizeof(uint32_t), &(readBuff[0]));
        printf("\nreadBuff[1]:\n");
        printBits(sizeof(uint32_t), &(readBuff[1]));*/
        if(++totalReadedBlocks == self->archInfo.numberOfBlocks) {
            isLastBlock = true;
            //printf("TRUEEE!!\n");
        }

        cNodePtr = self->root;

        for (currentBlock = 0; currentBlock < readedBlocksNumber; ++currentBlock) {
            currentBitMask = 1;
            
            while(currentBitMask) {
                

                if(cNodePtr->symb != 0) {
                    /*printf("\nWrited symb - %c\n", cNodePtr->symb);
                    printf("currentWriteBuffByte - %d\n", currentWriteBuffByte);
*/
                    writeBuff[currentWriteBuffByte] = cNodePtr->symb;
                    currentWriteBuffByte++; 
                    cNodePtr = self->root;

                    if(currentWriteBuffByte == BUFFER_SIZE) {
                        fwrite(writeBuff, sizeof(uint8_t), BUFFER_SIZE, dstFile);
                        memset(writeBuff, 0, sizeof(writeBuff));
                        currentWriteBuffByte = 0;
                    }
                }

                cNodePtr = (readBuff[currentBlock] & currentBitMask) ? cNodePtr->rchild : cNodePtr->lchild;

                if(cNodePtr == NULL) {
                    printf("BAAAADDD\n");
                    goto finish;
                }

                currentBitMask <<= 1;

                if(isLastBlock) {
                    remainingSignificantBits--;
                    //printf("remainingSignificantBits - %d\n", remainingSignificantBits);
                    if (remainingSignificantBits < 0) {
                        goto finish;
                    }
                }
/*                printf("\nbit mask - ");
                printBits(sizeof(uint32_t), &currentBitMask);
                printf("\n");*/
            
            }

            if (currentBlock == (readedBlocksNumber - 1) && (cNodePtr->symb == 0)) {
                //printf("Houston, we have a problem... \n");
            }
        }

        memset(readBuff, 0, sizeof(writeBuff));
    }

finish:

    if(currentWriteBuffByte > 0) {
        //printf("\ncurrentWriteBuffByte - %d\n", currentWriteBuffByte);
        fwrite(writeBuff, sizeof(uint8_t), currentWriteBuffByte, dstFile);
    }

    return true;
}

static void rebuildNodes(qtreeNode* root, uint8_t length, uint32_t code, uint8_t symb) {
    qtreeNode *newNode = initQTreeNode();

    if(length > 0) {
        if (code & 1) {
            if (root->rchild == NULL) 
                root->rchild = newNode;

            rebuildNodes(root->rchild, length - 1, code >> 1, symb);
        } else {
            if (root->lchild == NULL) 
                root->lchild = newNode;

            rebuildNodes(root->lchild, length - 1, code >> 1, symb);
        }
    } else {
/*        printf("------------------------\n");
        printBits(sizeof(uint32_t), &(code));
        printf("\n");
        printf("char - %c\n", symb);
        printf("------------------------\n");*/
        root->symb = symb;
    }
}

static bool _rebuildTree(qtree* self, FILE *srcFile) {
    uint8_t numberOfCodes = self->archInfo.tableLength;
    
    //printf("Table length - %d\n", numberOfCodes);
    codeInfo codes[numberOfCodes];

    fread(codes, sizeof(codeInfo), numberOfCodes, srcFile);
    self->root = initQTreeNode();
    
    for (int i = 0; i < numberOfCodes; ++i) {
        rebuildNodes(self->root, codes[i].length, codes[i].code, codes[i].character);
    }

    return true;
}

qtreeNode* initQTreeNode(void) {
    qtreeNode *newElement = (qtreeNode*) malloc(sizeof(qtreeNode));
 
    newElement->rchild = NULL;
    newElement->lchild = NULL;
    newElement->nextNode = NULL;
    newElement->symb = 0;

    return newElement;
}

qtree* initQTree(void) {
    qtree *self = (qtree*) calloc(1, sizeof(qtree));
    self->numberOfCodes = 0;
    self->progress = (uint8_t*) calloc(1, sizeof(uint8_t));
    self->head = NULL;
    self->tail = NULL;
    self->root = NULL;
    self->compress = compress;
    self->decompress = decompress;

    return self;
}