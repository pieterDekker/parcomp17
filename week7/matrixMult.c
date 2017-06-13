#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <CL/cl.h>

static long holdrand = 23;
long random() {
    return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

static double timer(void) {
    struct timeval tm;
    gettimeofday(&tm, NULL);
    return tm.tv_sec + tm.tv_sec / 1000000.0;
}

// OpenCL kernel to perform an element-wise add of two arrays
const char* programSource =
    "kernel void MatrixMultKernel(global float * Pd,                \n"
    "const global float * Md,                                       \n"
    "const global float * Nd) {                                     \n"
    "}                                                              \n";



float *M;
float *N;
float *P_opencl;
float *P_seq;
int width;

// Fill f width size many random float values
void fill(float *f, int size) {
    for (int i = 0; i < size; i += 1) {
        f[i] = ((float) random());
    }
}

// Compares every pair lhs[i] and rhs[i] for i < width
void compare(float *lhs, float *rhs, int width) {
    int errors = 0;
    for (int i = 0; i < width; i += 1) {
        // because of floating point operations
        if (abs(lhs[i] - rhs[i]) > 0.00001) {
            errors += 1;
        }
    }
    if (errors > 0) {
        fprintf(stdout, "%d errors occurred.\n", errors);
    } else {
        fprintf(stdout, "No errors occurred.\n");
    }
    fflush(stdout);
}

// Sequential matrix multiplication
void MatrixMulSeq() {
    int Col, Row, k;
    for (Col = 0; Col < width; ++Col) {
        for (Row = 0; Row < width; ++Row) {
            float sum = 0;
            for (k = 0; k < width; k += 1) {
                sum += M[Row * width + k] * N[k * width + Col];
            }
            P_seq[Row * width + Col] = sum;
        }
    }
}

// ######################################################
// Start OpenCL code
int err;

cl_int status = 0;
cl_uint numPlatforms = 0;
cl_platform_id *platforms = NULL;
cl_uint numDevices = 0;
cl_device_id *devices = NULL;
cl_context context = NULL;
cl_command_queue cmdQueue = NULL;
cl_program program = NULL;
cl_kernel kernel = NULL;

// initialize OpenCL
void initOpenCL() {
    // select OpenCL platform with clGetPlatformIDs(...)
    fprintf (stdout, "platform selected\n");

    // select OpenCL device with clGetDeviceIDs(...). Ensure that only GPUs will be selected
    fprintf (stdout, "device selected\n");

    // create OpenCL context with clCreateContext(...)
    fprintf (stdout, "context created\n");

    // create OpenCL command queue with clCreateCommandQueue(...)
    fprintf (stdout, "commandQueue created\n");
}

void executeCode(/* Insert your parameters */) {

    // Create a program using clCreateProgramWithSource()
    fprintf (stdout, "program created\n");

    // build OpenCL program with clBuildProgram(...)
    fprintf (stdout, "program build successfully\n");

    // Use clCreateKernel() to create a kernel from the vector addition function
    // (named "vecadd")
    fprintf (stdout, "kernel created\n");

    // Associate the buffers with the kernel using clSetKernelArg()
    fprintf (stdout, "set kernel arguments\n");
    
    // Enqueue OpenCL kernel with clEnqueuNDRangeKernel(...)
    fprintf(stdout, "enqueued kernel\n");
}

void MatrixMulOpenCL(float *M, float *N, float *P, int width) {
    int size = width * width * sizeof(float);

    // create OpenCL buffer for M with clCreateBuffer(...)
    // and copy to device with clEnqueueWriteBuffer(...)
    fprintf(stdout, "buffer for M created and copied\n");

    // create OpenCL buffer for N with clCreateBuffer(...)
    // and copy to device with clEnqueueWriteBuffer(...)
    fprintf(stdout, "buffer for N created and copied\n");

    // create OpenCL buffer for P with clCreateBuffer(...)
    fprintf(stdout, "buffer for P created\n");

    // set OpenCL kernel arguments with clSetKernelArg(...)
    fprintf(stdout, "kernel arguments set\n");

    // enqueue OpenCL kernel with clEnqueuNDRangeKernel(...)
    fprintf(stdout, "enqueued kernel\n");

    // Wait for the command cmdQueue to get serviced before reading back results
    clFinish(cmdQueue);

    // read data from OpenCL buffer for P into P with clEnqueueReadBuffer(...)
    fprintf(stdout, "read buffer P\n");
}

// end OpenCL code
// ######################################################

void init() {
    if (argc == 2) {
        width = atoi(argv[1]);
    } else {
        printf("Usage: matrixMult <elements>\nWhere elements is the length and width of the matrix.\n");
        return -1;
    }

    M = (float *) malloc(width * width * sizeof(float));
    N = (float *) malloc(width * width * sizeof(float));
    P_opencl = (float *) malloc(width * width * sizeof(float));
    P_seq = (float *) malloc(width * width * sizeof(float));

    fill(M, width * width);
    fill(N, width * width);
    initOpenCL();
    createKernel();
};

int main(void) {
    struct timeval start, end;
    init();

    gettimeofday(&start, NULL);
    MatrixMulOpenCL(M, N, P_opencl, width);
    gettimeofday(&end, NULL);
    printf("time elapsed OpenCL: %fmsecs\n",
           (float) (1000.0 * (end.tv_sec - start.tv_sec)
                    + 0.001 * (end.tv_usec - start.tv_usec)));

    gettimeofday(&start, NULL);
    MatrixMulSeq();
    gettimeofday(&end, NULL);
    printf("time elapsed Seq: %fmsecs\n",
           (float) (1000.0 * (end.tv_sec - start.tv_sec)
                    + 0.001 * (end.tv_usec - start.tv_usec)));

    compare(P_seq, P_opencl, width * width);

    // output the username, time and requested values from the Nestor questions

    // Don't forget to cleanup resources

    return 0;
}
