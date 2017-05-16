// File: prime.c
//
// A simple program for computing the amount of prime numbers within the
// interval [a,b].

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include <sys/time.h>

#define FALSE 0
#define TRUE  1
//tags
#define PARAM 99
#define SOLUTION 100
#define TIME 101

//ranks
#define MASTER 0

//other
#define SINGLE 1

static double timer(void) {
    struct timeval tm;
    gettimeofday(&tm, NULL);
    return tm.tv_sec + tm.tv_usec / 1000000.0;
}

int *safeMalloc(int size) {
    int *p = malloc(size * sizeof(int));
    if (p == NULL) {
        printf("ERR: malloc failed\n");
        exit(-1);
    }
    return p;
}

static int isPrime(unsigned int p) {
    int i, root;
    if (p == 1)
        return FALSE;
    if (p == 2)
        return TRUE;
    if (p % 2 == 0)
        return FALSE;

    root = (int) (1 + sqrt(p));
    for (i = 3; (i < root) && (p % i != 0); i += 2);
    return i < root ? FALSE : TRUE;
}

void readInput(int *a, int *b) {
    fprintf(stdout, "Enter two integer number a, b such that 1<=a<=b: ");
    fflush(stdout);
    scanf("%u %u", a, b);
}

void receiveSolutions(int *sol, const int nthreads) {
//    int busy = nthreads - 1;
    int buf;
    MPI_Status status;
    for (int i = 1; i < nthreads; ++i) {
        MPI_Recv(&buf, SINGLE, MPI_INT, MPI_ANY_SOURCE, SOLUTION, MPI_COMM_WORLD, &status);
        *sol += buf;
    }
}

void sendSolution(int sol) {
    MPI_Send(&sol, SINGLE, MPI_INT, MASTER, SOLUTION, MPI_COMM_WORLD);
}

void receiveTime(double *total_time, const int nthreads) {
    double buf;
    MPI_Status status;
    for (int i = 1; i < nthreads; ++i) {
        MPI_Recv(&buf, SINGLE, MPI_DOUBLE, MPI_ANY_SOURCE, TIME, MPI_COMM_WORLD, &status);
        *total_time += buf;
    }
}

void sendTime(double time) {
    MPI_Send(&time, SINGLE, MPI_DOUBLE, MASTER, TIME, MPI_COMM_WORLD);
}

void sendParams(int *bs, const int nthreads) {
    for (int i = 1; i < nthreads; ++i) {
        //bs + (i - 1) is the a of thread i, bs + i is the b of thread i
        MPI_Send(bs + (i - 1), 2, MPI_INT, i, PARAM, MPI_COMM_WORLD);
    }
}

void receiveParam(int *a, int *b) {
    int *buf = safeMalloc(2);
    MPI_Status status;
    MPI_Recv(buf, 2, MPI_INT, MASTER, PARAM, MPI_COMM_WORLD, &status);
    *a = buf[0];
    *b = buf[1];
    free(buf);
}

int calcUPB(int range, int idx, int nthreads, int prev) {
    return (range / (idx + nthreads - 1)) + prev;
}

int main(int argc, char **argv) {
    unsigned int i, a, b, cnt = 0;
    int rank, nthreads;
    int local_a, local_b;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nthreads);

    if (rank == MASTER) {
        // Prints current date and user. DO NOT MODIFY
        system("date");
        system("echo $USER");
        readInput(&a, &b);
        if (a <= 2) {
            cnt = 1;
            a = 3;
        }

        if (a % 2 == 0) {
            a++;
        }

        int range = b - a;
        int *bs = safeMalloc(nthreads);
        bs[MASTER] = calcUPB(range, 0, nthreads, a);

        for (int i = 1; i < nthreads; ++i) {
            bs[i] = calcUPB(range, i, nthreads, bs[i-1]);
        }

        bs[nthreads-1] = b;

        sendParams(bs, nthreads);

        local_a = a;
        local_b = bs[MASTER];

        free(bs);
    } else {
        receiveParam(&local_a, &local_b);
    }

    //sync a, b

    //time at start of computation
    double t1 = timer();

    //do the work
    for (int i = local_a; i < local_b; ++i) {
        cnt += isPrime(i);
    }

    //time after computation
    double t2 = timer();
    double delta_t = t2 - t1;

    if (rank == MASTER) {
        receiveSolutions(&cnt, nthreads);
        receiveTime(&delta_t, nthreads);
        double realTime = timer() - t1;
        fprintf(stdout, "\n#primes=%u\n", cnt);
        fprintf(stdout, "found in %.2lf seconds (real time), worked for %.2lf seconds\n\n\n", realTime, delta_t);
        fflush(stdout);
    } else {
        sendSolution(cnt);
        sendTime(delta_t);
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}
