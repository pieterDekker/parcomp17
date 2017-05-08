/* File: lup.c
 * Author: Arnold Meijster (a.meijster@rug.nl)
 * Version: 1.0 (01 March 2008)
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>

#define real float

#define ABS(a) ((a)<0 ? (-(a)) : (a))
#define TRUE 1

/* forward references */
static void *safeMalloc(size_t n);

static real **allocMatrix(size_t height, size_t width);

static void freeMatrix(real **mat);

static real *allocVector(size_t length);

static void freeVector(real *vec);

static void showMatrix(size_t n, real **A);

static void showVector(size_t n, real *vec);

static void decomposeLUP(size_t n, real **A, size_t *P);

static void LUPsolve(size_t n, real **LU, size_t *P, real *x, real *b);

static void solve(size_t n, real **A, real *x, real *b);

/********************/

void *safeMalloc(size_t n) {
    void *ptr;
    ptr = malloc(n);
    if (ptr == NULL) {
        fprintf(stderr, "Error: malloc(%lu) failed\n", n);
        exit(-1);
    }
    return ptr;
}

real **allocMatrix(size_t height, size_t width) {
    real **matrix;
    size_t row;

    matrix = safeMalloc(height * sizeof(real *));
    matrix[0] = safeMalloc(width * height * sizeof(real));

    for (row = 1; row < height; ++row)
        matrix[row] = matrix[row - 1] + width;
    return matrix;
}

void freeMatrix(real **mat) {
    free(mat[0]);
    free(mat);
}

real *allocVector(size_t length) {
    return safeMalloc(length * sizeof(real));
}

void freeVector(real *vec) {
    free(vec);
}

void showMatrix(size_t n, real **A) {
    size_t i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            printf("%.2f ", A[i][j]);
        }
        printf("\n");
    }
}

void showVector(size_t n, real *vec) {
    size_t i;
    for (i = 0; i < n; ++i) {
        printf("%f ", vec[i]);
    }
    printf("\n");
}

void decomposeLUP(size_t n, real **A, size_t *P) {
    /* computes L, U, P such that A=L*U*P */

    int h, i, j, k, row;
    real pivot, absval, tmp;

#pragma omp parallel for
    for (i = 0; i < n; ++i) {
        P[i] = i;
    }

    #pragma omp parallel
    {
        for (k = 0; k < n - 1; ++k) {
            row = -1;
            pivot = 0;

            for (i = k; i < n; ++i) {
                absval = (A[i][k] >= 0 ? A[i][k] : -A[i][k]);
                if (absval > pivot) {
                    pivot = absval;
                    row = i;
                }
            }
            printf("%d\n", row);
            if (row == -1) {
                printf("Singular matrix\n");
                exit(-1);
            }
    #pragma omp barrier

            /* swap(P[k],P[row]) */
            h = P[k];
            P[k] = P[row];
            P[row] = h;
            /* swap rows */
    #pragma omp parallel for
            for (i = 0; i < n; ++i) {
                tmp = A[k][i];
                A[k][i] = A[row][i];
                A[row][i] = tmp;
            }

    #pragma omp parallel for
            for (i = k + 1; i < n; ++i) {
                A[i][k] /= A[k][k];
                for (j = k + 1; j < n; ++j) {
                    A[i][j] -= A[i][k] * A[k][j];
                }
            }
        }
    }
}

void LUPsolve(size_t n, real **LU, size_t *P, real *x, real *b) {
    real *y;
    size_t i, j;

    /* Solve Ly=Pb using forward substitution */
    y = x;  /* warning, y is an alias for x! It is safe, though. */
#pragma omp parallel for
    for (i = 0; i < n; ++i) {
        y[i] = b[P[i]];

        for (j = 0; j < i; ++j) {
            y[i] -= LU[i][j] * y[j];
        }
    }

    /* Solve Ux=y using backward substitution */
#pragma omp parallel
    {
        for (i = n-1; i > 0; --i) {
            x[i] = y[i];
#pragma omp for
            for (j = i + 1; j < n; ++j) {
                x[i] -= LU[i][j] * x[j];
            }
            x[i] /= LU[i][i];
        }
    }
}

void solve(size_t n, real **A, real *x, real *b) {
    size_t *P;

    /* Construct LUP decomposition */
    P = safeMalloc(n * sizeof(size_t));
    decomposeLUP(n, A, P);
    /* Solve by forward and backward substitution */
    LUPsolve(n, A, P, x, b);

    free(P);
}

int main(int argc, char **argv) {
    real **A, *x, *b;

    omp_set_nested(TRUE);
    int dim;
    do {
        printf("enter the size:  ");
        scanf("%d", &dim);
    } while (dim < 1 && printf("size must be greater than or equal to 1"));

    A = allocMatrix(dim, dim);
    x = allocVector(dim);
    b = allocVector(dim);

#pragma omp for
    for (int i = 0; i < dim; i++) {
        b[i] = 1;
        for (int j = 0; j < dim; j++) {
            if (i == j) {
                A[i][j] = -2;
            } else if (ABS(i - j) == 1) {
                A[i][j] = 1;
            } else {
                A[i][j] = 0;
            }
        }
    }

 //   showMatrix(dim, A);
    //   printf("\n");

    solve(dim, A, x, b);

    showVector(dim, x);

    freeMatrix(A);
    freeVector(x);
    freeVector(b);

    printf("\n");
    system("echo $USER");
    system("date");
    return EXIT_SUCCESS;
}
