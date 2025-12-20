/***************************************************************************
****************************************************************************
*  NHW Image Codec                                                         *
*  file: nhw_encoder_cli.c                                                 *
*  version: 0.3.1+33                                                       *
*  last update: $ 12202025 nhw exp $                                       *
*                                                                          *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -simple codec                                                   *
***************************************************************************/

/* Copyright (C) 2007-2025 NHW Project
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

#include "codec.h"

#define PROGRAM "nhw-enc"
#define VERSION "0.3.1+33"

#define NHW_QUALITY_MIN LOW20
#define NHW_QUALITY_MAX HIGH3

extern char bmp_header[54];

void show_usage()
{
	fprintf(stdout,
	"Usage: %s [-hV][-q<quality>] <image.bmp> <image.nhw>\n"
	"Convert image: bmp to nwh\n"
	" (with a bitmap color 512x512 image)\n"
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
	char *ifname, *ofname;
	int select, quality, ofoverwrite;

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

	read_image_bmp(ifname, &enc, &im, select);
	/* Encode Image */
	encode_image(&im, &enc, select);

	write_compressed_file(&im, &enc, ofname);

	return 0;
}

