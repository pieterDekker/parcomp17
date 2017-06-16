#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <assert.h>
#include <CL/cl.h>

static long holdrand = 23;
long random() {
    return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

const char *getErrorString(cl_int error) {
	switch(error){
			// run-time and JIT compiler errors
			case 0: return "CL_SUCCESS";
			case -1: return "CL_DEVICE_NOT_FOUND";
			case -2: return "CL_DEVICE_NOT_AVAILABLE";
			case -3: return "CL_COMPILER_NOT_AVAILABLE";
			case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
			case -5: return "CL_OUT_OF_RESOURCES";
			case -6: return "CL_OUT_OF_HOST_MEMORY";
			case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
			case -8: return "CL_MEM_COPY_OVERLAP";
			case -9: return "CL_IMAGE_FORMAT_MISMATCH";
			case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
			case -11: return "CL_BUILD_PROGRAM_FAILURE";
			case -12: return "CL_MAP_FAILURE";
			case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
			case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
			case -15: return "CL_COMPILE_PROGRAM_FAILURE";
			case -16: return "CL_LINKER_NOT_AVAILABLE";
			case -17: return "CL_LINK_PROGRAM_FAILURE";
			case -18: return "CL_DEVICE_PARTITION_FAILED";
			case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

			// compile-time errors
			case -30: return "CL_INVALID_VALUE";
			case -31: return "CL_INVALID_DEVICE_TYPE";
			case -32: return "CL_INVALID_PLATFORM";
			case -33: return "CL_INVALID_DEVICE";
			case -34: return "CL_INVALID_CONTEXT";
			case -35: return "CL_INVALID_QUEUE_PROPERTIES";
			case -36: return "CL_INVALID_COMMAND_QUEUE";
			case -37: return "CL_INVALID_HOST_PTR";
			case -38: return "CL_INVALID_MEM_OBJECT";
			case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
			case -40: return "CL_INVALID_IMAGE_SIZE";
			case -41: return "CL_INVALID_SAMPLER";
			case -42: return "CL_INVALID_BINARY";
			case -43: return "CL_INVALID_BUILD_OPTIONS";
			case -44: return "CL_INVALID_PROGRAM";
			case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
			case -46: return "CL_INVALID_KERNEL_NAME";
			case -47: return "CL_INVALID_KERNEL_DEFINITION";
			case -48: return "CL_INVALID_KERNEL";
			case -49: return "CL_INVALID_ARG_INDEX";
			case -50: return "CL_INVALID_ARG_VALUE";
			case -51: return "CL_INVALID_ARG_SIZE";
			case -52: return "CL_INVALID_KERNEL_ARGS";
			case -53: return "CL_INVALID_WORK_DIMENSION";
			case -54: return "CL_INVALID_WORK_GROUP_SIZE";
			case -55: return "CL_INVALID_WORK_ITEM_SIZE";
			case -56: return "CL_INVALID_GLOBAL_OFFSET";
			case -57: return "CL_INVALID_EVENT_WAIT_LIST";
			case -58: return "CL_INVALID_EVENT";
			case -59: return "CL_INVALID_OPERATION";
			case -60: return "CL_INVALID_GL_OBJECT";
			case -61: return "CL_INVALID_BUFFER_SIZE";
			case -62: return "CL_INVALID_MIP_LEVEL";
			case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
			case -64: return "CL_INVALID_PROPERTY";
			case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
			case -66: return "CL_INVALID_COMPILER_OPTIONS";
			case -67: return "CL_INVALID_LINKER_OPTIONS";
			case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

			// extension errors
			case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
			case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
			case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
			case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
			case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
			case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
			default: return "Unknown OpenCL error";
		}
}

// OpenCL kernel to perform an element-wise add of two arrays
const char* programSource =
    "__kernel void matrixMultKernel(__global int *Pd,            \n"
    "__global int *Md,                                           \n"
    "__global int *Nd, __global int *width) {                    \n"
    "  int row = get_global_id(0) / (*width);                    \n"
    "  int col = get_global_id(0) % (*width);                    \n"       
    "  int result = 0;                                           \n"
    "  int i;                                                    \n"
    "  for(i = 0; i < (*width); i+=1){                           \n"
    "    int Mval = Md[(*width) * col + i];                      \n"
    "    int Nval = Nd[(*width) * i + row];                      \n"
    "    result += Mval * Nval;                                  \n"
    "  }                                                         \n" 
    "                                                            \n"
    "  Pd[col * (*width) + row] = result;                        \n"
    "}                                                           \n";

int *M;
int *N;
int *P_opencl;
int *P_seq;

cl_mem M_mem;
cl_mem N_mem;
cl_mem P_mem;
cl_mem width_mem;

int width;
size_t datasize;
int *printPositions;
size_t globalWorkSize[1];


float getmsec (struct timeval start, struct timeval end) {
  return (float) (1000.0 * (end.tv_sec - start.tv_sec) 
                  + 0.001 * (end.tv_usec - start.tv_usec));
}

// Fill f width size many random int values
void fill(int *f, int size) {
    for (int i = 0; i < size; i += 1) {
        f[i] = ((int) random());
    }
}

// Compares every pair lhs[i] and rhs[i] for i < width
void compare(int *lhs, int *rhs, int width) {
    int errors = 0;
    for (int i = 0; i < width; i += 1) {
        // because of inting point operations
        if (lhs[i] != rhs[i]) {
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
            int sum = 0;
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
    fprintf(stdout, "platform selected \t\t\t\t>> %s (%d)\n", getErrorString(err), err);

    err = clGetDeviceIDs(platforms, CL_DEVICE_TYPE_GPU, 1, &devices, &numDevices);
    fprintf(stdout, "device selected \t\t\t\t>> %s (%d)\n", getErrorString(err), err);

    context = clCreateContext(NULL, 1, &devices, NULL, NULL, &err);
    fprintf(stdout, "context created \t\t\t\t>> %s (%d)\n", getErrorString(err), err);

    cmdQueue = clCreateCommandQueue(context, devices, 0, &err);
    fprintf(stdout, "commandQueue created \t\t\t\t>> %s (%d)\n", getErrorString(err), err);
}

void createKernel() {
    program = clCreateProgramWithSource(context, 1, &programSource, NULL, &err);
    fprintf(stdout, "program created \t\t\t\t>> %s (%d)\n", getErrorString(err), err);

    err = clBuildProgram(program, 1, &devices, NULL, NULL, NULL);
    fprintf(stdout, "program built successfully \t\t\t>> %s (%d)\n", getErrorString(err), err);
    if (err != CL_SUCCESS) {
      char *message = malloc(4096 * sizeof(char));
      assert(message != NULL);
      size_t retSize;
      clGetProgramBuildInfo(program, devices, CL_PROGRAM_BUILD_LOG, 4096, message, &retSize);
      printf("%s\n", message);
    }      

    kernel = clCreateKernel(program, "matrixMultKernel", &err);
    fprintf(stdout, "kernel created \t\t\t\t\t>> %s (%d)\n", getErrorString(err), err); 
}

void MatrixMulOpenCL(int *M, int *N, int *P, int width) {
    M_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &err);
    err = clEnqueueWriteBuffer(cmdQueue, M_mem, CL_TRUE, 0, datasize, M, 0, NULL, NULL);
    fprintf(stdout, "buffer for M created and copied \t\t>> %s (%d)\n", getErrorString(err), err);

    N_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &err);
    err = clEnqueueWriteBuffer(cmdQueue, N_mem, CL_TRUE, 0, datasize, N, 0, NULL, NULL);
    fprintf(stdout, "buffer for N created and copied \t\t>> %s (%d)\n", getErrorString(err), err);

    P_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, &err);
    fprintf(stdout, "buffer for P created \t\t\t\t>> %s (%d)\n", getErrorString(err), err);
    
    width_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int), NULL, &err);
    err = clEnqueueWriteBuffer(cmdQueue, width_mem, CL_TRUE, 0, sizeof(int), &width, 0, NULL, NULL);
    fprintf(stdout, "buffer for width created \t\t\t>> %s (%d)\n", getErrorString(err), err);

    // set OpenCL kernel arguments with clSetKernelArg(...)
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &P_mem);
    fprintf(stdout, "kernel argument Pd set \t\t\t\t>> %s (%d)\n", getErrorString(err), err);
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &M_mem);
    fprintf(stdout, "kernel argument Md set \t\t\t\t>> %s (%d)\n", getErrorString(err), err);
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &N_mem);
    fprintf(stdout, "kernel argument Nd set \t\t\t\t>> %s (%d)\n", getErrorString(err), err);
    err = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *) &width_mem);
    fprintf(stdout, "kernel argument width set \t\t\t>> %s (%d)\n", getErrorString(err), err);

    globalWorkSize[0] = width * width;
    
    // enqueue OpenCL kernel with clEnqueuNDRangeKernel(...)
    err = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
    fprintf(stdout, "enqueued kernel \t\t\t\t>> %s (%d)\n", getErrorString(err), err);

    // Wait for the command cmdQueue to get serviced before reading back results
    err = clFinish(cmdQueue);
    fprintf(stdout, "finished \t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
    
    // read data from OpenCL buffer for P into P with clEnqueueReadBuffer(...)
    clEnqueueReadBuffer(cmdQueue, P_mem, CL_TRUE, 0, datasize, P_opencl, 0, NULL, NULL);
    fprintf(stdout, "read buffer P \t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
}

void cleanup() {
    // Free OpenCL resources
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(cmdQueue);
    clReleaseContext(context);

    free(M);
    free(N);
    free(P_seq);
    free(P_opencl);
}

// end OpenCL code
// ######################################################

void init(int argc, char** argv) {
    if (argc < 2) {
				printf("Usage: matrixMult <size> <printrow,printcol>*\nWhere:\n \tsize is the length and width of the matrix.\n\tprintcol,printrow is a position in the result that you want printed\n");
				exit(-1);
		} else {
				width = atoi(argv[1]);
				datasize = sizeof(int) * width * width;
				printPositions = malloc(sizeof(int) * (argc - 2));
				if (printPositions == NULL) {
					printf("err: malloc failed\n");
					exit(-1);
				}
				
				for (int i = 2; i < argc; ++i) {
					printPositions[i - 2] = atoi(argv[i]);
				}
		}
    
    M = (int *) malloc(width * width * sizeof(int));
    N = (int *) malloc(width * width * sizeof(int));
    P_opencl = (int *) malloc(width * width * sizeof(int));
    P_seq = (int *) malloc(width * width * sizeof(int));

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
    printf("time elapsed OpenCL: %fmsecs\n", getmsec(start, end));

    gettimeofday(&start, NULL);
    MatrixMulSeq();
    gettimeofday(&end, NULL);
    printf("time elapsed Seq: %fmsecs\n", getmsec(start, end));

    compare(P_seq, P_opencl, width * width);

    for (int i = 0; i < argc-2; ++i) {
      if(printPositions[i] < width * width) {
        printf("\tP_opencl[%d] = %d\n", printPositions[i], P_opencl[printPositions[i]]);
      } else {
        printf("\tcannot print %d, out of range\n", printPositions[i]);
      }
    }
    // output the username, time and requested values from the Nestor questions
    system("date");
    system("echo $USER");
    
    // Don't forget to cleanup resources
    cleanup();

    return 0;
}
