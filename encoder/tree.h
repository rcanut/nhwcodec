/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: tree.h  														   *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  rmk: -huffman tree & lengths										       *
*       -a second huffman tree is required for certains images (next rev)  *
*       -a third tree is required for low resolution...			           *
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

#define DEPTH 290

static unsigned char extra_words1[19]={10,12,14,18,20,22,26,28,30,34,36,38,42,44,46,50,52,54,58};
static unsigned char extra_words2[19]={60,62,66,68,70,74,76,78,82,84,86,90,92,94,98,100,102,106,108};


static unsigned long huffman_tree[DEPTH]={
	0x0000,0x0002,0x0004,0x000a,
	0x000b,0x0006,0x0007,0x0018,
	0x0019,0x001a,0x0036,0x0037,
	0x0070,0x0071,0x00e8,0x00e9,
	0x00ea,0x00eb,0x00ec,0x00ed,
	0x00ee,0x00ef,0x00f0,0x00f1,
	0x00f2,0x00f3,0x01c8,0x01c9,
	0x01ca,0x01cb,0x01cc,0x01cd,
	0x01ce,0x01cf,0x01e8,0x01e9,
	0x01ea,0x01eb,0x01ec,0x01ed,
	0x01ee,0x01ef,0x03e8,0x03e9,
	0x03ea,0x03eb,0x03ec,0x03ed,
	0x03ee,0x03ef,0x03e4,0x03e5,
	0x03e6,0x03e7,0x07c0,0x07c1,
	0x07e0,0x07e1,0x07f0,0x07f1,
	0x07f2,0x07f3,0x07f4,0x07f5,
	0x07f6,0x07f7,0x07f8,0x07f9,
	0x07fa,0x07fb,0x07fc,0x07fd,
	0x07fe,0x07ff,0x07e8,0x07e9,
	0x07ea,0x07eb,0x07ec,0x07ed,
	0x07ee,0x07ef,0x0f88,0x0f89,
	0x0f8a,0x0f8b,0x0f8c,0x0f8d,
	0x0f8e,0x0f8f,0x0fc8,0x0fc9,
	0x0fca,0x0fcb,0x0fcc,0x0fcd,
	0x0fce,0x0fcf,0x1f08,0x1f09,
	0x1f0a,0x1f0b,0x3f10,0x3f11,
	0x3f12,0x3f13,0x3f14,0x3f15,
	0x3f16,0x3f17,0x1f0c0,0x1f0c1,
	0x1f0c2,0x1f0c3,0x1f0c4,0x1f0c5,
	0x1f0c6,0x1f0c7,0x1f0c8,0x1f0c9,
	0x1f0ca,0x1f0cb,0x1f0cc,0x1f0cd,
	0x1f0ce,0x1f0cf,0x1f0d0,0x1f0d1,
	0x1f0d2,0x1f0d3,0x1f0d4,0x1f0d5,
	0x1f0d6,0x1f0d7,0x1f0d8,0x1f0d9,
	0x1f0da,0x1f0db,0x1f0dc,0x1f0dd,
	0x1f0de,0x1f0df,0x1f0e0,0x1f0e1,
	0x1f0e2,0x1f0e3,0x1f0e4,0x1f0e5,
	0x1f0e6,0x1f0e7,0x1f0e8,0x1f0e9,
	0x1f0ea,0x1f0eb,0x1f0ec,0x1f0ed,
	0x1f0ee,0x1f0ef,0x1f0f0,0x1f0f1,
	0x1f0f2,0x1f0f3,0x1f0f4,0x1f0f5,
	0x1f0f6,0x1f0f7,0x1f0f8,0x1f0f9,
	0x1f0fa,0x1f0fb,0x1f0fc,0x1f0fd,
	0x1f0fe,0x1f0ff,0x1f8c0,0x1f8c1,
	0x1f8c2,0x1f8c3,0x1f8c4,0x1f8c5,
	0x1f8c6,0x1f8c7,0x1f8c8,0x1f8c9,
	0x1f8ca,0x1f8cb,0x1f8cc,0x1f8cd,
	0x1f8ce,0x1f8cf,0x1f8d0,0x1f8d1,
	0x1f8d2,0x1f8d3,0x1f8d4,0x1f8d5,
	0x1f8d6,0x1f8d7,0x1f8d8,0x1f8d9,
	0x1f8da,0x1f8db,0x1f8dc,0x1f8dd,
	0x1f8de,0x1f8df,0x1f8e0,0x1f8e1,
	0x1f8e2,0x1f8e3,0x1f8e4,0x1f8e5,
	0x1f8e6,0x1f8e7,0x1f8e8,0x1f8e9,
	0x1f8ea,0x1f8eb,0x1f8ec,0x1f8ed,
	0x3f1dc,0x3f1dd,0x3f1de,0x3f1df,
	0x3f1e0,0x3f1e1,0x3f1e2,0x3f1e3,
	0x3f1e4,0x3f1e5,0x3f1e6,0x3f1e7,
	0x7e3d0,0x7e3d1,0x7e3d2,0x7e3d3,
	0x7e3d4,0x7e3d5,0x7e3d6,0x7e3d7,
	0x7e3d8,0x7e3d9,0x7e3da,0x7e3db,
	0x7e3dc,0x7e3dd,0x7e3de,0x7e3df,
	0x7e3e0,0x7e3e1,0x7e3e2,0x7e3e3,
	0x7e3e4,0x7e3e5,0x7e3e6,0x7e3e7,
	0x7e3e8,0x7e3e9,0x7e3ea,0x7e3eb,
	0x7e3ec,0x7e3ed,0x7e3ee,0x7e3ef,
	0x7e3f0,0x7e3f1,0x7e3f2,0x7e3f3,
	0x7e3f4,0x7e3f5,0xfc7ec,0xfc7ed,0xfc7ee,0xfc7ef,
	0xfc7f0,0xfc7f1,0xfc7f2,0xfc7f3,0xfc7f4,0xfc7f5,0xfc7f6,0xfc7f7,
	0xfc7f8,0xfc7f9,0xfc7fa,0xfc7fb,0xfc7fc,0xfc7fd,0xfc7fe,0xfc7ff};

static unsigned char len[DEPTH]={
	2,3,3,4,4,4,4,5,5,5,6,6,7,7,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
	13,13,13,13,14,14,14,14,14,14,14,14,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
	17,17,17,17,17,17,17,17,17,17,18,18,18,18,18,18,18,18,18,18,18,18,19,19,19,19,19,19,
	19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,
	19,19,19,19,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20};


static char extra_table[109] = {
0,0,0,0,0,0,0,0,0,0,1,0,2,0,3,0,0,0,4,0,5,0,6,0,0,0,7,0,8,0,9,0,0,0,10,0, 
11,0,12,0,0,0,13,0,14,0,15,0,0,0,16,0,17,0,18,0,0,0,19,0, 
-1,0,-2,0,0,0,-3,0,-4,0,-5,0,0,0,-6,0,-7,0,-8,0,0,0,-9,0,-10,0, 
-11,0,0,0,-12,0,-13,0,-14,0,0,0,-15,0,-16,0,-17,0,0,0,-18,0,-19
};
