/*
 *  Simple OpenCL demo program
 *
 *  Written by Clifford Wolf <clifford@clifford.at> in 2009.
 *  Modified by Ivan Grubisic and Sinisa Segvic
 *  original code taken from: http://svn.clifford.at/tools/trunk/examples/cldemo.c
 *
 *  ----------------------------------------------------------------------
 *
 *  This is free and unencumbered software released into the public domain.
 *
 *  Anyone is free to copy, modify, publish, use, compile, sell, or
 *  distribute this software, either in source code form or as a compiled
 *  binary, for any purpose, commercial or non-commercial, and by any
 *  means.
 *
 *  In jurisdictions that recognize copyright laws, the author or authors
 *  of this software dedicate any and all copyright interest in the
 *  software to the public domain. We make this dedication for the benefit
 *  of the public at large and to the detriment of our heirs and
 *  successors. We intend this dedication to be an overt act of
 *  relinquishment in perpetuity of all present and future rights to this
 *  software under copyright law.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *  OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For more information, please refer to <http://unlicense.org/>
 *
 *  ----------------------------------------------------------------------
 *
 *  gcc -o cldemo -std=gnu99 -Wall -I/usr/include/nvidia-current cldemo.c -lOpenCL
 *
 */
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS  // clUnloadCompiler
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS  // clCreateCommandQueue

#if defined _MSC_VER
#pragma comment(lib, "OpenCL.lib")
#define NOTIFY_CONVENTION __stdcall
#else
#define NOTIFY_CONVENTION 
#endif

#include <CL/cl.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 //#include <unistd.h>

#define NUM_DATA 100

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

int main(int argc, char** argv)
{
    cl_platform_id platforms[100];
    cl_uint platforms_n = 0;
    CL_CHECK(clGetPlatformIDs(100, platforms, &platforms_n));

    cl_device_id devices[100];
    cl_uint devices_n = 0;
    int index_gpu_platform = -1;

    printf("=== %d OpenCL platform(s) found: ===\n", platforms_n);
    for (size_t i = 0; i < platforms_n; i++)
    {
        char buffer[1024];
        printf("  -- %d --\n", i);
        CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, 1024, buffer, NULL));
        printf("  PROFILE = %s\n", buffer);
        CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION, 1024, buffer, NULL));
        printf("  VERSION = %s\n", buffer);
        CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 1024, buffer, NULL));
        printf("  NAME = %s\n", buffer);
        CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 1024, buffer, NULL));
        printf("  VENDOR = %s\n", buffer);
        CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, 1024, buffer, NULL));
        printf("  EXTENSIONS = %s\n", buffer);

        devices_n = 0;
        cl_int err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 100, devices, &devices_n);
        printf("  N GPU DEVICES = %d\n", devices_n);
        if (err == CL_SUCCESS && devices_n > 0) {
            index_gpu_platform = i;
        }
    }

    if (platforms_n == 0 || index_gpu_platform == -1)
        return 1;

    // CL_CHECK(clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, 100, devices, &devices_n));
    CL_CHECK(clGetDeviceIDs(platforms[index_gpu_platform], CL_DEVICE_TYPE_GPU, 100, devices, &devices_n));
    printf("=== %d OpenCL device(s) found on platform %d:\n", devices_n, index_gpu_platform);

    for (size_t i = 0; i < devices_n; i++)
    {
        char buffer[1024];
        cl_uint buf_uint;
        cl_ulong buf_ulong;
        printf("  -- %d --\n", i);
        CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL));
        printf("  DEVICE_NAME = %s\n", buffer);
        CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_VENDOR, sizeof(buffer), buffer, NULL));
        printf("  DEVICE_VENDOR = %s\n", buffer);
        CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_VERSION, sizeof(buffer), buffer, NULL));
        printf("  DEVICE_VERSION = %s\n", buffer);
        CL_CHECK(clGetDeviceInfo(devices[i], CL_DRIVER_VERSION, sizeof(buffer), buffer, NULL));
        printf("  DRIVER_VERSION = %s\n", buffer);
        CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(buf_uint), &buf_uint, NULL));
        printf("  DEVICE_MAX_COMPUTE_UNITS = %u\n", (unsigned int)buf_uint);
        CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(buf_uint), &buf_uint, NULL));
        printf("  DEVICE_MAX_CLOCK_FREQUENCY = %u\n", (unsigned int)buf_uint);
        CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL));
        printf("  DEVICE_GLOBAL_MEM_SIZE = %llu\n", (unsigned long long)buf_ulong);
    }

    if (devices_n == 0)
        return 1;

    int buf_in[NUM_DATA], buf_out[NUM_DATA];
    for (int i = 0; i < NUM_DATA; i++) {
        buf_in[i] = i;
    }

    cl_int _err = CL_INVALID_VALUE;
    cl_context context = (cl_context)CL_CHECK_ERR(clCreateContext(NULL, 1, devices, &pfn_notify, NULL, &_err));

    const char* program_source[] = {
       "__kernel void simple_demo(__global int *src, __global int *dst, int factor)\n",
       "{\n",
       "	int i = get_global_id(0);\n",
       "	dst[i] = src[i] * factor;\n",
       "}\n"
    };
    cl_program program = (cl_program)CL_CHECK_ERR(clCreateProgramWithSource(context, sizeof(program_source) / sizeof(*program_source), program_source, NULL, &_err));
    if (clBuildProgram(program, 1, devices, "", NULL, NULL) != CL_SUCCESS) {
        char buffer[1024];
        clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
        fprintf(stderr, "CL Compilation failed:\n%s", buffer);
        abort();
    }
    // CL_CHECK(clUnloadCompiler());

    cl_command_queue queue = (cl_command_queue)CL_CHECK_ERR(clCreateCommandQueue(context, devices[0], 0, &_err));
    cl_mem input_buffer = (cl_mem)CL_CHECK_ERR(clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * NUM_DATA, NULL, &_err));
    cl_mem output_buffer = (cl_mem)CL_CHECK_ERR(clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * NUM_DATA, NULL, &_err));
    CL_CHECK(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0, sizeof(int) * NUM_DATA, buf_in, 0, NULL, NULL));
    int factor = 2;
    cl_kernel kernel = (cl_kernel)CL_CHECK_ERR(clCreateKernel(program, "simple_demo", &_err));
    CL_CHECK(clSetKernelArg(kernel, 0, sizeof(input_buffer), &input_buffer));
    CL_CHECK(clSetKernelArg(kernel, 1, sizeof(output_buffer), &output_buffer));
    CL_CHECK(clSetKernelArg(kernel, 2, sizeof(factor), &factor));
    size_t global_work_size[1] = { NUM_DATA };
    CL_CHECK(clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL));
    CL_CHECK(clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(int) * NUM_DATA, buf_out, 0, NULL, NULL));

    CL_CHECK(clReleaseMemObject(input_buffer));
    CL_CHECK(clReleaseMemObject(output_buffer));

    CL_CHECK(clReleaseKernel(kernel));
    CL_CHECK(clReleaseProgram(program));
    CL_CHECK(clReleaseContext(context));

    printf("Result:");
    for (int i = 0; i < NUM_DATA; i++) {
        printf(" %d", buf_out[i]);
    }
    printf("\n");

    return 0;
}

