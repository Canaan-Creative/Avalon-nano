#include <stdio.h>

#define DATA_LEN    (22*4)

typedef unsigned int uint32_t;

unsigned char buf[DATA_LEN];

/*
 * gen 88 bytes data
 * */
unsigned int gen_test_a3233(uint32_t *buf){
    buf[0] = 0x11111111;
    buf[1] = 0x1;
    buf[2] = 0x00000000;
    buf[3] = 0x4ac1d001;
    buf[4] = 0x89517050;
    buf[5] = 0x087e051a;
    buf[6] = 0x06b168ae;
    buf[7] = 0x62a5f25c;
    buf[8] = 0x00639107;
    buf[9] = 0x13cdfd7b;
    buf[10] = 0xfa77fe7d;
    buf[11] = 0x9cb18a17;
    buf[12] = 0x65c90d1e;
    buf[13] = 0x8f41371d;
    buf[14] = 0x974bf4bb;
    buf[15] = 0x7145fd6d;
    buf[16] = 0xc44192c0;
    buf[17] = 0x12146495;
    buf[18] = 0xd8f8ef67;
    buf[19] = 0xa2cb45c1;
    buf[20] = 0x1bee2ba0;
    buf[21] = 0xaaaaaaaa;
    return buf[20] + 0x6000;
}

int main(){
    int bufindex=0;
    gen_test_a3233((uint32_t*)buf);

    for(;bufindex<DATA_LEN;bufindex++){
        printf("%02x",buf[bufindex]);
    }
    printf("\n");

    printf("unsiged char arry[]={");
    bufindex=0;
    for(;bufindex<DATA_LEN;bufindex++){
        printf("\\x%02x",buf[bufindex]);
    }
    printf("}\n");

   return 0;
}
