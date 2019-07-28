#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define A_V "av_fifo"               //输入管道名
#define V_A "va_fifo"               //输出管道名

__attribute__((section(".rodata.transmit"), aligned(0x10000))) const char ProbeTable[0x10000] = {'x'};       //64Kb

__asm__(".text\n.globl gadget\ngadget:\n"       //编到.text段，导出gadget符号
        "xorl %eax, %eax\n"                     //清空eax
        "movb (%rdx), %ah\n"                    //rdx可以被攻击者控制
        "movl ProbeTable(%eax), %eax\n"     //访存
        "retq\n");

char *banner = "  oggsa v1.0";
char secret[128]={'x'};

int main(int argc, char *argv[]){
    int pip_av, pip_va;
    char buffer[1024];
    int res;
    char *end;

    memset(buffer, '\0', sizeof(buffer));

    if(argc != 2){
        fprintf(stderr, "Usage: victim secret");
        exit(EXIT_FAILURE);
    }

    strcpy(secret, argv[1]);           //拷贝机密字符串

    pip_av = open(A_V, O_RDONLY);      //读、阻塞模式打开输入管道
    pip_va = open(V_A, O_WRONLY);      //写、阻塞模式打开输出管道

    
    if((pip_av != -1) && (pip_va != -1)){
        do{
            memset(buffer, '\0', sizeof(buffer));
            read(pip_av, buffer, 1024);   //循环读取管道
            res = strtoul(buffer, &end, 0);
            if(*end != '\0'){
                fprintf(stderr, "error in %s, line: %d", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            printf("0x%x\n", res);
            sprintf(buffer, "0x%x %s", res, banner);           //rdx寄存器的值为res被攻击者控制
            write(pip_va, buffer, strlen(buffer));
        }while(1);
    }
    else{
        fprintf(stderr, "Open fifo failed");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}