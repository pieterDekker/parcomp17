#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>

#define f(x) (4.0/(1.0+(x)*(x)))

// 3.1415926535897932384
static double timer(void) {
    struct timeval tm;
    gettimeofday(&tm, NULL);
    return tm.tv_sec + tm.tv_usec / 1000000.0;
}

int main(int argc, char **argv) {
    int N;
    double dx, sum, err;
    double clock;
    long approxArray[] = {1000,1000000,10000000000};
    int threadsArray[] = {1,4,8,12};

    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 4; ++j) {
            printf("Number of approximations: %ld\nNumber of threads: %d\n",approxArray[i],threadsArray[j]);
            omp_set_num_threads(threadsArray[j]);
            N = approxArray[i];
            /* start timer */
            clock = timer();
            /* Step (3): numerical integration */
            dx = 1.0 / N;
            sum = 0.0;
            #pragma omp parallel for reduction(+:sum)
            for (i = 0; i < N; i++) {
                sum = sum + f(dx * (i + 0.5));
            }
            sum *= dx;

            /* stop timer */
            clock = timer() - clock;
            printf("wallclock: %lf seconds\n", clock);

            /* Step (4): print the results */
            err = sum - 4 * atan(1);
            printf("%d: pi~%20.18lf   er~=%20.18lf\n", N, sum, err);
        }
    }

    printf("\n");
    system("echo $USER");
    system("date");
    return EXIT_SUCCESS;
}
