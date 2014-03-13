/* Written by Greg Anderson and Umair Naveed */

#include <stdio.h>
#include <string.h>
#include "bouncer.h"

int main(int argc, char **argv) {
	char correctExt[4] = {"jpg"};
	char *input_file;
	const char *ext;

	/* Check to make sure correct number of arguments are supplied. */
	if (argc != 2) {
		printf("Incorrect number of arguments.\n");
		usage();
		return 1;
	}

	input_file = argv[1];	/* Set input file */
	
	ext = strrchr(input_file, '.');
    if(!ext || ext == input_file || strncmp(ext+1, correctExt, 4)) {
		printf("Invalid filetype.\n");
		usage();
		return 1;
	}

	return 0;
}

/* Prints the usage message. */
void usage() {
	printf("Usage: bouncer <filename.jpg>\n");
}
