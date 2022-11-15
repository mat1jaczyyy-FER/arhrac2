// indeksi_pojavljivanja
#include <iostream>

void c(int c, int* niz, int n, int* indeksi) {
    int i = 1;
    for (int j = 0; j < n; j++)
        if (niz[j] == c)
            indeksi[i++] = j;
    *indeksi = i - 1;
}

_declspec(naked) void a(int c, int* niz, int n, int* indeksi) {
    _asm {
            push    ebp
            mov     ebp, esp

            push    ebx
            push    esi

            mov     ebx, [ebp+12]
            mov     ecx, [ebp+20]
            mov     [ecx], 0
            mov     edx, ecx
            add     edx, 4
            mov     eax, 0

        check:
            cmp     eax, [ebp+16]
            jge     end

            mov     esi, [ebx]
            cmp     esi, [ebp+8]
            jne     next

            add     [ecx], 1
            mov     [edx], eax
            add     edx, 4

        next:
            add     ebx, 4
            inc     eax
            jmp     check

        end:
            pop     esi
            pop     ebx
            pop     ebp
            ret
    }
}

int main() {
    int niz[] = {0, 6, 1, -2, 3, 6, 0, 0, -9, 6};
    #define N (sizeof(niz) / sizeof(niz[0]))
    int indeksi[N + 1];

    memset(indeksi, 0, sizeof(indeksi));
    c(6, niz, N, indeksi);

    std::cout << "C++:";
    for (int i = 0; i < indeksi[0]; i++)
        std::cout << " " << indeksi[i + 1];
    std::cout << std::endl;

    memset(indeksi, 0, sizeof(indeksi));
    a(6, niz, N, indeksi);

    std::cout << "ASM:";
    for (int i = 0; i < indeksi[0]; i++)
        std::cout << " " << indeksi[i + 1];
    std::cout << std::endl;
}    
