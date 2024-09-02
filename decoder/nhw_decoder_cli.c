/***************************************************************************
****************************************************************************
*  NHW Image Codec                                                         *
*  file: nhw_decoder_cli.c                                                 *
*  version: 0.3.0-rc1                                                      *
*  last update: $ 04182023 nhw exp $                                       *
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
#define VERSION "0.3.0-rc1"

int setup_bmp_header(char* bmp_header, nhw_image_s* im_ctx);
int write_image_bmp(image_buffer *im, char *file_name);
void show_usage();

unsigned char bmp_header[54]={66,77,54,0,12,0,0,0,0,0,
				54,0,0,0,40,0,0,0,0,2,
				0,0,0,2,0,0,1,0,24,0,
				0,0,0,0,0,0,12,0,0,0,
				0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int main(int argc, char **argv)
{
	image_buffer im;
	decode_state dec;
	nhw_image_s im_ctx;
	char *ifname, *ofname;

	if (argc<3)
	{
		show_usage();
		return 0;
	}
	ifname = argv[1];
	ofname = argv[2];

	/* Decode Image */
	decode_image(&im, &dec, ifname); // todo: setup image context

	/* this should come from image to convert */
	im_ctx.width = 512;
	im_ctx.height = 512;
	im_ctx.bpp = 24;
	setup_bmp_header((char*)bmp_header, &im_ctx);
	write_image_bmp(&im, ofname);

	return 0;
} // main

void show_usage()
{
	fprintf(stdout,
	"Usage: %s <image.nhw> <image.bmp>\n"
	"Convert image: nwh to bmp\n"
	" (with a bitmap color 512x512 image)\n"
	"\n"
	"  example: nhw-dec image.nhw image.bmp\n"
	"Copyright (C) 2007-2022 NHW Project (Raphael C.)\n",
	PROGRAM);
}


int write_image_bmp(image_buffer *im, char *file_name)
{

	int i,Y,U,V,R,G,B,m,t;
	unsigned char *icolorY,*icolorU,*icolorV,*iNHW;
	float Y_q_setting,Y_inv;

	char* output_file_name = file_name;
	// here to work on Windows Vista
	icolorY=(unsigned char*)im->im_bufferY;
	icolorU=(unsigned char*)im->im_bufferU;
	icolorV=(unsigned char*)im->im_bufferV;
	im->im_buffer4=(unsigned char*)malloc(3*IM_SIZE*sizeof(char));
	iNHW=(unsigned char*)im->im_buffer4;

	FILE* output_image_file = fopen(output_file_name,"wb");

	if (output_image_file == NULL)
	{
		printf("Failed to open output decompressed .bmp file %s\n",output_file_name);
	}

	// WRITE DECODED DATA
	fwrite(bmp_header,54,1,output_image_file);

	if (im->setup->quality_setting>=NORM)
	{
		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=0;i<(m+1)*IM_SIZE;i++,t+=3)
			{
				//Y = icolorY[i]*298;
				Y = icolorY[i];
				U = icolorU[i]-128;
				V = icolorV[i]-128;

				//Matrix  YCbCr (or YUV) to RGB
				/*R = ((Y         + 409*V + R_COMP)>>8);
				G = ((Y - 100*U - 208*V + G_COMP)>>8);
				B = ((Y + 516*U         + B_COMP)>>8);*/
				R = (int)(Y  + 1.402*V +0.5f);
				G = (int)(Y  -0.34414*U -0.71414*V +0.5f);
				B = (int)(Y  +1.772*U +0.5f);

				//Clip RGB Values
				if ((R>>8)!=0) iNHW[t]=( (R<0) ? 0 : 255 );
				else iNHW[t]=R;

				if ((G>>8)!=0) iNHW[t+1]=( (G<0) ? 0 : 255 );
				else iNHW[t+1]=G;

				if ((B>>8)!=0) iNHW[t+2]=( (B<0) ? 0 : 255 );
				else iNHW[t+2]=B;
			}

			fwrite(iNHW,3*IM_SIZE,1,output_image_file);
		}
	}
	else if (im->setup->quality_setting==LOW1 || im->setup->quality_setting==LOW2)
	{
		if (im->setup->quality_setting==LOW1) Y_inv=1.025641; // 1/0.975
		else if (im->setup->quality_setting==LOW2) Y_inv=1.075269; // 1/0.93

		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=0;i<(m+1)*IM_SIZE;i++,t+=3)
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

			fwrite(iNHW,3*IM_SIZE,1,output_image_file);
		}
	}
	else if (im->setup->quality_setting==LOW3)
	{
		Y_inv=1.063830; // 1/0.94

		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=0;i<(m+1)*IM_SIZE;i++,t+=3)
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

			fwrite(iNHW,3*IM_SIZE,1,output_image_file);
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
			for (i=m*IM_SIZE,t=0;i<(m+1)*IM_SIZE;i++,t+=3)
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

			fwrite(iNHW,3*IM_SIZE,1,output_image_file);
		}
	}

	fclose(output_image_file);
	free(im->im_bufferY);
	free(im->im_bufferU);
	free(im->im_bufferV);
	free(im->im_buffer4);
	return 0;
}

int setup_bmp_header(char* bmp_header, nhw_image_s* im_ctx)
{
	uint32_t tmpi32;
	char* ptrx;
	uint16_t BMP_SGN = (uint16_t)('B' | ('M' << 8));

	ptrx = bmp_header;
	*(uint16_t*)(ptrx+0) = BMP_SGN; // signature: 'BM' (0x4D42)
	tmpi32 = im_ctx->width*im_ctx->height*(im_ctx->bpp/8)+54;
	*(uint32_t*)(ptrx+2) = tmpi32; // length of the file
	tmpi32 = im_ctx->width;
	*(uint32_t*)(ptrx+18) = tmpi32; // width
	tmpi32 = im_ctx->height;
	*(uint32_t*)(ptrx+22) = tmpi32; // height
	tmpi32 = im_ctx->bpp;
	*(uint32_t*)(ptrx+28) = tmpi32; // number of bits per pixel
	tmpi32 = im_ctx->width*im_ctx->height*(im_ctx->bpp/8);
	*(uint32_t*)(ptrx+34) = tmpi32; // size of image data in bytes (including padding)
return 0;
}

