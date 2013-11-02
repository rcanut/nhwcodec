/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: nhw_decoder.c  										           *
*  version: 0.1.3 						     		     				   *
*  last update: $ 11022013 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -simple codec												   *
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

#include "codec.h"

#define CLIP(x) ( (x<0) ? 0 : ((x>255) ? 255 : x) );

void main(int argc, char **argv)
{
	image_buffer im;
	decode_state dec;
	FILE *res_image;
	int i,Y,U,V,R,G,B,len,m,t;
	unsigned char *icolorY,*icolorU,*icolorV,*iNHW;
	float Y_q_setting,Y_inv;
	char OutputFile[200];
	unsigned char bmp_header[54]={66,77,54,0,12,0,0,0,0,0,
							  54,0,0,0,40,0,0,0,0,2,
							  0,0,0,2,0,0,1,0,24,0,
							  0,0,0,0,0,0,12,0,0,0,
							  0,0,0,0,0,0,0,0,0,0,0,0,0,0};



	if (argv[1]==NULL || argv[1]==0)
	{
		printf("\n Copyright (C) 2007-2013 NHW Project (Raphael C.)\n");
		printf("\n-> nhw_decoder.exe filename.nhw\n");
		exit(-1);
	}

	/* Decode Image */
	decode_image(&im,&dec,argv);

	// here to work on Windows Vista
    icolorY=(unsigned char*)im.im_bufferY;
	icolorU=(unsigned char*)im.im_bufferU;
	icolorV=(unsigned char*)im.im_bufferV;
	im.im_buffer4=(unsigned char*)malloc(3*IM_SIZE*sizeof(char));
	iNHW=(unsigned char*)im.im_buffer4;

	// Create the Output Decompressed .BMP File
	len=strlen(argv[1]);
	memset(argv[1]+len-4,0,4);
	sprintf(OutputFile,"%sDEC.bmp",argv[1]);

	res_image = fopen(OutputFile,"wb");

	if( NULL == res_image )
	{
		printf("Failed to open output decompressed .bmp file %s\n",OutputFile);
	}

	// WRITE DECODED DATA
	fwrite(bmp_header,54,1,res_image);

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
				R = (int)(Y  + 1.402*V+0.5f);   
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

			fwrite(iNHW,3*IM_SIZE,1,res_image);
		}
	}
	else
	{
		if (im.setup->quality_setting==LOW1) Y_inv=1.06952; // 1/0.935
		else if (im.setup->quality_setting==LOW2) Y_inv=1.136364; // 1/0.88
		else Y_inv=1.242236; // 1/0.805

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

			fwrite(iNHW,3*IM_SIZE,1,res_image);
		}
	}

	fclose(res_image);
	free(im.im_bufferY);
	free(im.im_bufferU);
	free(im.im_bufferV);
	free(im.im_buffer4);
}

void decode_image(image_buffer *im,decode_state *os,char **argv)
{
	int nhw,stage,wavelet_order,end_transform,i,j,e=0,count,scan,*res_decompr,exw1,res,nhw_selectII;
	short *im_nhw,*im_nhw2;
	char *res256;
	unsigned char *nhw_scale,*nhw_chr;
	unsigned short *nhwresH1,*nhwresH2,*nhwresH1I;
	unsigned long *nhwresH3I;
	unsigned short *nhwres1,*nhwres2,*nhwres1I,*nhwres3I,*nhwres3,*nhwres4,*nhwres4I,*nhwres5,*nhwres6;

	wavelet_order=parse_file(im,os,argv);
	retrieve_pixel_Y_comp(im,os,4*IM_SIZE,os->packet1,im->im_process);
	free(os->packet1);
	im->im_jpeg=(short*)malloc(4*IM_SIZE*sizeof(short));
	im_nhw=(short*)im->im_jpeg;
	im_nhw2=(short*)im->im_process;

	// Y
	for (j=0,count=0;j<(IM_DIM<<1);)
	{
		for (i=0;i<IM_DIM;i++)
		{
			im_nhw[j]=im_nhw2[count];
			im_nhw[j+1]=im_nhw2[count+1];
			im_nhw[j+2]=im_nhw2[count+2];
			im_nhw[j+3]=im_nhw2[count+3];
	
			j+=(2*IM_DIM);
			im_nhw[j+3]=im_nhw2[count+4];
			im_nhw[j+2]=im_nhw2[count+5];
			im_nhw[j+1]=im_nhw2[count+6];
			im_nhw[j]=im_nhw2[count+7];

			j+=(2*IM_DIM);
			count+=8;
		}

		j-=((4*IM_SIZE)-4);
	}

	nhwres1I=(unsigned short*)calloc((os->nhw_res1_bit_len<<3),sizeof(short));stage=0;

	if (os->nhw_res1[0]==127)
	{
		count=1;
	}
	else 
	{
		nhwres1I[stage++]=(os->nhw_res1[0]<<1);count=0;
	}

	for (i=1;i<os->nhw_res1_len;i++)
	{
		if (os->nhw_res1[i]>=128)
		{
			e=(os->nhw_res1[i]-128);e>>=4;
			scan=os->nhw_res1[i]&15;
			if (os->nhw_res1[i-1]!=127) j=(nhwres1I[stage-1]&255)+(e<<1);
			else {os->nhw_res1[i]=127;count+=2;continue;}

			if (j>=254) {count++;os->nhw_res1[i]=127;}
			else nhwres1I[stage++]=(j)+(count<<8);

			j+=(scan<<1);
			if (j>=254) {count++;os->nhw_res1[i]=127;}
			else nhwres1I[stage++]=(j)+(count<<8);
		}
		else
		{
			if (os->nhw_res1[i]==127) count++;
			else
			{
				if (((os->nhw_res1[i]<<1)<(nhwres1I[stage-1]&255)) && (os->nhw_res1[i-1]!=127)) count++;

				nhwres1I[stage++]=(os->nhw_res1[i]<<1)+(count<<8);
			}
		}
	}

	for (i=0,count=0;i<os->nhw_res1_bit_len;i++)
	{
		nhwres1I[count++]+=os->nhw_res1_bit[i]>>7;
		nhwres1I[count++]+=(os->nhw_res1_bit[i]>>6)&1;
		nhwres1I[count++]+=(os->nhw_res1_bit[i]>>5)&1;
		nhwres1I[count++]+=(os->nhw_res1_bit[i]>>4)&1;
		nhwres1I[count++]+=(os->nhw_res1_bit[i]>>3)&1;
		nhwres1I[count++]+=(os->nhw_res1_bit[i]>>2)&1;
		nhwres1I[count++]+=(os->nhw_res1_bit[i]>>1)&1;
		nhwres1I[count++]+=os->nhw_res1_bit[i]&1;
	}

	free(os->nhw_res1);
	free(os->nhw_res1_bit);

	os->end_ch_res=0;os->d_size_tree1=0;
	for (i=0,count=0;i<os->nhw_res1_bit_len-1;i++)
	{
		if (!(os->nhw_res1_word[i]>>7)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res1_word[i]>>6)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res1_word[i]>>5)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res1_word[i]>>4)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res1_word[i]>>3)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res1_word[i]>>2)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res1_word[i]>>1)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!(os->nhw_res1_word[i]&1)) os->d_size_tree1++;else os->end_ch_res++;
	}

	nhwres1=(unsigned short*)malloc(os->end_ch_res*sizeof(short));
	nhwres2=(unsigned short*)malloc(os->d_size_tree1*sizeof(short));

	for (i=0,count=0,scan=0,res=0;i<os->nhw_res1_bit_len-1;i++)
	{
		if (!(os->nhw_res1_word[i]>>7)) nhwres2[scan++]=nhwres1I[count++];else nhwres1[res++]=nhwres1I[count++]; 
		if (!((os->nhw_res1_word[i]>>6)&1)) nhwres2[scan++]=nhwres1I[count++];else nhwres1[res++]=nhwres1I[count++];
		if (!((os->nhw_res1_word[i]>>5)&1)) nhwres2[scan++]=nhwres1I[count++];else nhwres1[res++]=nhwres1I[count++];
		if (!((os->nhw_res1_word[i]>>4)&1)) nhwres2[scan++]=nhwres1I[count++];else nhwres1[res++]=nhwres1I[count++];
		if (!((os->nhw_res1_word[i]>>3)&1)) nhwres2[scan++]=nhwres1I[count++];else nhwres1[res++]=nhwres1I[count++];
		if (!((os->nhw_res1_word[i]>>2)&1)) nhwres2[scan++]=nhwres1I[count++];else nhwres1[res++]=nhwres1I[count++];
		if (!((os->nhw_res1_word[i]>>1)&1)) nhwres2[scan++]=nhwres1I[count++];else nhwres1[res++]=nhwres1I[count++];
		if (!(os->nhw_res1_word[i]&1)) nhwres2[scan++]=nhwres1I[count++];else nhwres1[res++]=nhwres1I[count++];
	}

	free(nhwres1I);
	free(os->nhw_res1_word);

	os->nhw_res1_bit_len=os->end_ch_res;
	os->nhw_res2_bit_len=os->d_size_tree1;

