/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: tables.h  														   *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
***************************************************************************/

/* Copyright (C) 2007-2013 NHW Project
   Written by Raphael Canut - nhwcodec_at_gmail.com */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of NHW Codec, or NHW Project, nor the names of 
   specific contributors, may be used to endorse or promote products 
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEPTH1 354
#define ZONE1 110
#define UNZONE1 64
#define MSW 511


static char extra_table[ZONE1-1] = {
0,0,0,0,0,0,0,0,0,0,1,0,2,0,3,0,0,0,4,0,5,0,6,0,0,0,7,0,8,0,9,0,0,0,10,0, 
11,0,12,0,0,0,13,0,14,0,15,0,0,0,16,0,17,0,18,0,0,0,19,0, 
-1,0,-2,0,0,0,-3,0,-4,0,-5,0,0,0,-6,0,-7,0,-8,0,0,0,-9,0,-10,0, 
-11,0,0,0,-12,0,-13,0,-14,0,0,0,-15,0,-16,0,-17,0,0,0,-18,0,-19
};

static unsigned short nhw_table1[512] = {
1024, 0, 1537, 0, 1538, 0, 2053, 2054,
0, 0, 2051, 2052, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 2567, 
2568, 2569, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 3082, 3083, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 3596, 
3597, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 4110, 
4111, 4112, 4113, 4114, 4115, 4116, 4117, 4118, 
4119, 4120, 4121, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 4634, 
4635, 4636, 4637, 4638, 4639, 4640, 4641, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 4642, 
4643, 4644, 4645, 4646, 4647, 4648, 4649, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0};

static unsigned short nhw_table2[512] = {
5686, 0, 0, 0, 0, 0, 0, 0, 
5687, 0, 0, 0, 0, 0, 0, 0, 
6754, 0, 6755, 0, 6756, 0, 6757, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
6226, 0, 0, 0, 6227, 0, 0, 0, 
6228, 0, 0, 0, 6229, 0, 0, 0, 
6230, 0, 0, 0, 6231, 0, 0, 0, 
6232, 0, 0, 0, 6233, 0, 0, 0, 
5170, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5171, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5172, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5173, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5162, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5163, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5164, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5165, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5166, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5167, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5168, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5169, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 
5688, 0, 0, 0, 0, 0, 0, 0, 
5689, 0, 0, 0, 0, 0, 0, 0, 
7270, 7271, 7272, 7273, 7274, 7275, 7276, 7277, 
0, 0, 0, 0, 0, 0, 0, 0, 
6234, 0, 0, 0, 6235, 0, 0, 0, 
6236, 0, 0, 0, 6237, 0, 0, 0, 
6238, 0, 0, 0, 6239, 0, 0, 0, 
6240, 0, 0, 0, 6241, 0, 0, 0, 
5706, 0, 0, 0, 0, 0, 0, 0, 
5707, 0, 0, 0, 0, 0, 0, 0, 
5708, 0, 0, 0, 0, 0, 0, 0, 
5709, 0, 0, 0, 0, 0, 0, 0, 
5710, 0, 0, 0, 0, 0, 0, 0, 
5711, 0, 0, 0, 0, 0, 0, 0, 
5712, 0, 0, 0, 0, 0, 0, 0, 
5713, 0, 0, 0, 0, 0, 0, 0, 
5690, 0, 0, 0, 0, 0, 0, 0, 
5691, 0, 0, 0, 0, 0, 0, 0, 
5692, 0, 0, 0, 0, 0, 0, 0, 
5693, 0, 0, 0, 0, 0, 0, 0, 
5694, 0, 0, 0, 0, 0, 0, 0, 
5695, 0, 0, 0, 0, 0, 0, 0, 
5696, 0, 0, 0, 0, 0, 0, 0, 
5697, 0, 0, 0, 0, 0, 0, 0, 
5698, 0, 0, 0, 0, 0, 0, 0, 
5699, 0, 0, 0, 0, 0, 0, 0, 
5700, 0, 0, 0, 0, 0, 0, 0, 
5701, 0, 0, 0, 0, 0, 0, 0, 
5702, 0, 0, 0, 0, 0, 0, 0, 
5703, 0, 0, 0, 0, 0, 0, 0, 
5704, 0, 0, 0, 0, 0, 0, 0, 
5705, 0, 0, 0, 0, 0, 0, 0};


