/*
 * FIPS 180-2 SHA-224/256/384/512 implementation
 * Last update: 02/02/2007
 * Issue date:  04/30/2005
 *
 * Copyright (C) 2013, Con Kolivas <kernel@kolivas.org>
 * Copyright (C) 2005, 2007 Olivier Gay <olivier.gay@a3.epfl.ch>
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
#include <stdint.h>
#include "sha2.h"

uint32_t sha256_k[64] =
            {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
             0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
             0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
             0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
             0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
             0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
             0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
             0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
             0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
             0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
             0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
             0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
             0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
             0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
             0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
             0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void sha256_loc(const unsigned char *buf, unsigned int *per_a, unsigned int *per_b)
{
    uint32_t w[64];
    uint32_t wv[8];
    uint32_t t1, t2;
    int j;
    uint32_t *buf_word_p = (uint32_t *)buf;
    for(j = 0; j < 3; j++){
	    w[j] = buf_word_p[13+j];
    }

    for (j = 0; j < 8; j++) {
        wv[j] = buf_word_p[j];
    }

    for (j = 0; j < 64; j++) {
        t1 = wv[7] + SHA256_F2(wv[4]) + CH(wv[4], wv[5], wv[6]) + sha256_k[j] + w[j];
        t2 = SHA256_F1(wv[0]) + MAJ(wv[0], wv[1], wv[2]);
        wv[7] = wv[6];
        wv[6] = wv[5];
        wv[5] = wv[4];
        wv[4] = wv[3] + t1;
        wv[3] = wv[2];
        wv[2] = wv[1];
        wv[1] = wv[0];
        wv[0] = t1 + t2;
		per_a[j] = wv[0];
		per_b[j] = wv[4];
        if (j == 2) {
    	    break;
        }
    }
}

void data_convert(uint8_t *data){
	uint8_t tmpval,index=0;

	for(index = 0; index < 16; index++){
		tmpval = data[index];
		data[index] = data[31-index];
		data[31-index] = tmpval;
	}

	for(index = 0; index < 6; index++){
		tmpval = data[52+index];
		data[52+index] = data[52+11-index];
		data[52+11-index] = tmpval;
	}
}

void data_pkg(const uint8_t *data, uint8_t *out)
{
	uint8_t work[44];
	uint8_t *t;
	unsigned int per_a[3];
	unsigned int per_b[3];
	sha256_loc(data , per_a, per_b);

	memcpy(work, data, 32);
	memcpy(work + 32, data + 52, 12); /* Parser the Icarus protocl data */

	t = out + 12;
	memcpy(t, work + 32, 12); /* Task data */
	memcpy(t + 12 + 0, (unsigned char *)&per_a[1], 4);	/* a1 */
	memcpy(t + 12 + 4, (unsigned char *)&per_a[0], 4);	/* a0 */
	memcpy(t + 12 + 8, (unsigned char *)&per_b[2], 4);	/* e2 */
	memcpy(t + 12 + 12, (unsigned char *)&per_b[1], 4);	/* e1 */
	memcpy(t + 12 + 16, (unsigned char *)&per_b[0], 4);	/* e0 */
	memcpy(t + 12 + 20, work, 32);	/* Midstate */
	memcpy(t + 12 + 52, (unsigned char *)&per_a[2], 4);	/* a2 */

	out[0] = 0x11;
	out[1] = 0x11;
	out[2] = 0x11;
	out[3] = 0x11;		/* Head */

	out[4] = 0x1;
	out[5] = 0x0;
	out[6] = 0x0;
	out[7] = 0x0;		/* PLL CFG0 */

	out[8] = 0x0;
	out[9] = 0x0;
	out[10] = 0x0;
	out[11] = 0x0;		/* PLL CFG1 */

	out[80] = 0x0;
	out[81] = 0x0;
	out[82] = 0x0;
	out[83] = 0x0;		/* Nonce */

	out[84] = 0xaa;
	out[85] = 0xaa;
	out[86] = 0xaa;
	out[87] = 0xaa;		/* Tail */
}
