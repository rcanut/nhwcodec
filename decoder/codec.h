/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: codec.h  														   *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06202012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -image parameters, wavelet orders, ... (beta version)		   *
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

#ifndef _CODEC_H
#define _CODEC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*==========================================*/

//ENTER YOUR MODE
//#define LOSSLESS
//#define LOSSY  

// IMAGE SIZE (PER COMPONENT)
#define IM_SIZE 65536

// IMAGE DIMENSION 
#define IM_DIM 256	

// QUALITY SETTING
#define HIGH3 9
#define HIGH2 8
#define HIGH1 7
#define NORM 6
#define LOW1 5
#define LOW2 4
#define LOW3 3
#define LOW4 2
#define LOW5 1
#define LOW6 0

/*==========================================*/

// COLORSPACE DEFINITION
#define RGB 0
#define YUV 1
#define R_COMP -56992-128 
#define G_COMP  34784-128
#define B_COMP -70688-128 

// WAVELET TYPE DEFINITION
#define WVLTS_53 0
#define WVLTS_97 1

#define WVLT_ENERGY_NHW 123

// MAIN STRUCTURES
typedef struct{
	unsigned char colorspace;
	unsigned char wavelet_type;
	unsigned char RES_HIGH;
	unsigned char RES_LOW;
	unsigned char wvlts_order;
	unsigned char quality_setting;
}codec_setup;

typedef struct{
	short *im_process;
	short *im_jpeg;
	unsigned char *im_bufferY;
	unsigned char *im_bufferU;
	unsigned char *im_bufferV;
	unsigned char *im_buffer4;
	short *im_nhw3;
	unsigned char *scale;
	codec_setup *setup;
}image_buffer;

typedef struct{
	unsigned long *packet1;
	unsigned long *packet2;
	int d_size_data1;
	int d_size_data2;
	unsigned short nhw_res1_len;
	unsigned short nhw_res3_len;
	unsigned short nhw_res4_len;
	unsigned short nhw_res5_len;
	unsigned long nhw_res6_len;
	unsigned short nhw_res1_bit_len;
	unsigned short nhw_res2_bit_len;
	unsigned short nhw_res3_bit_len;
	unsigned short nhw_res5_bit_len;
	unsigned short nhw_res6_bit_len;
	unsigned short nhw_select1;
	unsigned short nhw_select2;
	unsigned char *nhw_res1;
	unsigned char *nhw_res3;
	unsigned char *nhw_res4;
	unsigned char *nhw_res5;
	unsigned char *nhw_res6;
	unsigned char *nhw_res1_bit;
	unsigned char *nhw_res1_word;
	unsigned char *nhw_res3_bit;
	unsigned char *nhw_res3_word;
	unsigned char *nhw_res5_bit;
	unsigned char *nhw_res5_word;
	unsigned char *nhw_res6_bit;
	unsigned char *nhw_res6_word;
	unsigned long *nhwresH3;
	unsigned long *nhwresH4;
	unsigned char *nhw_select_word1;
	unsigned char *nhw_select_word2;
	unsigned short *nhw_char_res1;
	unsigned short nhw_char_res1_len;
	unsigned short res_f1;
	unsigned short res_f2;
	unsigned short d_size_tree1;
	unsigned short d_size_tree2;
	unsigned char *d_tree1;
	unsigned char *d_tree2;
	unsigned short exw_Y_end;
	unsigned short end_ch_res;
	unsigned short qsetting3_len;
	unsigned long *high_qsetting3;
	unsigned short highres_comp_len;
	unsigned char *highres_comp;
	unsigned short tree_end;
	unsigned short *book;
	unsigned long *resolution;
	unsigned char *exw_Y;
	unsigned char *res_U_64;
	unsigned char *res_V_64;
	unsigned char *res_ch;
	unsigned char *res_comp;
}decode_state;


extern void decode_image(image_buffer *im,decode_state *os,char **argv);

extern int parse_file(image_buffer *imd,decode_state *os,char **argv);
extern int write_image_decompressed(char **argv,image_buffer *im);

extern void wavelet_synthesis(image_buffer *im,int norder,int last_stage,int Y);
extern void wavelet_synthesis2(image_buffer *im,decode_state *os,int norder,int last_stage,int Y);
extern void upfilter53(short *x,int M,short *res);
extern void upfilter53I(short *x,int M,short *res);
extern void upfilter53III(short *x,int M,short *res);
extern void upfilter53VI(short *x,int M,short *res);
extern void upfilter53VI_III(short *x,int M,short *res);
extern void upfilter53VI_IV(short *x,int M,short *res);
extern void upfilter53VI_V(short *x,int M,short *res);
extern void upfilter53II(short *_X,int M,short *_RES);
extern void upfilter53IV(short *_X,int M,short *_RES);
extern void upfilter53V(short *_X,int M,short *_RES);
extern void upfilter97(short *x,int M,int E,short *res);

extern void retrieve_pixel_Y_comp(image_buffer *imd,decode_state *os,int p1,unsigned long *d1,short *im3);
extern void retrieve_pixel_UV_comp(image_buffer *imd,decode_state *os,int p1,unsigned long *d1,short *im3);

#endif


