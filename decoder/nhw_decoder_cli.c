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
	int ft;
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
		for (int t=0,ft=0; t<ntiles; t++,ft++)
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
			nt2.first_tile = ft;
			
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
	
	nt1.first_tile = 0;

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