////////////////////////////////////////////////////////////////////////////////////////////////

	if (im->setup->quality_setting>=HIGH1)
	{

	nhwresH1I=(unsigned short*)calloc((os->nhw_res5_bit_len<<3),sizeof(short));stage=0;

	if (os->nhw_res5[0]==127)
	{
		count=1;
	}
	else 
	{
		nhwresH1I[stage++]=(os->nhw_res5[0]<<1);count=0;
	}

	for (i=1;i<os->nhw_res5_len;i++)
	{
		if (os->nhw_res5[i]>=128)
		{
			e=(os->nhw_res5[i]-128);e>>=4;
			scan=os->nhw_res5[i]&15;
			if (os->nhw_res5[i-1]!=127) j=(nhwresH1I[stage-1]&255)+(e<<1);
			else {os->nhw_res5[i]=127;count+=2;continue;}

			if (j>=254) {count++;os->nhw_res5[i]=127;}
			else nhwresH1I[stage++]=(j)+(count<<8);

			j+=(scan<<1);
			if (j>=254) {count++;os->nhw_res5[i]=127;}
			else nhwresH1I[stage++]=(j)+(count<<8);
		}
		else
		{
			if (os->nhw_res5[i]==127) count++;
			else
			{
				if (((os->nhw_res5[i]<<1)<(nhwresH1I[stage-1]&255)) && (os->nhw_res5[i-1]!=127)) count++;

				nhwresH1I[stage++]=(os->nhw_res5[i]<<1)+(count<<8);
			}
		}
	}

	for (i=0,count=0;i<os->nhw_res5_bit_len;i++)
	{
		nhwresH1I[count++]+=os->nhw_res5_bit[i]>>7;
		nhwresH1I[count++]+=(os->nhw_res5_bit[i]>>6)&1;
		nhwresH1I[count++]+=(os->nhw_res5_bit[i]>>5)&1;
		nhwresH1I[count++]+=(os->nhw_res5_bit[i]>>4)&1;
		nhwresH1I[count++]+=(os->nhw_res5_bit[i]>>3)&1;
		nhwresH1I[count++]+=(os->nhw_res5_bit[i]>>2)&1;
		nhwresH1I[count++]+=(os->nhw_res5_bit[i]>>1)&1;
		nhwresH1I[count++]+=os->nhw_res5_bit[i]&1;
	}

	free(os->nhw_res5);
	free(os->nhw_res5_bit);

	os->end_ch_res=0;os->d_size_tree1=0;
	for (i=0,count=0;i<os->nhw_res5_bit_len-1;i++)
	{
		if (!(os->nhw_res5_word[i]>>7)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res5_word[i]>>6)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res5_word[i]>>5)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res5_word[i]>>4)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res5_word[i]>>3)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res5_word[i]>>2)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res5_word[i]>>1)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!(os->nhw_res5_word[i]&1)) os->d_size_tree1++;else os->end_ch_res++;
	}

	nhwresH1=(unsigned short*)malloc(os->end_ch_res*sizeof(short));
	nhwresH2=(unsigned short*)malloc(os->d_size_tree1*sizeof(short));

	for (i=0,count=0,scan=0,res=0;i<os->nhw_res5_bit_len-1;i++)
	{
		if (!(os->nhw_res5_word[i]>>7)) nhwresH2[scan++]=nhwresH1I[count++];else nhwresH1[res++]=nhwresH1I[count++]; 
		if (!((os->nhw_res5_word[i]>>6)&1)) nhwresH2[scan++]=nhwresH1I[count++];else nhwresH1[res++]=nhwresH1I[count++];
		if (!((os->nhw_res5_word[i]>>5)&1)) nhwresH2[scan++]=nhwresH1I[count++];else nhwresH1[res++]=nhwresH1I[count++];
		if (!((os->nhw_res5_word[i]>>4)&1)) nhwresH2[scan++]=nhwresH1I[count++];else nhwresH1[res++]=nhwresH1I[count++];
		if (!((os->nhw_res5_word[i]>>3)&1)) nhwresH2[scan++]=nhwresH1I[count++];else nhwresH1[res++]=nhwresH1I[count++];
		if (!((os->nhw_res5_word[i]>>2)&1)) nhwresH2[scan++]=nhwresH1I[count++];else nhwresH1[res++]=nhwresH1I[count++];
		if (!((os->nhw_res5_word[i]>>1)&1)) nhwresH2[scan++]=nhwresH1I[count++];else nhwresH1[res++]=nhwresH1I[count++];
		if (!(os->nhw_res5_word[i]&1)) nhwresH2[scan++]=nhwresH1I[count++];else nhwresH1[res++]=nhwresH1I[count++];
	}

	free(nhwresH1I);
	free(os->nhw_res5_word);

	os->nhw_res5_bit_len=os->end_ch_res;
	os->nhw_res5_len=os->d_size_tree1;

	}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (im->setup->quality_setting>HIGH1)
	{

	nhwresH3I=(unsigned long*)calloc((os->nhw_res6_bit_len<<3),sizeof(long));stage=0;

	if (os->nhw_res6[0]==127)
	{
		count=IM_DIM;
	}
	else 
	{
		nhwresH3I[stage++]=(os->nhw_res6[0]<<1);count=0;
	}

	for (i=1;i<os->nhw_res6_len;i++)
	{
		if (os->nhw_res6[i]>=128)
		{
			e=(os->nhw_res6[i]-128);e>>=4;
			scan=os->nhw_res6[i]&15;
			if (os->nhw_res6[i-1]!=127) j=(nhwresH3I[stage-1]&255)+(e<<1);
			else {os->nhw_res6[i]=127;count+=(2*IM_DIM);continue;}

			if (j>=254) {count+=IM_DIM;os->nhw_res6[i]=127;}
			else nhwresH3I[stage++]=(j)+count;

			j+=(scan<<1);
			if (j>=254) {count+=IM_DIM;os->nhw_res6[i]=127;}
			else nhwresH3I[stage++]=(j)+count;
		}
		else
		{
			if (os->nhw_res6[i]==127) count+=IM_DIM;
			else
			{
				if (((os->nhw_res6[i]<<1)<(nhwresH3I[stage-1]&255)) && (os->nhw_res6[i-1]!=127)) count+=IM_DIM;

				nhwresH3I[stage++]=(os->nhw_res6[i]<<1)+count;
			}
		}
	}

	for (i=0,count=0;i<os->nhw_res6_bit_len;i++)
	{
		nhwresH3I[count++]+=os->nhw_res6_bit[i]>>7;
		nhwresH3I[count++]+=(os->nhw_res6_bit[i]>>6)&1;
		nhwresH3I[count++]+=(os->nhw_res6_bit[i]>>5)&1;
		nhwresH3I[count++]+=(os->nhw_res6_bit[i]>>4)&1;
		nhwresH3I[count++]+=(os->nhw_res6_bit[i]>>3)&1;
		nhwresH3I[count++]+=(os->nhw_res6_bit[i]>>2)&1;
		nhwresH3I[count++]+=(os->nhw_res6_bit[i]>>1)&1;
		nhwresH3I[count++]+=os->nhw_res6_bit[i]&1;
	}

	free(os->nhw_res6);
	free(os->nhw_res6_bit);

	os->end_ch_res=0;os->d_size_tree1=0;
	for (i=0,count=0;i<os->nhw_res6_bit_len-1;i++)
	{
		if (!(os->nhw_res6_word[i]>>7)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res6_word[i]>>6)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res6_word[i]>>5)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res6_word[i]>>4)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res6_word[i]>>3)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res6_word[i]>>2)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!((os->nhw_res6_word[i]>>1)&1)) os->d_size_tree1++;else os->end_ch_res++;
		if (!(os->nhw_res6_word[i]&1)) os->d_size_tree1++;else os->end_ch_res++;
	}

	os->nhwresH3=(unsigned long*)malloc(os->end_ch_res*sizeof(long));
	os->nhwresH4=(unsigned long*)malloc(os->d_size_tree1*sizeof(long));

	for (i=0,count=0,scan=0,res=0;i<os->nhw_res6_bit_len-1;i++)
	{
		if (!(os->nhw_res6_word[i]>>7)) os->nhwresH4[scan++]=nhwresH3I[count++];
		else os->nhwresH3[res++]=nhwresH3I[count++]; 

		if (!((os->nhw_res6_word[i]>>6)&1)) os->nhwresH4[scan++]=nhwresH3I[count++];
		else os->nhwresH3[res++]=nhwresH3I[count++];

		if (!((os->nhw_res6_word[i]>>5)&1)) os->nhwresH4[scan++]=nhwresH3I[count++];
		else os->nhwresH3[res++]=nhwresH3I[count++];

		if (!((os->nhw_res6_word[i]>>4)&1)) os->nhwresH4[scan++]=nhwresH3I[count++];
		else os->nhwresH3[res++]=nhwresH3I[count++];

		if (!((os->nhw_res6_word[i]>>3)&1)) os->nhwresH4[scan++]=nhwresH3I[count++];
		else os->nhwresH3[res++]=nhwresH3I[count++];

		if (!((os->nhw_res6_word[i]>>2)&1)) os->nhwresH4[scan++]=nhwresH3I[count++];
		else os->nhwresH3[res++]=nhwresH3I[count++];

		if (!((os->nhw_res6_word[i]>>1)&1)) os->nhwresH4[scan++]=nhwresH3I[count++];
		else os->nhwresH3[res++]=nhwresH3I[count++];

		if (!(os->nhw_res6_word[i]&1)) os->nhwresH4[scan++]=nhwresH3I[count++];
		else os->nhwresH3[res++]=nhwresH3I[count++];
	}

	free(nhwresH3I);
	free(os->nhw_res6_word);

	os->nhw_res6_bit_len=os->end_ch_res;
	os->nhw_res6_len=os->d_size_tree1;

	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (im->setup->quality_setting>=LOW1)
	{

	nhwres3I=(unsigned short*)calloc((os->nhw_res3_bit_len<<3),sizeof(short));stage=0;
	os->d_size_tree1=0;os->end_ch_res=0;os->res_f1=0;os->res_f2=0;

	if (os->nhw_res3[0]==127)
	{
		count=1;
	}
	else 
	{
		nhwres3I[stage++]=(os->nhw_res3[0]<<1);count=0;
	}

	for (i=1;i<os->nhw_res3_len;i++)
	{
		if (os->nhw_res3[i]>=128)
		{
			e=(os->nhw_res3[i]-128);e>>=4;
			scan=os->nhw_res3[i]&15;
			if (os->nhw_res3[i-1]!=127) j=(nhwres3I[stage-1]&255)+(e<<1);
			else {os->nhw_res3[i]=127;count+=2;continue;}

			if (j>=254) {count++;os->nhw_res3[i]=127;}
			else nhwres3I[stage++]=(j)+(count<<8);

			j+=(scan<<1);
			if (j>=254) {count++;os->nhw_res3[i]=127;}
			else nhwres3I[stage++]=(j)+(count<<8);
		}
		else
		{
			if (os->nhw_res3[i]==127) count++;
			else
			{
				if (((os->nhw_res3[i]<<1)<(nhwres3I[stage-1]&255)) && (os->nhw_res3[i-1]!=127)) count++;

				nhwres3I[stage++]=(os->nhw_res3[i]<<1)+(count<<8);
			}
		}
	}

	for (i=0,count=0;i<os->nhw_res3_bit_len;i++)
	{
		nhwres3I[count++]+=os->nhw_res3_bit[i]>>7;
		nhwres3I[count++]+=(os->nhw_res3_bit[i]>>6)&1;
		nhwres3I[count++]+=(os->nhw_res3_bit[i]>>5)&1;
		nhwres3I[count++]+=(os->nhw_res3_bit[i]>>4)&1;
		nhwres3I[count++]+=(os->nhw_res3_bit[i]>>3)&1;
		nhwres3I[count++]+=(os->nhw_res3_bit[i]>>2)&1;
		nhwres3I[count++]+=(os->nhw_res3_bit[i]>>1)&1;
		nhwres3I[count++]+=os->nhw_res3_bit[i]&1;
	}

	free(os->nhw_res3);
	free(os->nhw_res3_bit);

	nhwres3=(unsigned short*)malloc((os->nhw_res3_bit_len<<3)*sizeof(short));
	nhwres4=(unsigned short*)malloc((os->nhw_res3_bit_len<<3)*sizeof(short));
	nhwres5=(unsigned short*)malloc((os->nhw_res3_bit_len<<3)*sizeof(short));
	nhwres6=(unsigned short*)malloc((os->nhw_res3_bit_len<<3)*sizeof(short));

	for (i=0,count=0,scan=0,res=0;i<((os->nhw_res3_bit_len<<1)-2);i++)
	{
		nhw_selectII=((os->nhw_res3_word[i]>>6)&3);

		if (!nhw_selectII) {nhwres4[os->d_size_tree1++]=nhwres3I[count++];}
		else if (nhw_selectII==1) {nhwres3[os->end_ch_res++]=nhwres3I[count++];}
		else if (nhw_selectII==2) {nhwres5[os->res_f1++]=nhwres3I[count++];}
		else {nhwres6[os->res_f2++]=nhwres3I[count++];}

		nhw_selectII=((os->nhw_res3_word[i]>>4)&3);

		if (!nhw_selectII) {nhwres4[os->d_size_tree1++]=nhwres3I[count++];}
		else if (nhw_selectII==1) {nhwres3[os->end_ch_res++]=nhwres3I[count++];}
		else if (nhw_selectII==2) {nhwres5[os->res_f1++]=nhwres3I[count++];}
		else {nhwres6[os->res_f2++]=nhwres3I[count++];}

		nhw_selectII=((os->nhw_res3_word[i]>>2)&3);

		if (!nhw_selectII) {nhwres4[os->d_size_tree1++]=nhwres3I[count++];}
		else if (nhw_selectII==1) {nhwres3[os->end_ch_res++]=nhwres3I[count++];}
		else if (nhw_selectII==2) {nhwres5[os->res_f1++]=nhwres3I[count++];}
		else {nhwres6[os->res_f2++]=nhwres3I[count++];}

		nhw_selectII=(os->nhw_res3_word[i]&3);

		if (!nhw_selectII) {nhwres4[os->d_size_tree1++]=nhwres3I[count++];}
		else if (nhw_selectII==1) {nhwres3[os->end_ch_res++]=nhwres3I[count++];}
		else if (nhw_selectII==2) {nhwres5[os->res_f1++]=nhwres3I[count++];}
		else {nhwres6[os->res_f2++]=nhwres3I[count++];}


	}

	free(nhwres3I);
	free(os->nhw_res3_word);

	}

	for (i=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(2*IM_DIM);j++,scan++)
		{
			if (im_nhw[scan]>1000)
			{
				if (im_nhw[scan]==1008)
				{
					im->im_jpeg[scan-1]=5;im->im_jpeg[scan+1]=5;
					if (j<IM_DIM) im->im_jpeg[scan]=5;else im->im_jpeg[scan]=6;
				}
				else if (im_nhw[scan]==1009)
				{
					im->im_jpeg[scan-1]=-5;im->im_jpeg[scan+1]=-5;
					if (j<IM_DIM) im->im_jpeg[scan]=-6;else im->im_jpeg[scan]=-7;
				}
				else if (im_nhw[scan]==1010)
				{
					im->im_jpeg[scan]=5;im->im_jpeg[scan+1]=5;
					im->im_jpeg[scan+(2*IM_DIM)]=5;im->im_jpeg[scan+(2*IM_DIM+1)]=5;
				}
				else if (im_nhw[scan]==1011)
				{
					im->im_jpeg[scan]=-5;im->im_jpeg[scan+1]=-5;
					im->im_jpeg[scan+(2*IM_DIM)]=-5;im->im_jpeg[scan+(2*IM_DIM+1)]=-5;
				}
				else if (im_nhw[scan]==1006)
				{
					im->im_jpeg[scan]=-6;im->im_jpeg[scan+1]=-6;
				}
				else if (im_nhw[scan]==1007)
				{
					im->im_jpeg[scan]=6;im->im_jpeg[scan+1]=6;
				}
			}
		}
	}

	for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM);j++,scan++)
		{
			if (im_nhw[scan]>1000)
			{
				if (im_nhw[scan]==1008)
				{
					im->im_jpeg[scan-1]=5;im->im_jpeg[scan]=6;im->im_jpeg[scan+1]=5;
				}
				else if (im_nhw[scan]==1009)
				{
					im->im_jpeg[scan-1]=-5;im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-5;
				}
				else if (im_nhw[scan]==1006)
				{
					if (scan<(2*IM_SIZE)) {im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-7;}
					else if ((scan&511)<IM_DIM) {im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-7;}
					else {im->im_jpeg[scan-IM_DIM]=-7;im->im_jpeg[scan-(3*IM_DIM)]=-7;im->im_jpeg[scan]=0;}
				}
				else if (im_nhw[scan]==1007)
				{
					if (scan<(2*IM_SIZE)) {im->im_jpeg[scan]=7;im->im_jpeg[scan+1]=7;}
					else if ((scan&511)<IM_DIM) {im->im_jpeg[scan]=7;im->im_jpeg[scan+1]=7;}
					else {im->im_jpeg[scan-IM_DIM]=7;im->im_jpeg[scan-(3*IM_DIM)]=7;im->im_jpeg[scan]=0;}
				}
			}
		}
	}

	for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i+IM_DIM,j=IM_DIM;j<(2*IM_DIM);j++,scan++)
		{
			if (im_nhw[scan]>1000)
			{
				if (im_nhw[scan]==1008)
				{
					im->im_jpeg[scan-1]=5;im->im_jpeg[scan]=6;im->im_jpeg[scan+1]=5;
				}
				else if (im_nhw[scan]==1009)
				{
					im->im_jpeg[scan-1]=-5;im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-5;
				}
				else if (im_nhw[scan]==1006)
				{
					if (scan<(2*IM_SIZE)) {im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-7;}
					else if ((scan&511)<IM_DIM) {im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-7;}
					else {im->im_jpeg[scan-IM_DIM]=-7;im->im_jpeg[scan-(3*IM_DIM)]=-7;im->im_jpeg[scan]=0;}
				}
				else if (im_nhw[scan]==1007)
				{
					if (scan<(2*IM_SIZE)) {im->im_jpeg[scan]=7;im->im_jpeg[scan+1]=7;}
					else if ((scan&511)<IM_DIM) {im->im_jpeg[scan]=7;im->im_jpeg[scan+1]=7;}
					else {im->im_jpeg[scan-IM_DIM]=7;im->im_jpeg[scan-(3*IM_DIM)]=7;im->im_jpeg[scan]=0;}
				}
			}
			else if (abs(im_nhw[scan])>8 && abs(im_nhw[scan])<16 && im->setup->quality_setting<HIGH3)
			{
				if (j>IM_DIM && j<((2*IM_DIM)-1))
				{
					if (abs(im_nhw[scan-1])<8) count++;
					if (abs(im_nhw[scan+1])<8) count++;
					if (abs(im_nhw[scan-(2*IM_DIM)])<8) count++;
					if (abs(im_nhw[scan+(2*IM_DIM)])<8) count++;

					if (count>=2)
					{
						if (im_nhw[scan]>0) im_nhw[scan]++;
						else im_nhw[scan]--;
					}

					count=0;
				}
			}
		}
	}

	nhw=0;
	for (i=0;i<(4*IM_SIZE>>2);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++) 
		{
			im->im_jpeg[scan++]=os->res_comp[nhw++];
		}
	}

	for (i=0,e=0,count=0;i<os->nhw_res4_len;i++)
	{
		if (os->nhw_res4[i]==128) count++;
		else if (os->nhw_res4[i]>128) 
		{
			e=(count<<9)+os->nhw_res4[i]-129;
			if (!(im->im_jpeg[e]&1)) im->im_jpeg[e]++;
			if (!(im->im_jpeg[e+1]&1)) im->im_jpeg[e+1]++;
			if (!(im->im_jpeg[e+2]&1)) im->im_jpeg[e+2]++;
			if (!(im->im_jpeg[e+3]&1)) im->im_jpeg[e+3]++;
			//printf("\n %d",e);
			count++;
		}
		else 
		{
			e=(count<<9)+os->nhw_res4[i]-1;
			if (!(im->im_jpeg[e]&1)) im->im_jpeg[e]++;
			if (!(im->im_jpeg[e+1]&1)) im->im_jpeg[e+1]++;
			if (!(im->im_jpeg[e+2]&1)) im->im_jpeg[e+2]++;
			if (!(im->im_jpeg[e+3]&1)) im->im_jpeg[e+3]++;
			//printf("\n %d",e);
		}
	}

	free(os->nhw_res4);

	for (i=0,exw1=0;i<os->exw_Y_end;i+=3,exw1+=3)
	{
		if (!os->exw_Y[i] && !os->exw_Y[i+1]) break;

		if (os->exw_Y[i+1]>=128) {scan=os->exw_Y[i+2]+255;os->exw_Y[i+1]-=128;}
		else {scan=-os->exw_Y[i+2];}

		count=(os->exw_Y[i]<<9)+os->exw_Y[i+1];

		im->im_jpeg[count]=scan;

	}

	for (i=(2*IM_DIM);i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i+1,j=1;j<IM_DIM-1;j++,scan++) 
		{
			if (abs(im_nhw[scan])>8)
			{
				if (abs(im_nhw[scan-(2*IM_DIM+1)])>8) continue;
				if (abs(im_nhw[scan-(2*IM_DIM)])>8) continue;
				if (abs(im_nhw[scan-(2*IM_DIM-1)])>8) continue;
				if (abs(im_nhw[scan-1])>8) continue;
				if (abs(im_nhw[scan+1])>8) continue;
				if (abs(im_nhw[scan+(2*IM_DIM-1)])>8) continue;
				if (abs(im_nhw[scan+(2*IM_DIM)])>8) continue;
				if (abs(im_nhw[scan+(2*IM_DIM+1)])>8) continue;
			
				if (i>=IM_SIZE || j>=(IM_DIM>>1))
				{
					if (im_nhw[scan]>0) im_nhw[scan]--;
					else im_nhw[scan]++;
				}
			}
		}
	}

	end_transform=0;
	im->setup->wavelet_type=WVLTS_53;
	wavelet_order=im->setup->wvlts_order;
	//for (stage=wavelet_order-1;stage>=0;stage--) wavelet_synthesis(im,(2*IM_DIM)>>stage,end_transform++,1);
	im_nhw2=(short*)im->im_process;

	wavelet_synthesis(im,(2*IM_DIM)>>1,end_transform++,1);

	if (im->setup->quality_setting>=HIGH1)
	{
		for (i=0;i<os->nhw_res5_bit_len;i++) 
		{
			im_nhw2[((nhwresH1[i]&65280)<<1)+(nhwresH1[i]&255)]-=3;
		}
		free(nhwresH1);

		for (i=0;i<os->nhw_res5_len;i++) 
		{
			im_nhw2[((nhwresH2[i]&65280)<<1)+(nhwresH2[i]&255)]+=3;
		}
		free(nhwresH2);
	}

	for (i=0;i<os->nhw_res1_bit_len;i++) 
	{
		im_nhw2[((nhwres1[i]&65280)<<1)+(nhwres1[i]&255)]-=5;
	}
	free(nhwres1);

	for (i=0;i<os->nhw_res2_bit_len;i++) 
	{
		im_nhw2[((nhwres2[i]&65280)<<1)+(nhwres2[i]&255)]+=5;
	}
	free(nhwres2);

	if (im->setup->quality_setting>=LOW1)
	{
		for (i=0;i<os->end_ch_res;i++) 
		{
			im_nhw2[((nhwres3[i]&65280)<<1)+(nhwres3[i]&255)]-=4;
			im_nhw2[((nhwres3[i]&65280)<<1)+(nhwres3[i]&255)+(2*IM_DIM)]-=3;
		}
		free(nhwres3);

		for (i=0;i<os->d_size_tree1;i++) 
		{
			im_nhw2[((nhwres4[i]&65280)<<1)+(nhwres4[i]&255)]+=4;
			im_nhw2[((nhwres4[i]&65280)<<1)+(nhwres4[i]&255)+(2*IM_DIM)]+=3;
		}
		free(nhwres4);

		for (i=0;i<os->res_f1;i++) 
		{
			im_nhw2[((nhwres5[i]&65280)<<1)+(nhwres5[i]&255)]+=2;
			im_nhw2[((nhwres5[i]&65280)<<1)+(nhwres5[i]&255)+(2*IM_DIM)]+=2;
			im_nhw2[((nhwres5[i]&65280)<<1)+(nhwres5[i]&255)+(4*IM_DIM)]+=2;
		}
		free(nhwres5);

		for (i=0;i<os->res_f2;i++) 
		{
			im_nhw2[((nhwres6[i]&65280)<<1)+(nhwres6[i]&255)]-=2;
			im_nhw2[((nhwres6[i]&65280)<<1)+(nhwres6[i]&255)+(2*IM_DIM)]-=2;
			im_nhw2[((nhwres6[i]&65280)<<1)+(nhwres6[i]&255)+(4*IM_DIM)]-=2;
		}
		free(nhwres6);
	}

	for (i=(2*IM_DIM),stage=0;i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i+1,j=1;j<(IM_DIM-2);j++,scan++)
		{
			res	   =   (im_nhw2[scan]<<3) -
						im_nhw2[scan-1]-im_nhw2[scan+1]-
						im_nhw2[scan-(2*IM_DIM)]-im_nhw2[scan+(2*IM_DIM)]-
						im_nhw2[scan-(2*IM_DIM+1)]-im_nhw2[scan+(2*IM_DIM-1)]-
						im_nhw2[scan-(2*IM_DIM-1)]-im_nhw2[scan+(2*IM_DIM+1)];

			j++;scan++;

			count   =  (im_nhw2[scan]<<3) -
						im_nhw2[scan-1]-im_nhw2[scan+1]-
						im_nhw2[scan-(2*IM_DIM)]-im_nhw2[scan+(2*IM_DIM)]-
						im_nhw2[scan-(2*IM_DIM+1)]-im_nhw2[scan+(2*IM_DIM-1)]-
						im_nhw2[scan-(2*IM_DIM-1)]-im_nhw2[scan+(2*IM_DIM+1)];


			if (res>41 && res<108 && count<16)
			{
				im_nhw2[scan-1]+=16000;stage++;
			}
			else if (res<-41 && res>-108 && count>-16)
			{
				im_nhw2[scan-1]+=16000;stage++;
			}
			else if (count>41 && count<108 && res<16)
			{
				im_nhw2[scan]+=16000;stage++;
			}
			else if (count<-41 && count>-108 && res>-16)
			{
				im_nhw2[scan]+=16000;stage++;
			}
		}
	}

	nhwres1=(unsigned short*)malloc(stage*sizeof(short));

	for (i=(2*IM_DIM),count=0;i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++)
		{
			if (im_nhw2[scan]>10000)
			{
				nhwres1[count++]=(i>>1)+j;	
				im_nhw2[scan]-=16000;
			}
		}
	}

	for (i=0;i<IM_DIM;i++,im_nhw+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan+=(2*IM_DIM)) im_nhw[j]=im_nhw2[scan];
	}

	wavelet_synthesis2(im,os,(2*IM_DIM),end_transform,1);

	im_nhw=(short*)im->im_jpeg;

	for (i=0;i<count;i++)
	{
		scan=nhwres1[i]>>8;
		scan<<=10;
		scan+=(nhwres1[i]&255);

		res	   =   (im_nhw[scan]<<3) -
					im_nhw[scan-1]-im_nhw[scan+1]-
					im_nhw[scan-(2*IM_DIM)]-im_nhw[scan+(2*IM_DIM)]-
					im_nhw[scan-(2*IM_DIM+1)]-im_nhw[scan+(2*IM_DIM-1)]-
					im_nhw[scan-(2*IM_DIM-1)]-im_nhw[scan+(2*IM_DIM+1)];

		if (abs(res)<116)
		{
			im_nhw[scan]=((im_nhw[scan]<<2)
							+im_nhw[scan-1]+im_nhw[scan+1]+im_nhw[scan-(2*IM_DIM)]+im_nhw[scan+(2*IM_DIM)]+4)>>3;
		}
	}

	free(nhwres1);

	wavelet_synthesis(im,(2*IM_DIM),end_transform,3);
	
	free(im->im_jpeg);

	im_nhw=(short*)im->im_process;

	im->im_bufferY=(unsigned char*)malloc(4*IM_SIZE*sizeof(char));
	nhw_scale=(unsigned char*)im->im_bufferY;

	for (i=0;i<(4*IM_SIZE);i++)
	{
		if ((im_nhw[i]>>8)!=0)
		{
			if (im_nhw[i]<0) nhw_scale[i]=0;
			else if (im_nhw[i]>255) nhw_scale[i]=255;
		}
		else 
		{
			nhw_scale[i]=im_nhw[i];
		}
	}

	free(im->im_process);

	// U
	im->im_nhw3=(short*)calloc(2*IM_SIZE,sizeof(short));
	retrieve_pixel_UV_comp(im,os,(2*IM_SIZE-1),os->packet2,im->im_nhw3);
	free(os->packet2);

	im->im_jpeg=(short*)malloc(IM_SIZE*sizeof(short));
	im_nhw=(short*)im->im_jpeg;
	im_nhw2=(short*)im->im_nhw3;

	for (j=0,count=0;j<(IM_DIM);)
	{
		for (i=0;i<(IM_DIM>>1);i++)
		{
			im_nhw[j]=im_nhw2[count];
			im_nhw[j+1]=im_nhw2[count+2];
			im_nhw[j+2]=im_nhw2[count+4];
			im_nhw[j+3]=im_nhw2[count+6];
			im_nhw[j+4]=im_nhw2[count+8];
			im_nhw[j+5]=im_nhw2[count+10];
			im_nhw[j+6]=im_nhw2[count+12];
			im_nhw[j+7]=im_nhw2[count+14];
	
			j+=(IM_DIM);
			im_nhw[j+7]=im_nhw2[count+16];
			im_nhw[j+6]=im_nhw2[count+18];
			im_nhw[j+5]=im_nhw2[count+20];
			im_nhw[j+4]=im_nhw2[count+22];
			im_nhw[j+3]=im_nhw2[count+24];
			im_nhw[j+2]=im_nhw2[count+26];
			im_nhw[j+1]=im_nhw2[count+28];
			im_nhw[j]=im_nhw2[count+30];

			j+=(IM_DIM);
			count+=32;
		}

		j-=((IM_SIZE)-8);
	}

	/*for (i=0;i<(IM_SIZE>>1);i+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++) 
		{
			if (im->im_jpeg[scan]>0) im->im_jpeg[scan]--;
			else if (im->im_jpeg[scan]<0) im->im_jpeg[scan]++;
		}
	}*/


	for (i=0,nhw=(IM_SIZE>>2);i<(IM_SIZE>>2);i+=IM_DIM)
	{
		for (j=0;j<(IM_DIM>>2);j++) 
		{
			im->im_jpeg[j+i]=os->res_comp[nhw++];
		}
	}

	exw1+=2;
	i=exw1;

	for (;i<os->exw_Y_end;i+=3,exw1+=3)
	{
		if (!os->exw_Y[i] && !os->exw_Y[i+1]) break;

		if (os->exw_Y[i+1]>=128) {scan=os->exw_Y[i+2]+255;os->exw_Y[i+1]-=128;}
		else {scan=-os->exw_Y[i+2];}

		count=(os->exw_Y[i]<<8)+os->exw_Y[i+1];

		im->im_jpeg[count]=scan;

	}

	im->im_process=(short*)malloc(IM_SIZE*sizeof(short));

	end_transform=0;
	//for (stage=wavelet_order-1;stage>=0;stage--) wavelet_synthesis(im,IM_DIM>>stage,end_transform++,0);

	wavelet_synthesis(im,(IM_DIM>>1),end_transform++,0);

	im_nhw2=(short*)im->im_process;
	im_nhw=(short*)im->im_jpeg;

	for (i=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i+(IM_DIM>>1),j=(IM_DIM>>1);j<IM_DIM;j++,scan++)
		{
			if (im_nhw[scan]>5000)
			{
				if (im_nhw[scan]==5005)
				{
					im_nhw2[scan-(IM_DIM>>1)]-=4;im_nhw2[scan-(IM_DIM>>1)+1]-=4;im_nhw[scan]=0;
				}
				else if (im_nhw[scan]==5006)
				{
					im_nhw2[scan-(IM_DIM>>1)]+=4;im_nhw2[scan-(IM_DIM>>1)+1]+=4;im_nhw[scan]=0;
				}
				else if (im_nhw[scan]==5003) 
				{
					im_nhw2[scan-(IM_DIM>>1)]-=6;im_nhw[scan]=0;
				}
				else if (im_nhw[scan]==5004)
				{
					im_nhw2[scan-(IM_DIM>>1)]+=6;im_nhw[scan]=0;
				}
			}
		}
	}

	for (i=(IM_SIZE>>1);i<(IM_SIZE);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM);j++,scan++)
		{
			if (im_nhw[scan]>5000)
			{
				if (im_nhw[scan]==5005)
				{
					if (j<(IM_DIM>>1))
					{
						im_nhw2[scan-(IM_SIZE>>1)]-=4;im_nhw2[scan-(IM_SIZE>>1)+1]-=4;im_nhw[scan]=0;
					}
					else
					{
						im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)]-=4;im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)+1]-=4;im_nhw[scan]=0;
					}
				}
				else if (im_nhw[scan]==5006)
				{
					if (j<(IM_DIM>>1))
					{
						im_nhw2[scan-(IM_SIZE>>1)]+=4;im_nhw2[scan-(IM_SIZE>>1)+1]+=4;im_nhw[scan]=0;
					}
					else
					{
						im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)]+=4;im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)+1]+=4;im_nhw[scan]=0;
					}
				}
				else if (im_nhw[scan]==5003) 
				{
					if (j<(IM_DIM>>1))
					{
						im_nhw2[scan-(IM_SIZE>>1)]-=6;im_nhw[scan]=0;
					}
					else
					{
						im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)]-=6;im_nhw[scan]=0;
					}
				}
				else if (im_nhw[scan]==5004)
				{
					if (j<(IM_DIM>>1))
					{
						im_nhw2[scan-(IM_SIZE>>1)]+=6;im_nhw[scan]=0;
					}
					else 
					{
						im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)]+=6;im_nhw[scan]=0;
					}
				}
			}
		}
	}

	for (i=0;i<(IM_DIM>>1);i++,im_nhw+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan+=(IM_DIM)) im_nhw[j]=im_nhw2[scan];
	}

	wavelet_synthesis(im,IM_DIM,end_transform,0);

	free(im->im_jpeg);

	im_nhw=(short*)im->im_process;

	for (i=IM_DIM;i<IM_SIZE-IM_DIM;i+=(IM_DIM))
	{
		for (scan=i+1,j=1;j<(IM_DIM-1);j++,scan++)
		{
			res	   =   (im_nhw[scan]<<3) -
						im_nhw[scan-1]-im_nhw[scan+1]-
						im_nhw[scan-(IM_DIM)]-im_nhw[scan+(IM_DIM)]-
						im_nhw[scan-(IM_DIM+1)]-im_nhw[scan+(IM_DIM-1)]-
						im_nhw[scan-(IM_DIM-1)]-im_nhw[scan+(IM_DIM+1)];

			if (abs(res)>60)
			{
				if (res>0)
				{
					if (res>160) im_nhw[scan]+=3;
					else if (res>60) im_nhw[scan]+=2;
				}
				else 
				{
					if (res<-160) im_nhw[scan]-=3;
					else if (res<-60) im_nhw[scan]-=2;
				}
			}
		}
	}

	for (i=0;i<IM_SIZE;i++) 
	{
		if ((im_nhw[i]>>8)!=0) 
		{
			if (im_nhw[i]<0) im_nhw[i]=0;
			else if (im_nhw[i]>255) im_nhw[i]=255;
		}
	}

	im->scale=(unsigned char*)malloc(2*IM_SIZE*sizeof(char));
	nhw_scale=(unsigned char*)im->scale;

	/*for (j=0;j<IM_DIM;j++)
	{
		for (i=0;i<(IM_SIZE-IM_DIM);i+=(IM_DIM))
		{
			im->scale[j+(2*i)]=im->im_process[j+i];
			im->scale[j+(2*i)+IM_DIM]=((im->im_process[j+i]+im->im_process[j+i+IM_DIM]+1)>>1);
		}

		im->scale[j+(2*IM_SIZE-2*IM_DIM)]=im->im_process[j+(IM_SIZE-IM_DIM)];
		im->scale[j+(2*IM_SIZE-IM_DIM)]=im->im_process[j+(IM_SIZE-IM_DIM)];

	}*/

	// faster version
	for (j=0;j<IM_DIM;j++)
	{
		stage=j;count=j;
		for (;count<(IM_SIZE-IM_DIM);count+=IM_DIM,stage+=(2*IM_DIM)) 
		{
			nhw_scale[stage]=(im_nhw[count]);
			nhw_scale[stage+IM_DIM]=((im_nhw[count]+im_nhw[count+IM_DIM]+1)>>1);
		}

		nhw_scale[j+(2*IM_SIZE-2*IM_DIM)]=im_nhw[j+(IM_SIZE-IM_DIM)];
		nhw_scale[j+(2*IM_SIZE-IM_DIM)]=im_nhw[j+(IM_SIZE-IM_DIM)];

	}

	free(im->im_process);
	im->im_bufferU=(unsigned char*)malloc(4*IM_SIZE*sizeof(char));
	nhw_chr=(unsigned char*)im->im_bufferU;

	/*for (i=0,e=0;i<(4*IM_SIZE);i+=(IM_DIM*2),e+=IM_DIM)
	{
		for (j=0;j<IM_DIM-1;j++)
		{
			im->im_bufferU[(j*2)+i]=im->scale[j+e];
			im->im_bufferU[(j*2)+1+i]=(im->scale[j+e]+im->scale[j+e+1]+1)>>1;
		}

		im->im_bufferU[(IM_DIM*2)-2+i]=im->scale[IM_DIM-1+e];
		im->im_bufferU[(IM_DIM*2)-1+i]=im->scale[IM_DIM-1+e];
	}*/

	//faster version
	for (count=0,stage=0;count<(4*IM_SIZE);count+=(IM_DIM*2),stage+=IM_DIM)
	{
		for (j=0;j<IM_DIM-1;j++,stage++) 
		{
			nhw_chr[count++]=nhw_scale[stage];
			nhw_chr[count++]=(nhw_scale[stage]+nhw_scale[stage+1]+1)>>1;
		}

		im->im_bufferU[count++]=nhw_scale[stage];
		im->im_bufferU[count]=nhw_scale[stage];

		count-=(2*IM_DIM-1);
		stage-=(IM_DIM-1);
	}

	free(im->scale);

	im->im_jpeg=(short*)malloc(IM_SIZE*sizeof(short));

	// V

	im_nhw=(short*)im->im_jpeg;
	im_nhw2=(short*)im->im_nhw3;

	for (j=0,count=1;j<(IM_DIM);)
	{
		for (i=0;i<(IM_DIM>>1);i++)
		{
			im_nhw[j]=im_nhw2[count];
			im_nhw[j+1]=im_nhw2[count+2];
			im_nhw[j+2]=im_nhw2[count+4];
			im_nhw[j+3]=im_nhw2[count+6];
			im_nhw[j+4]=im_nhw2[count+8];
			im_nhw[j+5]=im_nhw2[count+10];
			im_nhw[j+6]=im_nhw2[count+12];
			im_nhw[j+7]=im_nhw2[count+14];
	
			j+=(IM_DIM);
			im_nhw[j+7]=im_nhw2[count+16];
			im_nhw[j+6]=im_nhw2[count+18];
			im_nhw[j+5]=im_nhw2[count+20];
			im_nhw[j+4]=im_nhw2[count+22];
			im_nhw[j+3]=im_nhw2[count+24];
			im_nhw[j+2]=im_nhw2[count+26];
			im_nhw[j+1]=im_nhw2[count+28];
			im_nhw[j]=im_nhw2[count+30];

			j+=(IM_DIM);
			count+=32;
		}

		j-=((IM_SIZE)-8);
	}

	free(im->im_nhw3);

	/*for (i=0;i<(IM_SIZE>>1);i+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++) 
		{
			if (im->im_jpeg[scan]>0) im->im_jpeg[scan]--;
			else if (im->im_jpeg[scan]<0) im->im_jpeg[scan]++;
		}
	}*/

	for (i=0,nhw=(IM_SIZE>>2)+(IM_SIZE>>4);i<(IM_SIZE>>2);i+=IM_DIM)
	{
		for (j=0;j<(IM_DIM>>2);j++) 
		{
			im->im_jpeg[j+i]=os->res_comp[nhw++];
		}
	}
	
	free(os->res_comp);

	exw1+=2;
	i=exw1;

	for (;i<os->exw_Y_end;i+=3)
	{
		if (os->exw_Y[i+1]>=128) {scan=os->exw_Y[i+2]+255;os->exw_Y[i+1]-=128;}
		else {scan=-os->exw_Y[i+2];}

		count=(os->exw_Y[i]<<8)+os->exw_Y[i+1];

		im->im_jpeg[count]=scan;

	}

	free(os->exw_Y);
	im->im_process=(short*)malloc(IM_SIZE*sizeof(short));

	end_transform=0;
	//for (stage=wavelet_order-1;stage>=0;stage--) wavelet_synthesis(im,IM_DIM>>stage,end_transform++,0);

	wavelet_synthesis(im,(IM_DIM>>1),end_transform++,0);

	im_nhw2=(short*)im->im_process;
	im_nhw=(short*)im->im_jpeg;

	for (i=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i+(IM_DIM>>1),j=(IM_DIM>>1);j<IM_DIM;j++,scan++)
		{
			if (im_nhw[scan]>5000)
			{
				if (im_nhw[scan]==5005)
				{
					im_nhw2[scan-(IM_DIM>>1)]-=4;im_nhw2[scan-(IM_DIM>>1)+1]-=4;im_nhw[scan]=0;
				}
				else if (im_nhw[scan]==5006)
				{
					im_nhw2[scan-(IM_DIM>>1)]+=4;im_nhw2[scan-(IM_DIM>>1)+1]+=4;im_nhw[scan]=0;
				}
				else if (im_nhw[scan]==5003) 
				{
					im_nhw2[scan-(IM_DIM>>1)]-=6;im_nhw[scan]=0;
				}
				else if (im_nhw[scan]==5004)
				{
					im_nhw2[scan-(IM_DIM>>1)]+=6;im_nhw[scan]=0;
				}
			}
		}
	}


	for (i=(IM_SIZE>>1);i<(IM_SIZE);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM);j++,scan++)
		{
			if (im_nhw[scan]>5000)
			{
				if (im_nhw[scan]==5005)
				{
					if (j<(IM_DIM>>1))
					{
						im_nhw2[scan-(IM_SIZE>>1)]-=4;im_nhw2[scan-(IM_SIZE>>1)+1]-=4;im_nhw[scan]=0;
					}
					else
					{
						im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)]-=4;im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)+1]-=4;im_nhw[scan]=0;
					}
				}
				else if (im_nhw[scan]==5006)
				{
					if (j<(IM_DIM>>1))
					{
						im_nhw2[scan-(IM_SIZE>>1)]+=4;im_nhw2[scan-(IM_SIZE>>1)+1]+=4;im_nhw[scan]=0;
					}
					else
					{
						im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)]+=4;im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)+1]+=4;im_nhw[scan]=0;
					}
				}
				else if (im_nhw[scan]==5003) 
				{
					if (j<(IM_DIM>>1))
					{
						im_nhw2[scan-(IM_SIZE>>1)]-=6;im_nhw[scan]=0;
					}
					else
					{
						im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)]-=6;im_nhw[scan]=0;
					}
				}
				else if (im_nhw[scan]==5004)
				{
					if (j<(IM_DIM>>1))
					{
						im_nhw2[scan-(IM_SIZE>>1)]+=6;im_nhw[scan]=0;
					}
					else 
					{
						im_nhw2[scan-(IM_SIZE>>1)-(IM_DIM>>1)]+=6;im_nhw[scan]=0;
					}
				}
			}
		}
	}

	for (i=0;i<(IM_DIM>>1);i++,im_nhw+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan+=(IM_DIM)) im_nhw[j]=im_nhw2[scan];
	}

	wavelet_synthesis(im,IM_DIM,end_transform,0);

	free(im->im_jpeg);

	im_nhw=(short*)im->im_process;

	for (i=IM_DIM;i<IM_SIZE-IM_DIM;i+=(IM_DIM))
	{
		for (scan=i+1,j=1;j<(IM_DIM-1);j++,scan++)
		{
			res	   =   (im_nhw[scan]<<3) -
						im_nhw[scan-1]-im_nhw[scan+1]-
						im_nhw[scan-(IM_DIM)]-im_nhw[scan+(IM_DIM)]-
						im_nhw[scan-(IM_DIM+1)]-im_nhw[scan+(IM_DIM-1)]-
						im_nhw[scan-(IM_DIM-1)]-im_nhw[scan+(IM_DIM+1)];

			if (abs(res)>60)
			{
				if (res>0)
				{
					if (res>160) im_nhw[scan]+=3;
					else if (res>60) im_nhw[scan]+=2;
				}
				else 
				{
					if (res<-160) im_nhw[scan]-=3;
					else if (res<-60) im_nhw[scan]-=2;
				}
			}
		}
	}

	for (i=0;i<IM_SIZE;i++) 
	{
		if ((im_nhw[i]>>8)!=0) 
		{
			if (im_nhw[i]<0) im_nhw[i]=0;
			else if (im_nhw[i]>255) im_nhw[i]=255;
		}
	}

	im->scale=(unsigned char*)malloc(2*IM_SIZE*sizeof(char));
	nhw_scale=(unsigned char*)im->scale;

	/*for (j=0;j<IM_DIM;j++)
	{
		for (i=0;i<(IM_SIZE-IM_DIM);i+=(IM_DIM))
		{
			im->scale[j+(2*i)]=im->im_process[j+i];
			im->scale[j+(2*i)+IM_DIM]=((im->im_process[j+i]+im->im_process[j+i+IM_DIM]+1)>>1);
		}

		im->scale[j+(2*IM_SIZE-2*IM_DIM)]=im->im_process[j+(IM_SIZE-IM_DIM)];
		im->scale[j+(2*IM_SIZE-IM_DIM)]=im->im_process[j+(IM_SIZE-IM_DIM)];

	}*/
	// faster version
	for (j=0;j<IM_DIM;j++)
	{
		stage=j;count=j;
		for (;count<(IM_SIZE-IM_DIM);count+=IM_DIM,stage+=(2*IM_DIM)) 
		{
			nhw_scale[stage]=(im_nhw[count]);
			nhw_scale[stage+IM_DIM]=((im_nhw[count]+im_nhw[count+IM_DIM]+1)>>1);
		}

		nhw_scale[j+(2*IM_SIZE-2*IM_DIM)]=im_nhw[j+(IM_SIZE-IM_DIM)];
		nhw_scale[j+(2*IM_SIZE-IM_DIM)]=im_nhw[j+(IM_SIZE-IM_DIM)];

	}

	free(im->im_process);
	im->im_bufferV=(unsigned char*)malloc(4*IM_SIZE*sizeof(char));
	nhw_chr=(unsigned char*)im->im_bufferV;

	/*for (i=0,e=0;i<(4*IM_SIZE);i+=(IM_DIM*2),e+=IM_DIM)
	{
		for (j=0;j<IM_DIM-1;j++)
		{
			im->im_bufferU[(j*2)+i]=im->scale[j+e];
			im->im_bufferU[(j*2)+1+i]=(im->scale[j+e]+im->scale[j+e+1]+1)>>1;
		}

		im->im_bufferU[(IM_DIM*2)-2+i]=im->scale[IM_DIM-1+e];
		im->im_bufferU[(IM_DIM*2)-1+i]=im->scale[IM_DIM-1+e];
	}*/

	//faster version
	for (count=0,stage=0;count<(4*IM_SIZE);count+=(IM_DIM*2),stage+=IM_DIM)
	{
		for (j=0;j<IM_DIM-1;j++,stage++) 
		{
			nhw_chr[count++]=nhw_scale[stage];
			nhw_chr[count++]=(nhw_scale[stage]+nhw_scale[stage+1]+1)>>1;
		}

		im->im_bufferV[count++]=nhw_scale[stage];
		im->im_bufferV[count]=nhw_scale[stage];

		count-=(2*IM_DIM-1);
		stage-=(IM_DIM-1);
	}

	free(im->scale);

}

