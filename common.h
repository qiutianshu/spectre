#include <stdio.h>
#include <stdlib.h>

//#define THRESHOLD 350

unsigned long get_long(char *input){
    char *end;
    int res = strtoul(input, &end, 0);
    if(*end != '\0'){
        fprintf(stderr, "%s translate error!", input);
        exit(EXIT_FAILURE);
    }
    return res;
}

void error_log(char *reason){
    fprintf(stderr, "%s failed ", reason);
    exit(EXIT_FAILURE);
}

void evict(void *ptr) {
    static char *space = NULL;
    int fd;
    if (space == NULL) {
        space = mmap(NULL, 0x4000000, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
//        check("mmap eviction space");
/*        fd = open("/dev/zero", O_RDWR);
        if(fd == -1){
            perror("open null");
            exit(EXIT_FAILURE);
        }
        space = mmap(NULL, 0x4000000, PROT_READ, MAP_PRIVATE, fd, 0);*/
        if(space == MAP_FAILED){
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }

    unsigned long off = ((unsigned long) ptr) & 0xfff;          //取低12位，确定cache-set
    volatile char *ptr1 = space + off;
    volatile char *ptr2 = ptr1 + 0x2000;                        //两次刷新
    for (int i = 0; i < 4000; i++) {
        *ptr2;
        *ptr1;                  //替换got所在的cache-set
        ptr2 += 0x1000;
        ptr1 += 0x1000;
    }
}