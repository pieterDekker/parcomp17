#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>
int main (int argc, char **argv)
{
	#pragma omp parallel
	{
		printf ("Hello ");
		sleep(1);
		printf ("world!\n");
	}
	#pragma omp parallel
	printf ("Have a nice day!\n");
	printf ("Have fun!\n");
	
	printf("\n");
	system("echo $USER");
	system("date");
	return EXIT_SUCCESS;
}

