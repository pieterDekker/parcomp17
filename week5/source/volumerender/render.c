// File: render.c
// Written by Arnold Meijster and Rob de Bruin.
// Restructured by Yannick Stoffers.
// 
// A simple orthogonal maximum intensity projection volume render. 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <assert.h>
#include <sys/time.h>
#include "image.h"
#include "volume.h"

#define NFRAMES 100

#define MASTER 0

#define WIDTH    0
#define HEIGHT    1
#define DEPTH    2

#define RESULT 99

static double timer(void) {
    struct timeval tm;
    gettimeofday(&tm, NULL);
    return tm.tv_sec + tm.tv_usec / 1000000.0;
}

void rotateVolume(double rotx, double roty, double rotz, Volume volume, Volume rotvolume) {
    // Rotate the volume around the x-axis with angle rotx, followed by a
    // rotation around the y-axis with angle roty, and finally around the
    // z-axis with angle rotz. The rotated volume is returned in rotvolume.
    int i, j, k, xi, yi, zi;
    int width = volume->width;
    int height = volume->height;
    int depth = volume->depth;
    byte ***vol = volume->voldata;
    byte ***rot = rotvolume->voldata;
    double x, y, z;
    double sinx, siny, sinz;
    double cosx, cosy, cosz;

    for (i = 0; i < depth; i++)
        for (j = 0; j < height; j++)
            for (k = 0; k < width; k++)
                rot[i][j][k] = 0;

    sinx = sin(rotx);
    siny = sin(roty);
    sinz = sin(rotz);
    cosx = cos(rotx);
    cosy = cos(roty);
    cosz = cos(rotz);

    for (i = 0; i < depth; i++) {
        for (j = 0; j < height; j++) {
            for (k = 0; k < width; k++) {
                xi = j - height / 2;
                yi = k - width / 2;
                zi = i - depth / 2;

                // Rotation around x-axis.
                x = (double) xi;
                y = (double) (yi * cosx + zi * sinx);
                z = (double) (zi * cosx - yi * sinx);
                xi = (int) x;
                yi = (int) y;
                zi = (int) z;

                // Rotation around y-axis.
                x = (double) (xi * cosy + zi * siny);
                y = (double) (yi);
                z = (double) (zi * cosy - xi * siny);
                xi = (int) x;
                yi = (int) y;
                zi = (int) z;

                // Rotation around z-axis.
                x = (double) (xi * cosz + yi * sinz);
                y = (double) (yi * cosz - xi * sinz);
                z = (double) zi;
                xi = (int) (x + height / 2);
                yi = (int) (y + width / 2);
                zi = (int) (z + depth / 2);

                if ((xi >= 0) && (xi < height) && (yi >= 0) && (yi < width) && (zi >= 0) && (zi < depth))
                    rot[zi][xi][yi] = vol[i][j][k];
            }
        }
    }
}

void contrastStretch(int low, int high, Image image) {
    // Stretch the dynamic range of the image to the range [low..high].
    int row, col, min, max;
    int width = image->width, height = image->height, **im = image->imdata;
    double scale;

    // Determine minimum and maximum.
    min = max = im[0][0];
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            min = im[row][col] < min ? im[row][col] : min;
            max = im[row][col] > max ? im[row][col] : max;
        }
    }

    // Compute scale factor.
    scale = (double) (high - low) / (max - min);

    // Stretch image.
    for (row = 0; row < height; row++)
        for (col = 0; col < width; col++)
            im[row][col] = (int) (scale * (im[row][col] - min));
}

void orthoGraphicRenderer(Volume volume, Image image) {
    // Render image from volume (othographic maximum intensity projection).
    int i, j, k;
    int width = volume->width;
    int height = volume->height;
    int depth = volume->depth;
    int **im = image->imdata;
    byte ***vol = volume->voldata;

    for (i = 0; i < height; i++)
        for (j = 0; j < width; j++)
            im[i][j] = 0;

    for (i = 0; i < depth; i++)
        for (j = 0; j < height; j++)
            for (k = 0; k < width; k++)
                im[j][k] += vol[i][j][k];

    contrastStretch(0, 255, image);
}

