#include <intrin.h>
#include <iomanip>
#include <iostream>
#include <Windows.h>

typedef unsigned char byte;
typedef unsigned int uint;

typedef struct {
    double totalTime;
    int sum;
    double avgOpTime;
    double bandwidth;
} result;

LARGE_INTEGER freq;
uint L2;

void GetL2CacheSize(uint* i) {
    int regs[4];
    __cpuid(regs, 0x80000006);
    *i = ((regs[2] >> 16) & 0xFFFF) * 1024;
}

#define M (L2 / sizeof(int))
#define N (1024)
#define BUFSIZE (N * M)

template <typename F>
__forceinline result run(const char* name, F&& f) {
    result r;

    std::cout << std::right << std::setw(25) << name << ": ";

    LARGE_INTEGER start, end;
    QueryPerformanceCounter(&start);

    r.sum = f();

    QueryPerformanceCounter(&end);

    r.totalTime = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    r.avgOpTime = r.totalTime / BUFSIZE;
    r.bandwidth = BUFSIZE * sizeof(int) / r.totalTime;

    std::cout
        << std::right << std::setw(10) << std::setprecision(4) << r.totalTime * 1000 << " ms runtime,  "
        << std::right << std::setw(10) << BUFSIZE << " ops,  "
        << std::right << std::setw(10) << r.sum << " sum,  "
        << std::right << std::setw(5) << std::setprecision(3) << r.avgOpTime * 1000000000 << " ns/op (average),  "
        << std::right << std::setw(6) << std::setprecision(2) << r.bandwidth / 1000000 << " GB/s (bandwidth)"
    << std::endl;
    
    return r;
}

int main() {
    QueryPerformanceFrequency(&freq);
    std::cout << std::fixed;

    GetL2CacheSize(&L2);
    std::cout << "L2 cache size: " << L2 << " bytes (" << L2 / 1024 << "kB)" << std::endl;
    std::cout << "Buffer size: " << N << " rows x " << M << " cols = " << BUFSIZE << " ints" << std::endl;

    volatile int* buf = new int[BUFSIZE];
    volatile int* buf2 = new int[BUFSIZE];
    volatile int* dest = new int[BUFSIZE];

    for (int i = 0; i < BUFSIZE; i++) {
        buf[i] = 3;
        buf2[i] = 0;
    }

    double t = run("read_rows_then_cols", [&]() {
        int sum = 0;
        for (int i = 0; i < N; i++)
            for (int j = 0; j < M; j++)
                sum += buf[i * M + j];
        return sum;
    }).totalTime / run("read_cols_then_rows", [&]() {
        int sum = 0;
        for (int j = 0; j < M; j++)
            for (int i = 0; i < N; i++)
                sum += buf[i * M + j];
        return sum;
    }).totalTime;

    std::cout << std::endl << "t(read_rows_then_cols) / t(read_cols_then_rows) = "
        << std::setprecision(3) << t << std::endl << std::endl;

    t = run("write_rows_then_cols", [&]() {
        int sum = 0;
        for (int i = 0; i < N; i++)
            for (int j = 0; j < M; j++)
                sum += (buf2[i * M + j] = 5);
        return sum;
    }).totalTime / run("write_cols_then_rows", [&]() {
        int sum = 0;
        for (int j = 0; j < M; j++)
            for (int i = 0; i < N; i++)
                sum += (buf2[i * M + j] = 5);
        return sum;
    }).totalTime;

    std::cout << std::endl << "t(write_rows_then_cols) / t(write_cols_then_rows) = "
        << std::setprecision(3) << t << std::endl << std::endl;

    t = run("sum_rows_then_cols", [&]() {
        int sum = 0;
        for (int i = 0; i < N; i++)
            for (int j = 0; j < M; j++)
                sum += (dest[i * M + j] = buf[i * M + j] + buf2[i * M + j]);
        return sum;
    }).totalTime / run("sum_cols_then_rows", [&]() {
        int sum = 0;
        for (int j = 0; j < M; j++)
            for (int i = 0; i < N; i++)
                sum += (dest[i * M + j] = buf[i * M + j] + buf2[i * M + j]);
        return sum;
    }).totalTime;

    std::cout << std::endl << "t(sum_rows_then_cols) / t(sum_cols_then_rows) = "
        << std::setprecision(3) << t << std::endl << std::endl;

    delete buf;
    delete buf2;
    delete dest;

    return 0;
}
