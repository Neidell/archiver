#include "prog_bar.h"

float progress = 0.0;

void* show_bar(void* progress) {
	while (*progress < 1.0) {
	    int barWidth = 70;

	    printf("[");
	    int pos = barWidth * (*progress);

	    for (int i = 0; i < barWidth; ++i) {
	        if (i < pos) {
	        	printf("=");
	        } else {
	        	if (i == pos) {
	        		printf(">");
	        	} else {
	        		printf(" ");
	        	}
	        }
	    }

	    printf("] %d  \r", (int)((*progress) * 100.0));
	    fflush(stdin);

	    //progress += 0.16; // for demonstration only
	}

	printf("\n");

	return NULL;
}