#include <iostream>

int c(int n) {
    int s = 0;
    for (int i = 0; i < n; i++) {
        s += i;
    }
    return s;
}

_declspec(naked) int a(int n) {
    _asm {
            mov     ecx, 0          // counter
            mov     eax, 0          // sum
            mov     edx, [esp+4]    // limit

        check:
            cmp     ecx, edx        // counter < limit
            jge     end             // if not, jump to end

            add     eax, ecx    // sum += counter
            inc     ecx         // counter++
            jmp     check       // jump to check

        end:
            ret
    }
}

int main() {
    while (1) {
        int n;
        std::cout << "n = ";
        std::cin >> n;
        std::cout << "C++: " << c(n) << std::endl;
        std::cout << "ASM: " << a(n) << std::endl;
    }
}    
