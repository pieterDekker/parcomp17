/**
 * testing scatterv
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

#define MASTER 0

typedef struct M {
	int **data;
	int r;
	int c;
} Matrix;

Matrix newMatrix(int r, int c) {
	Matrix m;
	m.r = r;
	m.c = c;
	m.data = malloc(m.r * sizeof(int*));
	int *arr = malloc(m.r * m.c * sizeof(int));
	assert(m.data != NULL && arr != NULL);
	
	for (int i = 0; i < m.r; ++i) {
		m.data[i] = arr + (i * m.c);
	}
	return m;
}

void printArray(int *a, int l) {
	for (int i = 0; i < l; ++i) {
			printf("%d\t", a[i]);
		}
}

void printMatrix(Matrix m) {
	for (int i = 0; i < m.r; ++i) {
		printArray(m.data[i], m.c);
		printf("\n\n");
	}
}

int doScattervInt(int *data, int size, int **localdata, int *localsize, int rank, int numprocss) {
	int *sendcounts = malloc(sizeof(int)*numprocss);
	int *displs = malloc(sizeof(int)*numprocss);
	assert(sendcounts != NULL && displs != NULL);
	
	int rem = size%numprocss;
	int sum = 0;
	for (int i = 0; i < numprocss; i++) {
			sendcounts[i] = size / numprocss;
			if (rem > 0) {
					sendcounts[i]++;
					rem--;
			}
			displs[i] = sum;
			sum += sendcounts[i];
	}
	
	*localdata = calloc(sendcounts[rank], sizeof(int));
	assert(localdata != NULL);
	*localsize = sendcounts[rank];
	
	return MPI_Scatterv(data, sendcounts, displs, MPI_INT, *localdata, sendcounts[rank], MPI_INT, MASTER, MPI_COMM_WORLD);
}

int doGathervInt(int *data, int size, int *localdata, int rank, int numprocss) {
	int *sendcounts = malloc(sizeof(int)*numprocss);
	int *displs = malloc(sizeof(int)*numprocss);
	assert(sendcounts != NULL && displs != NULL);
	
	int rem = size%numprocss;
	int sum = 0;
	for (int i = 0; i < numprocss; i++) {
			sendcounts[i] = size / numprocss;
			if (rem > 0) {
					sendcounts[i]++;
					rem--;
			}
			displs[i] = sum;
			sum += sendcounts[i];
	}
	
	return MPI_Gatherv(localdata, sendcounts[rank], MPI_INT, data, sendcounts, displs, MPI_INT, MASTER, MPI_COMM_WORLD);
}

int main (int argc, char** argv) {
	int r = 3, c = 4;
	int rank, numprocss;
	Matrix m = newMatrix(r, c);
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocss);
	
	if (rank == MASTER) {//fill the matrix
		int cnt = 1;
		for (int i = 0; i < m.r; ++i) {
			for (int j = 0; j < m.c; ++j) {
				m.data[i][j] = cnt++;
			}
		}
		printf("@master:\n");
		printMatrix(m);
	}
	
	int *rcvArr, rcvSize;
	
	doScattervInt(m.data[0], m.r * m.c, &rcvArr, &rcvSize, rank, numprocss);
	
	//printf("%d>>> ", rank);
	for (int i = 0; i < rcvSize; ++i) {
		//printf("%d\t", rcvArr[i]);
		rcvArr[i] += 1;
	}
	//printf("\n");

	MPI_Barrier(MPI_COMM_WORLD);
	
	doGathervInt(m.data[0], m.r * m.c, rcvArr, rank, numprocss);
	
	if (rank == MASTER) {//print the incremented matrix
		printf("@master:\n");
		printMatrix(m);
		fflush(stdout);
	}
	
	return 0;
}
