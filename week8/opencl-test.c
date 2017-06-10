#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <cl.h>

int main (int argc, char** argv) {
	cl_platform_id platform;
	cl_uint numPlatforms;
	clGetPlatformIDs(1, &platform,&numPlatforms);

	char* infoBuffer = malloc(1024 * sizeof(char));
	size_t bufferSize;
	assert(infoBuffer != NULL);

	printf("Found %ud platform(s)\n", (unsigned int) numPlatforms);

	if ((unsigned int) numPlatforms > 0) {
		clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(char*), *infoBuffer, &bufferSize);
		printf("\t platform is from vendor %s\n", infoBuffer);
	}

	return 0;
}



