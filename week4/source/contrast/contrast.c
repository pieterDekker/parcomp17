// File: contrast.c
// Written by Arnold Meijster and Rob de Bruin.
// Restructured by Yannick Stoffers.
// 
// A simple program for contrast stretching PPM images. 

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "image.h"

#define MASTER 0

void error (char *errmsg) {
    fputs (errmsg,stderr);
    exit (EXIT_FAILURE);
}

void *safeMalloc (int n) {
    void *ptr = malloc (n);
    if (ptr == NULL) {
        error ("Error: memory allocation failed.\n");
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

    //Barrier

    // Compute scale factor.
    scale = (float) (high - low) / (max - min);

    // Stretch image.
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            im[row][col] = scale * (im[row][col] - min);
        }
    }
}

Image sendChunks(Image image, int numtasks) {
    int chunkSize = (image->height / numtasks) * image->width;
    int *buf = safeMalloc(chunkSize*sizeof(int));

    MPI_Scatter(image->imdata[0], image->height * image->width, MPI_INT, buf,
                chunkSize, MPI_INT, MASTER, MPI_COMM_WORLD);

    Image workingImage;
    printf("%d \n", chunkSize);
    workingImage->width = chunkSize;
    workingImage->height = 1;
    workingImage->imdata[0] = buf;
    return workingImage;
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

    Image workingChunk = sendChunks(image, numtasks);

    // Do work
    //contrastStretch();

    if (rank == MASTER) {
        //image = collectChunks();
        writeImage(image, argv[2]);
        freeImage(image);
    }
    // Finalise MPI environment.
    MPI_Finalize();
    return EXIT_SUCCESS;
}
