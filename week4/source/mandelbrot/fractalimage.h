// File: fractalimage.h
// Written by Arnold Meijster and Rob de Bruin.
// Restructured by Yannick Stoffers.
// 
// Part of the contrast stretching program. 

#ifndef FRACTAL_IMAGE_H
#define FRACTAL_IMAGE_H

// Data type for storing 2D greyscale image.
typedef struct imagestruct
{
	int width, height;
	int **imdata;
} *Image;

Image makeImage (int w, int h);
void freeImage (Image im);
Image readImage (char *filename);
void writeImage (Image im, char *filename, int maxIter);

#endif
