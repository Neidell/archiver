#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include "huffman.h"
#include "prog_bar.h"

pthread_t tid;

int main(int argc, char **argv) {
    extern char* optarg;
    clock_t t1, t2;
    int c;

	//pthread_create(&(tid), NULL, &show_bar, NULL);
    ARCH* arch = initArch();

    while ((c = getopt(argc, argv, "c:d:")) != -1) {
        switch (c) {
            case 'c':
                t1 = clock();
                compress(arch, "compressed.huff", optarg);
                t2 = clock();
                printf("Encoding completed in %.5f sec\n", ((double)t2 - (double)t1) / CLOCKS_PER_SEC);
                break;
            case 'd':
                t1 = clock();
                decompress(arch, "decompressed.txt", optarg);
                t2 = clock();
                printf("Decoding completed in %.5f sec\n", ((double)t2 - (double)t1) / CLOCKS_PER_SEC); 
                break;
            default:
                abort();
        } 

    }

    return 0;
}
