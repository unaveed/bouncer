/* Written by Greg Anderson and Umair Naveed */
/* We are the greatest programmers the world has ever known. */

#include <stdio.h>
#include <string.h>

int flowed(int argc, char **argv) {
/* Delete this comment later */
	char ext[4] = {0};
	char correctExt[4] = {"jpg"};
	char *input_file;
	const char *ext;

	if (argc > 1)
		printf("argv[1]=%d\n", strlen(argv[1]));
	else {
		printf("No arguments supplied.\n");
		return 1;
	}

	input_file = argv[1];	/* Set input file */
	
	ext = strrchr(input_file, '.');

    if(!ext || ext == input_file) {
		printf("Invalid file.\n");
		return 1;
	}

    printf("ext=%s, correctExt=%s\n", ext + 1, correctExt);

	if (strncmp(ext+1, correctExt, 4)) {
		printf("Correct!\n");
	}
	else
		printf("WRONG!!!\n");

	return 0;
}