int parse_file(image_buffer *imd,decode_state *os,char** argv)
{
	FILE *compressed_file;
	int i,j,ch,e,a=0,mem,run,nhw;
	char uv_small_dc_offset[8][2]={{0,4},{0,-4},{4,0},{-4,0},{4,4},{4,-4},{-4,4},{-4,-4}};

	imd->setup=(codec_setup*)malloc(sizeof(codec_setup));

	compressed_file=fopen(argv[1],"rb");

	if (compressed_file==NULL)
	{
		printf("\nCould not open file");
		exit(-1);
	}

	fread(&imd->setup->RES_HIGH,1,1,compressed_file);
	fread(&imd->setup->quality_setting,1,1,compressed_file);

	if (imd->setup->RES_HIGH>6)
	{
		printf("\nNot an .nhw file");exit(-1);
	}

	imd->setup->colorspace=YUV;
	imd->setup->wavelet_type=WVLTS_53;
	imd->setup->wvlts_order=2;

	fread(&os->d_size_tree1,2,1,compressed_file);
	fread(&os->d_size_tree2,2,1,compressed_file);
	fread(&os->d_size_data1,4,1,compressed_file);
	fread(&os->d_size_data2,4,1,compressed_file);
	fread(&os->tree_end,2,1,compressed_file);
	fread(&os->exw_Y_end,2,1,compressed_file);
	fread(&os->nhw_res1_len,2,1,compressed_file);
	if (imd->setup->quality_setting>=LOW1)
	{
		fread(&os->nhw_res3_len,2,1,compressed_file);
		fread(&os->nhw_res3_bit_len,2,1,compressed_file);
	}
	fread(&os->nhw_res4_len,2,1,compressed_file);
	fread(&os->nhw_res1_bit_len,2,1,compressed_file);

	if (imd->setup->quality_setting>=HIGH1)
	{
		fread(&os->nhw_res5_len,2,1,compressed_file);
		fread(&os->nhw_res5_bit_len,2,1,compressed_file);

		os->nhw_res5=(unsigned char*)malloc(os->nhw_res5_len*sizeof(char));
		os->nhw_res5_bit=(unsigned char*)malloc(os->nhw_res5_bit_len*sizeof(char));
		os->nhw_res5_word=(unsigned char*)malloc(os->nhw_res5_bit_len*sizeof(char));
	}

	if (imd->setup->quality_setting>HIGH1)
	{
		fread(&os->nhw_res6_len,4,1,compressed_file);
		fread(&os->nhw_res6_bit_len,2,1,compressed_file);
		fread(&os->nhw_char_res1_len,2,1,compressed_file);

		os->nhw_res6=(unsigned char*)malloc(os->nhw_res6_len*sizeof(char));
		os->nhw_res6_bit=(unsigned char*)malloc(os->nhw_res6_bit_len*sizeof(char));
		os->nhw_res6_word=(unsigned char*)malloc(os->nhw_res6_bit_len*sizeof(char));
		os->nhw_char_res1=(unsigned short*)malloc(os->nhw_char_res1_len*sizeof(short));

		if (imd->setup->quality_setting>HIGH2)
		{
			fread(&os->qsetting3_len,2,1,compressed_file);
			os->high_qsetting3=(unsigned long*)malloc(os->qsetting3_len*sizeof(long));
		}
	}

	fread(&os->nhw_select1,2,1,compressed_file);
	fread(&os->highres_comp_len,2,1,compressed_file);
	fread(&os->end_ch_res,2,1,compressed_file);

	os->res_comp=(unsigned char*)malloc((96*IM_DIM+1)*sizeof(char));
	os->d_tree1=(unsigned char*)calloc(os->d_size_tree1,sizeof(char));
	os->d_tree2=(unsigned char*)calloc(os->d_size_tree2,sizeof(char));
	os->exw_Y=(unsigned char*)malloc(os->exw_Y_end*sizeof(char));
	os->nhw_res1=(unsigned char*)malloc(os->nhw_res1_len*sizeof(char));
	os->nhw_res1_bit=(unsigned char*)malloc(os->nhw_res1_bit_len*sizeof(char));
	os->nhw_res1_word=(unsigned char*)malloc(os->nhw_res1_bit_len*sizeof(char));
	if (imd->setup->quality_setting>=LOW1)
	{
		os->nhw_res3=(unsigned char*)malloc(os->nhw_res3_len*sizeof(char));
		os->nhw_res3_bit=(unsigned char*)malloc(os->nhw_res3_bit_len*sizeof(char));
		os->nhw_res3_word=(unsigned char*)malloc((os->nhw_res3_bit_len<<1)*sizeof(char));
	}
	os->nhw_res4=(unsigned char*)malloc(os->nhw_res4_len*sizeof(char));
	os->nhw_select_word1=(unsigned char*)malloc(os->nhw_select1*sizeof(char));
	os->res_U_64=(unsigned char*)malloc((IM_DIM<<1)*sizeof(char));
	os->res_V_64=(unsigned char*)malloc((IM_DIM<<1)*sizeof(char));
	os->highres_comp=(unsigned char*)malloc(os->highres_comp_len*sizeof(char));
	os->res_ch=(unsigned char*)malloc(os->end_ch_res*sizeof(char));
	os->packet1=(unsigned long*)malloc(os->d_size_data1*sizeof(long));
	os->packet2=(unsigned long*)malloc((os->d_size_data2-os->d_size_data1)*sizeof(long));

	// COMPRESSED FILE DATA
	fread(os->d_tree1,os->d_size_tree1,1,compressed_file);
	fread(os->d_tree2,os->d_size_tree2,1,compressed_file);
	fread(os->exw_Y,os->exw_Y_end,1,compressed_file);
	fread(os->nhw_res1,os->nhw_res1_len,1,compressed_file);
	fread(os->nhw_res1_bit,os->nhw_res1_bit_len,1,compressed_file);
	fread(os->nhw_res1_word,os->nhw_res1_bit_len,1,compressed_file);
	fread(os->nhw_res4,os->nhw_res4_len,1,compressed_file);

	if (imd->setup->quality_setting>=LOW1)
	{
		fread(os->nhw_res3,os->nhw_res3_len,1,compressed_file);
		fread(os->nhw_res3_bit,os->nhw_res3_bit_len,1,compressed_file);
		fread(os->nhw_res3_word,(os->nhw_res3_bit_len<<1),1,compressed_file);
	}

	if (imd->setup->quality_setting>=HIGH1)
	{
		fread(os->nhw_res5,os->nhw_res5_len,1,compressed_file);
		fread(os->nhw_res5_bit,os->nhw_res5_bit_len,1,compressed_file);
		fread(os->nhw_res5_word,os->nhw_res5_bit_len,1,compressed_file);
	}

	if (imd->setup->quality_setting>HIGH1)
	{
		fread(os->nhw_res6,os->nhw_res6_len,1,compressed_file);
		fread(os->nhw_res6_bit,os->nhw_res6_bit_len,1,compressed_file);
		fread(os->nhw_res6_word,os->nhw_res6_bit_len,1,compressed_file);
		fread(os->nhw_char_res1,os->nhw_char_res1_len,2,compressed_file);

		if (imd->setup->quality_setting>HIGH2)
		{
			fread(os->high_qsetting3,os->qsetting3_len,4,compressed_file);
		}
	}

	fread(os->nhw_select_word1,os->nhw_select1,1,compressed_file);
	fread(os->res_U_64,(IM_DIM<<1),1,compressed_file);
	fread(os->res_V_64,(IM_DIM<<1),1,compressed_file);
	fread(os->highres_comp,os->highres_comp_len,1,compressed_file);
	fread(os->res_ch,os->end_ch_res,1,compressed_file);
	fread(os->packet1,os->d_size_data1*4,1,compressed_file);
	fread(os->packet2,(os->d_size_data2-os->d_size_data1)*4,1,compressed_file);

	fclose(compressed_file);

	os->res_comp[0]=os->res_ch[0];

	if ((imd->setup->RES_HIGH&3)==1) goto L6; 
	if ((imd->setup->RES_HIGH&3)==2) goto L8;

	for (j=1,i=1,a=0,mem=0;j<(IM_SIZE>>2);i++)
	{
		if (os->res_ch[i]>=128)
		{
			os->res_comp[j++]=os->highres_comp[a++];
			os->res_comp[j++]=((os->res_ch[i]-128)<<1);
		}
		else
		{
			if (os->res_ch[i]<16)
			{
				run=(os->res_ch[i]>>3)&1;
				nhw=os->res_comp[j-1];
				for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
				if ((os->res_ch[i]&7)==0) continue;
				else if ((os->res_ch[i]&7)==1) {os->res_comp[j]=os->res_comp[j-1]+2;j++;}
				else if ((os->res_ch[i]&7)==2) 
				{
					os->res_comp[j]=os->res_comp[j-1]+2;j++;
					os->res_comp[j]=os->res_comp[j-1]-2;j++;
				}
				else if ((os->res_ch[i]&7)==3) 
				{
					os->res_comp[j]=os->res_comp[j-1]+2;j++;
					os->res_comp[j]=os->res_comp[j-1];j++;
					//os->res_comp[j]=os->res_comp[j-1];j++;
				}
				else if ((os->res_ch[i]&7)==4) 
				{
					os->res_comp[j]=os->res_comp[j-1]-2;j++;
					os->res_comp[j]=os->res_comp[j-1]+2;j++;
					//os->res_comp[j]=os->res_comp[j-1];j++;
				}
				else if ((os->res_ch[i]&7)==5) 
				{
					os->res_comp[j]=os->res_comp[j-1]-2;j++;
					os->res_comp[j]=os->res_comp[j-1];j++;
				}
				else if ((os->res_ch[i]&7)==6) 
				{
					os->res_comp[j]=os->res_comp[j-1]-2;j++;
				}
				else if ((os->res_ch[i]&7)==7) 
				{
					os->res_comp[j]=os->res_comp[j-1]+4;j++;
				}
			}
			else if (os->res_ch[i]<32)
			{
				if (os->res_ch[i]>=24)
				{
					os->res_comp[j]=os->res_comp[j-1]+4;
					j++;

					ch = ((os->res_ch[i])&7);
					ch <<=1;
					os->res_comp[j]= (ch-8)+ os->res_comp[j-1];
					j++;
				}
				else
				{
					os->res_comp[j]=os->res_comp[j-1]+2;
					j++;

					ch = ((os->res_ch[i])&7);
					ch <<=1;
					os->res_comp[j]= (ch-8)+ os->res_comp[j-1];
					j++;
				}
			}
			else if (os->res_ch[i]<64)
			{
				os->res_ch[i]-=32;
				ch = (os->res_ch[i])>>3;
				ch <<=1;
				os->res_comp[j]= (ch-6)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&7);
				ch <<=1;
				os->res_comp[j]= (ch-8)+ os->res_comp[j-1];
				j++;
			}
			else
			{
				os->res_ch[i]-=64;
				ch = ((os->res_ch[i])>>1)&31;
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&1);
				ch<<=3;
				i++;
				ch |= ((os->res_ch[i])>>5);
				ch <<=1;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;
				
				ch = ((os->res_ch[i])&31);
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;
			}
		}
	}

	goto L7;

