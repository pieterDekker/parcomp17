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

/* Your probably going to need some more cl_specific type variables*/

void initOpenCL() {
    cl_int err = clGetPlatformIDs(1, &platforms[0], &numPlatforms);
    fprintf(stdout, "platform selected\n");

    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &devices[0], &numDevices);
    fprintf(stdout, "device selected\n");

    context = clCreateContext(NULL, 1, &devices[0], NULL, NULL, &err);
    fprintf(stdout, "context created\n");

    cmdQueue = clCreateCommandQueue(context, devices, 0, &err);
    fprintf(stdout, "commandQueue created\n");
}

void createMemBuffers(int datasize) {

    cl_mem A_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &err);
    cl_mem B_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &err);
    cl_mem C_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, &err);
    fprintf(stdout, "memory buffers created\n");

    cl_int err = clEnqueueWriteBuffer(cmdQueue, A_mem, CL_TRUE, 0, datasize, A, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(cmdQueue, B_mem, CL_TRUE, 0, datasize, B, 0, NULL, NULL);
    fprintf(stdout, "queue created\n");
}

void executeCode(/* Insert your parameters */) {

    // Create a program using clCreateProgramWithSource()
    fprintf(stdout, "program created\n");

    // build OpenCL program with clBuildProgram(...)
    fprintf(stdout, "program build successfully\n");

    // Use clCreateKernel() to create a kernel from the vector addition function
    // (named "vecadd")
    fprintf(stdout, "kernel created\n");

    // Associate the buffers with the kernel using clSetKernelArg()
    fprintf(stdout, "set kernel arguments\n");

    // Enqueue OpenCL kernel with clEnqueuNDRangeKernel(...)
    fprintf(stdout, "enqueued kernel\n");
}

void cleanup(/* Insert your parameters */) {
    // Free OpenCL resources
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(cmdQueue);
    clReleaseContext(context);
}

int main(int argc, char **argv) {
    // This code executes on the OpenCL host

    // Elements in each array
    int elements;

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
    createMemBuffers(/* Insert your parameters */);
    executeCode(/* Insert your parameters */);

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
    free(A);
    free(B);
    free(C);

    return 0;
}