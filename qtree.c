#include "qtree.h"

static qtreeNode* _initQTreeNode(void);
static bool _insertToQueue(qtree*, qtreeNode*, qtreeNode*, bool);
static void _traverseTree(qtreeNode*, void (*)(qtreeNode*, uint8_t, uint32_t, codeInfo[]), 
                            uint8_t, uint32_t, codeInfo[]);
static inline void _visitNode(qtreeNode*, uint8_t, uint32_t, codeInfo[]);
static inline bool _add(qtree*, uint8_t, uint32_t);
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

static inline bool _add(qtree* self, uint8_t symb, uint32_t value) {
    qtreeNode* newNode = _initQTreeNode();
    qtreeNode *cPtr = self->head;
    
    newNode->symb = symb;
    newNode->weight = value;

    if (cPtr != NULL)  {
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
    
    while ((bool)(readedChars = fread(buff, sizeof(uint8_t), BUFFER_SIZE, text))) {
        for (i = 0; i < readedChars; i++) {
            symbols[buff[i]]++;    
        }
    }

    for (i = 0; i < 256; i++) {
        if (symbols[i] > 0) {
            _add(self, i, symbols[i]);
        }
    }

    fclose(text);
    return true;
}

static bool _buildTree(qtree* self) {
    qtreeNode *firstNode, *secondNode, *newNode, *cPtr;

    if (self->head == self->tail) {
        self->root = _initQTreeNode();
        self->root->lchild = self->head;

        return true;
    }

    while (self->head != self->tail) {     
        firstNode = self->head;
        secondNode = firstNode->nextNode;
        newNode = _initQTreeNode();

        newNode->weight = firstNode->weight + secondNode->weight;
        newNode->lchild = firstNode;
        newNode->rchild = secondNode;
        newNode->nextNode = secondNode->nextNode;

        if (self->head->nextNode != self->tail) {
            if (newNode->nextNode->weight < newNode->weight) {
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

    return true;
}

//assumes little endian
void printBits(uint32_t const size, void const * const ptr) {
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1; i >= 0; i--) {
        for (j=7; j >= 0; j--) {
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

static inline void _visitNode(qtreeNode* root, uint8_t depth,
                        uint32_t code, codeInfo codes[]) {

    uint32_t reversedCode;
    if (root->symb) {
        reversedCode = _reverse_bits(code, depth);
        codes[root->symb] = (codeInfo){root->symb, depth, reversedCode};
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
    codeInfo *codeTable = self->codes;
    _traverseTree(self->root, _visitNode, (uint8_t)0, (uint32_t)0, self->codes);

    for (int i = 0; i < 256; ++i) {
        if ((codeTable[i].length) > 0) {
            (self->numberOfCodes)++;
        }
    }

    return 0;
}

static bool _writeCodesToFile(qtree* self, const char* dstFileName) {
    codeInfo codes[self->numberOfCodes];
    codeInfo *codeTable = self->codes;

    FILE *dstFile = fopen(dstFileName, "w+");

    for (int i = 0, j = 0; i < 256; ++i) {
        if ((codeTable[i].length) > 0) {
            codes[j++] = codeTable[i];
        }
    }

    self->archInfo.tableLength = self->numberOfCodes;
    fseek(dstFile, sizeof(archiveInfo), SEEK_SET);    
    fwrite(codes, sizeof(codeInfo), self->numberOfCodes, dstFile);

    fclose(dstFile);

    return true;
}

static bool _writeDataToFile(qtree* self, const char* dstFileName, const char* srcFileName) {

    FILE *dstFile = fopen(dstFileName, "a+");
    FILE *srcFile = fopen(srcFileName, "r");

    codeInfo *codeTable = self->codes;
    uint8_t readBuff[BUFFER_SIZE];
    uint8_t nextReadedCharAsciiCode;
    uint8_t nextReadedCharCodeLength;

    uint32_t writeBuff[BUFFER_SIZE] = {0};
    uint32_t readedChars;
    uint32_t tempCode_1 = 0, tempCode_2 = 0;
    uint32_t writedBlocks = 0;
    uint32_t i;

    int32_t freeBlocks = BUFFER_SIZE, freeBits = BITS_IN_BLOCK;
    int32_t availableBits = 0;

    while ((freeBlocks) > 0) {
        if ((bool)(readedChars = fread(readBuff, sizeof(uint8_t), BUFFER_SIZE, srcFile))) {

            for (i = 0; i < readedChars; i++) {
                nextReadedCharAsciiCode = readBuff[i];
                nextReadedCharCodeLength = codeTable[nextReadedCharAsciiCode].length;

                availableBits = freeBits - nextReadedCharCodeLength;

                if (availableBits > 0) {
                    tempCode_1 = codeTable[nextReadedCharAsciiCode].code;
                    tempCode_1 <<= (BITS_IN_BLOCK - freeBits);
                    //write code of another readed character
                    writeBuff[BUFFER_SIZE - freeBlocks] |= tempCode_1;

                    freeBits -= nextReadedCharCodeLength;
                } else {
                    tempCode_1 = (codeTable[nextReadedCharAsciiCode].code << (BITS_IN_BLOCK - nextReadedCharCodeLength + abs(availableBits)));
                    tempCode_2 = (codeTable[nextReadedCharAsciiCode].code >> freeBits);
                    freeBits = BITS_IN_BLOCK + availableBits;
                    writeBuff[BUFFER_SIZE - freeBlocks] |= tempCode_1;
                    freeBlocks--;
               
                    if (freeBlocks == 0) {
                        fwrite(writeBuff, sizeof(uint32_t), BUFFER_SIZE, dstFile);
                        writedBlocks++;
                        memset(writeBuff, 0, sizeof(writeBuff));
                        
                        freeBlocks = BUFFER_SIZE;
                        freeBits = BITS_IN_BLOCK;
                
                        if (i >= readedChars) break;
                        
                        i--;
                    } else {
                        writeBuff[BUFFER_SIZE - freeBlocks] |= tempCode_2;
                    }
                }
            }
        } else {
            if (freeBlocks - 1 < BUFFER_SIZE) {
                fwrite(writeBuff, sizeof(uint32_t), BUFFER_SIZE - freeBlocks + 1, dstFile);
                writedBlocks++;
            }

            self->archInfo.remainingBits = (sizeof(writeBuff) * 8) - (((freeBlocks - 1) * BITS_IN_BLOCK) + freeBits);
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

    fseek(dstFile, 0, SEEK_SET);
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
        return true;
    } else {
        return false;
    }
}

static bool _decodeFile(qtree* self, FILE* dstFile, FILE* srcFile) {
    int32_t remainingSignificantBits = self->archInfo.remainingBits;
    uint32_t readBuff[BUFFER_SIZE] = {0};
    uint8_t writeBuff[BUFFER_SIZE] = {0};
    uint32_t currentWriteBuffByte = 0;
    uint32_t readedBlocksNumber = 0;
    uint32_t totalReadedBlocks = 0;
    register uint32_t currentBitMask = 1;
    uint32_t currentBlock = 0;
    bool isLastBlock = false;

    qtreeNode* cNodePtr = self->root;

    while ((bool)(readedBlocksNumber = fread(readBuff, sizeof(uint32_t), BUFFER_SIZE, srcFile))) {
        if (++totalReadedBlocks == self->archInfo.numberOfBlocks) {
            isLastBlock = true;
        }

        cNodePtr = self->root;

        for (currentBlock = 0; currentBlock < readedBlocksNumber; ++currentBlock) {
            currentBitMask = 1;
            
            while (currentBitMask) {

                if (cNodePtr->symb != 0) {
                    writeBuff[currentWriteBuffByte] = cNodePtr->symb;
                    currentWriteBuffByte++; 

                    cNodePtr = self->root;

                    if (currentWriteBuffByte == BUFFER_SIZE) {
                        fwrite(writeBuff, sizeof(uint8_t), BUFFER_SIZE, dstFile);
                        memset(writeBuff, 0, sizeof(writeBuff));
                        currentWriteBuffByte = 0;
                    }
                }

                cNodePtr = (readBuff[currentBlock] & currentBitMask) ? cNodePtr->rchild : cNodePtr->lchild;

                if (cNodePtr == NULL) {
                    goto finish;
                }

                currentBitMask <<= 1;

                if (isLastBlock) {
                    remainingSignificantBits--;
                    if (remainingSignificantBits < 0) {
                        goto finish;
                    }
                }
            }
        }

        memset(readBuff, 0, sizeof(writeBuff));
    }

finish:

    if(currentWriteBuffByte > 0) {
        fwrite(writeBuff, sizeof(uint8_t), currentWriteBuffByte, dstFile);
    }

    return true;
}

static void rebuildNodes(qtreeNode* root, uint8_t length, uint32_t code, uint8_t symb) {
    qtreeNode *newNode = _initQTreeNode();

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
        root->symb = symb;
    }
}

static bool _rebuildTree(qtree* self, FILE *srcFile) {
    uint8_t numberOfCodes = self->archInfo.tableLength;
    codeInfo codes[numberOfCodes];

    fread(codes, sizeof(codeInfo), numberOfCodes, srcFile);
    self->root = _initQTreeNode();
    
    for (int i = 0; i < numberOfCodes; ++i) {
        rebuildNodes(self->root, codes[i].length, codes[i].code, codes[i].character);
    }

    return true;
}

qtreeNode* _initQTreeNode(void) {
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