L6: for (j=1,i=1,a=0;j<(IM_SIZE>>2);i++)
	{
		if (os->res_ch[i]>=128)
		{
			os->res_comp[j++]=os->highres_comp[a++];
			os->res_comp[j++]=((os->res_ch[i]-128)<<1);
		}
		else
		{
			if (os->res_ch[i]<32)
			{
				run=(os->res_ch[i]>>2)&7;
				nhw=os->res_comp[j-1];
				for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
				if ((os->res_ch[i]&3)==0) continue;
				else if ((os->res_ch[i]&3)==1) {os->res_comp[j]=os->res_comp[j-1]+2;j++;}
				else if ((os->res_ch[i]&3)==2) {os->res_comp[j]=os->res_comp[j-1]-2;j++;}
				else if ((os->res_ch[i]&3)==3) {os->res_comp[j]=os->res_comp[j-1];j++;}
			}
			else if (os->res_ch[i]<64)
			{
				os->res_ch[i]-=32;
				ch = (os->res_ch[i])>>3;
				ch <<=1;
				os->res_comp[j]= (ch-4)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&7);
				ch <<=1;
				os->res_comp[j]= (ch-8)+ os->res_comp[j-1];
				j++;
			}
			else
			{
				os->res_ch[i]-=64;
				ch = ((os->res_ch[i])>>1)&31;
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&1);
				ch<<=3;
				i++;
				ch |= ((os->res_ch[i])>>5);
				ch <<=1;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;
				
				ch = ((os->res_ch[i])&31);
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;
			}
		}
	}

	goto L7;

