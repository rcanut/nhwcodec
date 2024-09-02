/***************************************************************************
****************************************************************************
*  NHW Image Codec														   *
*  file: codec.h											               *
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

#define LOSSY  

// IMAGE SIZE (PER COMPONENT)
#define IM_SIZE 65536

// IMAGE DIMENSION 
#define IM_DIM 256

//QUALITY SETTINGS
#define HIGH3 23
#define HIGH2 22
#define HIGH1 21
#define NORM 20
#define LOW1 19
#define LOW2 18
#define LOW3 17
#define LOW4 16
#define LOW5 15
#define LOW6 14
#define LOW7 13
#define LOW8 12
#define LOW9 11
#define LOW10 10
#define LOW11 9
#define LOW12 8
#define LOW13 7
#define LOW14 6
#define LOW15 5
#define LOW16 4
#define LOW17 3
#define LOW18 2
#define LOW19 1
#define LOW20 0

/*==========================================*/

// COLORSPACE DEFINITION
#define RGB 0
#define YUV 1

// WAVELET TYPE DEFINITION
#define WVLTS_53 0
#define WVLTS_97 1

#define WVLT_ENERGY_NHW 123

//#define NHW_BOOKS

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
	unsigned char *im_bufferU;
	unsigned char *im_bufferV;
	unsigned char *im_buffer4;
	unsigned char *im_nhw;
	short *im_wavelet_first_order;
	short *im_quality_setting;
	short *im_wavelet_band;
	codec_setup *setup;
}image_buffer;

typedef struct{
	unsigned int *encode;
	unsigned char *tree1;
	unsigned char *tree2;
	unsigned short nhw_res1_len;
	unsigned short nhw_res3_len;
	unsigned short nhw_res4_len;
	unsigned short nhw_res5_len;
	unsigned int nhw_res6_len;
	unsigned short nhw_res1_word_len;
	unsigned short nhw_res3_word_len;
	unsigned short nhw_res5_word_len;
	unsigned short nhw_res6_word_len;
	unsigned short nhw_res1_bit_len;
	unsigned short nhw_res3_bit_len;
	unsigned short nhw_res5_bit_len;
	unsigned short nhw_res6_bit_len;
	unsigned char *nhw_res1;
	unsigned char *nhw_res3;
	unsigned char *nhw_res4;
	unsigned char *nhw_res5;
	unsigned char *nhw_res6;
	unsigned char *nhw_res1_bit;
	unsigned char *nhw_res3_bit;
	unsigned char *nhw_res5_bit;
	unsigned char *nhw_res6_bit;
	unsigned char *nhw_res1_word;
	unsigned char *nhw_res3_word;
	unsigned char *nhw_res5_word;
	unsigned char *nhw_res6_word;
	unsigned short *nhw_char_res1;
	unsigned short nhw_char_res1_len;
	unsigned short nhw_select1;
	unsigned short nhw_select2;
	unsigned char *nhw_select_word1;
	unsigned char *nhw_select_word2;
	int size_data1;
	int size_data2;
	unsigned short size_tree1;
	unsigned short size_tree2;
	unsigned short tree_end;
	unsigned short Y_res_comp;
	unsigned short exw_Y_end;
	unsigned short end_ch_res;
	unsigned short qsetting3_len;
	unsigned int *high_qsetting3;
	unsigned short highres_mem_len;
	unsigned short highres_comp_len;
	unsigned short *highres_mem;
	unsigned char *highres_comp;
	unsigned char *highres_word;
	unsigned char *res_U_64;
	unsigned char *res_V_64;
	unsigned char *exw_Y;
	unsigned char *ch_res;
	unsigned int *high_res;
}encode_state;


extern void encode_image(image_buffer *im,encode_state *enc, int ratio);

extern int read_image_bmp(char *file_name, encode_state *os, image_buffer *im, int rate);
extern int write_compressed_file(image_buffer *im,encode_state *enc,char *file_name);

extern void downsample_YUV420(image_buffer *im, int rate);

extern void wavelet_analysis(image_buffer *im,int norder,int last_stage,int Y);
extern void wavelet_synthesis(image_buffer *im,int norder,int last_stage,int Y);
extern void downfilter53(short *x,int N,int decalage,short *res);
extern void downfilter53II(short *x,int N,int decalage,short *res);
extern void downfilter53IV(short *x,int N,int decalage,short *res);
extern void downfilter53VI(short *x,int N,int decalage,short *res);
extern void downfilter97(short *x,int N,int decalage,short *res);
extern void upfilter53(short *x,int M,short *res);
extern void upfilter53I(short *x,int M,short *res);
extern void upfilter53III(short *x,int M,short *res);
extern void upfilter53VI(short *x,int M,short *res);
extern void upfilter53II(short *_X,int M,short *_RES);
extern void upfilter97(short *_X,int M,int E,short *_RES);

extern void pre_processing(image_buffer *im);
extern void pre_processing_UV(image_buffer *im);
extern void block_variance_avg(image_buffer *im);
extern void offsetY(image_buffer *im,int m1);
extern void offsetY_recons256(image_buffer *im, encode_state *enc, int m1, int part);
extern void im_recons_wavelet_band(image_buffer *im);
extern void wavelet_synthesis_high_quality_settings(image_buffer *im,encode_state *enc);
extern void offsetUV_recons256(image_buffer *im, int m1, int comp);
extern void offsetUV(image_buffer *im,int m2);
extern void quantizationY(image_buffer *im);
extern void quantizationUV(image_buffer *im);

extern void Y_highres_compression(image_buffer *im,encode_state *enc);
extern void highres_compression(image_buffer *im,encode_state *enc);
extern int wavlts2packet(image_buffer *im,encode_state *enc);

#endif


