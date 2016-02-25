#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include "qtree.h"
#include "prog_bar.h"

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
                qt->compress(qt, "compressed.huff", optarg);
                t2 = clock();
                printf("Encoding completed in %.10f sec\n", ((double)t2 - (double)t1) / CLOCKS_PER_SEC);
                break;
            case 'd':
                t1 = clock();
                qt->decompress(qt, "decompressed.txt", optarg);
                t2 = clock();
                printf("Decoding completed in %.10f sec\n", ((double)t2 - (double)t1) / CLOCKS_PER_SEC); 
                break;
            default:
                abort();
        } 

    }

    return 0;
}
