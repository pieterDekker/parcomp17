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

Image sendChunks(Image image, int numtasks, int chunkSize, int *sendCounts, int *displs, int rank, int imw, int imh) {
	int *buf = safeMalloc(sendCounts[rank] * sizeof(int));
	//if (rank != MASTER) {
		//image = makeImage(imw, imh);
	printf("sendchunks: buf malloc'd\n");
	MPI_Scatterv(image->imdata[0], sendCounts, displs, MPI_INT, buf,
							 sendCounts[rank], MPI_INT, MASTER, MPI_COMM_WORLD);
	printf("sendchunks: scatterv'd\n");

	Image workingImage = safeMalloc(sizeof(struct imagestruct));
	printf("sendchunks: Image malloc'd\n");
	printf("rank: %d, sendcounts[%d]: %d\n", rank, rank, sendCounts[rank]);
	workingImage->width = sendCounts[rank];
	workingImage->height = 1;
	workingImage->imdata = safeMalloc(workingImage->height * sizeof(int*));
	workingImage->imdata[0] = buf;
	return workingImage;
}

int main(int argc, char **argv) {
	Image image;
	int rank;
	int numtasks;
	int chunkSize;

	// Initialise MPI environment.
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (argc != 3) {
			printf("Usage: %s input.pgm output.pgm\n", argv[0]);
			exit(EXIT_FAILURE);
	}

	int *sendCount;
	int *displs;

	if (rank == MASTER) {
			// Prints current date and user. DO NOT MODIFY
			system("date");
			system("echo $USER");
			//Only the master thread executes this
			image = readImage(argv[1]);
			
	}
	
	MPI_Bcast(&chunkSize, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

	sendCount = safeMalloc(numtasks * sizeof(int));
	displs = safeMalloc(numtasks * sizeof(int));
	
	
	
	int imw, imh;
	if (rank == MASTER) {
		imh = image->height;
		imw = image->width;
	}
	MPI_Bcast(&imh, SINGLE, MPI_INT, MASTER, MPI_COMM_WORLD);
	MPI_Bcast(&imw, SINGLE, MPI_INT, MASTER, MPI_COMM_WORLD);
	
	int size = imh * imw;
	int rem = size%numtasks;
	int sum = 0;
	for (int i = 0; i < numtasks; i++) {
			sendCount[i] = size / numtasks;
			if (rem > 0) {
					sendCount[i]++;
					rem--;
			}
			displs[i] = sum;
			sum += sendCount[i];
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	Image workingChunk = sendChunks(image, numtasks, chunkSize, sendCount, displs, rank, imw, imh);
	// Do work
	contrastStretch(0, 255, workingChunk);

	MPI_Gatherv(workingChunk->imdata[0], sendCount[rank], MPI_INT,
									image->imdata[0], sendCount, displs, MPI_INT, MASTER, MPI_COMM_WORLD);
	
	if (rank == MASTER) {
			writeImage(image, argv[2]);
			freeImage(image);
	}

	// Finalise MPI environment.
	MPI_Finalize();
	return EXIT_SUCCESS;
}
