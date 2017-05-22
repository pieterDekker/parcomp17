// File: contrast.c
// Written by Arnold Meijster and Rob de Bruin.
// Restructured by Yannick Stoffers.
// 
// A simple program for contrast stretching PPM images. 

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include "image.h"

#define MASTER 0
#define SINGLE 1

void error(char *errmsg) {
	fputs(errmsg, stderr);
	exit(EXIT_FAILURE);
}

void *safeMalloc(int n) {
	void *ptr = malloc(n);
	if (ptr == NULL) {
			error("Error: memory allocation failed.\n");
	}
	return ptr;
}

// Stretches the contrast of an image. 
void contrastStretch(int low, int high, Image image) {
	int row, col, min, max;
	int width = image->width, height = image->height, **im = image->imdata;
	float scale;

	// Determine minimum and maximum.
	min = max = im[0][0];

	for (row = 0; row < height; row++) {
			for (col = 0; col < width; col++) {
					min = im[row][col] < min ? im[row][col] : min;
					max = im[row][col] > max ? im[row][col] : max;
			}
	}

	int absMin, absMax;

	MPI_Reduce(&min, &absMin, SINGLE, MPI_INT, MPI_MIN, MASTER, MPI_COMM_WORLD);
	MPI_Reduce(&max, &absMax, SINGLE, MPI_INT, MPI_MAX, MASTER, MPI_COMM_WORLD);
	MPI_Bcast(&absMin, SINGLE, MPI_INT, MASTER, MPI_COMM_WORLD);
	MPI_Bcast(&absMax, SINGLE, MPI_INT, MASTER, MPI_COMM_WORLD);

	min = absMin;
	max = absMax;

	// Compute scale factor.
	scale = (float) (high - low) / (max - min);

	// Stretch image.
	for (row = 0; row < height; row++) {
			for (col = 0; col < width; col++) {
					im[row][col] = scale * (im[row][col] - min);
			}
	}
}

/**
 * This funtion wraps MPI_Gatherv, must be integers.
 * @param data The data at root to scatter
 * @param size The size of the data at root to scatter
 * @param localdata Pointer to the receiving array
 * @param localsize Size of the receiving array
 * @param rank
 * @param numprocss
 * @return
 */
int doScattervInt(int *data, int size, int **localdata, int *localsize, int rank, int numprocss) {
	printf("@sctr size %d numprocss %d for %d\n", size, numprocss, rank);
	fflush(stdout);
	
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

	printf("@sctr sendcount,displs calc'd for %d\n", rank);
	
	*localdata = calloc(sendcounts[rank], sizeof(int));
	assert(localdata != NULL);
	*localsize = sendcounts[rank];
	
	MPI_Barrier(MPI_COMM_WORLD);
	
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
	printf("@gthr sendcount,displs calc'd\n");
	
	return MPI_Gatherv(localdata, sendcounts[rank], MPI_INT, data, sendcounts, displs, MPI_INT, MASTER, MPI_COMM_WORLD);
}


int main(int argc, char **argv) {
	Image image;
	int rank;
	int numtasks;

	// Initialise MPI environment.
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (argc != 3) {
			printf("Usage: %s input.pgm output.pgm\n", argv[0]);
			exit(EXIT_FAILURE);
	}

	if (rank == MASTER) {
			// Prints current date and user. DO NOT MODIFY
			system("date");
			system("echo $USER");
			//Only the master thread executes this
			image = readImage(argv[1]);
			
	}

	MPI_Barrier(MPI_COMM_WORLD);

	printf("init'd, rank %d, commsize %d\n", rank, numtasks);
	fflush(stdout);

	MPI_Barrier(MPI_COMM_WORLD);
	
	int imw, imh;
	if (rank == MASTER) {
		imh = image->height;
		imw = image->width;
	}
	MPI_Bcast(&imh, SINGLE, MPI_INT, MASTER, MPI_COMM_WORLD);
	MPI_Bcast(&imw, SINGLE, MPI_INT, MASTER, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);
	int *rcvArr, rcvSize;

	printf("size bcast'd %d * %d\n", imh, imw);
	fflush(stdout);

	doScattervInt((rank ==  MASTER ? image->imdata[0] : NULL), imh * imw, &rcvArr, &rcvSize, rank, numtasks);
	printf("scattered\n");

	Image workingChunk;
	workingChunk->height = 1;
	workingChunk->width = rcvSize;
	workingChunk->imdata = safeMalloc(1 * sizeof(int*));
	workingChunk->imdata[0] = rcvArr;

	// Do work
	contrastStretch(0, 255, workingChunk);
	printf("stretched\n");

	MPI_Barrier(MPI_COMM_WORLD);

	doGathervInt((rank ==  MASTER ? image->imdata[0] : NULL), imh * imw, rcvArr, rank, numtasks);
	printf("gathered\n");
	if (rank == MASTER) {
		writeImage(image, argv[2]);
		freeImage(image);
	}

	// Finalise MPI environment.
	MPI_Finalize();
	return EXIT_SUCCESS;
}
