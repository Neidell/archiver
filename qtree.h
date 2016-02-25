#ifndef QTREE_H
#define QTREE_H

#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h>
#include <stdbool.h>
#include <string.h> 
#include <math.h>

#define BUFFER_SIZE 1024

#define minWeightNode(a,b) (((a->weight)<(b->weight))?(a):(b))
#define maxWeightNode(a,b) (((a->weight)>(b->weight))?(a):(b))

typedef struct qtreeNode qtreeNode;
typedef struct qtree qtree;
typedef struct codeInfo codeInfo;
typedef struct archiveInfo archiveInfo;

struct archiveInfo {
    uint8_t tableLength;
    uint32_t numberOfBlocks;
    uint32_t remainingBits;
};

struct codeInfo {
    uint8_t character;
	uint8_t length;
	uint32_t code;
};

struct qtreeNode {
    qtreeNode *rchild;
    qtreeNode *lchild;
    qtreeNode *nextNode;
    uint32_t weight;
    uint8_t symb;
};

struct qtree {
    qtreeNode *head;
    qtreeNode *tail;
    qtreeNode *root;
    uint8_t *progress;
    codeInfo codes[256];
    archiveInfo archInfo;
    uint8_t numberOfCodes;
    bool (*compress)(qtree*, const char*, const char*);
    bool (*decompress)(qtree*, const char*, const char*);    
};


bool compress(qtree* self, const char* dstFileName, const char* srcFileName);
bool decompress(qtree* self, const char* dstFileName, const char* srcFileName);
qtree* initQTree(void);
qtreeNode* initQTreeNode(void);

#endif
