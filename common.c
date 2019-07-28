#include <stdio.h>
#include <stdlib.h>

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
    fprintf(stderr, "%s failed in %s, line:%d", reason, __FILE__, __LINE__);
    exit(EXIT_FAILURE);
}