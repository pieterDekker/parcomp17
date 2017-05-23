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
#include "mpiwrapper.h"

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

	int imw, imh;
	if (rank == MASTER) {
		imh = image->height;
		imw = image->width;
	}
	MPI_Bcast(&imh, SINGLE, MPI_INT, MASTER, MPI_COMM_WORLD);
	MPI_Bcast(&imw, SINGLE, MPI_INT, MASTER, MPI_COMM_WORLD);

	int *rcvArr, rcvSize;

	doScattervInt((rank ==  MASTER ? image->imdata[0] : NULL), imh * imw, &rcvArr, &rcvSize, rank, numtasks);

	Image workingChunk;
	workingChunk->height = 1;
	workingChunk->width = rcvSize;
	workingChunk->imdata = safeMalloc(1 * sizeof(int*));
	workingChunk->imdata[0] = rcvArr;

	// Do work
	contrastStretch(0, 255, workingChunk);

	MPI_Barrier(MPI_COMM_WORLD);

	doGathervInt((rank ==  MASTER ? image->imdata[0] : NULL), imh * imw, rcvArr, rank, numtasks);

	if (rank == MASTER) {
		writeImage(image, argv[2]);
		freeImage(image);
	}

	// Finalise MPI environment.
	MPI_Finalize();
	return EXIT_SUCCESS;
}
