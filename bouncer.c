/* Written by Greg Anderson and Umair Naveed */

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	char ext[4] = {0};
	char correctExt[4] = {"jpg"};
	char *input_file;
	const char *dot;

	if (argc > 1)
		printf("argv[1]=%d\n", strlen(argv[1]));
	else {
		printf("No arguments supplied.\n");
		return 1;
	}

	input_file = argv[1];	/* Set input file */
	
	dot = strrchr(input_file, '.');

    if(!dot || dot == input_file) {
		printf("Invalid file.\n");
		return 1;
	}

    printf("ext=%s\n", dot + 1);

	//strncpy(ext, input_file+dot+1, strlen(input_file)-dot+1);

	if (strncmp(ext, correctExt, 4)) {
		printf("Correct!\n");
		printf("ext=%s, correctExt=%s\n", ext, correctExt);
	}
	else
		printf("WRONG!!!\n");

	return 0;
}
