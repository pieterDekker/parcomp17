// File: mandelbrot.c
// Written by Arnold Meijster and Rob de Bruin
// Restructured by Yannick Stoffers
// 
// A simple program for computing images of the Mandelbrot set. 

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include "fractalimage.h"

#define WIDTH  4096
#define HEIGHT 3072
#define MAXITER 3000

int rank, size;

// Function for computing mandelbrot fractals.
void mandelbrotSet (double centerX, double centerY, double scale, Image image)
{
	int w = image->width, h = image->height, **im = image->imdata;
	double a, b;
	double x, y, z;
	int i, j, k;
	
	for (i = 0; i < h; i++)
	{
		b = centerY + i * scale - ((h / 2) * scale);
		for (j = 0; j < w; j++)
		{
			a = centerX + j * scale - ((w / 2) * scale);
			x = a;
			y = b;
			k = 0;
			while ((x * x + y * y <= 100) && (k < MAXITER))
			{
				z = x;
				x = x * x - y * y + a;
				y = 2 * z * y + b;
				k++;
			}
			im[i][j] = k;
		}
	}
}

int main (int argc, char **argv)
{
	Image mandelbrot;
	
	// Prints current date and user. DO NOT MODIFY
	system("date"); system("echo $USER");
	
	// Initialise MPI environment.
	MPI_Init (&argc, &argv);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);
	MPI_Comm_size (MPI_COMM_WORLD, &size);

	mandelbrot = makeImage (WIDTH, HEIGHT);
	mandelbrotSet (-0.65, 0, 2.5 / HEIGHT, mandelbrot);

	writeImage (mandelbrot, "mandelbrot.ppm", MAXITER);

	freeImage (mandelbrot);
	
	// Finalise MPI environment.
	MPI_Finalize ();
	return EXIT_SUCCESS;
}
