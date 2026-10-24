/***************************************************************************
****************************************************************************
*  NHW Image Codec                                                         *
*  file: nhw_encoder_cli.c                                                 *
*  version: 0.3.4+8                                                        *
*  last update: $ 10232026 nhw exp $                                       *
*                                                                          *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -simple codec                                                   *
***************************************************************************/

/* Copyright (C) 2007-2026 NHW Project
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
#ifdef _OPENMP
#include <omp.h>
#endif

#include "codec.h"

#define PROGRAM "nhw-enc"
#define VERSION "0.3.4+8"

/* tiled container: magic, uint32 width, uint32 height, then per tile uint32 length + legacy stream */
#define NHWT_MAGIC "NHWT"

#define NHW_QUALITY_MIN LOW19
#define NHW_QUALITY_MAX HIGH3

extern char bmp_header[54];

void show_usage()
{
	fprintf(stdout,
	"Usage: %s [-hV][-q<quality>] <image.bmp> <image.nhw>\n"
	"Convert image: bmp to nwh\n"
	" (with a 24-bit bitmap color image of any size)\n"
	"Options:\n"
	"  -q#       image quality #:[1..23] {default: 20}\n"
	"  -h        print this help\n"
	"  -V        show version and legal information\n\n"
	"  example: nhw-enc -q15 image.bmp image.nhw\n",
	PROGRAM);
}

void show_version()
{
	fprintf(stdout,
	""PROGRAM" "VERSION"\n"
	"NHW Image encoder\n"
	"Copyright (C) 2007-2022 Raphael Canut\n"
	"\n"
	"This software is provided by the copyright holders and contributors\n"
	"``as is'' and any express or implied warranties are disclaimed.\n"
	"See the License file for details.\n"
	);
}

