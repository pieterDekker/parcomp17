// File: fractalimage.c
// Written by Arnold Meijster and Rob de Bruin.
// Restructured by Yannick Stoffers.
// 
// Part of the contrast stretching program. 

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "fractalimage.h"

// Print error message and terminate execution.
static void error (char *errmsg)
{
	fprintf (stderr, errmsg);
	exit (EXIT_FAILURE);
}

// Wrapper function for malloc. This function adds error checking.
static void *safeMalloc (int n)
{
	void *ptr = malloc (n);
	if (ptr == NULL)
	{
		error ("Error: memory allocation failed.\n");
	}
	return ptr;
}

// Allocates memory for images.
Image makeImage (int w, int h)
{
	Image im;
	int row;
	im = safeMalloc (sizeof (struct imagestruct));
	im->width  = w;
	im->height = h;
	im->imdata = safeMalloc (h * sizeof (int *));
	im->imdata[0] = safeMalloc (h * w * sizeof (int));
	for (row = 0; row < h; row++)
	{
		im->imdata[row] = &(im->imdata[0][row * w]);
	}
	return im;
}

// Frees allocated memory for images.
void freeImage (Image im)
{
	free (im->imdata[0]);
	free (im->imdata);
	free (im);
}

// Reads images from PGM file.
Image readPGM (char *filename)
{
	int c, w, h, maxval, row, col;
	FILE *f;
	Image im;
	unsigned char *scanline;
	
	if ((f = fopen (filename, "rb")) == NULL)
	{
		error ("Opening of PGM file failed\n");
	}
	// Parse header of image file (must be 'P5').
	if ((fgetc (f) != 'P') || (fgetc (f) != '5') || (fgetc (f) != '\n'))
	{
		error ("File is not a valid PGM file\n");
	}
	// Skip comments in file (if any).
	while ((c=fgetc (f)) == '#')
	{
		while ((c=fgetc (f)) != '\n');
	}
	ungetc (c, f);
	// Read width, height of image.
	fscanf (f, "%d %d\n", &w, &h);
	// Read maximum greyvalue (dummy).
	fscanf (f, "%d\n", &maxval);
	if (maxval > 255)
	{
		error ("Sorry, readPGM() supports 8 bits PGM files only.\n");
	}
	// Allocate memory for image.
	im = makeImage (w, h);
	// Read image data.
	scanline = safeMalloc (w * sizeof (unsigned char));
	for (row = 0; row < h; row++)
	{
		fread (scanline, 1, w, f);
		for (col = 0; col < w; col++)
		{
			im->imdata[row][col] = scanline[col];
		}
	}
	free (scanline);
	fclose (f);
	return im;
}

// Function for writing image data to a PGM image file.
// This function is for debugging purposes, disable its use when the results 
// are no longer relevant to obtain.
static void writePPM(Image im, char *filename, int maxIter)
{
	int row, col;
	unsigned char *scanline, colour[maxIter][3];
	int i, h, idx;

	FILE *f = fopen(filename, "wb");
	if (f == NULL)
	{
		error("Opening of file failed\n");
	}
	// Write header of image file (P6).
	fprintf(f, "P6\n");
	fprintf(f, "%d %d\n255\n", im->width, im->height);

	// Compute colours.
	for (i = 0; i < maxIter; i++)
	{
		double angle = 3.14159265*i/maxIter;
		colour[i][0] = (unsigned char)(128 + 127 * sin (100 * angle));
		colour[i][1] = (unsigned char)(128 + 127 * sin (50 * angle));
		colour[i][2] = (unsigned char)(128 + 127 * sin (10 * angle));
	}

	// Write image data.
	scanline = malloc(im->width * 3 * sizeof (unsigned char));
	for (row = 0; row < im->height; row++)
	{
		idx = 0;
		for (col = 0; col < im->width; col++)
		{
			h = im->imdata[row][col];
			scanline[idx++] = colour[h][0];
			scanline[idx++] = colour[h][1];
			scanline[idx++] = colour[h][2];
		}    
		fwrite (scanline, 3, im->width, f);
	}
	free (scanline);
	fclose (f);
}

// Wrapper function for reading images.
Image readImage (char *filename)
{
	Image im;
	im = readPGM (filename);
	return im;
}

// Wrapper function for writing images.
void writeImage (Image im, char *filename, int maxIter)
{
	writePPM (im, filename, maxIter);
}
