/***************************************************************************
****************************************************************************
*  NHW Image Codec                                                         *
*  file: nhw_encoder_cli.c                                                 *
*  version: 0.2.7                                                          *
*  last update: $ 19112022 nhw exp $                                       *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec.h"
#include "tgetopt.h"

#define PROGRAM "nhw"
#define VERSION "0.2.7"

#define NHW_QUALITY_MIN LOW20
#define NHW_QUALITY_MAX HIGH3

extern char bmp_header[54];

#define CLIP(x) ( (x<0) ? 0 : ((x>255) ? 255 : x) );

static const tchar *const optstring = T("hq:V");

// main program options
struct options
{
	int quality;
};

// help()
void show_usage(char* prog)
{
	fprintf(stdout,
	"Usage: %s [-hq] <image.bmp> <image.nhw>\n"
	"Convert image: bmp to nwh\n"
	" (with a bitmap color 512x512 image)\n"
	"Options:\n"
	"  -q X      image quality [1..24] {default: 20}\n"
	"  -h        print this help\n"
	"  -V        show version and legal information\n",
	prog);
}

// version()
void show_version(void)
{
	fprintf(stdout,
	"nhw "VERSION"\n"
	"NHW Image encoder\n"
	"Copyright (C) 2007-2022 Raphael Canut\n"
	"\n"
	"This software is provided by the copyright holders and contributors\n"
	"``as is'' and any express or implied warranties are disclaimed.\n"
	"See the License file for details.\n"
	);
}

// main()
int main(int argc, char *argv[])
{
	image_buffer im;
	encode_state enc;
	char *input_file_name, *output_file_name;
	int select;
	char *arg;
	struct options options;
	int opt_char;

options.quality = NORM;

if (argc < 3)
{
	show_usage(argv[0]);
	return 1;
}
while ((opt_char = tgetopt(argc, argv, optstring)) != -1)
	{
	switch (opt_char)
		{
		case 'h':
			show_usage(argv[0]);
			return 0;
		case 'q':
			arg = toptarg;
			if (!arg || (arg[0] == T('\0')))
			{
				printf("invalid quality\n");
				exit(1);
			}
			options.quality = atoi(arg);
			options.quality -= 1;
			if((options.quality<NHW_QUALITY_MIN) || (options.quality>NHW_QUALITY_MAX))
			{
				printf("invalid quality (out of range)\n");
				exit(1);
			}
			break;
		case 'V':
			show_version();
			return 0;
		default:
			printf("\n"); show_usage(argv[0]);
			return 1;
		}
	}

	if(toptind==argc-2)
	{
		input_file_name = argv[toptind++];
		output_file_name = argv[toptind];
		if((strcmp(input_file_name, output_file_name))==0)
		{
			fprintf(stdout, "These are the same file: %s\n", input_file_name);
			return 1;
		}
	}
	else
	{
		fprintf(stdout, "Not enough arguments\n");
		return 1;
	}

	im.setup = (codec_setup*)malloc(sizeof(codec_setup));
	im.setup->quality_setting = options.quality;

	menu(input_file_name,&im,&enc,select);

	/* Encode Image */
	encode_image(&im,&enc,select);

	write_compressed_file(&im,&enc,output_file_name);

	return 0;
}

