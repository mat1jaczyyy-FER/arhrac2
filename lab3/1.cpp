#include <iostream>

#define NUM 0x12345678

int main() {
    unsigned char buf[4];
    *((int*)buf) = NUM;

    if (buf[0] == (NUM & 0xFF)) {
        std::cout << "little";
    } else if (buf[3] == (NUM & 0xFF)) {
        std::cout << "big";
    }
    std::cout << "\t" << std::hex;
    
    for (int i = 0; i < 4; i++) {
        std::cout << (int)buf[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}
