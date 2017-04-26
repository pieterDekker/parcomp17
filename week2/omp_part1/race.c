#include <stdio.h>
#include <stdlib.h>
#define N 1000000
int main (int argc, char **argv)
{
	int i, x;
	int histogram[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	for (i = 0; i < N; i++)
	{
		x = 1;
		#pragma omp parallel sections shared(x)
		{
			#pragma omp section
			{
				int y = x;
				y++;
				x = y;
			}
			#pragma omp section
			{
				int y = x;
				y = 3*y;
				x = y;
			}
		}
		histogram[x]++;
	}
	for (i = 0; i < 10; i++) {
		printf ("%d: %d\n", i, histogram[i]);
	}
	
	printf("\n");
	system("echo $USER");
	system("date");
	return EXIT_SUCCESS;
}

