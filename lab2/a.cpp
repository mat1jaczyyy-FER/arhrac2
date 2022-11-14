#include <iostream>

int potprogram_c(int a, int b, int c) {
    return (a + b) * c;
}

// ASM Tutorial
_declspec(naked) int potprogram_asm(int a, int b, int c) {
    _asm {
        /* cdecl prolog: */
        push    ebp             /* spremi ebp */
        mov     ebp, esp        /* ubaci esp u ebp */

        /* zauzmi 4 bajta za lokalne varijable: */
        sub     esp, 4          /* lokalne varijable su "ispod" ebp */

        /* glavni dio potprograma */
        /* [ebp] je pohranjena vrijednost ebp  */
        /* [ebp+4] je povratna adresa!         */
        mov     eax, [ebp+12]   /* b */
        add     eax, [ebp+8]    /* a */
        imul    eax, [ebp+16]   /* c */

        /* oslobodi lokalne varijable:*/
        mov     esp, ebp

        /* cdecl epilog: */
        pop     ebp             /* umjesto 'add esp,4, pop ebp' može biti 'leave'*/
        ret                     /* povratak iz potprograma */  
    }
}

// Optimized ASM
_declspec(naked) int potprogram_asm_noebp(int a, int b, int c) {
    _asm {
                                /* [esp] je povratna adresa! */
        mov     eax, [esp+8]    /* b */
        add     eax, [esp+4]    /* a */
        imul    eax, [esp+12]   /* c */
        ret                     /* povratak iz potprograma */  
    }
}

int main() {
    std::cout << "ASM: " << potprogram_asm(3, 5, 6) << std::endl;
    std::cout << "ASM_noebp: " << potprogram_asm_noebp(3, 5, 6) << std::endl;
    std::cout << "C++: " << potprogram_c(3, 5, 6) << std::endl;
}    
