// Dominik Matijaca 0036524568
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>

typedef struct {
	double totalTime;
	double writeTime;
	double kernelTime;
	double readTime;
} gpu_result;

LARGE_INTEGER freq;

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS  // clUnloadCompiler
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS  // clCreateCommandQueue

#if defined _MSC_VER
#pragma comment(lib, "OpenCL.lib")
#define NOTIFY_CONVENTION __stdcall
#else
#define NOTIFY_CONVENTION 
#endif

#include <CL/cl.h>

#define CL_CHECK(_expr)                                                         \
   do {                                                                         \
     cl_int _err = _expr;                                                       \
     if (_err == CL_SUCCESS)                                                    \
       break;                                                                   \
     fprintf(stderr, "OpenCL Error: '%s' returned %d!\n", #_expr, (int)_err);   \
     abort();                                                                   \
   } while (0)

void* CL_CHECK_ERR(void* _val) {
	if (_val != NULL) {
		return _val;
	}
	fprintf(stderr, "OpenCL Error\n");
	abort();
	return NULL;
}

void NOTIFY_CONVENTION pfn_notify(const char* errinfo, const void* private_info, size_t cb, void* user_data)
{
	fprintf(stderr, "OpenCL Error (via pfn_notify): %s\n", errinfo);
}

cl_context context;
cl_program program;
cl_command_queue queue;
cl_kernel kernel;

void gpu_init() {
	cl_platform_id platforms[5];
	cl_uint platforms_n = 0;
	CL_CHECK(clGetPlatformIDs(5, platforms, &platforms_n));

	cl_device_id devices[5];
	cl_uint devices_n = 0;
	int index_gpu_platform = -1;

	for (size_t i = 0; i < platforms_n; i++) {
		devices_n = 0;
		cl_int err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 5, devices, &devices_n);

		if (err == CL_SUCCESS && devices_n > 0) {
			index_gpu_platform = i;
		}
	}

	if (platforms_n == 0 || index_gpu_platform == -1)
		exit(1);

	CL_CHECK(clGetDeviceIDs(platforms[index_gpu_platform], CL_DEVICE_TYPE_GPU, 5, devices, &devices_n));

	if (devices_n == 0)
		exit(1);

	cl_int _err = CL_INVALID_VALUE;
	context = (cl_context)CL_CHECK_ERR(clCreateContext(NULL, 1, devices, &pfn_notify, NULL, &_err));

	const char* program_source[] = {
		"__kernel void mat_mul(__global float* R, __global float* A, __global float* B, int m, int k)\n",
		"{\n",
		"    int i = get_global_id(0);\n",
		"    int j = get_global_id(1);\n",
		"    \n",
		"    float sum = 0;\n",
		"    for (int l = 0; l < m; l++) {\n",
		"		sum += A[i * m + l] * B[l * k + j];\n",
		"    }\n",
		"    R[i * k + j] = sum;\n",
		"}\n"
	};

	program = (cl_program)CL_CHECK_ERR(clCreateProgramWithSource(context, sizeof(program_source) / sizeof(*program_source), program_source, NULL, &_err));
	if (clBuildProgram(program, 1, devices, "", NULL, NULL) != CL_SUCCESS) {
		char buffer[1024];
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
		fprintf(stderr, "CL Compilation failed:\n%s", buffer);
		abort();
	}

	queue = (cl_command_queue)CL_CHECK_ERR(clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, &_err));
	kernel = (cl_kernel)CL_CHECK_ERR(clCreateKernel(program, "mat_mul", &_err));
}

