#include <functional>
#include <intrin.h>
#include <iomanip>
#include <iostream>
#include <malloc.h>
#include <Windows.h>

typedef unsigned char byte;
typedef unsigned int uint;

const char* opNames[3] = { "+", "*", "/" };
const char* sizeDef[3] = { "L1/8", "L2/8", " >L2" };

typedef struct {
    double totalTime;
    int size;
} result;

LARGE_INTEGER freq;
uint L1, L2;

void GetL1CacheSize(uint* i) {
    int regs[4];
    __cpuid(regs, 0x80000005);
    *i = ((regs[2] >> 24) & 0xFF) * 1024;
}

void GetL2CacheSize(uint* i) {
    int regs[4];
    __cpuid(regs, 0x80000006);
    *i = ((regs[2] >> 16) & 0xFFFF) * 1024;
}

volatile int magic = 23;

template <typename T>
__forceinline void run(result* r, volatile T* buf, uint N, T (*f)(T)) {
    LARGE_INTEGER start, end;
    QueryPerformanceCounter(&start);

    for (int i = 0; i < N; i++)
        buf[i] = f(buf[i]);
    
    QueryPerformanceCounter(&end);

    r->size = N * sizeof(T);
    r->totalTime = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
}

template <typename T>
__forceinline void run_size(result r[3], uint N, T (*f)()) {
    N *= 0.995;
    N &= 0xFFFFFFF0u;

    volatile T* buf = (volatile T*)_aligned_malloc(N, 16);

    N /= sizeof(T);

    for (int i = 0; i < N; i++)
        buf[i] = f();

    run<T>(r,     buf, N, [](T i) { return i += magic; });
    run<T>(r + 1, buf, N, [](T i) { return i *= magic; });
    run<T>(r + 2, buf, N, [](T i) { return i /= magic; });

    _aligned_free((void*)buf);
}

template <typename T>
__forceinline void run_type(result r[3][3], T (*f)()) {
    run_size<T>(r[0], L1 / 8, f);
    run_size<T>(r[1], L2 / 8, f);
    run_size<T>(r[2], L2 * 1.25, f);
}

int main() {
    QueryPerformanceFrequency(&freq);
    std::cout << std::fixed << std::setprecision(1);
    GetL1CacheSize(&L1);
    GetL2CacheSize(&L2);
    srand(0);

    std::cout << "L1 cache size: " << L1 / 1024 << "kB" << std::endl;
    std::cout << "L2 cache size: " << L2 / 1024 << "kB" << std::endl << std::endl;

    result r[5][3][3];

    run_type<int>(r[0], []() { return (int)rand(); });
    run_type<char>(r[1], []() { return (char)(rand() & 0xFF); });
    run_type<short>(r[2], []() { return (short)(rand() & 0xFFFF); });
    run_type<float>(r[3], []() { return (float)rand() / RAND_MAX; });
    run_type<double>(r[4], []() { return (double)rand() / RAND_MAX; });

    std::cout << std::setw(16) << "int = " << sizeof(int)
              << std::setw(32) << "char = " << sizeof(char)
              << std::setw(32) << "short = " << sizeof(short)
              << std::setw(32) << "float = " << sizeof(float)
              << std::setw(32) << "double = " << sizeof(double)
    << std::endl;

    std::cout << std::setw(4) << "";

    for (int j = 0; j < 5; j++) {
        std::cout << std::setw(12) << "";

        for (int k = 0; k < 3; k++) {
            std::cout << std::setw(7) << opNames[k];
        }
    }
    std::cout << std::endl;

    for (int i = 0; i < 3; i++) {
        std::cout << sizeDef[i];

        for (int j = 0; j < 5; j++) {
            std::cout << std::setw(12) << r[j][i][0].size;
            
            for (int k = 0; k < 3; k++) {
                std::cout << std::setw(7) << r[j][i][k].totalTime * 1000000;
            }
        }

        std::cout << std::endl;
    }

    return 0;
}