int main(int argc, char **argv)
{	
	image_buffer im;
	encode_state enc;
	ntile nt2;
	char *ifname, *ofname;
	int select, quality, ofoverwrite = 0;
	int width, height, stride, t1, t2;
	unsigned char *image;
	FILE *compressed;

	quality = NORM;

	while (argc>1 && *argv[1]=='-')
	{
		for (int i=1; argv[1][i]!='\0'; i++)
		{
		switch (argv[1][i])
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			;
			break;
		case 'q':
			if ((argv[1][i+1]!='\0') && (argv[1][i+1]>='0') && (argv[1][i+1]<='9'))
			{
				quality = atoi(&argv[1][i+1]);
				if ((quality<NHW_QUALITY_MIN) || (quality>NHW_QUALITY_MAX))
				{
					printf("quality=%d out of range\n", quality);
					exit(1);
				}
			}
			else
			{
				printf("invalid quality='%s'\n", &argv[1][i+1]);
				exit(1);
			}
			break;
		case 'f':
			ofoverwrite=1;
			break;
		case 'h':
			show_usage();
			exit(0);
			break;
		case 'V':
			show_version();
			exit(0);
			break;
		default:
			fprintf(stderr, "Unknown option '-%c'\n", argv[1][i]);
			exit(1);
		}
		}
	argc--;
	argv++;
	}

	if (argc<3)
	{
		printf("Not enough arguments. Check help.\n");
		show_usage();
		return 0;
	}
	ifname = argv[1];
	ofname = argv[2];
	if (strcmp(ifname, ofname) == 0)
	{
		fprintf(stdout, "Input and output are the same file: '%s'.\n", ifname);
		return 1;
	}
	if (!ofoverwrite)
	{
		FILE* fy=fopen(ofname, "rb");
		if (fy)
		{
			fclose(fy);
			fprintf(stderr, "File '%s' already exists. Try `-f' to overwrite.\n", ofname);
			return 1;
		}
	}

	im.setup=(codec_setup*)malloc(sizeof(codec_setup));
	im.setup->quality_setting=quality;
	select = 8;

	image = read_image_bmp(ifname, &width, &height, &stride);

	compressed = fopen(ofname, "wb");
	if (compressed == NULL)
	{
		printf("Failed to create file: %s\n", ofname);
		return 0;
	}
	
	nt2.width = width;
	
	nt2.height = height;
	
	t1 = width>>9;
		
	t1 = t1<<9;
		
	if ((width-t1)!=0) t1 = (width>>9)+1; 
	else t1 = 0;
		
	t2 = height>>9;
		
	t2 = t2<<9;
	
	if ((height-t2)!=0) t2 = (height>>9)+1; 
	else t2 = 0;

	if (width == 2*IM_DIM && height == 2*IM_DIM)
	{
		/* exact 512x512: legacy single-tile stream */
		load_tile(&im, image, stride, 0, 0, select);
		/* Encode Image */
		nt2.tile_number = 0;
		
		encode_image(&im, &enc, &nt2 ,select);
		write_compressed_file(&im, &enc, &nt2, compressed);
	}
	else
	{
		/* any other size: pad to whole 512x512 tiles, write "NHWT" container */
		int tiles_x=(width+2*IM_DIM-1)/(2*IM_DIM), tiles_y=(height+2*IM_DIM-1)/(2*IM_DIM);
		int pstride=tiles_x*3*2*IM_DIM, ntiles=tiles_x*tiles_y, batch=1;
		uint32_t dim;
		unsigned char *padded;
		image_buffer *tim;
		encode_state *tenc;
		ntile *nt1;

		padded = pad_image(image, width, height, stride, tiles_x*2*IM_DIM, tiles_y*2*IM_DIM);
		free(image);
		
		image = padded;

		fwrite(NHWT_MAGIC,4,1,compressed);
		dim=width;  fwrite(&dim,4,1,compressed);
		dim=height; fwrite(&dim,4,1,compressed);

		/* tiles are encoded in parallel batches (when built with OpenMP), then written in order */
#ifdef _OPENMP
		batch=4*omp_get_max_threads();
#endif
		if (batch>ntiles) batch=ntiles;
		tim=(image_buffer*)calloc(batch,sizeof(image_buffer));
		tenc=(encode_state*)calloc(batch,sizeof(encode_state));
		nt1=(ntile*)calloc(batch,sizeof(ntile));
		
		for (int b=0; b<batch; b++)
		{
			tim[b].setup=(codec_setup*)malloc(sizeof(codec_setup));
			tim[b].setup->quality_setting=quality;
		}
			
		/* tiles in bottom-up raster order, each prefixed with its length */
		for (int first=0; first<ntiles; first+=batch)
		{
			int n=(ntiles-first<batch) ? ntiles-first : batch;

			#pragma omp parallel for schedule(dynamic)
			for (int b=0; b<n; b++)
			{	
				memset(&tenc[b], 0, sizeof(encode_state));
				load_tile(&tim[b], image, pstride, (first+b)%tiles_x, (first+b)/tiles_x, select);
				
				nt1[b].width = width;
	
				nt1[b].height = height;
				
				nt1[b].tile_number = 0;
				
				if (t1>0)
				{
					if ((((first+b)%tiles_x)+1)<t1) nt1[b].tile_number = 0;
					else nt1[b].tile_number = 1;
				}
				
				if (t2>0)
				{
					if ((((first+b)/tiles_x)+1)<t2)
					{
						if (nt1[b].tile_number==0) nt1[b].tile_number = 0;
					}
					else nt1[b].tile_number = 2;
				}
				
				if ((((first+b)%tiles_x)+1)==t1 && (((first+b)/tiles_x)+1)==t2) nt1[b].tile_number = 3;
				
				encode_image(&tim[b], &tenc[b], &nt1[b], select);
			}

			for (int b=0; b<n; b++)
			{
				long start, end;
				uint32_t len = 0;

				start = ftell(compressed);
				fwrite(&len,4,1,compressed);
				write_compressed_file(&tim[b], &tenc[b], &nt1[b], compressed);
				end = ftell(compressed);
				len = (uint32_t)(end-start-4);
				fseek(compressed, start, SEEK_SET);
				fwrite(&len,4,1,compressed);
				fseek(compressed, end, SEEK_SET);
			}
		}

		for (int b=0; b<batch; b++) free(tim[b].setup);
		free(tim);
		free(tenc);
	}

	fclose(compressed);
	free(image);

	return 0;
}

