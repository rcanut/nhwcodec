/***************************************************************************
****************************************************************************
*  NHW Image Codec														   *
*  file: colorspace.c											           *
*  version: 0.1.3 						     		     				   *
*  last update: $ 12072012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -colorspace conversion : YCbCr , YUV , RGB			           *
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
#include <math.h>

#include "codec.h"

#define CLIP(x) ( (x<0) ? 0 : ((x>255) ? 255 : x) );

void downsample_YUV420(image_buffer *im,encode_state *enc,int rate)
{
	int i,j,Y,U,V,Qtz;
	short *colorsY;
	unsigned char *colors,*colorsU,*colorsV;
	float color_balance,Y_quant;

	colors=(unsigned char*)im->im_buffer4;
	im->im_jpeg=(short*)malloc(4*IM_SIZE*sizeof(short));
	colorsY=(short*)im->im_jpeg;

	if (im->setup->quality_setting>=NORM)
	{
		for (i=0,j=0;i<12*IM_SIZE;i+=3,j++)
		{

		Y = (int)(0.299*colors[i] + 0.587*colors[i+1] +  0.114*colors[i+2]+0.5f);
		/*U = (int)(-0.1687*colors[i] -  0.3313*colors[i+1] + 0.5*colors[i+2] + 128.5f);
		V = (int)(0.5*colors[i] -  0.4187*colors[i+1] -  0.0813*colors[i+2] + 128.5f);*/

		color_balance = -0.1687*colors[i] -  0.3313*colors[i+1] + 0.5*colors[i+2];
		if (color_balance>=0) U = (int)(color_balance + 128.5f);
		else U = (int)(color_balance + 128.4f);

		color_balance = 0.5*colors[i] -  0.4187*colors[i+1] -  0.0813*colors[i+2];
		if (color_balance>=0) V = (int)(color_balance + 128.5f);
		else V = (int)(color_balance + 128.4f);

		//Clip YUV values
		//if ((Y>>8)!=0) colorsY[j]=( (Y<0) ? 0 : 255 );
		//else colorsY[j]=Y;
		colorsY[j]=Y;

		if ((U>>8)!=0) 
		{
			colors[i+1]=( (U<0) ? 0 : 255);
		}
		else colors[i+1]=U;


		if ((V>>8)!=0) 
		{
			colors[i+2]=( (V<0) ? 0 : 255 );
		}
		else colors[i+2]=V;
		}
	}
	else if (im->setup->quality_setting==LOW1|| im->setup->quality_setting==LOW2)
	{
		if (im->setup->quality_setting==LOW1) Y_quant=0.975;
		else if (im->setup->quality_setting==LOW2) Y_quant=0.93;

		for (i=0,j=0;i<12*IM_SIZE;i+=3,j++)
		{

			Y = (int)( (0.299*colors[i] + 0.587*colors[i+1] +  0.114*colors[i+2])*Y_quant+0.5f);
			/*U = (int)(-0.1687*colors[i] -  0.3313*colors[i+1] + 0.5*colors[i+2] + 128.5f);
			V = (int)(0.5*colors[i] -  0.4187*colors[i+1] -  0.0813*colors[i+2] + 128.5f);*/

			color_balance = -0.1687*colors[i] -  0.3313*colors[i+1] + 0.5*colors[i+2];
			if (color_balance>=0) U = (int)(color_balance + 128.5f);
			else U = (int)(color_balance + 128.4f);

			color_balance = 0.5*colors[i] -  0.4187*colors[i+1] -  0.0813*colors[i+2];
			if (color_balance>=0) V = (int)(color_balance + 128.5f);
			else V = (int)(color_balance + 128.4f);

			colorsY[j]=Y;

			if ((U>>8)!=0) 
			{
				colors[i+1]=( (U<0) ? 0 : 255);
			}
			else colors[i+1]=U;


			if ((V>>8)!=0) 
			{
				colors[i+2]=( (V<0) ? 0 : 255 );
			}
			else colors[i+2]=V;
		}
	}
	else if (im->setup->quality_setting==LOW3)
	{
		for (i=0,j=0;i<12*IM_SIZE;i+=3,j++)
		{

		Y = (int)((0.299*colors[i] + 0.587*colors[i+1] +  0.114*colors[i+2])*0.94+0.5f);

		color_balance = (-0.1687*colors[i] -  0.3313*colors[i+1] + 0.5*colors[i+2])*0.94;
		if (color_balance>=0) U = (int)(color_balance + 128.5f);
		else U = (int)(color_balance + 128.4f);

		color_balance = (0.5*colors[i] -  0.4187*colors[i+1] -  0.0813*colors[i+2])*0.94;
		if (color_balance>=0) V = (int)(color_balance + 128.5f);
		else V = (int)(color_balance + 128.4f);

		//Clip YUV values
		
		colorsY[j]=Y;

		if ((U>>8)!=0) 
		{
			colors[i+1]=( (U<0) ? 0 : 255);
		}
		else colors[i+1]=U;


		if ((V>>8)!=0) 
		{
			colors[i+2]=( (V<0) ? 0 : 255 );
		}
		else colors[i+2]=V;
		}

	}
	else if (im->setup->quality_setting<=LOW4)
	{
		if (im->setup->quality_setting==LOW4) Qtz=32375;
		else if (im->setup->quality_setting==LOW5) Qtz=31262;
		else if (im->setup->quality_setting==LOW6) Qtz=28786;
		else if (im->setup->quality_setting==LOW7) Qtz=27607;
		else if (im->setup->quality_setting==LOW8) Qtz=27830;
		else if (im->setup->quality_setting==LOW9) Qtz=27522;
		else if (im->setup->quality_setting==LOW10) Qtz=25570;
		else if (im->setup->quality_setting==LOW11) Qtz=23540;
		else if (im->setup->quality_setting==LOW12) Qtz=21540;
		else if (im->setup->quality_setting==LOW13) Qtz=20640;
		else if (im->setup->quality_setting==LOW14) Qtz=19670;
		else if (im->setup->quality_setting==LOW15) Qtz=18820;
		else if (im->setup->quality_setting==LOW16) Qtz=18000;
		else if (im->setup->quality_setting==LOW17) Qtz=17100;

		for (i=0,j=0;i<12*IM_SIZE;i+=3,j++)
		{
			//Convert RGB to YCbCr or YUV
			colorsY[j] = ((( 66*colors[i] + 129*colors[i+1] +  25*colors[i+2])*Qtz + 4194304)>>23)+ 16;
			U = (((-38*colors[i] -  74*colors[i+1] + 112*colors[i+2])*Qtz + 4194304)>>23)+128;
			V = (((112*colors[i] -  94*colors[i+1] -  18*colors[i+2])*Qtz + 4194304)>>23)+128;

			//colorsY[j]=Y;

			if ((U>>8)!=0) 
			{
				colors[i+1]=( (U<0) ? 0 : 255);
			}
			else colors[i+1]=U;


			if ((V>>8)!=0) 
			{
				colors[i+2]=( (V<0) ? 0 : 255 );
			}
			else colors[i+2]=V;
		}
	}

	/*my >>=18; if (my>255) my=255;else if (my<0) my=0; enc->m_y=my;
	mu >>=18; if (mu>255) mu=255;else if (mu<0) mu=0; enc->m_u=mu;
	mv >>=18; if (mv>255) mv=255;else if (mv<0) mv=0; enc->m_v=mv;*/

	for (;colors<(im->im_buffer4+(12*IM_SIZE));colors+=(3*IM_DIM*2))
	{
		//im->im_buffer[i]=colors[i];
		//im->im_buffer[i+1]=colors[1+i];
		//im->im_buffer[i+2]=colors[2+i];
		colors[1]=(colors[1]+colors[4]+1)>>1;
		colors[2]=(colors[2]+colors[5]+1)>>1;

		for (j=6;j<(3*IM_DIM*2);j+=6)
		{
			//im->im_buffer[j+i]=(colors[j-3+i]+colors[j+i]*2+colors[j+3+i]+2)>>2;
			colors[j+1]=(colors[j-2]+colors[j+1]*2+colors[j+4]+2)>>2;
			colors[j+2]=(colors[j-1]+colors[j+2]*2+colors[j+5]+2)>>2;
		}
	}

	im->im_bufferU=(unsigned char*)malloc(IM_SIZE*sizeof(char));
	im->im_bufferV=(unsigned char*)malloc(IM_SIZE*sizeof(char));
	colorsU=(unsigned char*)im->im_bufferU;
	colorsV=(unsigned char*)im->im_bufferV;

	for (j=0,V=0;j<(3*IM_DIM*2);j+=6,V++)
	{
		//im->im_buffer[j+1]=colors[j+1+i];
		//im->im_buffer[j+2]=colors[j+2+i];
		colors=im->im_buffer4;
		colorsU[V]=(colors[j+1]+colors[j+1+(3*IM_DIM*2)]+1)>>1;
		colorsV[V]=(colors[j+2]+colors[j+2+(3*IM_DIM*2)]+1)>>1;

		for (colors=(im->im_buffer4+(3*IM_DIM*4)),U=(IM_DIM);U<(IM_SIZE);colors+=(3*IM_DIM*4),U+=(IM_DIM))
		{
			//im->im_buffer[j+i-(3*IM_DIM*2)]=(colors[j+i-(3*IM_DIM*2)]+colors[j+i]*2+colors[j+i+(3*IM_DIM*2)]+2)>>2;
			colorsU[V+U]=(colors[j+1-(3*IM_DIM*2)]+colors[j+1]*2+colors[j+1+(3*IM_DIM*2)]+2)>>2;
			colorsV[V+U]=(colors[j+2-(3*IM_DIM*2)]+colors[j+2]*2+colors[j+2+(3*IM_DIM*2)]+2)>>2;
		}

	}

	free(im->im_buffer4);

}