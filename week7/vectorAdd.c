// This program implements a vector addition using OpenCL
// Rewritten by Guus Klinkenberg

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

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

float getmsec (struct timeval start, struct timeval end) {
	return (float) (1000.0 * (end.tv_sec - start.tv_sec) + 0.001 * (end.tv_usec - start.tv_usec));
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
const char *programSource =
								"__kernel void vecadd(__global int *A,               \n"
								"            __global int *B,                        \n"
								"            __global int *C)                        \n"
								"{                                                   \n"
								"   long globalId = get_global_id(0);                \n"
								"   C[globalId] = A[globalId] + B[globalId];         \n"
								"}                                                   \n";
//size_t programSourceSize;

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

size_t globalWorkSize[1];

void initOpenCL() {
  
  
    err = clGetPlatformIDs(0, NULL, &numPlatforms);
    fprintf(stdout, "platforms available: %d\t\t\t\t>> %s (%d)\n", numPlatforms, getErrorString(err), err);
    platforms = malloc(sizeof(cl_platform_id) * numPlatforms);
		err = clGetPlatformIDs(numPlatforms, platforms, &numPlatforms);
		fprintf(stdout, "platform selected\t\t\t\t>> %s (%d)\n", getErrorString(err), err);

		err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
    fprintf(stdout, "devices available: %d\t\t\t\t>> %s (%d)\n", numDevices, getErrorString(err), err);
    devices = malloc(sizeof(cl_device_id) * numDevices);
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, numDevices, devices, &numDevices);
		fprintf(stdout, "device selected\t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);

		context = clCreateContext(NULL, numDevices, devices, NULL, NULL, &err);
		fprintf(stdout, "context created\t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);

		cmdQueue = clCreateCommandQueue(context, devices[0], 0, &err);
		fprintf(stdout, "commandQueue created\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
}

void createMemBuffers(size_t datasize, int *A, int *B) {

		A_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &err);
    fprintf(stdout, "memory buffers created\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
		B_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &err);
    fprintf(stdout, "memory buffers created\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
		C_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, &err);
		fprintf(stdout, "memory buffers created\t\t\t\t>> %s (%d)\n", getErrorString(err), err);

		err = clEnqueueWriteBuffer(cmdQueue, A_mem, CL_FALSE, 0, datasize, A, 0, NULL, NULL);
    fprintf(stdout, "queue created\t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
		err = clEnqueueWriteBuffer(cmdQueue, B_mem, CL_FALSE, 0, datasize, B, 0, NULL, NULL);
		fprintf(stdout, "queue created\t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
}

void executeCode(int elements, int *C) {

		program = clCreateProgramWithSource(context, 1, &programSource, NULL, &err);
		fprintf(stdout, "program created\t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);

		err = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);
		fprintf(stdout, "program built successfully\t\t\t>> %s (%d)\n", getErrorString(err), err);
    if (err != CL_SUCCESS) {
      char *message = malloc(4096 * sizeof(char));
      assert(message != NULL);
      size_t retSize;
      clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 4096, message, &retSize);
      printf("%s\n", message);
    }
    

		kernel = clCreateKernel(program, "vecadd", &err);
		fprintf(stdout, "kernel created\t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);

		err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &A_mem);
    fprintf(stdout, "set kernel arguments\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
		
		err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &B_mem);
    fprintf(stdout, "set kernel arguments\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
		
		err = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &C_mem);
		fprintf(stdout, "set kernel arguments\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
    
    globalWorkSize[0] = elements;
    
		err = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
		fprintf(stdout, "enqueued kernel\t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
}

void cleanup(int *A, int *B, int *C) {
		// Free OpenCL resources
		clReleaseKernel(kernel);
		clReleaseProgram(program);
		clReleaseCommandQueue(cmdQueue);
		clReleaseContext(context);
    clReleaseMemObject(A_mem);
    clReleaseMemObject(B_mem);
    clReleaseMemObject(C_mem);
		free(A);
		free(B);
		free(C);
}

void getArgs(int argc, char **argv, int *elements, size_t *datasize, int **printPositions) {
	if (argc < 2) {
				printf("Usage: vectorAdd <elements> <printPosition>* \nWhere:\n \telements is the length of the vectors. Should not be larger then 2^31\n\tprintPosition is a position in the result that you want printed\n");
				exit(-1);
		} else {
				*elements = atoi(argv[1]);
				*datasize = sizeof(int) * (*elements);
				*printPositions = malloc(sizeof(int) * (argc - 2));
				if (*printPositions == NULL) {
					printf("err: malloc failed\n");
					exit(-1);
				}
				
				for (int i = 2; i < argc; ++i) {
					(*printPositions)[i-2] = atoi(argv[i]);
				}
		}
}

int main(int argc, char **argv) {
		// This code executes on the OpenCL host
		//programSourceSize = strlen(programSource) * sizeof(char);
		// Elements in each array
		int elements;
		size_t datasize;
		int *printPositions;

		getArgs(argc, argv, &elements, &datasize, &printPositions);

		// Host data
		int *A = malloc(datasize); // Input array
		int *B = malloc(datasize);  // Input array
		int *C = malloc(datasize);  // Output array

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
		executeCode(elements, C);
		cl_int err = clEnqueueReadBuffer(cmdQueue, C_mem, CL_FALSE, 0, datasize, C, 0, NULL, NULL);
		fprintf(stdout, "buffer read\t\t\t\t\t>> %s (%d)\n", getErrorString(err), err);
    
		gettimeofday(&end, NULL);
		printf("time elapsed OpenCL: %fmsecs\n", getmsec(start, end));

		/* Write and time a sequential vector addition */

		int* C_seq = malloc(datasize);
		struct timeval start_seq, end_seq;

		gettimeofday(&start_seq, NULL);

		for(int i = 0; i < elements; i++){
      C_seq[i] = A[i] + B[i];
		}

		gettimeofday(&end_seq, NULL);

		// Verify the output
		bool result = true;
		for (int i = 0; i < elements; i++) {
				if (C[i] != A[i] + B[i]) {
						result = false;
						printf("Output is incorrect on:\n");
            printf("A[%d]: %d B[%d]:%d C[%d]:%d\n", i, A[i], i, B[i], i, C[i]);
						break;
				}
		}
		
		float speedup = getmsec(start_seq, end_seq) / getmsec(start, end );

		if (result) {
				printf("Output is correct\n");
				for (int i = 0; i < argc-2; ++i) {
					if(printPositions[i] < elements) {
						printf("\tC[%d] = %d\n", printPositions[i], C[printPositions[i]]);
					} else {
						printf("\tcannot print %d, out of range\n", printPositions[i]);
          }
				}
		}
		

		// output the username, time and requested values from the Nestor questions
		printf("%f\n", speedup);
		system("date");
		system("echo $USER");

		cleanup(A, B, C);

		return 0;
}
