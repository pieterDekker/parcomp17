#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "waveio.h"
#include <mpi/mpi.h>
#include <time.h>

typedef float real;
typedef unsigned char byte;

#define ABS(a) ((a)<0 ? (-(a)) : (a))

static real ***initialize(int N, int NFRAMES, real dx, real *dt, real v, int nsrc, int **src, int **ampl);

static void boundary(real ***u, int N, int iter, int nsrc, int *src, int *ampl, real timespacing);

static void
solveWave(real ***u, int N, int NFRAMES, int nsrc, int *src, int *ampl, real gridspacing, real timespacing, real speed);

static real ***initialize(int N, int NFRAMES, real dx, real *dt, real v, int nsrc, int **src, int **ampl) {
    real ***u;
    int i, j, k;
    real lambda;

    // Determine lambda, adjust timespacing if needed.
    lambda = v * *dt / dx;
    if (lambda > 0.5 * sqrt(2)) {
        fprintf(stdout, "Error: Convergence criterion is violated.\n");
        fprintf(stdout, "speed * timespacing / gridspacing = ");
        fprintf(stdout, "%lf * %lf / %lf = %f > 0.5 * sqrt(2)\n", v, *dt, dx, lambda);
        *dt = (real) (dx * sqrt(2) / (2 * v));
        fprintf(stdout, "Timestep changed into: timespacing=%lf\n", *dt);
        fflush(stdout);
    }

    // Draw n random locations of wave sources.
    srand(time(NULL));  // Initialize random generator with time.
    *src = malloc(2 * nsrc * sizeof(int));
    *ampl = malloc(nsrc * sizeof(int));
    for (i = 0; i < nsrc; i++) {
        (*src)[2 * i] = random() % N;
        (*src)[2 * i + 1] = random() % N;
        (*ampl)[i] = 1; // Change this to modify the amplitude of the waves.
    }

    // Allocate memory for u.
    u = malloc(NFRAMES * sizeof(real **));
    for (k = 0; k < NFRAMES; k++) {
        u[k] = malloc(N * sizeof(real *));
        u[k][0] = malloc(N * N * sizeof(real));
        for (i = 1; i < N; i++)
            u[k][i] = &(u[k][0][i * N]);
    }

    // Initialize first two time steps.
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            u[0][i][j] = 0;

    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            u[1][i][j] = 0;

    return u;
}

static void boundary(real ***u, int N, int iter, int nsrc, int *src, int *ampl, real timespacing) {
    // Initialise new frame by inserting boundary values and wave sources.
    int i, j;
    real t = iter * timespacing;

    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            u[iter][i][j] = 0;

    for (i = 0; i < nsrc; i++)
        u[iter][src[2 * i]][src[2 * i + 1]] = (real) (ampl[i] * sin(t));
}

static void solveWave(real ***u, int N, int NFRAMES, int nsrc, int *src, int *ampl, real gridspacing, real timespacing,
                      real speed, int start, int end) {
    // Computes all NFRAMES consecutive frames.
    real sqlambda;
    int i, j, iter;
    sqlambda = speed * timespacing / gridspacing;
    sqlambda = sqlambda * sqlambda;

    for (iter = 2; iter < NFRAMES; iter++) {
        boundary(u, N, iter, nsrc, src, ampl, timespacing);
        for (i = start; i < end; i++)
            for (j = 1; j < N - 1; j++)
                u[iter][i][j] += sqlambda * (u[iter - 1][i + 1][j] + u[iter - 1][i - 1][j] + u[iter - 1][i][j + 1] +
                                             u[iter - 1][i][j - 1]) + (2 - 4 * sqlambda) * u[iter - 1][i][j] -
                                 u[iter - 2][i][j];
        MPI_Allgather(const void *sendbuf, int sendcount, MPI_INT, void *recvbuf, int recvcount, MPI_INT, MPI_COMM_WORLD);
    }
}

int main(int argc, char **argv) {
    real ***u;            // Holds the animation data.
    int N, NFRAMES;        // Dimensions and amount of frames.
    int *src = NULL;    // Source coordinates.
    int *ampl = NULL;    // Amplitudes of sources.
    int n;                // Number of sources.
    int bw;            // Colour(=0) or greyscale(=1) frames.
    real dt;            // Time spacing (delta time).
    real dx;            // Grid spacing (distance between grid cells.
    real v;            // Velocity of waves.

    int rank;           // Rank of the current process
    int size;           // Size of the World group

    struct timeval start, end;
    double fstart, fend;

    // Default values.
    n = 10;            // Number of sources.
    bw = 0;            // Black and white disabled.
    NFRAMES = 100;    // Number of images/frames.
    N = 300;        // Grid cells (width,height).
    dt = 0.1;        // Timespacing.
    dx = 0.1;        // Gridspacing.
    v = 0.5;        // Wave velocity.

    // Parse command line options.
    parseIntOpt(argc, argv, "-f", &NFRAMES);
    parseIntOpt(argc, argv, "-src", &n);
    parseIntOpt(argc, argv, "-bw", &bw);
    parseIntOpt(argc, argv, "-n", &N);
    parseRealOpt(argc, argv, "-t", &dt);
    parseRealOpt(argc, argv, "-g", &dx);
    parseRealOpt(argc, argv, "-s", &v);

    // Init.
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    u = initialize(N, NFRAMES, dx, &dt, v, n, &src, &ampl);

    // Start timer.
    fprintf(stdout, "Computing waves\n");
    gettimeofday(&start, NULL);

    // Render all frames.
    solveWave(u, N, NFRAMES, n, src, ampl, dx, dt, v);

    // Stop timer; compute flop/s.
    gettimeofday(&end, NULL);
    fstart = (start.tv_sec * 1000000.0 + start.tv_usec) / 1000000.0;
    fend = (end.tv_sec * 1000000.0 + end.tv_usec) / 1000000.0;
    fprintf(stdout, "wallclock: %lf seconds (ca. %5.2lf Gflop/s)\n", fend - fstart,
            (9.0 * N * N * NFRAMES / (fend - fstart)) / (1024 * 1024 * 1024));

    // Save separate images.
    fprintf(stdout, "Saving frames\n");
    stretchContrast(u, N, NFRAMES);
    saveFrames(u, N, NFRAMES, bw);
    MPI_Finalize();

    fprintf(stdout, "Done\n");
    return EXIT_SUCCESS;
}