void smoothImage(Image image, Image smooth) {
    int width = image->width, height = image->height;
    int **im = image->imdata, **sm = smooth->imdata;
    int i, j, ii, jj, sum, cnt;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            cnt = 0;
            sum = 0;
            for (ii = i - 1; ii <= i + 1; ii++) {
                if ((ii >= 0) && (ii < height)) {
                    for (jj = j - 1; jj <= j + 1; jj++) {
                        if ((jj >= 0) && (jj < width) && (im[ii][jj] != 0)) {
                            cnt++;
                            sum += im[ii][jj];
                        }
                    }
                }
            }
            sm[i][j] = cnt == 0 ? 0 : sum / cnt;
        }
    }
}

void computeFrame(int frame, double rotx, double roty, double rotz, Volume vol, Volume rot, Image image, Image smooth, int rank) {
    rotateVolume(rotx, roty, rotz, vol, rot);
    orthoGraphicRenderer(rot, image);
    smoothImage(image, smooth);
}

int main(int argc, char **argv) {
    int width, height, depth;
    Volume vol, rot;
    Image im, smooth;
    int frame;
    double rotx, roty, rotz;
    int rank, numtasks;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <volume.vox>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //prepare receive dimensions
    int *dims = malloc(3 * sizeof(int));
    assert(dims != NULL);

    //read volume
    if (rank == MASTER) {
        vol = readVolume(argv[1]);
        dims[WIDTH] = vol->width;
        dims[HEIGHT] = vol->height;
        dims[DEPTH] = vol->depth;
    }

    MPI_Bcast(dims, 3, MPI_INT, MASTER, MPI_COMM_WORLD);

    width = dims[WIDTH];
    height = dims[HEIGHT];
    depth = dims[DEPTH];

    rot = makeVolume(width, height, depth);

    im = makeImage(width, height);
    smooth = makeImage(width, height);

    //prepare to receive the original volume
    if (rank != MASTER) {
        vol = makeVolume(width, height, depth);
    }

    //distribute vol
    MPI_Bcast(vol->voldata[0][0], vol->width * vol->height * vol->depth, MPI_BYTE, MASTER, MPI_COMM_WORLD);

    //compute numframes, startframe per task
    int *numframes = malloc(sizeof(int) * numtasks);
    int *startframe = malloc(sizeof(int) * numtasks);
    assert(numframes != NULL && startframe != NULL);

    int size = NFRAMES;
    int rem = size % numtasks;
    int sum = 0;
    for (int i = 0; i < numtasks; i++) {
        numframes[i] = size / numtasks;
        if (rem > 0) {
            numframes[i]++;
            rem--;
        }
        startframe[i] = sum;
        sum += numframes[i];
    }

    Image result;
    int resultframe;
    if (rank == MASTER) {
        result = makeImage(width, height);
    }

    double time;
    if (rank == MASTER) {
        time = timer();
    }

    // Compute frames.
    for (frame = startframe[rank]; frame < startframe[rank] + numframes[rank]; frame++) {
        rotx = roty = rotz = 0;
        switch (3 * frame / NFRAMES) {
            case 0:
                rotx = 6 * 3.1415927 * frame / NFRAMES;
                break;
            case 1:
                roty = 6 * 3.1415927 * frame / NFRAMES;
                break;
            case 2:
                rotz = 6 * 3.1415927 * frame / NFRAMES;
                break;
        }
        computeFrame(frame, rotx, roty, rotz, vol, rot, im, smooth, rank);

        //gather
        if (rank == MASTER) {
            char fnm[256];
            sprintf(fnm, "frame%04d.pgm", frame);
            writeImage(smooth, fnm);

            //recv loop
            MPI_Status status;
            for (int source = 1; source < numtasks; ++source) {
                resultframe = startframe[source] + frame;
                //if the resultframe is computed, receive it
                if (resultframe < (startframe[source] + numframes[source])) {
                    MPI_Recv(result->imdata[0], width * height, MPI_INT, source, RESULT, MPI_COMM_WORLD, &status);
                    sprintf(fnm, "frame%04d.pgm", resultframe);
                    writeImage(result, fnm);
                }
            }

        } else {
            //send
            MPI_Send(smooth->imdata[0], width * height, MPI_INT, MASTER, RESULT, MPI_COMM_WORLD);
        }
        //end gather
    }

    if (rank == MASTER) {
        time = timer() - time;
        printf("this took %lf seconds\n", time);
        fflush(stdout);
    }

    freeVolume(rot);
    freeVolume(vol);
    freeImage(im);
    freeImage(smooth);
    if (rank == MASTER) {
        freeImage(result);
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}