gpu_result gpu_mat_mul(float* R, float* A, float* B, int n, int m, int k) {
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);

	cl_int _err = CL_INVALID_VALUE;
	int Asize = n * m * sizeof(float);
	int Bsize = m * k * sizeof(float);
	int Rsize = n * k * sizeof(float);
	
	cl_mem gpu_A = (cl_mem)CL_CHECK_ERR(clCreateBuffer(context, CL_MEM_READ_ONLY, Asize, NULL, &_err));
	cl_mem gpu_B = (cl_mem)CL_CHECK_ERR(clCreateBuffer(context, CL_MEM_READ_ONLY, Bsize, NULL, &_err));
	cl_mem gpu_R = (cl_mem)CL_CHECK_ERR(clCreateBuffer(context, CL_MEM_WRITE_ONLY, Rsize, NULL, &_err));

	cl_event Aevent, Bevent;
	CL_CHECK(clEnqueueWriteBuffer(queue, gpu_A, CL_FALSE, 0, Asize, A, 0, NULL, &Aevent));
	CL_CHECK(clEnqueueWriteBuffer(queue, gpu_B, CL_FALSE, 0, Bsize, B, 0, NULL, &Bevent));
	
	CL_CHECK(clWaitForEvents(1, &Bevent));
	CL_CHECK(clFinish(queue));
	
	cl_ulong writeStart, writeEnd;
	CL_CHECK(clGetEventProfilingInfo(Aevent, CL_PROFILING_COMMAND_START, sizeof(writeStart), &writeStart, NULL));
	CL_CHECK(clGetEventProfilingInfo(Bevent, CL_PROFILING_COMMAND_END, sizeof(writeEnd), &writeEnd, NULL));
	
	CL_CHECK(clSetKernelArg(kernel, 0, sizeof(gpu_R), &gpu_R));
	CL_CHECK(clSetKernelArg(kernel, 1, sizeof(gpu_A), &gpu_A));
	CL_CHECK(clSetKernelArg(kernel, 2, sizeof(gpu_B), &gpu_B));
	CL_CHECK(clSetKernelArg(kernel, 3, sizeof(m), &m));
	CL_CHECK(clSetKernelArg(kernel, 4, sizeof(k), &k));

	size_t global_work_size[2] = { n, k };
	
	cl_event kernelEvent;
	CL_CHECK(clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, &kernelEvent));

	CL_CHECK(clWaitForEvents(1, &kernelEvent));
	CL_CHECK(clFinish(queue));

	cl_ulong kernelStart, kernelEnd;
	CL_CHECK(clGetEventProfilingInfo(kernelEvent, CL_PROFILING_COMMAND_START, sizeof(kernelStart), &kernelStart, NULL));
	CL_CHECK(clGetEventProfilingInfo(kernelEvent, CL_PROFILING_COMMAND_END, sizeof(kernelEnd), &kernelEnd, NULL));
	
	cl_event Revent;
	CL_CHECK(clEnqueueReadBuffer(queue, gpu_R, CL_FALSE, 0, Rsize, R, 0, NULL, &Revent));
	
	CL_CHECK(clWaitForEvents(1, &Revent));
	CL_CHECK(clFinish(queue));

	cl_ulong readStart, readEnd;
	CL_CHECK(clGetEventProfilingInfo(Revent, CL_PROFILING_COMMAND_START, sizeof(readStart), &readStart, NULL));
	CL_CHECK(clGetEventProfilingInfo(Revent, CL_PROFILING_COMMAND_END, sizeof(readEnd), &readEnd, NULL));

	CL_CHECK(clReleaseMemObject(gpu_A));
	CL_CHECK(clReleaseMemObject(gpu_B));
	CL_CHECK(clReleaseMemObject(gpu_R));
	
	QueryPerformanceCounter(&end);
	
	gpu_result result;
	result.totalTime = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
	result.writeTime = (writeEnd - writeStart) / 1000000000.0;
	result.kernelTime = (kernelEnd - kernelStart) / 1000000000.0;
	result.readTime = (readEnd - readStart) / 1000000000.0;

	return result;
}

void gpu_deinit() {
	CL_CHECK(clReleaseKernel(kernel));
	CL_CHECK(clReleaseProgram(program));
	CL_CHECK(clReleaseContext(context));
}

double cpu_mat_mul(float* R, float* A, float* B, int n, int m, int k) {
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);
	
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < k; j++) {
			float sum = 0;
			for (int l = 0; l < m; l++) {
				sum += A[i * m + l] * B[l * k + j];
			}
			R[i * k + j] = sum;
		}
	}
	
	QueryPerformanceCounter(&end);
	return (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
}

void mat_print(const float* A, const int n, const int m) {
	printf("[%dx%d] = {\n", n, m);
	for (int i = 0; i < n; i++) {
		printf("\t");
		for (int j = 0; j < m; j++) {
			printf("%.0f ", A[i * m + j]);
		}
		printf("\n");
	}
	printf("}\n\n");
}

float A[6] = { 1, 2, 3, 4, 5, 6 };
float B[6] = { 1, 2, 3, 4, 5, 6 };
float C[35] = {
	1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21,
	22, 23, 24, 25, 26, 27, 28,
	29, 30, 31, 32, 33, 34, 35
};
float D[56] = {
	1, 2, 3, 4, 5, 6, 7, 8,
	9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24,
	25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, 52, 53, 54, 55, 56
};