L8: for (j=1,i=1,a=0;j<(IM_SIZE>>2);i++)
	{
		if (os->res_ch[i]>=128)
		{
			os->res_comp[j++]=os->highres_comp[a++];
			os->res_comp[j++]=((os->res_ch[i]-128)<<1);
		}
		else
		{
			if (os->res_ch[i]<64)
			{
				run=os->res_ch[i]&63;
				nhw=os->res_comp[j-1];
				for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
			}
			else
			{
				os->res_ch[i]-=64;
				ch = ((os->res_ch[i])>>1)&31;
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&1);
				ch<<=3;
				i++;
				ch |= ((os->res_ch[i])>>5);
				ch <<=1;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;
				
				ch = ((os->res_ch[i])&31);
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;
			}
		}
	}

L7:	os->res_comp[(IM_SIZE>>2)]=os->res_ch[i++];

	free(os->highres_comp);

	//for (i=0;i<1200;i+=4) printf("%d %d %d %d\n",os->res_comp[i],os->res_comp[i+1],os->res_comp[i+2],os->res_comp[i+3]);

	for (j=((IM_SIZE>>2)+1);j<((IM_SIZE>>2)+(IM_SIZE>>3));i++)
	{
		if (os->res_ch[i]>=192)
		{
			os->res_ch[i]-=192;

			ch = (os->res_ch[i])>>2;
			os->res_comp[j]= uv_small_dc_offset[ch][0]+ os->res_comp[j-1];
			j++;
			os->res_comp[j]= uv_small_dc_offset[ch][1]+ os->res_comp[j-1];
			j++;

			ch=os->res_ch[i]&3;

			if (!ch)
			{
				os->res_comp[j]=os->res_comp[j-1];j++;
			}
			else if (ch==1)
			{
				os->res_comp[j]=os->res_comp[j-1]+4;j++;
			}
			else if (ch==2)
			{
				os->res_comp[j]=os->res_comp[j-1]-4;j++;
			}
			else
			{
				os->res_comp[j]=os->res_comp[j-1]+8;j++;
			}
		}
		else if (os->res_ch[i]>=128)
		{
			os->res_comp[j++]=(os->res_ch[i]-128)<<2;
		}
		else
		{
			if (os->res_ch[i]>=64)
			{
				run=(os->res_ch[i]>>3)&7;
				nhw=os->res_comp[j-1];
				if (run==7)
				{
					run=(os->res_ch[i]&7)+7;
					for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
				}
				else
				{
					for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
					if ((os->res_ch[i]&7)==0) continue;
					else if ((os->res_ch[i]&7)==1) {os->res_comp[j]=os->res_comp[j-1]+4;j++;}
					else if ((os->res_ch[i]&7)==2) 
					{
						os->res_comp[j]=os->res_comp[j-1]+4;j++;
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
					}
					else if ((os->res_ch[i]&7)==3) 
					{
						os->res_comp[j]=os->res_comp[j-1]+4;j++;
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
						os->res_comp[j]=os->res_comp[j-1];j++;
					}
					else if ((os->res_ch[i]&7)==4) 
					{
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
						os->res_comp[j]=os->res_comp[j-1]+4;j++;
						os->res_comp[j]=os->res_comp[j-1];j++;
					}
					else if ((os->res_ch[i]&7)==5) 
					{
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
						os->res_comp[j]=os->res_comp[j-1]+4;j++;
					}
					else if ((os->res_ch[i]&7)==6) 
					{
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
					}
					else if ((os->res_ch[i]&7)==7) 
					{
						os->res_comp[j]=os->res_comp[j-1]+8;j++;
					}
				}
			}
			else
			{
				ch = (os->res_ch[i])>>3;
				ch <<=2;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&7);
				ch <<=2;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;
				//if (os->res_ch[i]&1) { os->res_comp[j]=os->res_comp[j-1];j++;}
			}
		}
	}

	free(os->res_ch);

	for (i=0,e=(IM_SIZE>>2);i<(IM_DIM<<1);i++)
	{
		ch=(os->res_U_64[i]>>7);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_U_64[i]>>6)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_U_64[i]>>5)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_U_64[i]>>4)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_U_64[i]>>3)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_U_64[i]>>2)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_U_64[i]>>1)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_U_64[i])&1);os->res_comp[e++]+=(ch<<1);
	}

	free(os->res_U_64);

	for (i=0,e=((IM_SIZE>>2)+(IM_SIZE>>4));i<(IM_DIM<<1);i++)
	{
		ch=(os->res_V_64[i]>>7);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_V_64[i]>>6)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_V_64[i]>>5)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_V_64[i]>>4)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_V_64[i]>>3)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_V_64[i]>>2)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_V_64[i]>>1)&1);os->res_comp[e++]+=(ch<<1);

		ch=((os->res_V_64[i])&1);os->res_comp[e++]+=(ch<<1);
	}

	free(os->res_V_64);


	// IMAGE MEMORY ALLOCATION FOR DECODING
	imd->im_process=(short*)calloc(4*IM_SIZE,sizeof(short));

	return(imd->setup->wvlts_order);
}
