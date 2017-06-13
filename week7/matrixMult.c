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
    "__kernel void matrixMultKernel(__global float *Pd,            \n"
    "const __global float *Md,                                     \n"
    "const __global float *Nd) {                                   \n"
    "  int globalId = get_global_id(0);                            \n"
    "  int width = sqrt(get_global_size(0)/sizeof(float));         \n"
    "  int col = globalId % width;                                 \n"
    "  int row = globalId / width;                                 \n"       
    "  float result = 0;                                           \n"
    "  for(int i = 0; i < width; ++i){                             \n"
    "    float Mval = M[width * i + col];                          \n"
    "    float Nval = N[width * row + i];                          \n"
    "    result += Mval * Nval;                                    \n"
    "  }                                                           \n" 
    "                                                              \n"
    "  Pd[globalId] = result;                                      \n"
    "}                                                             \n";
size_t programSourceSize;

float *M;
float *N;
float *P_opencl;
float *P_seq;

cl_mem M_mem;
cl_mem N_mem;
cl_mem P_mem;

int width;
size_t datasize;

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
cl_platform_id platforms = NULL;
cl_uint numDevices = 0;
cl_device_id devices = NULL;
cl_context context = NULL;
cl_command_queue cmdQueue = NULL;
cl_program program = NULL;
cl_kernel kernel = NULL;

// initialize OpenCL
void initOpenCL() {
  
    err = clGetPlatformIDs(1, &platforms, &numPlatforms);
    fprintf(stdout, "platform selected\n");

    err = clGetDeviceIDs(platforms, CL_DEVICE_TYPE_GPU, 1, &devices, 
                          &numDevices);
    fprintf(stdout, "device selected\n");

    context = clCreateContext(NULL, 1, &devices, NULL, NULL, &err);
    fprintf(stdout, "context created\n");

    cmdQueue = clCreateCommandQueue(context, devices, 0, &err);
    fprintf(stdout, "commandQueue created\n");
}

void createKernel() {
    program = clCreateProgramWithSource(context, 1, &programSource,  
                                          &programSourceSize, &err);
    fprintf(stdout, "program created\n");

    err = clBuildProgram(program, 1, &devices, NULL, NULL, NULL);
    fprintf(stdout, "program built successfully\n");

    kernel = clCreateKernel(program, "matrixMultKernel", &err);
    fprintf(stdout, "kernel created\n"); 
}

void MatrixMulOpenCL(float *M, float *N, float *P, int width) {
    int size = width * width * sizeof(float);

    M_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, 
                            &err);
    err = clEnqueueWriteBuffer(cmdQueue, M_mem, CL_TRUE, 0, datasize, M, 
                                0, NULL, NULL);
    fprintf(stdout, "buffer for M created and copied\n");

    N_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, 
                            &err);
    err = clEnqueueWriteBuffer(cmdQueue, N_mem, CL_TRUE, 0, datasize, N, 
                                0, NULL, NULL);
    fprintf(stdout, "buffer for N created and copied\n");

    P_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, 
                            &err);
    fprintf(stdout, "buffer for P created\n");

    // set OpenCL kernel arguments with clSetKernelArg(...)
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &M_mem);
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &N_mem);
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &P_mem);
    fprintf(stdout, "kernel arguments set\n");

    // enqueue OpenCL kernel with clEnqueuNDRangeKernel(...)
    err = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, &datasize, 
                                  NULL, 0, NULL, NULL);
    fprintf(stdout, "enqueued kernel\n");

    // Wait for the command cmdQueue to get serviced before reading back results
    clFinish(cmdQueue);

    // read data from OpenCL buffer for P into P with clEnqueueReadBuffer(...)
    clEnqueueReadBuffer(cmdQueue, P_mem, CL_TRUE, 0, datasize, P_opencl, 0,
                        NULL, NULL);
    fprintf(stdout, "read buffer P\n");
}

// end OpenCL code
// ######################################################

void init(int argc, char** argv) {
    if (argc == 2) {
        width = atoi(argv[1]);
    } else {
        printf("Usage: matrixMult <elements>\nWhere elements is the length and width of the matrix.\n");
        exit(-1);
    }
    
    programSourceSize = strlen(programSource) * sizeof(char);
    datasize = width * width * sizeof(float);

    M = (float *) malloc(width * width * sizeof(float));
    N = (float *) malloc(width * width * sizeof(float));
    P_opencl = (float *) malloc(width * width * sizeof(float));
    P_seq = (float *) malloc(width * width * sizeof(float));

    fill(M, width * width);
    fill(N, width * width);
    initOpenCL();
    createKernel();
};

int main(int argc, char** argv) {
    struct timeval start, end;
    init(argc, argv);

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
