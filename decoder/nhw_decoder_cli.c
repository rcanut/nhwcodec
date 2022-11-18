/***************************************************************************
****************************************************************************
*  NHW Image Codec                                                         *
*  file: nhw_decoder_cli.c                                                 *
*  version: 0.2.7                                                         *
*  last update: $ 18112022 nhw exp $                                       *
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
#include "nhw_decoder.h"

int main(int argc, char **argv)
{
	image_buffer im;
	decode_state dec;
	FILE *input_image_file, *output_image_file;
	char *input_file_name, *output_file_name;
	int i,Y,U,V,R,G,B,len,m,t;
	unsigned char *icolorY,*icolorU,*icolorV,*iNHW;
	float Y_q_setting,Y_inv;
	char OutputFileName[256];
	unsigned char bmp_header[54]={66,77,54,0,12,0,0,0,0,0,
					54,0,0,0,40,0,0,0,0,2,
					0,0,0,2,0,0,1,0,24,0,
					0,0,0,0,0,0,12,0,0,0,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	if (argc<3)
	{
		printf("\n Copyright (C) 2007-2013 NHW Project (Raphael C.)\n");
		printf("\n%s <file.nhw> <file.bmp>\n", argv[0]);
		exit(-1);
	}
	input_file_name = argv[1];
	output_file_name = argv[2];

	/* Decode Image */
	decode_image(&im,&dec,argv[1]);

	// here to work on Windows Vista
	icolorY=(unsigned char*)im.im_bufferY;
	icolorU=(unsigned char*)im.im_bufferU;
	icolorV=(unsigned char*)im.im_bufferV;
	im.im_buffer4=(unsigned char*)malloc(3*IM_SIZE*sizeof(char));
	iNHW=(unsigned char*)im.im_buffer4;

	output_image_file = fopen(output_file_name,"wb");

	if (output_image_file == NULL)
	{
		printf("Failed to open output decompressed .bmp file %s\n",output_file_name);
	}

	// WRITE DECODED DATA
	fwrite(bmp_header,54,1,output_image_file);

	if (im.setup->quality_setting>=NORM)
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
	else if (im.setup->quality_setting==LOW1 || im.setup->quality_setting==LOW2)
	{
		if (im.setup->quality_setting==LOW1) Y_inv=1.025641; // 1/0.975
		else if (im.setup->quality_setting==LOW2) Y_inv=1.075269; // 1/0.93

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
	else if (im.setup->quality_setting==LOW3) 
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
	else if (im.setup->quality_setting<LOW3) 
	{
		if (im.setup->quality_setting==LOW4) Y_inv=1.012139; // 1/0.94
		else if (im.setup->quality_setting==LOW5) Y_inv=1.048174; // 1/0.906
		else if (im.setup->quality_setting==LOW6) Y_inv=1.138331; // 1/0.8
		else if (im.setup->quality_setting==LOW7) Y_inv=1.186945; 
		else if (im.setup->quality_setting==LOW8) Y_inv=1.177434;
		else if (im.setup->quality_setting==LOW9) Y_inv=1.190611; 
		else if (im.setup->quality_setting==LOW10) Y_inv=1.281502; 
		else if (im.setup->quality_setting==LOW11) Y_inv=1.392014;
		else if (im.setup->quality_setting==LOW12) Y_inv=1.521263;
		else if (im.setup->quality_setting==LOW13) Y_inv=1.587597;
		else if (im.setup->quality_setting==LOW14) Y_inv=1.665887;
		else if (im.setup->quality_setting==LOW15) Y_inv=1.741126;
		else if (im.setup->quality_setting==LOW16) Y_inv=1.820444;
		else if (im.setup->quality_setting==LOW17) Y_inv=1.916257;
		else if (im.setup->quality_setting==LOW18) Y_inv=1.985939;
		else if (im.setup->quality_setting==LOW19) Y_inv=2.060881;


		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=0;i<(m+1)*IM_SIZE;i++,t+=3)
			{
				Y = icolorY[i]*298;
				U = icolorU[i];
				V = icolorV[i];

				//Matrix  YCbCr (or YUV) to RGB
				R =(((int)((Y         + 409*V + R_COMP)*Y_inv +128.5f))>>8); 
				G =(((int)((Y - 100*U - 208*V + G_COMP)*Y_inv +128.5f)) >>8);  
				B = (((int)((Y + 516*U         + B_COMP)*Y_inv +128.5f)) >>8);

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
	free(im.im_bufferY);
	free(im.im_bufferU);
	free(im.im_bufferV);
	free(im.im_buffer4);
	return 0;
}

