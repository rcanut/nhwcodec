/***************************************************************************
****************************************************************************
*  NHW Image Codec                                                         *
*  file: nhw_decoder_cli.c                                                 *
*  version: 0.3.4+8                                                      *
*  last update: $ 10232026 nhw exp $                                       *
*                                                                          *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -simple codec                                                   *
***************************************************************************/

/* Copyright (C) 2007-2022 NHW Project
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec.h"
#include "nhw_decoder.h"

#define PROGRAM "nhw-dec"
#define VERSION "0.3.4+8"

int setup_bmp_header(char* bmp_header, nhw_image_s* im_ctx);
void convert_tile_rgb(image_buffer *im, unsigned char *iNHW);
void init_color_tables(void);
int write_image_bmp(unsigned char *image, nhw_image_s *im_ctx, int stride, char *file_name);
void show_usage();

/* tiled container written by nhw-enc for images other than 512x512 */
#define NHWT_MAGIC "NHWT"

unsigned char bmp_header[54]={66,77,54,0,12,0,0,0,0,0,
				54,0,0,0,40,0,0,0,0,2,
				0,0,0,2,0,0,1,0,24,0,
				0,0,0,0,0,0,12,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int main(int argc, char **argv)
{
	image_buffer im;
	decode_state dec;
	ntile nt1;
	nhw_image_s im_ctx;
	char *ifname, *ofname;
	FILE *compressed_file;
	unsigned char *image;
	char magic[4];

	if (argc<3)
	{
		show_usage();
		return 0;
	}
	ifname = argv[1];
	ofname = argv[2];
	init_color_tables();

	compressed_file=fopen(ifname,"rb");
	if (compressed_file==NULL)
	{
		printf("\nCould not open file\n");
		exit(-1);
	}

	if (fread(magic,4,1,compressed_file)==1 && memcmp(magic,NHWT_MAGIC,4)==0)
	{
		/* tiled container: decode each 512x512 tile into a padded canvas, then crop */
		uint32_t width, height, len;
		int tiles_x, tiles_y, ntiles, cstride, tstride=3*2*IM_DIM;
		long *offset, file_size;

		if (fread(&width,4,1,compressed_file)!=1 || fread(&height,4,1,compressed_file)!=1
			|| width==0 || height==0 || width>65535 || height>65535)
		{
			printf("\nInvalid tiled .nhw header\n");
			exit(-1);
		}
		
		nt1.width = width;
		nt1.height = height;
		
		tiles_x=(width+2*IM_DIM-1)/(2*IM_DIM);
		tiles_y=(height+2*IM_DIM-1)/(2*IM_DIM);
		ntiles=tiles_x*tiles_y;
		cstride=tiles_x*tstride;

		image=(unsigned char*)malloc((size_t)cstride*tiles_y*2*IM_DIM);
		offset=(long*)malloc(ntiles*sizeof(long));
		if (image==NULL || offset==NULL)
		{
			printf("\nNot enough memory\n");
			exit(-1);
		}

		/* find where each tile stream starts */
		fseek(compressed_file, 0, SEEK_END);
		file_size=ftell(compressed_file);
		fseek(compressed_file, 12, SEEK_SET);
		for (int t=0; t<ntiles; t++)
		{
			if (fread(&len,4,1,compressed_file)!=1 || ftell(compressed_file)+(long)len>file_size)
			{
				printf("\nTruncated .nhw file\n");
				exit(-1);
			}
			offset[t]=ftell(compressed_file);
			fseek(compressed_file, len, SEEK_CUR);
		}
		fclose(compressed_file);

		/* decode the tiles in parallel (when built with OpenMP), each from its own file handle */
		#pragma omp parallel for schedule(dynamic)
		for (int t=0; t<ntiles; t++)
		{
			image_buffer tim;
			decode_state tdec;
			ntile nt2;
			unsigned char *tile=(unsigned char*)malloc(3*4*IM_SIZE*sizeof(char));
			FILE *tile_file=fopen(ifname,"rb");

			if (tile==NULL || tile_file==NULL)
			{
				printf("\nCould not decode tile\n");
				exit(-1);
			}
		
			memset(&tim,0,sizeof(tim));
			memset(&tdec,0,sizeof(tdec));
			memset(&nt2,0,sizeof(nt2));
			fseek(tile_file, offset[t], SEEK_SET);
			nhw_srand(1);
			
			nt2.width = width;
			nt2.height = height;
			
			decode_image(&tim, &tdec, &nt2, tile_file);
			fclose(tile_file);
			convert_tile_rgb(&tim, tile);
			free(tim.setup);
			for (int y=0; y<2*IM_DIM; y++)
			{
				memcpy(image+(size_t)((t/tiles_x)*2*IM_DIM+y)*cstride+(t%tiles_x)*tstride, tile+y*tstride, tstride);
			}
			free(tile);
		}
		free(offset);

		im_ctx.width = width;
		im_ctx.height = height;
		im_ctx.bpp = 24;
		setup_bmp_header((char*)bmp_header, &im_ctx);
		write_image_bmp(image, &im_ctx, cstride, ofname);
		free(image);

		return 0;
	}
	rewind(compressed_file);

	/* Decode Image */
	decode_image(&im, &dec, &nt1, compressed_file); // todo: setup image context
	fclose(compressed_file);

	image=(unsigned char*)malloc(3*4*IM_SIZE*sizeof(char));
	convert_tile_rgb(&im, image);

	/* this should come from image to convert */
	im_ctx.width = 512;
	im_ctx.height = 512;
	im_ctx.bpp = 24;
	setup_bmp_header((char*)bmp_header, &im_ctx);
	write_image_bmp(image, &im_ctx, 3*2*IM_DIM, ofname);
	free(image);

	return 0;
} // main

void show_usage()
{
	fprintf(stdout,
	"Usage: %s <image.nhw> <image.bmp>\n"
	"Convert image: nwh to bmp\n"
	" (to a 24-bit bitmap color image)\n"
	"\n"
	"  example: nhw-dec image.nhw image.bmp\n"
	"Copyright (C) 2007-2022 NHW Project (Raphael C.)\n",
	PROGRAM);
}


/* write the bottom-left width x height region of image (stride bytes per row), rows padded to mod 4 */
int write_image_bmp(unsigned char *image, nhw_image_s *im_ctx, int stride, char *file_name)
{
	FILE* output_image_file = fopen(file_name,"wb");
	static const unsigned char pad[3]={0,0,0};
	int y, row=im_ctx->width*3;

	if (output_image_file == NULL)
	{
		printf("Failed to open output decompressed .bmp file %s\n",file_name);
		return -1;
	}

	// WRITE DECODED DATA
	setvbuf(output_image_file,NULL,_IOFBF,1<<20);	/* 1 MB buffer: far fewer write calls than one per row */
	fwrite(bmp_header,54,1,output_image_file);
	for (y=0;y<im_ctx->height;y++)
	{
		fwrite(image+(size_t)y*stride,row,1,output_image_file);
		if (row&3) fwrite(pad,4-(row&3),1,output_image_file);
	}

	fclose(output_image_file);
	return 0;
}

/* Lookup tables for the NORM conversion below. With Y an integer, (int)(Y + x + 0.5f) equals
   Y + (int)(255 + x + 0.5f) - 255 except when x + 0.5 is exactly an integer (a tie), where the
   double rounding can depend on Y; those G entries hold G_TIE and use the original formula.
   Checked against the formula for all 2^24 Y,U,V inputs: identical. */
#define G_TIE (-32768)
static int r_ofs[256], b_ofs[256];
static short g_ofs[256*256];
static unsigned char clip_tab[1024];

static int is_tie(double x)
{
	double t=x+1000.5, f=t-(int)t;
	return f<1e-6 || f>1-1e-6;
}

void init_color_tables(void)
{
	int i,U,V;

	for (i=0;i<1024;i++) clip_tab[i]=(i<512) ? 0 : ((i>767) ? 255 : i-512);
	for (V=-128;V<128;V++) r_ofs[V+128]=(int)(255 + 1.402*V +0.5f)-255;
	for (U=-128;U<128;U++) b_ofs[U+128]=(int)(255 +1.772*U +0.5f)-255;
	for (U=-128;U<128;U++) for (V=-128;V<128;V++)
	{
		if (is_tie(-0.34414*U -0.71414*V)) g_ofs[(U+128)*256+V+128]=G_TIE;
		else g_ofs[(U+128)*256+V+128]=(int)(255 -0.34414*U -0.71414*V +0.5f)-255;
	}
}

/* convert one decoded 512x512 YUV tile to bottom-up BGR rows in iNHW (3*4*IM_SIZE bytes) */
void convert_tile_rgb(image_buffer *im, unsigned char *iNHW)
{
	int i,Y,U,V,R,G,B,m,t;
	unsigned char *icolorY,*icolorU,*icolorV;
	float Y_q_setting,Y_inv;

	// here to work on Windows Vista
	icolorY=(unsigned char*)im->im_bufferY;
	icolorU=(unsigned char*)im->im_bufferU;
	icolorV=(unsigned char*)im->im_bufferV;

	if (im->setup->quality_setting>=NORM)
	{
		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=3*m*IM_SIZE;i<(m+1)*IM_SIZE;i++,t+=3)
			{
				//Y = icolorY[i]*298;
				Y = icolorY[i]+512;
				U = icolorU[i];
				V = icolorV[i];

				//Matrix  YCbCr (or YUV) to RGB, from the tables in init_color_tables
				G = g_ofs[(U<<8)+V];
				iNHW[t]=clip_tab[Y+r_ofs[V]];
				if (G!=G_TIE) iNHW[t+1]=clip_tab[Y+G];
				else
				{
					G = (int)(icolorY[i]  -0.34414*(U-128) -0.71414*(V-128) +0.5f);
					if ((G>>8)!=0) iNHW[t+1]=( (G<0) ? 0 : 255 );
					else iNHW[t+1]=G;
				}
				iNHW[t+2]=clip_tab[Y+b_ofs[U]];
			}
		}
	}
	else if (im->setup->quality_setting==LOW1 || im->setup->quality_setting==LOW2)
	{
		if (im->setup->quality_setting==LOW1) Y_inv=1.025641; // 1/0.975
		else if (im->setup->quality_setting==LOW2) Y_inv=1.075269; // 1/0.93

		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=3*m*IM_SIZE;i<(m+1)*IM_SIZE;i++,t+=3)
			{
				//Y = icolorY[i]*298;
				Y_q_setting =(float)(icolorY[i]*Y_inv);
				U = icolorU[i]-128;
				V = icolorV[i]-128;

				//Matrix  YCbCr (or YUV) to RGB
				/*R = ((Y         + 409*V + R_COMP)>>8);
				G = ((Y - 100*U - 208*V + G_COMP)>>8);
				B = ((Y + 516*U         + B_COMP)>>8);*/
				R = (int)(Y_q_setting  + 1.402*V +0.5f);
				G = (int)(Y_q_setting  -0.34414*U -0.71414*V +0.5f);
				B = (int)(Y_q_setting  +1.772*U +0.5f);

				//Clip RGB Values
				if ((R>>8)!=0) iNHW[t]=( (R<0) ? 0 : 255 );
				else iNHW[t]=R;

				if ((G>>8)!=0) iNHW[t+1]=( (G<0) ? 0 : 255 );
				else iNHW[t+1]=G;

				if ((B>>8)!=0) iNHW[t+2]=( (B<0) ? 0 : 255 );
				else iNHW[t+2]=B;
			}
		}
	}
	else if (im->setup->quality_setting==LOW3)
	{
		Y_inv=1.063830; // 1/0.94

		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=3*m*IM_SIZE;i<(m+1)*IM_SIZE;i++,t+=3)
			{
				//Y = icolorY[i]*298;
				Y = icolorY[i];
				U = icolorU[i]-128;
				V = icolorV[i]-128;

				//Matrix  YCbCr (or YUV) to RGB
				/*R = ((Y         + 409*V + R_COMP)>>8);
				G = ((Y - 100*U - 208*V + G_COMP)>>8);
				B = ((Y + 516*U         + B_COMP)>>8);*/
				R = (int)((Y + 1.402*V)*Y_inv +0.5f);
				G = (int)((Y -0.34414*U -0.71414*V)*Y_inv +0.5f);
				B = (int)((Y +1.772*U)*Y_inv +0.5f);

				//Clip RGB Values
				if ((R>>8)!=0) iNHW[t]=( (R<0) ? 0 : 255 );
				else iNHW[t]=R;

				if ((G>>8)!=0) iNHW[t+1]=( (G<0) ? 0 : 255 );
				else iNHW[t+1]=G;

				if ((B>>8)!=0) iNHW[t+2]=( (B<0) ? 0 : 255 );
				else iNHW[t+2]=B;
			}
		}
	}
	else if (im->setup->quality_setting<LOW3)
	{
		if (im->setup->quality_setting==LOW4) Y_inv=1.012139; // 1/0.94
		else if (im->setup->quality_setting==LOW5) Y_inv=1.048174; // 1/0.906
		else if (im->setup->quality_setting==LOW6) Y_inv=1.138331; // 1/0.8
		else if (im->setup->quality_setting==LOW7) Y_inv=1.186945;
		else if (im->setup->quality_setting==LOW8) Y_inv=1.177434;
		else if (im->setup->quality_setting==LOW9) Y_inv=1.190611;
		else if (im->setup->quality_setting==LOW10) Y_inv=1.281502;
		else if (im->setup->quality_setting==LOW11) Y_inv=1.392014;
		else if (im->setup->quality_setting==LOW12) Y_inv=1.521263;
		else if (im->setup->quality_setting==LOW13) Y_inv=1.587597;
		else if (im->setup->quality_setting==LOW14) Y_inv=1.665887;
		else if (im->setup->quality_setting==LOW15) Y_inv=1.741126;
		else if (im->setup->quality_setting==LOW16) Y_inv=1.820444;
		else if (im->setup->quality_setting==LOW17) Y_inv=1.916257;
		else if (im->setup->quality_setting==LOW18) Y_inv=1.985939;
		else if (im->setup->quality_setting==LOW19) Y_inv=2.060881;


		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=3*m*IM_SIZE;i<(m+1)*IM_SIZE;i++,t+=3)
			{
				Y = icolorY[i]*298;
				U = icolorU[i];
				V = icolorV[i];

				//Matrix  YCbCr (or YUV) to RGB
				R = (((int)((Y         + 409*V + R_COMP)*Y_inv +128.5f))>>8);
				G = (((int)((Y - 100*U - 208*V + G_COMP)*Y_inv +128.5f))>>8);
				B = (((int)((Y + 516*U         + B_COMP)*Y_inv +128.5f))>>8);

				//Clip RGB Values
				if ((R>>8)!=0) iNHW[t]=( (R<0) ? 0 : 255 );
				else iNHW[t]=R;

				if ((G>>8)!=0) iNHW[t+1]=( (G<0) ? 0 : 255 );
				else iNHW[t+1]=G;

				if ((B>>8)!=0) iNHW[t+2]=( (B<0) ? 0 : 255 );
				else iNHW[t+2]=B;
			}
		}
	}

	free(im->im_bufferY);
	free(im->im_bufferU);
	free(im->im_bufferV);
}

int setup_bmp_header(char* bmp_header, nhw_image_s* im_ctx)
{
	uint32_t tmpi32;
	char* ptrx;
	uint16_t BMP_SGN = (uint16_t)('B' | ('M' << 8));

	ptrx = bmp_header;
	*(uint16_t*)(ptrx+0) = BMP_SGN; // signature: 'BM' (0x4D42)
	tmpi32 = ((im_ctx->width*im_ctx->bpp+31)/32)*4*im_ctx->height+54;
	*(uint32_t*)(ptrx+2) = tmpi32; // length of the file
	tmpi32 = im_ctx->width;
	*(uint32_t*)(ptrx+18) = tmpi32; // width
	tmpi32 = im_ctx->height;
	*(uint32_t*)(ptrx+22) = tmpi32; // height
	tmpi32 = im_ctx->bpp;
	*(uint32_t*)(ptrx+28) = tmpi32; // number of bits per pixel
	tmpi32 = ((im_ctx->width*im_ctx->bpp+31)/32)*4*im_ctx->height;
	*(uint32_t*)(ptrx+34) = tmpi32; // size of image data in bytes (including padding)
return 0;
}

