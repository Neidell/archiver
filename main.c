#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "qtree.h"
#include "prog_bar.h"

#define BUFFER_SIZE 1024

pthread_t tid;

int main(int argc, char **argv) {
    extern char* optarg;
    clock_t t1, t2;
    int c;

	//pthread_create(&(tid), NULL, &show_bar, NULL);
    qtree* qt = NULL;
    qt = initQTree();

    while ((c = getopt(argc, argv, "c:d:")) != -1) {
        switch (c) {
            case 'c':
                t1 = clock();
                pthread_create(&(tid), NULL, &show_bar, qt->progress);
                qt->compress(qt, "compressed.huff", optarg);
                t2 = clock();
                printf("Encoding completed in %.1f sec\n", ((float)t2 - (float)t1) / CLOCKS_PER_SEC);
                break;
            case 'd':
                t1 = clock();
                qt->decompress(qt, "decompressed.txt", optarg);
                t2 = clock();
                printf("Decoding completed in %.1f sec\n", ((float)t2 - (float)t1) / CLOCKS_PER_SEC); 
                break;
            default:
                abort();
        } 

    }
/*
    qt = initQTree();

    qt->compress(qt, "compressed.huff", "big.txt");

    free(qt);

    t2 = clock();

    qt = initQTree();
    qt->decompress(qt, "decompressed.txt", "compressed.huff");

    free(qt);

    t3 = clock();

    printf("\n");
    printf("Encoding time: %.1f sec\n", ((float)t2 - (float)t1) / CLOCKS_PER_SEC);
    printf("Decoding time: %.1f sec\n", ((float)t3 - (float)t2) / CLOCKS_PER_SEC);
*/
    return 0;
}
