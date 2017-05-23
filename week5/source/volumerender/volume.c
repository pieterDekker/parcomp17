// File: volume.c
// Written by Arnold Meijster and Rob de Bruin.
// Restructured by Yannick Stoffers.
//
// Part of the volume rendering program.

#include <stdlib.h>
#include <stdio.h>
#include "volume.h"

// Print error message and terminate execution.
static void error(char *errmsg) {
    fprintf(stderr, errmsg);
    exit(EXIT_FAILURE);
}

// Wrapper function for malloc. This function adds error checking.
static void *safeMalloc(int n) {
    void *ptr = malloc(n);
    if (ptr == NULL) {
        error("Error: memory allocation failed.\n");
    }
    return ptr;
}

// Allocates memory for a volume structure.
Volume makeVolume(int w, int h, int d) {
    Volume vol;
    int slice, row;
    vol = safeMalloc(sizeof(struct volumestruct));
    vol->width = w;
    vol->height = h;
    vol->depth = d;

    vol->voldata = safeMalloc(d * sizeof(int **));
    vol->voldata[0] = safeMalloc(d * h * sizeof(int *));
    vol->voldata[0][0] = safeMalloc(d * h * w * sizeof(int));
    for (slice = 0; slice < d; slice++) {
        vol->voldata[slice] = &(vol->voldata[0][slice * h]);
        for (row = 0; row < h; row++)
            vol->voldata[slice][row] = &(vol->voldata[0][0][slice * h * w + row * w]);
    }
    return vol;
}

// Frees allocated memory for volumes.
void freeVolume(Volume vol) {
    free(vol->voldata[0][0]);
    free(vol->voldata[0]);
    free(vol->voldata);
    free(vol);
}

// Reads a volume from a VOX file.
Volume readVolume(char *filename) {
    Volume volume;
    byte ***vol;
    int i, j, k, width, height, depth;
    FILE *f;
    byte *scanline;

    f = fopen(filename, "rb");
    if (!f) error("Error opening volume file");

    fscanf(f, "D3\n%d %d %d\n255\n", &width, &height, &depth);

    volume = makeVolume(width, height, depth);
    vol = volume->voldata;
    scanline = safeMalloc(width * sizeof(byte));
    for (i = 0; i < depth; i++) {
        for (j = 0; j < height; j++) {
            fread(scanline, sizeof(byte), width, f);
            for (k = 0; k < width; k++)
                vol[i][j][k] = scanline[k];
        }
    }
    free(scanline);
    fclose(f);
    return volume;
}
