#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"


__attribute__((section(".rodata.transmit"), aligned(0x10000))) const char ProbeTable[0x10000] = {'x'};       //64Kb
__attribute__((constructor)) void init() {
    int i;
    for (i = 0; i < sizeof(ProbeTable)/0x1000; i++)
        *(volatile char *) &ProbeTable[i*0x1000];
}

__asm__(".text\n.globl gadget\ngadget:\n"       //编到.text段，导出gadget符号
        "xorl %eax, %eax\n"                     //清空eax
        "movb (%rdx), %ah\n"                    //rdx可以被攻击者控制
        "movl ProbeTable(%eax), %eax\n"         //访存
        "retq\n");

char *banner = "  oggsa v1.0";
char secret[128]={'x'};

int main(int argc, char *argv[]){
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    int client_len;
    char buf[32];
    char send[32];

    if(argc != 2){
        fprintf(stderr, "Usage: victim secret");
        exit(EXIT_FAILURE);
    }

    strcpy(secret, argv[1]);           //拷贝机密字符串

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8888);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(server_sockfd, &server_address, sizeof(server_address));
    listen(server_sockfd, 5);

    while(1){
        long a;
        client_sockfd = accept(server_sockfd, &client_address, (socklen_t*)&client_len);
        while(read(client_sockfd, buf, 32)){
            sscanf(buf, "%ld\n", &a);
            sprintf(send, "%ld\n", a);
            write(client_sockfd, send, 16);
        }
        close(client_sockfd);
    }
    close(server_sockfd);
    exit(EXIT_SUCCESS);
}
