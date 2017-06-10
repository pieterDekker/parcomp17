// This program implements a vector addition using OpenCL
// Rewritten by Guus Klinkenberg

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

// OpenCL includes
#include <CL/cl.h>

// DO NOT CHANGE OTHERWISE THE RESULTS WILL DIFFER
// Random number generator with set seed.
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
const char *programSource =
        "__kernel                                            \n"
                "void vecadd(__global int *A,                        \n"
                "            __global int *B,                        \n"
                "            __global int *C)                        \n"
                "{                                                   \n"
                "   int localId = get_local_id(0);                   \n"
                "   C[localId] = A[localId] + B[localId]             \n"
                "}                                                   \n";

const int sourceSize = sizeof(const char) * 54 * 8;

// Use this to check the output of each API call
cl_int status = 0;
cl_uint numPlatforms = 0;
cl_platform_id *platforms = NULL;
cl_uint numDevices = 0;
cl_device_id *devices = NULL;
cl_context context = NULL;
cl_command_queue cmdQueue = NULL;
cl_program program = NULL;
cl_kernel kernel = NULL;

cl_int err;
cl_mem A_mem;
cl_mem B_mem;
cl_mem C_mem;

void initOpenCL() {
    err = clGetPlatformIDs(1, &platforms[0], &numPlatforms);
    fprintf(stdout, "platform selected\n");

    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &devices[0], &numDevices);
    fprintf(stdout, "device selected\n");

    context = clCreateContext(NULL, 1, &devices[0], NULL, NULL, &err);
    fprintf(stdout, "context created\n");

    cmdQueue = clCreateCommandQueue(context, devices, 0, &err);
    fprintf(stdout, "commandQueue created\n");
}

void createMemBuffers(int datasize, int *A, int *B) {

    A_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &err);
    B_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &err);
    C_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, &err);
    fprintf(stdout, "memory buffers created\n");

    err = clEnqueueWriteBuffer(cmdQueue, A_mem, CL_TRUE, 0, datasize, A, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(cmdQueue, B_mem, CL_TRUE, 0, datasize, B, 0, NULL, NULL);
    fprintf(stdout, "queue created\n");
}

void executeCode(int datasize, int *C) {

    program = clCreateProgramWithSource(context, 1, programSource, sourceSize, &err);
    fprintf(stdout, "program created\n");

    err = clBuildProgram(program, 1, &devices[0], NULL, NULL, NULL);
    fprintf(stdout, "program built successfully\n");

    kernel = clCreateKernel(program, "vecadd", &err);
    fprintf(stdout, "kernel created\n");

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &A_mem);
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &B_mem);
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &C_mem);
    fprintf(stdout, "set kernel arguments\n");

    err = clEnqueueNDRangeKernel(cmdQueue, C_mem, CL_TRUE, 0, datasize, C, 0, NULL, NULL);
    fprintf(stdout, "enqueued kernel\n");
}

void cleanup(int *A, int *B, int *C) {
    // Free OpenCL resources
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(cmdQueue);
    clReleaseContext(context);

    free(A);
    free(B);
    free(C);
}

int main(int argc, char **argv) {
    // This code executes on the OpenCL host

    // Elements in each array
    int elements, datasize;

    if (argc == 2) {
        elements = atoi(argv[1]);
        datasize = sizeof(int) * elements;
    } else {
        printf("Usage: vectorAdd <elements>\nWhere elements is the length of the vectors. Should not be larger then 2^31\n");
        return -1;
    }

    // Host data
    int *A = NULL;  // Input array
    int *B = NULL;  // Input array
    int *C = NULL;  // Output array

    // Allocate space for input/output data
    A = (int *) malloc(datasize);
    B = (int *) malloc(datasize);
    C = (int *) malloc(datasize);

    // Initialize the input data
    // DO NOT CHANGE THIS AS THIS WILL INFLUENCE THE OUTCOME AND THUS RESULTS 
    // IN INCORRECT ANSWERS
    for (int i = 0; i < elements; i++) {
        A[i] = random() % 147483647;
        B[i] = random() % 147483647;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);

    initOpenCL();
    createMemBuffers(datasize, A, B);
    executeCode(datasize, C);

    gettimeofday(&end, NULL);
    printf("time elapsed OpenCL: %fmsecs\n",
           (float) (1000.0 * (end.tv_sec - start.tv_sec)
                    + 0.001 * (end.tv_usec - start.tv_usec)));

    /* Write and time a sequential vector addition */

    // Verify the output
    bool result = true;
    for (int i = 0; i < elements; i++) {
        if (C[i] != A[i] + B[i]) {
            result = false;
            printf("Output is incorrect on:\n");
            printf("A[i]: %d B[i]:%d C[i]:%d\n", A[i], B[i], C[i]);
            break;
        }
    }

    if (result) {
        printf("Output is correct\n");
        printf("%d\n", C[529]);
    }

    // output the username, time and requested values from the Nestor questions

    system("date");
    system("echo $USER");

    cleanup();

    return 0;
}