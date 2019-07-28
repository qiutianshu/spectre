#include <stdio.h>


__asm__("loop:\n"
        "call 0x400680\n"
        "jmp loop\n");

int main(){
    exit(0);
}