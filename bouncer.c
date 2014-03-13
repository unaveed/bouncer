/* Written by Greg Anderson and Umair Naveed */

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	char correctExt[4] = {"jpg"};
	char *input_file;
	const char *ext;
	int result;

	if (argc != 2) {
		printf("Invalid arguments.\n");
		// TODO: Print usage message
		return 1;
	}

	input_file = argv[1];	/* Set input file */
	
	ext = strrchr(input_file, '.');

    if(!ext || ext == input_file) {
		printf("Invalid file.\n");
		return 1;
	}

    printf("ext=%s, correctExt=%s\n", ext + 1, correctExt);

	if (!strncmp(ext+1, correctExt, 4))
		printf("Correct!\n");
	else
		printf("WRONG!!!\n");

	return 0;
}
