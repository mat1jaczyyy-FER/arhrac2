#include <iostream>

_declspec(naked) int potprogram_asm() {
    _asm {
        mov     eax, 42
        mov     ebx, 0x42

        push    eax
        mov     eax, ecx
        mov     ecx, 0xFFFF0000
        mov     cx, ax
        pop     eax
        
        mov     dl, 0xFF
        ret
    }
}

int main() {
    int a = potprogram_asm();
    std::cout << a << std::endl;
}
