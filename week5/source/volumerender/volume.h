// File: volume.h
// Written by Arnold Meijster and Rob de Bruin.
// Restructured by Yannick Stoffers.
// 
// Part of the volume rendering program. 

#ifndef VOLUME_H
#define VOLUME_H

typedef unsigned char byte;

// Data type for storing 3D greyscale volumes.
typedef struct volumestruct
{
	int width, height, depth;
	byte ***voldata;
} *Volume;

Volume makeVolume (int w, int h, int d);
void freeVolume (Volume vol);
Volume readVolume (char *filename);

#endif
