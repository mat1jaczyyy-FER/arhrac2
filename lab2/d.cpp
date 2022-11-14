#include <iostream>
#include <Windows.h>

void sum_c(float const* A, float const* B, int count, float* R) {
    for (int i = 0; i < count; i++) {
        R[i] = A[i] + B[i];
    }
}

_declspec(naked) void sum_x87(float const* A, float const* B, int count, float* R) {
    _asm {
                mov     eax, [esp+4]
                mov     ecx, [esp+8]
                mov     edx, [esp+12]
                mov     esi, [esp+16]

            start:
                fld     [eax]
                fadd    [ecx]
                fstp    [esi]
                
                add     eax, 4
                add     ecx, 4
                add     esi, 4

                dec     edx
                jnz     start

                ret
    }
}

_declspec(naked) void sum_sse(float const* A, float const* B, int count, float* R) {
    _asm {
                mov     eax, [esp+4]
                mov     ecx, [esp+8]
                mov     edx, [esp+12]
                mov     esi, [esp+16]

            start:
                movups  xmm0, [eax]
                movups   xmm1, [ecx]
                addps   xmm0, xmm1
                movups  [esi], xmm0

                add     eax, 16
                add     ecx, 16
                add     esi, 16

                sub     edx, 4
                jnz     start

                ret
    }
}

#define N 5000000
float A[N], B[N], R[N];

template <class Function>
void run(const char* name, Function&& f) {
	std::cout << name << ": ";
	
	LARGE_INTEGER freq, start, end;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);
    f();
	QueryPerformanceCounter(&end);   
    
    for (int i = 0; i < 10; i++) {
		std::cout << R[i] << " ";
	}
	std::cout << (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart << " ms" << std::endl;
}

int main() {
    for (int i = 0; i < N; i++) {
		A[i] = i;
		B[i] = i * 2;
	}

    run("C++", [&] { sum_c(A, B, N, R); });
    run("x87", [&] { sum_x87(A, B, N, R); });
    run("SSE", [&] { sum_sse(A, B, N, R); });
}
