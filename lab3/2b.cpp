#include <iomanip>
#include <iostream>
#include <string>
#include <Windows.h>

/* generated with 2a.cpp */

// AMD Ryzen 7 2700 Eight-Core Processor
// 16 threads @ 3200MHz
// Windows 10 Education

// L1I: 64kB per core, 4-way associativity, 1 lines per tag, 64B line size
// L1D: 32kB per core, 8-way associativity, 1 lines per tag, 64B line size
#define S1 (32*1024)
#define B1 (64)

// L2: 512kB per core, 8 to 15-way associativity, 1 lines per tag, 64B line size
#define S2 (512*1024)
#define B2 (64)

// L3: 16MB total, 16 to 31-way associativity, 1 lines per tag, 64B line size
#define S3 (32*512*1024)
#define B3 (64)

// potprogram A: svi bajtovi memorijskog spremnika veličine S1 redom se uvećavaju za jedan 
#define A_SIZE (S1)
#define A_STEP (1)

// potprogram B: za jedan se uvećava svaki B1-ti bajt memorijskog spremnika veličine 2*S1
#define B_SIZE (2*S1)
#define B_STEP (B1)

// potprogram C: za jedan se uvećava svaki B2-ti bajt memorijskog spremnika veličine 2*S2
#define C_SIZE (2*S2)
#define C_STEP (B2)

// potprogram D: za jedan se uvećava svaki B3-ti bajt memorijskog spremnika veličine 2*S3
#define D_SIZE (2*S3)
#define D_STEP (B3)

// broj pristupa memoriji za svaki potprogram
#define N (200000000)

// povećanje pomaka (onemogućuje pretpribavljanje)
#define DELTA (8)      

typedef unsigned char byte;
typedef unsigned int uint;

typedef struct {
    double totalTime;
    int cntAccesses;
    int sum;
    double avgAccessTime;
    double bandwidth;
} result;

LARGE_INTEGER freq;

__forceinline result run(uint size, uint step, uint delta, const char* name) {
    result r = {};

    std::cout << std::right << std::setw(3) << name << " ("
        << std::right << std::setw(5) << size / 1024 << " kB buffer, "
        << std::right << std::setw(2) << step << " step, "
        << std::right << std::setw(1) << delta << " delta): ";

    size *= delta;
    byte* buf = new byte[size];
    memset(buf, 0, size);
   
    LARGE_INTEGER start, end;
    QueryPerformanceCounter(&start);
   
    uint i = 0;
    for (r.cntAccesses = 0; r.cntAccesses < N; r.cntAccesses++) {
        buf[i]++;
        i = (i + step * delta) % size;
    }
    
    QueryPerformanceCounter(&end);

    for (int i = 0; i < size; i++)
        r.sum += buf[i];

    delete buf;
   
    r.totalTime = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    r.avgAccessTime = r.totalTime / r.cntAccesses;
    r.bandwidth = r.cntAccesses / r.totalTime;

    std::cout
        << std::right << std::setw(10) << std::setprecision(4) << r.totalTime * 1000 << " ms runtime,  "
        << std::right << std::setw(10) << r.cntAccesses << " accesses,  "
        << std::right << std::setw(10) << r.sum << " sum,  "
        << std::right << std::setw(5) << std::setprecision(3) << r.avgAccessTime * 1000000000 << " ns/access (average),  "
        << std::right << std::setw(6) << std::setprecision(2) << r.bandwidth / 1000000 << " GB/s (bandwidth)"
    << std::endl;

    return r;
}

#define sub(name, delta) result r##name = run(name##_SIZE, name##_STEP, delta, #name)

int main() {
    QueryPerformanceFrequency(&freq);
    std::cout << std::fixed;

    sub(A, 1);
    sub(B, DELTA);
    sub(C, DELTA);
    sub(D, DELTA);

    std::cout << std::endl
        << " t(L2) / t(L1) = " << std::setw(4) << std::setprecision(3) << rB.totalTime / rA.totalTime << std::endl
        << " t(L3) / t(L2) = " << std::setw(4) << std::setprecision(3) << rC.totalTime / rB.totalTime << std::endl
        << "t(RAM) / t(L3) = " << std::setw(4) << std::setprecision(3) << rD.totalTime / rC.totalTime << std::endl;

    return 0;
}
