#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <time.h>

#define num_steps 1000000000

int main() {
    long i;
    double x, sum = 0;
    double step = 1.0 / num_steps;
    struct timespec start, stop;
    clock_gettime(CLOCK_REALTIME, &start);

    for (i = 0; i < num_steps; i++) {
        x = (i + 0.5) * step;
        sum += 4.0 / (1 + x * x);
    }
    double pi = step * sum;

    clock_gettime(CLOCK_REALTIME, &stop);
    double elapsed = (stop.tv_sec - start.tv_sec) * 1e6 + (stop.tv_nsec - start.tv_nsec) / 1e3;
    printf("PI = %.10f\n", pi);
    printf("Elapsed time: %f\n", elapsed);

    return 0;
}