int main() {
	gpu_init();
	QueryPerformanceFrequency(&freq);
	
	printf("Small matrices\n");

	printf("A");
	mat_print(A, 2, 3);

	printf("B");
	mat_print(B, 3, 2);
	
	float R[4];
	cpu_mat_mul(R, A, B, 2, 3, 2);
	
	printf("CPU_R");
	mat_print(R, 2, 2);

	for (int i = 0; i < 4; i++) {
		R[i] = 0;
	}
	gpu_mat_mul(R, A, B, 2, 3, 2);

	printf("GPU_R");
	mat_print(R, 2, 2);

	printf("Mid-sized matrices\n");
	float S[40];
	cpu_mat_mul(S, C, D, 5, 7, 8);

	printf("CPU_S");
	mat_print(S, 5, 8);

	for (int i = 0; i < 40; i++) {
		S[i] = 0;
	}
	gpu_mat_mul(S, C, D, 5, 7, 8);
	
	printf("GPU_S");
	mat_print(S, 5, 8);

	printf("Benchmarking\n");
	{
		#define ITERATIONS (10)
		cl_int _err = CL_INVALID_VALUE;

		size_t size = 0x80000000llu;
		double gb = size / (double)0x40000000 * ITERATIONS;
		volatile float* ram = (volatile float*)malloc(size);
		
		cl_mem gpu = (cl_mem)CL_CHECK_ERR(clCreateBuffer(context, CL_MEM_READ_WRITE, size, NULL, &_err));
		
		double time = 0.0;
		
		for (int i = 0; i < ITERATIONS; i++) {
			cl_event event;
			CL_CHECK(clEnqueueWriteBuffer(queue, gpu, CL_FALSE, 0, size, ram, 0, NULL, &event));

			CL_CHECK(clWaitForEvents(1, &event));
			CL_CHECK(clFinish(queue));

			cl_ulong writeStart, writeEnd;
			CL_CHECK(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(writeStart), &writeStart, NULL));
			CL_CHECK(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(writeEnd), &writeEnd, NULL));
			
			time += writeEnd - writeStart;
		}
		time /= 1000000000.0; // seconds
		printf("RAM -> GPU took %.3lf s for %.2lf GB (%.3lf GB/s)\n", time, gb, gb / time);

		time = 0.0;
		for (int i = 0; i < ITERATIONS; i++) {
			cl_event event;

			CL_CHECK(clEnqueueReadBuffer(queue, gpu, CL_FALSE, 0, size, ram, 0, NULL, &event));

			CL_CHECK(clWaitForEvents(1, &event));
			CL_CHECK(clFinish(queue));

			cl_ulong readStart, readEnd;
			CL_CHECK(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(readStart), &readStart, NULL));
			CL_CHECK(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(readEnd), &readEnd, NULL));
			
			time += readEnd - readStart;
		}
		time /= 1000000000.0; // seconds
		printf("GPU -> RAM took %.3lf s for %.2lf GB (%.3lf GB/s)\n", time, gb, gb / time);

		free(ram);
		CL_CHECK(clReleaseMemObject(gpu));
	}

	printf("\nProfiling\n");
	srand(time(NULL));
	for (int n = 128; n <= 2048; n *= 2) {
		float* A = (float*)malloc(n * n * sizeof(float));
		float* B = (float*)malloc(n * n * sizeof(float));
		float* Rcpu = (float*)malloc(n * n * sizeof(float));
		float* Rgpu = (float*)malloc(n * n * sizeof(float));
		
		if (A == NULL || B == NULL || Rcpu == NULL || Rgpu == NULL) {
			printf("Failed to allocate memory for profiling matrices of size %d\n", n);
			return 1;
		}

		for (int i = 0; i < n * n; i++) {
			A[i] = 10 * (double)rand() / RAND_MAX;
			B[i] = 10 * (double)rand() / RAND_MAX;
			Rcpu[i] = 0;
			Rgpu[i] = 0;
		}

		double cpuTime = cpu_mat_mul(Rcpu, A, B, n, n, n);
		gpu_result gpuResult = gpu_mat_mul(Rgpu, A, B, n, n, n);
		
		for (int i = 0; i < n * n; i++) {
			if (Rcpu[i] != Rgpu[i]) {
				printf("Results differ at index %d\n", i);
				return 1;
			}
		}

		printf("n = %d\n", n);
		printf("\tCPU time: %.3f ms\n", cpuTime * 1000);
		printf("\tGPU total time: %.3f ms\n", gpuResult.totalTime * 1000);
		printf("\tGPU write time: %.3f ms\n", gpuResult.writeTime * 1000);
		printf("\tGPU kernel time: %.3f ms\n", gpuResult.kernelTime * 1000);
		printf("\tGPU read time: %.3f ms\n", gpuResult.readTime * 1000);

		free(A);
		free(B);
		free(Rcpu);
		free(Rgpu);
	}
	
	gpu_deinit();
	return 0;
}