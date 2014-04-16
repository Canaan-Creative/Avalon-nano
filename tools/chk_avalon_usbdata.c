/*
 * verify avalon usb data
 * Last update: 04/16/2014
 * Issue date:  04/16/2014
 *
 * Copyright (C) 2014, Mikeqin <Fengling.Qin@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#include <stdio.h>
#include <string.h>
#include "sha2.h"

#define ICA_DAT_LEN     (64*2)
#define A3233_DAT_LEN   (88)

/*
 * data wanted from convdata
 * */
unsigned char data_wanted[] ={"\x11\x11\x11\x11\x01\x00\x00\x00\x00\x00\x00\x00\x01\xd0\xc1\x4a\x50\x70\x51\x89\x1a\x05\x7e\x08\xae\x68\xb1\x06\x5c\xf2\xa5\x62\x07\x91\x63\x00\x7b\xfd\xcd\x13\x7d\xfe\x77\xfa\x17\x8a\xb1\x9c\x1e\x0d\xc9\x65\x1d\x37\x41\x8f\xbb\xf4\x4b\x97\x6d\xfd\x45\x71\xc0\x92\x41\xc4\x95\x64\x14\x12\x67\xef\xf8\xd8\xc1\x45\xcb\xa2\xa0\x2b\xee\x1b\xaa\xaa\xaa\xaa"};

int main(int argc,char *argv[]){
    unsigned int    buf_index = 0;
    unsigned char   a3233_datbuf[A3233_DAT_LEN];
    unsigned char   icarus_datbuf[ICA_DAT_LEN/2];
    unsigned char   cmd_buf[ICA_DAT_LEN+50];
    size_t          getbuflen = 0;
    FILE            *fpipe = NULL;
    
    if(argc != 2){
        printf("%s icarus_dat\n", argv[0]);
        printf("Release Date:%s\n", __DATE__);
        return 1;
    }

    if(strlen(argv[1]) != ICA_DAT_LEN){
        printf("icarus data len err!\n");    
        return 1;
    } 

    memset(cmd_buf,0,ICA_DAT_LEN+50);
    sprintf(cmd_buf, "echo -n %s | xxd -r -p", argv[1]); 

    fpipe = popen(cmd_buf,"r");
    if(NULL == fpipe){
        printf("open pipe failed!\n");
        return 1;
    }

    getbuflen = fread((void*)icarus_datbuf,sizeof(char),ICA_DAT_LEN/2,fpipe);
    if(getbuflen != (ICA_DAT_LEN/2)) {
        printf("convert a3233 buf failed!\n");
        pclose(fpipe);
        return 1;
    }
    pclose(fpipe);

    printf("icarus buf:\n");
    for(buf_index=0;buf_index<(ICA_DAT_LEN/2);buf_index++){
        printf("%02x", icarus_datbuf[buf_index]);
    }
    printf("\n");

    memset(a3233_datbuf,0,A3233_DAT_LEN);
    data_pkg(icarus_datbuf,a3233_datbuf);

    for(buf_index=0;buf_index<A3233_DAT_LEN;buf_index++){
        if(a3233_datbuf[buf_index] != data_wanted[buf_index]) {
            break; 
        }
    }

    if(buf_index == A3233_DAT_LEN){
        printf("data correct\n");
    }
    else{
        printf("data error\n");

        printf("data wanted:\n");
        for(buf_index=0;buf_index<A3233_DAT_LEN;buf_index++){
            printf("%02x",data_wanted[buf_index]);
        }
        printf("\n");

        printf("a3233 databuf:\n");
        for(buf_index=0;buf_index<A3233_DAT_LEN;buf_index++){
            printf("%02x",a3233_datbuf[buf_index]);
        }
        printf("\n");
    }

    return 0;
}

