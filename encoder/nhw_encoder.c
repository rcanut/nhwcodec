/***************************************************************************
****************************************************************************
*  NHW Image Codec                                                         *
*  file: nhw_encoder.c                                                     *
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

#define PROGRAM "nhw-enc"
#define VERSION "0.2.8a"

#define NHW_QUALITY_MIN LOW19
#define NHW_QUALITY_MAX HIGH3


#define BITMAPCOREHEADER_SIZE            12
#define BITMAPINFOHEADER_SIZE            40
#define BITMAPV2INFOHEADER_SIZE          52
#define BITMAPV3INFOHEADER_SIZE          56
#define BITMAPV4HEADER_SIZE             108
#define BITMAPV5HEADER_SIZE             124

#define HEADER_CHECK_MINIMAL_AMOUNT            34  /* this is the very minimal amount of bytes needed for a proper bitmap check */
#define HEADER_CHECK_NO_ERROR                   0
#define HEADER_CHECK_ERROR_NULL_POINTER       -10
#define HEADER_CHECK_ERROR_FSEEK              -11
#define HEADER_CHECK_ERROR_NO_DATA            -12
#define HEADER_CHECK_ERROR_NO_VALID_SIG       -13
#define HEADER_CHECK_ERROR_BIH_NOT_SUPPORTED  -14
#define HEADER_CHECK_ERROR_MALF_PLANES        -15
#define HEADER_CHECK_ERROR_WRONG_IMG_FORMAT   -16

unsigned char bmp_header[HEADER_CHECK_MINIMAL_AMOUNT];

#define CLIP(x) ( (x<0) ? 0 : ((x>255) ? 255 : x) );

/* copied from https://winprotocoldoc.blob.core.windows.net/productionwindowsarchives/MS-WMF/%5bMS-WMF%5d.pdf */
typedef enum
{
    BI_BITCOUNT_0 = 0x0000,
    BI_BITCOUNT_1 = 0x0001,
    BI_BITCOUNT_2 = 0x0004,
    BI_BITCOUNT_3 = 0x0008,
    BI_BITCOUNT_4 = 0x0010,
    BI_BITCOUNT_5 = 0x0018,
    BI_BITCOUNT_6 = 0x0020
} BitCount;

/* copied from https://winprotocoldoc.blob.core.windows.net/productionwindowsarchives/MS-WMF/%5bMS-WMF%5d.pdf */
typedef enum
{
    BI_RGB =       0x0000,
    BI_RLE8 =      0x0001,
    BI_RLE4 =      0x0002,
    BI_BITFIELDS = 0x0003,
    BI_JPEG =      0x0004,
    BI_PNG =       0x0005,
    BI_CMYK =      0x000B,
    BI_CMYKRLE8 =  0x000C,
    BI_CMYKRLE4 =  0x000D
} Compression;

void encode_image(image_buffer *im,encode_state *enc, int ratio)
{
	int stage,wavelet_order,end_transform,i,j,e=0,a=0,Y,count,scan,res,res_setting,res_uv,y_wavelet,y_wavelet2;
	unsigned char *highres,*ch_comp,*scan_run,*nhw_res1I_word,*nhw_res3I_word,*nhw_res5I_word;
	unsigned char wvlt_thrx1,wvlt_thrx2,wvlt_thrx3,wvlt_thrx4,wvlt_thrx5,wvlt_thrx6,wvlt_thrx7;
	short *res256,*resIII,*nhw_process;

	im->im_process=(short*)malloc(4*IM_SIZE*sizeof(short));

	//if (im->setup->quality_setting<=LOW6) block_variance_avg(im);

	nhw_process=(short*)im->im_process;

	if (im->setup->quality_setting<HIGH2) 
	{
		pre_processing(im);
	}

	end_transform=0;
	wavelet_order=im->setup->wvlts_order;
	//for (stage=0;stage<wavelet_order;stage++) wavelet_analysis(im,(2*IM_DIM)>>stage,end_transform++,1); 

	wavelet_analysis(im,(2*IM_DIM),end_transform++,1);

	res256=(short*)malloc(IM_SIZE*sizeof(short));

	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++) 
		{
			res256[count++]=im->im_jpeg[scan++];
		}
	}

	im->setup->RES_HIGH=0;

	wavelet_analysis(im,(2*IM_DIM)>>1,end_transform,1);
	
	if (im->setup->quality_setting>LOW14) 
	{
		
	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=(2*IM_DIM),count+=IM_DIM)
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++) 
		{
			if (i>=IM_SIZE || j>=(IM_DIM>>1))
			{
				stage=nhw_process[scan];

				if (stage<-7)
				{
					if (((-stage)&7)==7) res256[count+j]+=16000;
					else if (!((-stage)&7)) res256[count+j]+=16000;
				}
				else if (stage<-4) res256[count+j]+=12000;
				else if (stage>=0)
				{
					if (stage>=2 && stage<5) 
					{
						if (scan>=(2*IM_DIM+1) && (i+j)<(2*IM_SIZE-2*IM_DIM-1))
						{
							if (abs(nhw_process[scan-(2*IM_DIM+1)])!=0 || abs(nhw_process[scan+(2*IM_DIM+1)])!=0)
							{
								res256[count+j]+=12000;
							}
							//else res256[count+j]+=8000;
						}
					}
					else if (!(stage&7)) res256[count+j]+=12000;
					else if ((stage&7)==1) res256[count+j]+=12000;
					else if (stage>4 && stage<=7) res256[count+j]+=16000;
				}
			}
		}
	}

	offsetY_recons256(im,enc,ratio,1);

	wavelet_synthesis(im,(2*IM_DIM)>>1,end_transform-1,1);

	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=(2*IM_DIM),count+=IM_DIM)
	{
		for (j=0;j<IM_DIM;j++) 
		{
			if (res256[count+j]>14000) 
			{
				res256[count+j]-=16000;
				if (i<IM_SIZE && j>=(IM_DIM>>1)) nhw_process[(i>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM)]++;
				else if (i>=IM_SIZE && j<(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+(j<<10)+1]++; 
				else if (i>=IM_SIZE && j>=(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM+1)]++; 
			}
			else if (res256[count+j]>10000) 
			{
				res256[count+j]-=12000;
				if (i<IM_SIZE && j>=(IM_DIM>>1)) nhw_process[(i>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM)]--;
				else if (i>=IM_SIZE && j<(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+(j<<10)+1]--; 
				else if (i>=IM_SIZE && j>=(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM+1)]--;
			}
			/*else if (res256[count+j]>6000) 
			{
				res256[count+j]-=8000;
				if (i<IM_SIZE && j>=(IM_DIM>>1)) nhw_process[(i>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM)]-=2;
				else if (i>=IM_SIZE && j<(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+(j<<10)+1]-=2; 
				else if (i>=IM_SIZE && j>=(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM+1)]-=2; 
			}
			else if (res256[count+j]>2000) 
			{
				res256[count+j]-=4000;
				if (i<IM_SIZE && j>=(IM_DIM>>1)) nhw_process[(i>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM)]+=2;
				else if (i>=IM_SIZE && j<(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+(j<<10)+1]+=2; 
				else if (i>=IM_SIZE && j>=(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM+1)]+=2; 
			}*/
		}
	}

	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=(2*IM_DIM))
	{
		for (e=i,j=0;j<IM_DIM;j++,count++,e++)
		{
			scan=nhw_process[e]-res256[count];

			if(scan>11) {im->im_jpeg[e]=res256[count]-7;nhw_process[e]-=7;}
			else if(scan>7) {im->im_jpeg[e]=res256[count]-4;nhw_process[e]-=4;}
			else if(scan>5) {im->im_jpeg[e]=res256[count]-2;nhw_process[e]-=2;}
			else if(scan>4) {im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;}
			else if (scan<-11) {im->im_jpeg[e]=res256[count]+7;nhw_process[e]+=7;}
			else if (scan<-7) {im->im_jpeg[e]=res256[count]+4;nhw_process[e]+=4;}
			else if (scan<-5) {im->im_jpeg[e]=res256[count]+2;nhw_process[e]+=2;}
			else if (scan<-4) {im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;}
			else if (abs(scan)>1)
			{
				a=(nhw_process[e+1]-res256[count+1]);
				if (abs(a)>4)
				{
					if (a>0)
					{
						if (a>11) a-=7;
						else if (a>7) a-=4;
						else if (a>5) a-=2;
						else a--;
					}
					else
					{
						if (a<-11) a+=7;
						else if (a<-7) a+=4;
						else if (a<-5) a+=2;
						else a++;
					}
				}

				a+=(nhw_process[e-1]-res256[count-1]);

				if (scan>=4 && a>=1) {im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;}
				else if (scan<=-4 && a<=-1) {im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;}
				else if (scan==3 && a>=0) {im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;}
				else if (scan==-3 && a<=0) {im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;}
				else if (abs(a)>=3) 
				{
					if (scan>0 && a>0) {im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;}
					else if (scan<0 && a<0) {im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;}
					else if (a>=5) {im->im_jpeg[e]=res256[count]-2;nhw_process[e]-=2;}
					else if (a<=-5) {im->im_jpeg[e]=res256[count]+2;nhw_process[e]+=2;}
					else if (a>=4)
					{
						im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;
					}
					else if (a<=-4)
					{
						im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;
					}
					else im->im_jpeg[e]=res256[count];
				}
				else im->im_jpeg[e]=res256[count];
			}
			else im->im_jpeg[e]=res256[count];
		}
	}

	wavelet_analysis(im,(2*IM_DIM)>>1,end_transform,1);
	
	}
	
	if (im->setup->quality_setting<=LOW9)
	{
		if (im->setup->quality_setting>LOW14) wvlt_thrx1=10;else wvlt_thrx1=11;
			
		for (i=IM_SIZE;i<(2*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<wvlt_thrx1) 
				{	
					if (abs(nhw_process[scan-1])<ratio && abs(nhw_process[scan+1])<ratio) 
					{
						nhw_process[scan]=0;
					}
					else if (abs(nhw_process[scan])==ratio)
					{
						if (abs(nhw_process[scan-1])<ratio || abs(nhw_process[scan+1])<ratio) 
						{
							nhw_process[scan]=0;
						}
					}
				}
			}
		}
	}

	if (im->setup->quality_setting<LOW7)
	{
		if (im->setup->quality_setting==LOW8)
		{
			wvlt_thrx1=8;
			wvlt_thrx2=13;
			wvlt_thrx3=6;
			wvlt_thrx4=11;
			wvlt_thrx5=34;
			wvlt_thrx6=14;
		}
		else if (im->setup->quality_setting<=LOW9 && im->setup->quality_setting>=LOW12)
		{
			wvlt_thrx1=8;
			wvlt_thrx2=13;
			wvlt_thrx3=6;
			wvlt_thrx4=11;
			wvlt_thrx5=34;
			wvlt_thrx6=15;
			wvlt_thrx7=15;
		}
		else if (im->setup->quality_setting==LOW13)
		{
			wvlt_thrx1=10;
			wvlt_thrx2=15;
			wvlt_thrx3=9;
			wvlt_thrx4=14;
			wvlt_thrx5=36;
			wvlt_thrx6=17;
			wvlt_thrx7=17;
		}
		else if (im->setup->quality_setting<=LOW14 && im->setup->quality_setting>=LOW16)
		{
			wvlt_thrx1=11;
			wvlt_thrx2=15;
			wvlt_thrx3=10;
			wvlt_thrx4=15;
			wvlt_thrx5=36;
			wvlt_thrx6=17;
			wvlt_thrx7=17;
		}
		else if (im->setup->quality_setting==LOW17)
		{
			wvlt_thrx1=11;
			wvlt_thrx2=15;
			wvlt_thrx3=10;
			wvlt_thrx4=15;
			wvlt_thrx5=36;
			wvlt_thrx6=18;
			wvlt_thrx7=18;
		}
		else if (im->setup->quality_setting==LOW18)
		{
			wvlt_thrx1=11;
			wvlt_thrx2=15;
			wvlt_thrx3=10;
			wvlt_thrx4=15;
			wvlt_thrx5=36;
			wvlt_thrx6=19;
			wvlt_thrx7=20;
		}
		else if (im->setup->quality_setting==LOW19)
		{
			wvlt_thrx1=11;
			wvlt_thrx2=15;
			wvlt_thrx3=10;
			wvlt_thrx4=15;
			wvlt_thrx5=36;
			wvlt_thrx6=20;
			wvlt_thrx7=21;
		}
			
		for (i=0,scan=0;i<(IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>1)-4;j++,scan++)
			{
				if (abs(nhw_process[scan+4]-nhw_process[scan])<wvlt_thrx1 && abs(nhw_process[scan+4]-nhw_process[scan+3])<wvlt_thrx1 && abs(nhw_process[scan+1]-nhw_process[scan])<wvlt_thrx1 && 
					abs(nhw_process[scan+3]-nhw_process[scan+1])<wvlt_thrx1 && abs(nhw_process[scan+3]-nhw_process[scan+2])<(wvlt_thrx2-2))
				{
						
						if ((nhw_process[scan+3]-nhw_process[scan+1])>5 && (nhw_process[scan+2]-nhw_process[scan+3]>=0))
						{
							nhw_process[scan+2]=nhw_process[scan+3];
						}
						else if ((nhw_process[scan+1]-nhw_process[scan+3])>5 && (nhw_process[scan+2]-nhw_process[scan+3]<=0))
						{
							nhw_process[scan+2]=nhw_process[scan+3];
						}
						else if ((nhw_process[scan+1]-nhw_process[scan+3])>5 && (nhw_process[scan+2]-nhw_process[scan+1]>=0))
						{
							nhw_process[scan+2]=nhw_process[scan+1];
						}
						else if ((nhw_process[scan+3]-nhw_process[scan+1])>5 && (nhw_process[scan+2]-nhw_process[scan+1]<=0))
						{
							nhw_process[scan+2]=nhw_process[scan+1];
						}
						else if ((nhw_process[scan+3]-nhw_process[scan+2])>0 && (nhw_process[scan+2]-nhw_process[scan+1]>0))
						{
						}
						else if ((nhw_process[scan+1]-nhw_process[scan+2])>0 && (nhw_process[scan+2]-nhw_process[scan+3]>0))
						{
						}
						else nhw_process[scan+2]=(nhw_process[scan+3]+nhw_process[scan+1])>>1;
							
						for (count=1;count<4;count++)
						{
							if (abs(nhw_process[((scan+count)<<1)+IM_DIM])<wvlt_thrx6) nhw_process[((scan+count)<<1)+IM_DIM]=0;
							if (abs(nhw_process[((scan+count)<<1)+IM_DIM+1])<wvlt_thrx6) nhw_process[((scan+count)<<1)+IM_DIM+1]=0;
							if (abs(nhw_process[((scan+count)<<1)+(3*IM_DIM)])<wvlt_thrx6) nhw_process[((scan+count)<<1)+(3*IM_DIM)]=0;
							if (abs(nhw_process[((scan+count)<<1)+(3*IM_DIM)+1])<wvlt_thrx6) nhw_process[((scan+count)<<1)+(3*IM_DIM)+1]=0;
							
							if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)])<(wvlt_thrx6+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)]=0;
							if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+1])<(wvlt_thrx6+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+1]=0;
							if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt_thrx6+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
							if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt_thrx6+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
							
							e=(2*IM_SIZE)+IM_DIM;
							if (abs(nhw_process[((scan+count)<<1)+e])<wvlt_thrx5) nhw_process[((scan+count)<<1)+e]=0;
							if (abs(nhw_process[((scan+count)<<1)+e+1])<wvlt_thrx5) nhw_process[((scan+count)<<1)+e+1]=0;
							if (abs(nhw_process[((scan+count)<<1)+e+(2*IM_DIM)])<wvlt_thrx5) nhw_process[((scan+count)<<1)+e+(2*IM_DIM)]=0;
							if (abs(nhw_process[((scan+count)<<1)+e+(2*IM_DIM)+1])<wvlt_thrx5) nhw_process[((scan+count)<<1)+e+(2*IM_DIM)+1]=0;
						}
							
						if (im->setup->quality_setting<=LOW9)
						{
							for (count=1;count<4;count++)
							{
								if (abs(nhw_process[scan+count+(IM_DIM>>1)])<11) nhw_process[scan+count+(IM_DIM>>1)]=0;
								if (abs(nhw_process[scan+count+IM_SIZE])<12) nhw_process[scan+count+IM_SIZE]=0;
								if (abs(nhw_process[scan+count+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[scan+count+IM_SIZE+(IM_DIM>>1)]=0;
							}
						}
							
				}
				else if (abs(nhw_process[scan+4]-nhw_process[scan])<(wvlt_thrx2+1) && abs(nhw_process[scan+4]-nhw_process[scan+3])<(wvlt_thrx2+1) && abs(nhw_process[scan+1]-nhw_process[scan])<(wvlt_thrx2+1))
				{
					if (abs(nhw_process[scan+3]-nhw_process[scan+1])<(wvlt_thrx2+6) && abs(nhw_process[scan+3]-nhw_process[scan+2])<(wvlt_thrx2+6))
					{
						if (((nhw_process[scan+3]-nhw_process[scan+2])>=0 && (nhw_process[scan+2]-nhw_process[scan+1])>=0) ||
							((nhw_process[scan+3]-nhw_process[scan+2])<=0 && (nhw_process[scan+2]-nhw_process[scan+1])<=0)) 
						{
							//nhw_process[scan+2]=(nhw_process[scan+3]+nhw_process[scan+1]+1)>>1;
							
							for (count=1;count<4;count++)
							{
								if (abs(nhw_process[((scan+count)<<1)+IM_DIM])<wvlt_thrx6) nhw_process[((scan+count)<<1)+IM_DIM]=0;
								if (abs(nhw_process[((scan+count)<<1)+IM_DIM+1])<wvlt_thrx6) nhw_process[((scan+count)<<1)+IM_DIM+1]=0;
								if (abs(nhw_process[((scan+count)<<1)+(3*IM_DIM)])<wvlt_thrx6) nhw_process[((scan+count)<<1)+(3*IM_DIM)]=0;
								if (abs(nhw_process[((scan+count)<<1)+(3*IM_DIM)+1])<wvlt_thrx6) nhw_process[((scan+count)<<1)+(3*IM_DIM)+1]=0;
							
								if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)])<(wvlt_thrx6+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)]=0;
								if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+1])<(wvlt_thrx6+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+1]=0;
								if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt_thrx6+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
								if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt_thrx6+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
							
								e=(2*IM_SIZE)+IM_DIM;
								if (abs(nhw_process[((scan+count)<<1)+e])<wvlt_thrx5) nhw_process[((scan+count)<<1)+e]=0;
								if (abs(nhw_process[((scan+count)<<1)+e+1])<wvlt_thrx5) nhw_process[((scan+count)<<1)+e+1]=0;
								if (abs(nhw_process[((scan+count)<<1)+e+(2*IM_DIM)])<wvlt_thrx5) nhw_process[((scan+count)<<1)+e+(2*IM_DIM)]=0;
								if (abs(nhw_process[((scan+count)<<1)+e+(2*IM_DIM)+1])<wvlt_thrx5) nhw_process[((scan+count)<<1)+e+(2*IM_DIM)+1]=0;
							}
							
							if (im->setup->quality_setting<=LOW9)
							{
								for (count=1;count<4;count++)
								{
									if (abs(nhw_process[scan+count+(IM_DIM>>1)])<11) nhw_process[scan+count+(IM_DIM>>1)]=0;
									if (abs(nhw_process[scan+count+IM_SIZE])<12) nhw_process[scan+count+IM_SIZE]=0;
									if (abs(nhw_process[scan+count+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[scan+count+IM_SIZE+(IM_DIM>>1)]=0;
								}
							}
						}
					}
				}
			}
		}
		
		for (i=0,scan=0;i<(IM_SIZE)-(4*IM_DIM);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
			{
				if (abs(nhw_process[scan+1]-nhw_process[scan+(4*IM_DIM)+1])<wvlt_thrx3 && abs(nhw_process[scan+(2*IM_DIM)]-nhw_process[scan+(2*IM_DIM)+2])<wvlt_thrx3)
				{
					if (abs(nhw_process[scan+(2*IM_DIM)+1]-nhw_process[scan+(2*IM_DIM)])<(wvlt_thrx4-1) && abs(nhw_process[scan+1]-nhw_process[scan+(2*IM_DIM)+1])<wvlt_thrx4)
					{
						e=(nhw_process[scan+1]+nhw_process[scan+(4*IM_DIM)+1]+nhw_process[scan+(2*IM_DIM)]+nhw_process[scan+(2*IM_DIM)+2]+2)>>2;
							
						if (abs(e-nhw_process[scan+(2*IM_DIM)])<5 || abs(e-nhw_process[scan+(2*IM_DIM)+2])<5)
						{
							nhw_process[scan+(2*IM_DIM)+1]=e;
						}
						
						count=scan+(2*IM_DIM)+1;
							
						if (abs(nhw_process[(count<<1)+IM_DIM])<wvlt_thrx6) nhw_process[(count<<1)+IM_DIM]=0;
						if (abs(nhw_process[(count<<1)+IM_DIM+1])<wvlt_thrx6) nhw_process[(count<<1)+IM_DIM+1]=0;
						if (abs(nhw_process[(count<<1)+(3*IM_DIM)])<wvlt_thrx6) nhw_process[(count<<1)+(3*IM_DIM)]=0;
						if (abs(nhw_process[(count<<1)+(3*IM_DIM)+1])<wvlt_thrx6) nhw_process[(count<<1)+(3*IM_DIM)+1]=0;
							
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)]=0;
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+1])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)+1]=0;
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
							
						e=(2*IM_SIZE)+IM_DIM;
						if (abs(nhw_process[(count<<1)+e])<32) nhw_process[(count<<1)+e]=0;
						if (abs(nhw_process[(count<<1)+e+1])<32) nhw_process[(count<<1)+e+1]=0;
						if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)])<32) nhw_process[(count<<1)+e+(2*IM_DIM)]=0;
						if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)+1])<32) nhw_process[(count<<1)+e+(2*IM_DIM)+1]=0;
						
						if (im->setup->quality_setting<=LOW9)
						{
							for (e=0;e<3;e++)
							{
								if (abs(nhw_process[count+e-1+(IM_DIM>>1)])<11) nhw_process[count+e-1+(IM_DIM>>1)]=0;
								if (abs(nhw_process[count+e-1+IM_SIZE])<12) nhw_process[count+e-1+IM_SIZE]=0;
								if (abs(nhw_process[count+e-1+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[count+e-1+IM_SIZE+(IM_DIM>>1)]=0;
							}
						}
					}
				}
			}
		}
		
		for (i=0,scan=0;i<(IM_SIZE)-(4*IM_DIM);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
			{
				if (abs(nhw_process[scan+2]-nhw_process[scan+1])<wvlt_thrx3 && abs(nhw_process[scan+1]-nhw_process[scan])<wvlt_thrx3)
				{
					if (abs(nhw_process[scan]-nhw_process[scan+(2*IM_DIM)])<wvlt_thrx3 && abs(nhw_process[scan+2]-nhw_process[scan+(2*IM_DIM)+2])<wvlt_thrx3)
					{
						if (abs(nhw_process[scan+(4*IM_DIM)+1]-nhw_process[scan+(2*IM_DIM)])<wvlt_thrx3 && abs(nhw_process[scan+(2*IM_DIM)]-nhw_process[scan+(2*IM_DIM)+1])<wvlt_thrx4) 
						{
							e=(nhw_process[scan+1]+nhw_process[scan+(4*IM_DIM)+1]+nhw_process[scan+(2*IM_DIM)]+nhw_process[scan+(2*IM_DIM)+2]+1)>>2;
							
							if (abs(e-nhw_process[scan+(2*IM_DIM)])<5 || abs(e-nhw_process[scan+(2*IM_DIM)+2])<5)
							{
								nhw_process[scan+(2*IM_DIM)+1]=e;
							}
							
							count=scan+(2*IM_DIM)+1;
							
							if (abs(nhw_process[(count<<1)+IM_DIM])<wvlt_thrx6) nhw_process[(count<<1)+IM_DIM]=0;
							if (abs(nhw_process[(count<<1)+IM_DIM+1])<wvlt_thrx6) nhw_process[(count<<1)+IM_DIM+1]=0;
							if (abs(nhw_process[(count<<1)+(3*IM_DIM)])<wvlt_thrx6) nhw_process[(count<<1)+(3*IM_DIM)]=0;
							if (abs(nhw_process[(count<<1)+(3*IM_DIM)+1])<wvlt_thrx6) nhw_process[(count<<1)+(3*IM_DIM)+1]=0;
							
							if (abs(nhw_process[(count<<1)+(2*IM_SIZE)])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)]=0;
							if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+1])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)+1]=0;
							if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
							if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
							
							e=(2*IM_SIZE)+IM_DIM;
							if (abs(nhw_process[(count<<1)+e])<32) nhw_process[(count<<1)+e]=0;
							if (abs(nhw_process[(count<<1)+e+1])<32) nhw_process[(count<<1)+e+1]=0;
							if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)])<32) nhw_process[(count<<1)+e+(2*IM_DIM)]=0;
							if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)+1])<32) nhw_process[(count<<1)+e+(2*IM_DIM)+1]=0;
						}
						
						if (im->setup->quality_setting<=LOW9)
						{
							for (e=0;e<3;e++)
							{
								if (abs(nhw_process[count+e-1+(IM_DIM>>1)])<11) nhw_process[count+e-1+(IM_DIM>>1)]=0;
								if (abs(nhw_process[count+e-1+IM_SIZE])<12) nhw_process[count+e-1+IM_SIZE]=0;
								if (abs(nhw_process[count+e-1+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[count+e-1+IM_SIZE+(IM_DIM>>1)]=0;
							}
						}
					}
				}
			}
		}
		
		if (im->setup->quality_setting<=LOW9)
		{
			for (i=0,scan=0;i<(IM_SIZE);i+=(2*IM_DIM))
			{
				for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
				{
					if (abs(nhw_process[scan+2]-nhw_process[scan+1])<wvlt_thrx7 && abs(nhw_process[scan+2]-nhw_process[scan])<wvlt_thrx7  && abs(nhw_process[scan+1]-nhw_process[scan])<wvlt_thrx7)
					{
						count=scan+1;
							
						if (abs(nhw_process[(count<<1)+IM_DIM])<wvlt_thrx6) nhw_process[(count<<1)+IM_DIM]=0;
						if (abs(nhw_process[(count<<1)+IM_DIM+1])<wvlt_thrx6) nhw_process[(count<<1)+IM_DIM+1]=0;
						if (abs(nhw_process[(count<<1)+(3*IM_DIM)])<wvlt_thrx6) nhw_process[(count<<1)+(3*IM_DIM)]=0;
						if (abs(nhw_process[(count<<1)+(3*IM_DIM)+1])<wvlt_thrx6) nhw_process[(count<<1)+(3*IM_DIM)+1]=0;
							
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)]=0;
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+1])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)+1]=0;
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt_thrx6+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
							
						e=(2*IM_SIZE)+IM_DIM;
						if (abs(nhw_process[(count<<1)+e])<34) nhw_process[(count<<1)+e]=0;
						if (abs(nhw_process[(count<<1)+e+1])<34) nhw_process[(count<<1)+e+1]=0;
						if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)])<34) nhw_process[(count<<1)+e+(2*IM_DIM)]=0;
						if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)+1])<34) nhw_process[(count<<1)+e+(2*IM_DIM)+1]=0;
						
						//for (e=0;e<3;e++)
						//{
								if (abs(nhw_process[count+(IM_DIM>>1)])<11) nhw_process[count+(IM_DIM>>1)]=0;
								if (abs(nhw_process[count+IM_SIZE])<12) nhw_process[count+IM_SIZE]=0;
								if (abs(nhw_process[count+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[count+IM_SIZE+(IM_DIM>>1)]=0;
						//}
					}
				}
			}
		}
	}
	
	resIII=(short*)malloc(IM_SIZE*sizeof(short));
	
	for (i=0,count=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++)
		{
			resIII[count++]=nhw_process[scan++];
		}
	}

	enc->tree1=(unsigned char*)malloc(((96*IM_DIM)+1)*sizeof(char));
	enc->exw_Y=(unsigned char*)malloc(32*IM_DIM*sizeof(short));
	
	if (im->setup->quality_setting>LOW3)
	{
	for (i=0,count=0,res=0,e=0,stage=0;i<((4*IM_SIZE)>>2);i+=(2*IM_DIM))
	{
		for (count=i,j=0;j<(((2*IM_DIM)>>2)-3);j++,count++)
		{
			if ((nhw_process[count]&1)==1 && (nhw_process[count+1]&1)==1 && (nhw_process[count+2]&1)==1 && (nhw_process[count+3]&1)==1 && abs(nhw_process[count]-nhw_process[count+3])>1)
			{
				nhw_process[count]+=24000;nhw_process[count+1]+=16000;//printf("\n %d ",count);
				nhw_process[count+2]+=16000;nhw_process[count+3]+=16000;

				res++;stage++;j+=3;count+=3;
			}
		}

		if (!stage) res++;
		stage=0;
	}

	enc->nhw_res4_len=res;
	enc->nhw_res4=(unsigned char*)malloc(enc->nhw_res4_len*sizeof(char));
	}

	enc->ch_res=(unsigned char*)malloc((IM_SIZE>>2)*sizeof(char));

	for (i=0,a=0,e=0,count=0,res=0,Y=0,stage=0;i<((4*IM_SIZE)>>2);i+=(2*IM_DIM))
	{
		for (count=i,j=0;j<((2*IM_DIM)>>2);j++,count++)
		{
			scan=nhw_process[count];

			if (im->setup->quality_setting>LOW3 && scan>10000) 
			{
				if (scan>20000) 
				{
					scan-=24000;enc->nhw_res4[res++]=j+1;stage++;
				}
				else scan-=16000;
			}
			/*else if (im->setup->quality_setting<LOW3 && j>0 && j<((IM_DIM>>1)-1) && abs(nhw_process[count-1]-nhw_process[count+1])<1 && abs(nhw_process[count-1]-scan)<=5)
		 	{
				scan=nhw_process[count-1];//nhw_process[count+1]=nhw_process[count-1];
			}*/
			else if ((scan&1)==1 && count>i && (nhw_process[count+1]&1)==1 /*&& !(nhw_process[count-1]&1)*/)
			{
				if (j<((IM_DIM>>1)-2) && (nhw_process[count+2]&1)==1 /*&& !(nhw_process[count+3]&1)*/) 
				{
					if (abs(scan-nhw_process[count+2])>1 && im->setup->quality_setting>LOW3) nhw_process[count+1]++;
				}
				/*else if (j<((IM_DIM>>1)-4) && (nhw_process[count+2]&1)==1 && (nhw_process[count+3]&1)==1
						&& !(nhw_process[count+4]&1)) 
				{
					nhw_process[count+2]++;
				}*/
				else if (i<(IM_SIZE-(2*IM_DIM)-2) && (nhw_process[count+(2*IM_DIM)]&1)==1
							&& (nhw_process[count+(2*IM_DIM+1)]&1)==1 && !(nhw_process[count+(2*IM_DIM+2)]&1))
				{
					if (nhw_process[count+(2*IM_DIM)]<10000 && im->setup->quality_setting>LOW3) 
					{
						nhw_process[count+(2*IM_DIM)]++;
					}
				}
			}
			else if ((scan&1)==1 && i>=(2*IM_DIM) && i<(IM_SIZE-(6*IM_DIM)))
			{
				if ((nhw_process[count+(2*IM_DIM)]&1)==1 && (nhw_process[count+(2*IM_DIM+1)]&1)==1)
				{
					if ((nhw_process[count+(4*IM_DIM)]&1)==1 && !(nhw_process[count+(6*IM_DIM)]&1)) 
					{
						if (nhw_process[count+(2*IM_DIM)]<10000 && im->setup->quality_setting>LOW3) 
						{
							nhw_process[count+(2*IM_DIM)]++;
						}
					}
				}
			}

			if (scan>255 && (j>0 || i>0))
			{
				enc->exw_Y[e++]=(i>>9);	enc->exw_Y[e++]=j+128;
				Y=scan-255;if (Y>255) Y=255;enc->exw_Y[e++]=Y;
				enc->tree1[a]=enc->tree1[a-1];enc->ch_res[a]=enc->tree1[a-1];a++;nhw_process[count]=0;

			}
			else if (scan<0 && (j>0 || i>0))  
			{
				enc->exw_Y[e++]=(i>>9);enc->exw_Y[e++]=j;
				if (scan<-255) scan=-255;
				enc->exw_Y[e++]=-scan;
				enc->tree1[a]=enc->tree1[a-1];enc->ch_res[a]=enc->tree1[a-1];a++;nhw_process[count]=0;
			}
			else 
			{
				if (scan>255) scan=255;else if (scan<0) scan=0;
				enc->ch_res[a]=scan;enc->tree1[a++]=scan&254;nhw_process[count]=0;
			}

		}

		if (im->setup->quality_setting>LOW3)
		{
			if (!stage) enc->nhw_res4[res++]=128;
			else enc->nhw_res4[res-1]+=128;
			stage=0;
		}
	}

	enc->exw_Y_end=e;

	Y_highres_compression(im,enc);

	free(enc->ch_res);

	for (i=0,count=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++)
		{ 
			im->im_process[scan++]=resIII[count++];
		}
	}

	//free(resIII);
	
	if (im->setup->quality_setting>LOW8)
	{

	offsetY_recons256(im,enc,ratio,0);

	wavelet_synthesis(im,(2*IM_DIM)>>1,end_transform-1,1);

	if (im->setup->quality_setting>HIGH1)
	{
		im->im_wavelet_first_order=(short*)malloc(IM_SIZE*sizeof(short));

		for (i=0,count=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<IM_DIM;j++) 
			{
				im->im_wavelet_first_order[count++]=im->im_jpeg[scan++];
			}
		}
	}
	
	}

	free(im->im_jpeg);

	if (im->setup->quality_setting<NORM && im->setup->quality_setting>LOW5)
	{
		for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<IM_DIM;j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<9) 
				{	
					 if (nhw_process[scan]>0) nhw_process[scan]=7;else nhw_process[scan]=-7;	
				}
			}

			for (scan=i+(IM_DIM),j=(IM_DIM);j<(2*IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<=14) 
				{	
					 if (nhw_process[scan]>0) nhw_process[scan]=7;else nhw_process[scan]=-7;	
				}
			}
		}
	}
	else if (im->setup->quality_setting<=LOW5 && im->setup->quality_setting>=LOW6)
	{ 
		if (im->setup->quality_setting==LOW5) wvlt_thrx1=11;
		else if (im->setup->quality_setting==LOW6) wvlt_thrx1=11;
		
		if (im->setup->quality_setting==LOW5) wvlt_thrx2=19;
		else if (im->setup->quality_setting==LOW6) wvlt_thrx2=20;

		for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM);j++,scan++)
			{		
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<wvlt_thrx1) 
				{	
					nhw_process[scan]=0;			
				}
			}

			for (scan=i+(IM_DIM),j=(IM_DIM);j<(2*IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<wvlt_thrx2) 
				{	
					if (nhw_process[scan]>=14) nhw_process[scan]=7;
					else if (nhw_process[scan]<=-14) nhw_process[scan]=-7;	
					else 	nhw_process[scan]=0;	
				}
			}
		}
	}
	else if (im->setup->quality_setting<LOW6)
	{ 
		if (im->setup->quality_setting==LOW7) {wvlt_thrx1=15;wvlt_thrx2=27;wvlt_thrx3=10;wvlt_thrx4=6;wvlt_thrx5=3;}
		else if (im->setup->quality_setting<=LOW8)
		{
			wvlt_thrx1=16;wvlt_thrx2=28;wvlt_thrx3=11;wvlt_thrx4=8;wvlt_thrx5=5;
			
			for (i=(2*IM_SIZE),count=0;i<(4*IM_SIZE);i++)
			{
				if (abs(nhw_process[i])>=12) count++;
			}
			
			//if (count>15000) {wvlt_thrx1=20;wvlt_thrx2=32;wvlt_thrx3=13;wvlt_thrx4=8;wvlt_thrx5=5;}
			if (count>12500) {wvlt_thrx1=19;wvlt_thrx2=31;wvlt_thrx3=13;wvlt_thrx4=9;wvlt_thrx5=6;}
			else if (count>10000) {wvlt_thrx1=18;wvlt_thrx2=30;wvlt_thrx3=12;wvlt_thrx4=8;wvlt_thrx5=6;}
			else if (count>=7000) {wvlt_thrx1=17;wvlt_thrx2=29;wvlt_thrx3=11;wvlt_thrx4=8;wvlt_thrx5=5;}
			
			if (im->setup->quality_setting==LOW9) 
			{
				if (count>12500) 
				{
					wvlt_thrx1++;wvlt_thrx2++;wvlt_thrx3++;wvlt_thrx4++;wvlt_thrx5++;
				}
				else wvlt_thrx1++;
			}
			else if (im->setup->quality_setting<=LOW10) 
			{
				if (count>12500) 
				{
					wvlt_thrx1+=3;wvlt_thrx2+=3;wvlt_thrx3+=2;wvlt_thrx4+=3;wvlt_thrx5+=3;
				}
				else 
				{
					wvlt_thrx1+=3;wvlt_thrx2+=2;wvlt_thrx3+=2;wvlt_thrx4+=2;wvlt_thrx5+=2;
				}
			}
		}
		
		for (i=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i+IM_DIM,j=IM_DIM;j<(2*IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<(wvlt_thrx3+2)) 
				{	
					if (abs(resIII[(((i>>1)+(j-IM_DIM))>>1)+(IM_DIM>>1)])<wvlt_thrx4) nhw_process[scan]=0;
					else if (abs(nhw_process[scan]+nhw_process[scan-1])<wvlt_thrx5 && abs(nhw_process[scan+1])<wvlt_thrx5) 
					{
						nhw_process[scan]=0;nhw_process[scan-1]=0;
					}
					else if (abs(nhw_process[scan]+nhw_process[scan+1])<wvlt_thrx5 && abs(nhw_process[scan-1])<wvlt_thrx5) 
					{
						nhw_process[scan]=0;nhw_process[scan+1]=0;
					}
				}
				
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<wvlt_thrx3) 
				{	
					if (abs(nhw_process[scan-1])<ratio && abs(nhw_process[scan+1])<ratio) 
					{
						nhw_process[scan]=0;
					}
				}
			}
		}

		for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<(wvlt_thrx1+2)) 
				{	
					if (abs(resIII[((((i-(2*IM_SIZE))>>1)+j)>>1)+(IM_SIZE>>1)])<wvlt_thrx4) nhw_process[scan]=0;
					else if (abs(nhw_process[scan]+nhw_process[scan-1])<wvlt_thrx5 && abs(nhw_process[scan+1])<wvlt_thrx5) 
					{
						nhw_process[scan]=0;nhw_process[scan-1]=0;
					}
					else if (abs(nhw_process[scan]+nhw_process[scan+1])<wvlt_thrx5 && abs(nhw_process[scan-1])<wvlt_thrx5) 
					{
						nhw_process[scan]=0;nhw_process[scan+1]=0;
					}
				}
				
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<wvlt_thrx1) 
				{	
					if (abs(nhw_process[scan-1])<(ratio) && abs(nhw_process[scan+1])<(ratio)) 
					{
						nhw_process[scan]=0;		
					}	
					else if (abs(nhw_process[scan])<(wvlt_thrx1-4)) 
					{
						nhw_process[scan]=0;
					}
				}
			}

			for (scan=i+(IM_DIM),j=(IM_DIM);j<((2*IM_DIM)-1);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<(wvlt_thrx2+1)) 
				{	
					if (abs(resIII[((((i-(2*IM_SIZE))>>1)+(j-IM_DIM))>>1)+((IM_SIZE>>1)+(IM_DIM>>1))])<(wvlt_thrx4+1)) nhw_process[scan]=0;
					else if (abs(nhw_process[scan]+nhw_process[scan-1])<wvlt_thrx5 && abs(nhw_process[scan+1])<wvlt_thrx5) 
					{
						nhw_process[scan]=0;nhw_process[scan-1]=0;
					}
					else if (abs(nhw_process[scan]+nhw_process[scan+1])<wvlt_thrx5 && abs(nhw_process[scan-1])<wvlt_thrx5) 
					{
						nhw_process[scan]=0;nhw_process[scan+1]=0;
					}
				}
				
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<wvlt_thrx2) 
				{	
					if (abs(nhw_process[scan-1])<ratio && abs(nhw_process[scan+1])<ratio) 
					{
						if (im->setup->quality_setting>LOW10)
						{
							if (nhw_process[scan]>=16) nhw_process[scan]=7;
							else if (nhw_process[scan]<=-16) nhw_process[scan]=-7;	
							else nhw_process[scan]=0;
						}
						else nhw_process[scan]=0;
					}
					else if (abs(nhw_process[scan])<(wvlt_thrx2-5))
					{
						if (im->setup->quality_setting>LOW10)
						{
							if (nhw_process[scan]>=16) nhw_process[scan]=7;
							else if (nhw_process[scan]<=-16) nhw_process[scan]=-7;
							else nhw_process[scan]=0;							
						}
						else nhw_process[scan]=0;
					}
				}
			}
		}
	}
	
	if (im->setup->quality_setting>LOW4)
	{ 
	for (i=(2*IM_DIM),count=0,res=0;i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i+(IM_DIM+1),j=(IM_DIM+1);j<((2*IM_DIM)-1);j++,scan++)
		{
			if (nhw_process[scan]>4 && nhw_process[scan]<8)
			{
				if (nhw_process[scan-1]>3 && nhw_process[scan-1]<=7)
				{
					if (nhw_process[scan+1]>3 && nhw_process[scan+1]<=7)
					{
						nhw_process[scan]=12700;nhw_process[scan-1]=10100;nhw_process[scan+1]=10100;
					}
				}
			}
			else if (nhw_process[scan]<-4 && nhw_process[scan]>-8)
			{
				if (nhw_process[scan-1]<-3 && nhw_process[scan-1]>=-7)
				{
					if (nhw_process[scan+1]<-3 && nhw_process[scan+1]>=-7)
					{
						nhw_process[scan]=12900;nhw_process[scan-1]=10100;nhw_process[scan+1]=10100;	 
					}
				}
			}
			else if ((nhw_process[scan]==-7) && (nhw_process[scan+1]==-6 || nhw_process[scan+1]==-7))
			{
				nhw_process[scan]=10204;nhw_process[scan+1]=10100;
			}
			else if (nhw_process[scan]==7 && nhw_process[scan+1]==7)
			{
				nhw_process[scan]=10300;nhw_process[scan+1]=10100;
			}
			else if (nhw_process[scan]==8)
			{
				if ((nhw_process[scan-1]&65534)==6 || (nhw_process[scan+1]&65534)==6) nhw_process[scan]=10;
				else if (nhw_process[scan+1]==8) {nhw_process[scan]=9;nhw_process[scan+1]=9;}
			}
			else if (nhw_process[scan]==-8)
			{
				if (((-nhw_process[scan-1])&65534)==6 || ((-nhw_process[scan+1])&65534)==6) nhw_process[scan]=-9;
				else if (nhw_process[scan+1]==-8) {nhw_process[scan]=-9;nhw_process[scan+1]=-9;}
			}
		}
	}

	for (i=((2*IM_SIZE)+(2*IM_DIM));i<((4*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i+1,j=1;j<(IM_DIM-1);j++,scan++)
		{
			if (nhw_process[scan]>4 && nhw_process[scan]<8)
			{
				if (nhw_process[scan-1]>3 && nhw_process[scan-1]<=7)
				{
					if (nhw_process[scan+1]>3 && nhw_process[scan+1]<=7)
					{
						nhw_process[scan]=12700;nhw_process[scan-1]=10100;nhw_process[scan+1]=10100;
					}
				}
			}
			else if (nhw_process[scan]<-4 && nhw_process[scan]>-8)
			{
				if (nhw_process[scan-1]<-3 && nhw_process[scan-1]>=-7)
				{
					if (nhw_process[scan+1]<-3 && nhw_process[scan+1]>=-7)
					{
						nhw_process[scan]=12900;nhw_process[scan-1]=10100;nhw_process[scan+1]=10100;
					}
				}
			}
			else if (nhw_process[scan]==-6 || nhw_process[scan]==-7)
			{
				if (nhw_process[scan+1]==-7)
				{
					nhw_process[scan]=10204;nhw_process[scan+1]=10100;
				}
				else if (nhw_process[scan-(2*IM_DIM)]==-7)
				{
					if (abs(nhw_process[scan+IM_DIM])<8) nhw_process[scan+IM_DIM]=10204;nhw_process[scan]=10100;
				}
				
			}
			else if (nhw_process[scan]==7)
			{
				if (nhw_process[scan+1]==7)
				{
					nhw_process[scan]=10300;nhw_process[scan+1]=10100;
				}
				else if (nhw_process[scan-(2*IM_DIM)]==7)
				{
					if (abs(nhw_process[scan+IM_DIM])<8) nhw_process[scan+IM_DIM]=10300;nhw_process[scan]=10100;
				}
			}
			else if (nhw_process[scan]==8)
			{
				if ((nhw_process[scan-1]&65534)==6 || (nhw_process[scan+1]&65534)==6) nhw_process[scan]=10;
			}
			else if (nhw_process[scan]==-8)
			{
				if (((-nhw_process[scan-1])&65534)==6 || ((-nhw_process[scan+1])&65534)==6) nhw_process[scan]=-9;
			}
		}
	}
	}

	if (im->setup->quality_setting>=NORM) res_setting=3;
	else if (im->setup->quality_setting>=LOW2) res_setting=4;
	else if (im->setup->quality_setting>=LOW5) res_setting=6;
	else if (im->setup->quality_setting>=LOW7) res_setting=8;
	
	if (im->setup->quality_setting>LOW8)
	{

	for (j=0,count=0,res=0,stage=0,e=0;j<IM_DIM;j++)
	{
		for (scan=j,count=j,i=0;i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM),scan+=(2*IM_DIM),count+=IM_DIM)
		{
			res=nhw_process[scan]-res256[count];a=nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM];

			if (res==2 && a==2 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])>=2)
			{
				if ((nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])<5 || (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])>6)
				{
					res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
				}
			}
			else if (((res==2 && a==3) || (res==3 && a==2)) && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])>1 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])<6)
			{
				res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
			}
			else if ((res==3 && a==3))
			{
				if ((nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])>0 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])<6)
				{
					res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
				}
				else if (im->setup->quality_setting>=LOW1)
				{
					res256[count]=12100;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
			}
			else if (a==-4 && (res==2 || res==3) && ((nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==2 || (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==3))
			{
				if (res==2 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==2) nhw_process[scan+(2*IM_DIM)]++;
				else {res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;}
			}
			else if (res==1 && a==3 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==2)
			{
				if (i>0) 
				{
					if ((nhw_process[scan-(2*IM_DIM)]-res256[count-IM_DIM])>=0) 
					{
						res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
					}
				}
			}
			else if ((res==3 || res==4 || res==5 || res>6) && ((nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])==3 || ((nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])&65534)==4))
			{
				if ((res)>6) {res256[count]=12500;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];}
				else if (im->setup->quality_setting>=LOW1)
				{
					res256[count]=12100;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
				else if (im->setup->quality_setting==LOW2)
				{
					if (res<5 && a==5) res256[count+IM_DIM]=14100;
					else if (res>=5) res256[count]=14100;
					else if (res==3 && a>=4) res256[count+IM_DIM]=14100;
					
					nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
			}
			else if ((res==2 || res==3) && (a==2 || a==3))
			{ 
				if (!(nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)]) || (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==1)
				{
					if ((nhw_process[scan+1]-res256[count+1])==2 || (nhw_process[scan+1]-res256[count+1])==3)
					{
						if ((nhw_process[scan+(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==2 || (nhw_process[scan+(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==3)
						{
							if ((nhw_process[scan+(4*IM_DIM+1)]-res256[count+(2*IM_DIM+1)])>0)
							{
								res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
							}
						}
					}
				}
			}
			else if (a==4 && (res==-2 || res==-3) && ((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])==2 || (res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])==3))
			{
				if (res==-2 && (res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])==2) nhw_process[scan+(2*IM_DIM)]--;
				else {res256[count]=12300;nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;}
			}
			else if ((res==-3 || res==-4 || res==-5 || res<-7) && ((nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])==-3 || (nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])==-4 || (nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])==-5))
			{
				if (res<-7) 
				{
					res256[count]=12600;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
				else if (im->setup->quality_setting>=LOW1)
				{
					res256[count]=12200;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
				else if (im->setup->quality_setting==LOW2)
				{
					if (res>-5 && a==-5) res256[count+IM_DIM]=14000;
					else if (res<=-5) res256[count]=14000;
					else if (res==-3 && a<=-4) res256[count+IM_DIM]=14000;
					
					nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
			}
			else if (a==-2 || a==-3)
			{
				if (res==-2 || res==-3)
				{
					if((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])>0)
					{
						res256[count]=12300;
						nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
					}
					else if (res==-3 && im->setup->quality_setting>=HIGH1)
					{
						res256[count]=14500;
					}
					else if (!(res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)]))
					{
						if ((nhw_process[scan+1]-res256[count+1])==-2 || (nhw_process[scan+1]-res256[count+1])==-3)
						{
							if ((nhw_process[scan+(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==-2 || (nhw_process[scan+(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==-3)
							{
								if ((nhw_process[scan+(4*IM_DIM+1)]-res256[count+(2*IM_DIM+1)])<0)
								{
									res256[count]=12300;
									nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
								}
							}
						}
					}
					else if (res==-2) goto L_W2;
					else goto L_W3;
				}
				else if (res==-1 && a==-3 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==-2)
				{
					if (i>0) 
					{
						if ((nhw_process[scan-(2*IM_DIM)]-res256[count-IM_DIM])<=0) 
						{
							res256[count]=12300;
							nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
						}
					}
				}
				else if (res==-1)
				{
					if ((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])==3)
					{
							res256[count]=12300;nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
					}
					else goto L_W1;

				}
				else if (res==-4)
				{
					if ((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])>1)
					{
						if((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])<4)
						{
							res256[count]=12300;
							nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
						}
						else goto L_W5;
					}
					else goto L_W5;
				}
			}
			else if (!res || res==-1)
			{
L_W1:			stage = (j<<9)+(i>>9)+(IM_DIM);

				if (nhw_process[stage]==7)
				{
					if (nhw_process[stage-1]>=0 && nhw_process[stage-1]<8) nhw_process[stage]+=2;
				}
				else if (nhw_process[stage]==8)
				{
					if (nhw_process[stage-1]>=-2 && nhw_process[stage-1]<8) nhw_process[stage]+=2;
				}
			}
			else if (res==-2)
			{
L_W2:			stage = (j<<9)+(i>>9)+(IM_DIM);

				if (nhw_process[stage]<-14)
				{
					if (!((-nhw_process[stage])&7) || ((-nhw_process[stage])&7)==7) nhw_process[stage]++;
				}
				else if (nhw_process[stage]==7 || (nhw_process[stage]&65534)==8)
				{
					if (nhw_process[stage-1]>=-2) nhw_process[stage]+=3;
				}
			}
			else if (res==-3) 
			{
L_W3:			if (im->setup->quality_setting>=HIGH1) {res256[count]=14500;}
				else if (nhw_process[(j<<9)+(i>>9)+(IM_DIM)]<-14)
				{
					if (!((-nhw_process[(j<<9)+(i>>9)+(IM_DIM)])&7) || ((-nhw_process[(j<<9)+(i>>9)+(IM_DIM)])&7)==7)
					{
						nhw_process[(j<<9)+(i>>9)+(IM_DIM)]++;
					}
				}
				else if (nhw_process[(j<<9)+(i>>9)+(IM_DIM)]>=0 && ((nhw_process[(j<<9)+(i>>9)+(IM_DIM)]+2)&65532)==8)
				{
					if (nhw_process[(j<<9)+(i>>9)+(IM_DIM-1)]>=-2) nhw_process[(j<<9)+(i>>9)+(IM_DIM)]=10;
				}
				else if (nhw_process[(j<<9)+(i>>9)+(IM_DIM)]>14 && (nhw_process[(j<<9)+(i>>9)+(IM_DIM)]&7)==7)
				{
					nhw_process[(j<<9)+(i>>9)+(IM_DIM)]++;
				}
			}
			else if (res<(-res_setting))
			{
L_W5:			res256[count]=14000;

				if (res==-4)
				{
					stage = (j<<9)+(i>>9)+(IM_DIM);

					if (nhw_process[stage]==-7 || nhw_process[stage]==-8) 
					{
						if (nhw_process[stage-1]<2 && nhw_process[stage-1]>-8) nhw_process[stage]=-9;
					}
				}
				else if (res<-6)
				{
					if (res<-7 && im->setup->quality_setting>=HIGH1) {res256[count]=14900;}
					else
					{

						stage = (j<<9)+(i>>9)+(IM_DIM);

						if (nhw_process[stage]<-14)
						{
							if (!((-nhw_process[stage])&7) || ((-nhw_process[stage])&7)==7) nhw_process[stage]++;
						}
						else if (nhw_process[stage]==7 || nhw_process[stage]==8)
						{
							if (nhw_process[stage-1]>=-1 && nhw_process[stage-1]<8) nhw_process[stage]+=3;
						}
					}
				}
			}
		}	
	}

	enc->nhw_res1_word_len=0;enc->nhw_res3_word_len=0;enc->nhw_res5_word_len=0;Y=0;

	for (i=0,count=0,e=0,stage=0,res=0,a=0;i<((4*IM_SIZE)>>1);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++,count++)
		{
			if (res256[count]<12000)
			{
				res= nhw_process[scan]-res256[count];res256[count]=0;

				if (!res || res==1)
				{
					stage = (j<<9)+(i>>9)+(IM_DIM);

					if (nhw_process[stage]==-7 || nhw_process[stage]==-8) 
					{
						if (nhw_process[stage-1]<2 && nhw_process[stage-1]>-8) nhw_process[stage]=-9;
					}
				}
				else if (res==2)
				{
					stage = (j<<9)+(i>>9)+(IM_DIM);

					if (nhw_process[stage]>15 && !(nhw_process[stage]&7)) nhw_process[stage]--;
					else if (nhw_process[stage]==-7 || nhw_process[stage]==-8) 
					{
						if (nhw_process[stage-1]<=1) nhw_process[stage]=-9;
					}
					else if (nhw_process[stage]==-6) 
					{
						if (nhw_process[stage-1]<=-1 && nhw_process[stage-1]>-8) nhw_process[stage]=-9;
					}
				}
				else if (res==3)
				{
					if (im->setup->quality_setting>=HIGH1) {res256[count]=144;enc->nhw_res5_word_len++;}
					else
					{
						stage = (j<<9)+(i>>9)+(IM_DIM);

						if (nhw_process[stage]>15 && !(nhw_process[stage]&7)) nhw_process[stage]--;
						else if (nhw_process[stage]<=0 && ((((-nhw_process[stage])+2)&65532)==8)) 
						{
							if (nhw_process[stage-1]<=2) nhw_process[stage]=-10;
						}
					}
				}
				else if (res>res_setting) 
				{
					res256[count]=141;enc->nhw_res1_word_len++;

					if (res==4)
					{
						stage = (j<<9)+(i>>9)+(IM_DIM);

						if (nhw_process[stage]==7 || (nhw_process[stage]&65534)==8)
						{
							if (nhw_process[stage-1]>=0 && nhw_process[stage-1]<8) nhw_process[stage]+=2;
						}
					}
					else if (res>6) 
					{
						if (res>7 && im->setup->quality_setting>=HIGH1) 
						{
							res256[count]=148;enc->nhw_res5_word_len++;enc->nhw_res1_word_len++;
						}
						else
						{
							stage = (j<<9)+(i>>9)+(IM_DIM);

							if (nhw_process[stage]>15 && !(nhw_process[stage]&7)) nhw_process[stage]--;
							else if (nhw_process[stage]==-6 || nhw_process[stage]==-7 || nhw_process[stage]==-8) 
							{
								if (nhw_process[stage-1]<0 && nhw_process[stage-1]>-8) nhw_process[stage]=-9;
							}
						}
					}
				}
			}
			else 
			{
				if (res256[count]==14000) {res256[count]=140;enc->nhw_res1_word_len++;} 
				else if (res256[count]==14500) {res256[count]=145;enc->nhw_res5_word_len++;}
				else if (res256[count]==12200) {res256[count]=122;enc->nhw_res3_word_len++;} 
				else if (res256[count]==12100) {res256[count]=121;enc->nhw_res3_word_len++;}
				else if (res256[count]==12300) {res256[count]=123;enc->nhw_res3_word_len++;} 
				else if (res256[count]==12400) {res256[count]=124;enc->nhw_res3_word_len++;}
				else if (res256[count]==14100) {res256[count]=141;enc->nhw_res1_word_len++;} 
				else if (res256[count]==12500) {res256[count]=125;enc->nhw_res3_word_len++;enc->nhw_res1_word_len++;}
				else if (res256[count]==12600) {res256[count]=126;enc->nhw_res3_word_len++;enc->nhw_res1_word_len++;}
				else if (res256[count]==14900) {res256[count]=149;enc->nhw_res5_word_len++;enc->nhw_res1_word_len++;}
			}
		}	
	}
	
	}

	highres=(unsigned char*)malloc(((96*IM_DIM)+1)*sizeof(char));

	if (im->setup->quality_setting>HIGH1)
	{
		for (i=0,count=0;i<IM_SIZE;i+=IM_DIM)
		{
			for (scan=i,j=0;j<IM_DIM-2;j++,scan++)
			{
				if (res256[scan]!=0)
				{
					count=(j<<8)+(i>>8);

					if (res256[scan]==141) 
					{
						im->im_wavelet_first_order[count]-=5;
					}
					else if (res256[scan]==140) 
					{
						im->im_wavelet_first_order[count]+=5;
					}
					else if (res256[scan]==144) 
					{
						im->im_wavelet_first_order[count]-=3;
					}
					else if (res256[scan]==145) 
					{
						im->im_wavelet_first_order[count]+=3;
					}
					else if (res256[scan]==121) 
					{
						im->im_wavelet_first_order[count]-=4;
						im->im_wavelet_first_order[count+1]-=3;
					}
					else if (res256[scan]==122) 
					{
						im->im_wavelet_first_order[count]+=4;
						im->im_wavelet_first_order[count+1]+=3;
					}
					else if (res256[scan]==123) 
					{
						im->im_wavelet_first_order[count]+=2;
						im->im_wavelet_first_order[count+1]+=2;
						im->im_wavelet_first_order[count+2]+=2;
					}
					else if (res256[scan]==124) 
					{
						im->im_wavelet_first_order[count]-=2;
						im->im_wavelet_first_order[count+1]-=2;
						im->im_wavelet_first_order[count+2]-=2;
					}
					else if (res256[scan]==126) 
					{
						im->im_wavelet_first_order[count]+=9;
						im->im_wavelet_first_order[count+1]+=3;
					}
					else if (res256[scan]==125) 
					{
						count=(j<<8)+(i>>8);
						im->im_wavelet_first_order[count]-=9;
						im->im_wavelet_first_order[count+1]-=3;
					}
					else if (res256[scan]==148) 
					{
						im->im_wavelet_first_order[count]-=8;
					}
					else if (res256[scan]==149) 
					{
						im->im_wavelet_first_order[count]+=8;
					}
				}
			}
		}
	}
	
	if (im->setup->quality_setting>LOW8)
	{
		
	nhw_res1I_word=(unsigned char*)malloc(enc->nhw_res1_word_len*sizeof(char));
	
	for (i=0,count=0,res=0,e=0;i<IM_SIZE;i+=IM_DIM)
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++)
		{
			if (j==(IM_DIM-2))
			{
				res256[scan]=0;
				res256[scan+1]=0;
				highres[count++]=(IM_DIM-2);j++; 
			}
			else if (res256[scan]!=0)
			{
				if (res256[scan]==141) 
				{
					highres[count++]=j;
					res256[scan]=0;nhw_res1I_word[e++]=1;
				}
				else if (res256[scan]==140) 
				{
					highres[count++]=j;
					res256[scan]=0;nhw_res1I_word[e++]=0;
				}
				else if (res256[scan]==126) 
				{
					highres[count++]=j;
					res256[scan]=122;nhw_res1I_word[e++]=0;
				}
				else if (res256[scan]==125) 
				{
					highres[count++]=j;
					res256[scan]=121;nhw_res1I_word[e++]=1;
				}
				else if (res256[scan]==148) 
				{
					highres[count++]=j;
					res256[scan]=144;nhw_res1I_word[e++]=1;
				}
				else if (res256[scan]==149) 
				{
					highres[count++]=j;
					res256[scan]=145;nhw_res1I_word[e++]=0;
				}
			}
		}
	}

	ch_comp=(unsigned char*)malloc(count*sizeof(char));
	memcpy(ch_comp,highres,count*sizeof(char));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(IM_DIM-2))
		{
			if (ch_comp[i-1]!=(IM_DIM-2) && ch_comp[i+1]!=(IM_DIM-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->nhw_res1_len=res;
	enc->nhw_res1_word_len=e;
	enc->nhw_res1=(unsigned char*)malloc((enc->nhw_res1_len)*sizeof(char));

	for (i=0;i<enc->nhw_res1_len;i++) enc->nhw_res1[i]=highres[i];

	scan_run=(unsigned char*)malloc((enc->nhw_res1_len+8)*sizeof(char));

	for (i=0;i<enc->nhw_res1_len;i++) scan_run[i]=enc->nhw_res1[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->nhw_res1_len-1;i++)
	{
		if ((scan_run[i]-scan_run[i-1])>=0 && (scan_run[i]-scan_run[i-1])<8)
		{
			if ((scan_run[i+1]-scan_run[i])>=0 && (scan_run[i+1]-scan_run[i])<16)
			{
				highres[count++]=128+((scan_run[i]-scan_run[i-1])<<4)+scan_run[i+1]-scan_run[i];
				i++;
			}
			else highres[count++]=scan_run[i];
		}
		else highres[count++]=scan_run[i];
	}

	for (i=0,stage=0;i<enc->nhw_res1_len;i++) 
	{
		if (enc->nhw_res1[i]!=254) scan_run[stage++]=enc->nhw_res1[i];
	}

	for (i=stage;i<stage+8;i++) scan_run[i]=0;

	enc->nhw_res1_bit_len=((stage>>3)+1);

	enc->nhw_res1_bit=(unsigned char*)malloc(enc->nhw_res1_bit_len*sizeof(char));

	Y=stage>>3;

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res1_bit[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}


	enc->nhw_res1_len=count;

	Y=enc->nhw_res1_word_len>>3;
	free(scan_run);
	scan_run=(unsigned char*)nhw_res1I_word;
	enc->nhw_res1_word=(unsigned char*)malloc((Y+1)*sizeof(char));

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res1_word[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}

	enc->nhw_res1_word_len=stage;

	for (i=0;i<count;i++) enc->nhw_res1[i]=highres[i];

	free(nhw_res1I_word);
	
	}

//////////////////////////////////////////////////////////////////////////////////////////////

	if (im->setup->quality_setting>=LOW1)
	{

	nhw_res3I_word=(unsigned char*)malloc(enc->nhw_res3_word_len*sizeof(char));

	for (i=0,count=0,res=0,e=0;i<IM_SIZE;i+=IM_DIM)
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++)
		{
			if (j==(IM_DIM-2))
			{
				res256[scan]=0;
				res256[scan+1]=0;
				highres[count++]=(IM_DIM-2);j++;
			}
			else if (res256[scan]==121) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res3I_word[e++]=1;
			}
			else if (res256[scan]==122) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res3I_word[e++]=0;
			}
			else if (res256[scan]==123) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res3I_word[e++]=2;
			}
			else if (res256[scan]==124) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res3I_word[e++]=3;
			}
		}
	}

	ch_comp=(unsigned char*)malloc(count*sizeof(char));
	memcpy(ch_comp,highres,count*sizeof(char));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(IM_DIM-2))
		{
			if (ch_comp[i-1]!=(IM_DIM-2) && ch_comp[i+1]!=(IM_DIM-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->nhw_res3_len=res;
	enc->nhw_res3_word_len=e;
	enc->nhw_res3=(unsigned char*)malloc((enc->nhw_res3_len)*sizeof(char));

	for (i=0;i<enc->nhw_res3_len;i++) enc->nhw_res3[i]=highres[i];

	scan_run=(unsigned char*)malloc((enc->nhw_res3_len+8)*sizeof(char));

	for (i=0;i<enc->nhw_res3_len;i++) scan_run[i]=enc->nhw_res3[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->nhw_res3_len-1;i++)
	{
		if ((scan_run[i]-scan_run[i-1])>=0 && (scan_run[i]-scan_run[i-1])<8)
		{
			if ((scan_run[i+1]-scan_run[i])>=0 && (scan_run[i+1]-scan_run[i])<16)
			{
				highres[count++]=128+((scan_run[i]-scan_run[i-1])<<4)+scan_run[i+1]-scan_run[i];
				i++;
			}
			else highres[count++]=scan_run[i];
		}
		else highres[count++]=scan_run[i];
	}

	for (i=0,stage=0;i<enc->nhw_res3_len;i++) 
	{
		if (enc->nhw_res3[i]!=254) scan_run[stage++]=enc->nhw_res3[i];
	}

	for (i=stage;i<stage+8;i++) scan_run[i]=0;

	enc->nhw_res3_bit_len=((stage>>3)+1);

	enc->nhw_res3_bit=(unsigned char*)malloc(enc->nhw_res3_bit_len*sizeof(char));

	Y=stage>>3;

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res3_bit[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}

	enc->nhw_res3_len=count;

	Y=enc->nhw_res3_word_len>>3;
	free(scan_run);
	scan_run=(unsigned char*)nhw_res3I_word;
	enc->nhw_res3_word=(unsigned char*)malloc((enc->nhw_res3_bit_len<<1)*sizeof(char));

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res3_word[stage++]=((scan_run[i]&3)<<6)|((scan_run[i+1]&3)<<4)|
								   ((scan_run[i+2]&3)<<2)|(scan_run[i+3]&3);

		enc->nhw_res3_word[stage++]=((scan_run[i+4]&3)<<6)|((scan_run[i+5]&3)<<4)|
								   ((scan_run[i+6]&3)<<2)|(scan_run[i+7]&3);
	}

	enc->nhw_res3_word_len=stage;

	for (i=0;i<count;i++) enc->nhw_res3[i]=highres[i];

	
	free(nhw_res3I_word);

	}
	
/////////////////////////////////////////////////////////////////////////////////////////////

	if (im->setup->quality_setting>=HIGH1)
	{

	nhw_res5I_word=(unsigned char*)malloc(enc->nhw_res5_word_len*sizeof(char));

	for (i=0,count=0,res=0,e=0;i<IM_SIZE;i+=IM_DIM)
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++)
		{
			if (j==(IM_DIM-2))
			{
				res256[scan]=0;
				res256[scan+1]=0;
				highres[count++]=(IM_DIM-2);j++;
			}
			else if (res256[scan]==144) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res5I_word[e++]=1;
			}
			else if (res256[scan]==145) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res5I_word[e++]=0;
			}
		}
	}

	ch_comp=(unsigned char*)malloc(count*sizeof(char));
	memcpy(ch_comp,highres,count*sizeof(char));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(IM_DIM-2))
		{
			if (ch_comp[i-1]!=(IM_DIM-2) && ch_comp[i+1]!=(IM_DIM-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->nhw_res5_len=res;
	enc->nhw_res5_word_len=e;
	enc->nhw_res5=(unsigned char*)malloc((enc->nhw_res5_len)*sizeof(char));

	for (i=0;i<enc->nhw_res5_len;i++) enc->nhw_res5[i]=highres[i];

	scan_run=(unsigned char*)malloc((enc->nhw_res5_len+8)*sizeof(char));

	for (i=0;i<enc->nhw_res5_len;i++) scan_run[i]=enc->nhw_res5[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->nhw_res5_len-1;i++)
	{
		if ((scan_run[i]-scan_run[i-1])>=0 && (scan_run[i]-scan_run[i-1])<8)
		{
			if ((scan_run[i+1]-scan_run[i])>=0 && (scan_run[i+1]-scan_run[i])<16)
			{
				highres[count++]=128+((scan_run[i]-scan_run[i-1])<<4)+scan_run[i+1]-scan_run[i];
				i++;
			}
			else highres[count++]=scan_run[i];
		}
		else highres[count++]=scan_run[i];
	}

	for (i=0,stage=0;i<enc->nhw_res5_len;i++) 
	{
		if (enc->nhw_res5[i]!=254) scan_run[stage++]=enc->nhw_res5[i];
	}

	for (i=stage;i<stage+8;i++) scan_run[i]=0;

	enc->nhw_res5_bit_len=((stage>>3)+1);

	enc->nhw_res5_bit=(unsigned char*)malloc(enc->nhw_res5_bit_len*sizeof(char));

	Y=stage>>3;

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res5_bit[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}

	enc->nhw_res5_len=count;

	Y=enc->nhw_res5_word_len>>3;
	free(scan_run);
	scan_run=(unsigned char*)nhw_res5I_word;
	enc->nhw_res5_word=(unsigned char*)malloc((enc->nhw_res5_bit_len<<1)*sizeof(char));

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res5_word[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}

	enc->nhw_res5_word_len=stage;

	for (i=0;i<count;i++) enc->nhw_res5[i]=highres[i];

	
	free(nhw_res5I_word);
	}


	free(highres);
	free(res256);

	for (i=0,count=0,stage=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++)
		{ 
			if (j<(IM_DIM>>1) && i<IM_SIZE) 
			{
				if (resIII[count]>8000) 
				{
					im->im_process[scan++]=resIII[count++];
				}
				else 
				{
					im->im_process[scan++]=0;count++;
				}
			}
			else im->im_process[scan++]=resIII[count++];
		}
	}

	free(resIII);

	if (im->setup->quality_setting>HIGH2) 
	{
		y_wavelet=8;y_wavelet2=4;
	}
	else
	{
		y_wavelet=9;y_wavelet2=9;
	}

	for (i=(2*IM_DIM),count=0,scan=0;i<((4*IM_SIZE>>1)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (j=(IM_DIM+1);j<(2*IM_DIM-1);j++)
		{
			if (abs(nhw_process[i+j])>=(ratio-2)) 
			{
				a=i+j;

				if (abs(nhw_process[a])<y_wavelet2)
				{
					if (((abs(nhw_process[a-1]))+2)>=8) scan++;
					if (((abs(nhw_process[a+1]))+2)>=8) scan++;
					if (((abs(nhw_process[a-(2*IM_DIM)]))+2)>=8) scan++;
					if (((abs(nhw_process[a+(2*IM_DIM)]))+2)>=8) scan++;

					if (scan<3 && nhw_process[a]<y_wavelet && nhw_process[a]>-y_wavelet) 
					{
						//printf("%d %d %d\n",nhw_process[a-1],nhw_process[a],nhw_process[a+1]);
						if (nhw_process[a]<-6) nhw_process[a]=-7;
						else if (nhw_process[a]>6) nhw_process[a]=7;
					}
					/*else if (!scan && abs(nhw_process[a])<9) 
					{
						if (nhw_process[a]<-6) nhw_process[a]=-7;
						else if (nhw_process[a]>6) nhw_process[a]=7;
					}*/

					scan=0;
				}
			}
			else nhw_process[i+j]=0;

			//if (abs(nhw_process[i+j])<9) nhw_process[i+j]=0;
			if (abs(nhw_process[i+j])>6)
			{
			e=nhw_process[i+j];

			if (e>=8 && (e&7)<2) 
			{
				if (nhw_process[i+j+1]>7 && nhw_process[i+j+1]<10000) nhw_process[i+j+1]--;
				//if (nhw_process[i+j+(2*IM_DIM)]>8) nhw_process[i+j+(2*IM_DIM)]--;
			}
			else if (e==-7 && nhw_process[i+j+1]==8) nhw_process[i+j]=-8;
			else if (e==8 && nhw_process[i+j+1]==-7) nhw_process[i+j+1]=-8;
			else if (e<-7 && ((-e)&7)<2)
			{
				if (nhw_process[i+j+1]<-14 && nhw_process[i+j+1]<10000)
				{
					if (((-nhw_process[i+j+1])&7)==7) nhw_process[i+j+1]++;
					else if (((-nhw_process[i+j+1])&7)<2 && j<((2*IM_DIM)-2) && nhw_process[i+j+2]<=0) nhw_process[i+j+1]++;
				}
			}
			}
		}
	}

	if (im->setup->quality_setting>HIGH2) 
	{
		y_wavelet=8;y_wavelet2=4;
	}
	else if (im->setup->quality_setting>LOW3)
	{
		y_wavelet=8;y_wavelet2=9;
	}
	else 
	{
		y_wavelet=9;y_wavelet2=9;
	}

	for (i=((4*IM_SIZE)>>1),scan=0;i<(4*IM_SIZE-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (j=1;j<(IM_DIM);j++)
		{
			if (abs(nhw_process[i+j])>=(ratio-2)) 
			{	
				a=i+j;

				if (abs(nhw_process[a])<y_wavelet2)
				{
					if (((abs(nhw_process[a-1]))+2)>=8) scan++;
					if (((abs(nhw_process[a+1]))+2)>=8) scan++;
					if (((abs(nhw_process[a-(2*IM_DIM)]))+2)>=8) scan++;
					if (((abs(nhw_process[a+(2*IM_DIM)]))+2)>=8) scan++;

					if (scan<3 && nhw_process[a]<y_wavelet && nhw_process[a]>(-y_wavelet))
					{
						if (nhw_process[a]<0) nhw_process[a]=-7;else nhw_process[a]=7;
					}
					else if (!scan && abs(nhw_process[a])<y_wavelet2)
					{
						if (nhw_process[a]<0) nhw_process[a]=-7;else nhw_process[a]=7;
					}

					scan=0;
				}
			}
			else nhw_process[i+j]=0;

			if (abs(nhw_process[i+j])>6)
			{

			e=nhw_process[i+j];

			if (e>=8 && (e&7)<2) 
			{
				if (nhw_process[i+j+1]>7 && nhw_process[i+j+1]<10000) nhw_process[i+j+1]--;
				//else if (nhw_process[i+j+(2*IM_DIM)]>8 && nhw_process[i+j+(2*IM_DIM)]<10000) nhw_process[i+j+(2*IM_DIM)]--;
			}
			else if (e==-7 && nhw_process[i+j+1]==8) nhw_process[i+j]=-8;
			else if (e==8 && nhw_process[i+j+1]==-7) nhw_process[i+j+1]=-8;
			else if (e<-7 && ((-e)&7)<2)
			{
				if (nhw_process[i+j+1]<-14 && nhw_process[i+j+1]<10000)
				{
					if (((-nhw_process[i+j+1])&7)==7) nhw_process[i+j+1]++;
					else if (((-nhw_process[i+j+1])&7)<2 && j<(IM_DIM-2) && nhw_process[i+j+2]<=0) nhw_process[i+j+1]++;
				}
			}
			}
		}
	}

	if (im->setup->quality_setting>HIGH2) y_wavelet=8;
	else y_wavelet=11;

	for (i=((4*IM_SIZE)>>1),scan=0,count=0;i<(4*IM_SIZE-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (j=(IM_DIM+1);j<(2*IM_DIM-1);j++)
		{
			if (abs(nhw_process[i+j])>=(ratio-1)) 
			{	
				a=i+j;

				if (abs(nhw_process[a])<y_wavelet)
				{
					if (((abs(nhw_process[a-1]))+2)>=8) scan++;
					if (((abs(nhw_process[a+1]))+2)>=8) scan++;
					if (((abs(nhw_process[a-(2*IM_DIM)]))+2)>=8) scan++;
					if (((abs(nhw_process[a+(2*IM_DIM)]))+2)>=8) scan++;

					if (scan<3 && nhw_process[a]<y_wavelet && nhw_process[a]>-y_wavelet)
					{
						if (nhw_process[a]<0) nhw_process[a]=-7;else nhw_process[a]=7;
					}
					/*else if (!scan && abs(nhw_process[a])<11) 
					{
						if (nhw_process[a]<0) nhw_process[a]=-7;else nhw_process[a]=7;
					}*/

					scan=0;
				}
			}
			else nhw_process[i+j]=0;

			if (abs(nhw_process[i+j])>6)
			{
			e=nhw_process[i+j];

			if (e>=8 && (e&7)<2) 
			{
				if (nhw_process[i+j+1]>7 && nhw_process[i+j+1]<10000) nhw_process[i+j+1]--;
				//else if (nhw_process[i+j+(2*IM_DIM)]>7 && nhw_process[i+j+(2*IM_DIM)]<10000) nhw_process[i+j+(2*IM_DIM)]--;
			}
			else if (e==-7 && nhw_process[i+j+1]==8) nhw_process[i+j]=-8;
			else if (e==8 && nhw_process[i+j+1]==-7) nhw_process[i+j+1]=-8;
			else if (e<-7 && ((-e)&7)<2)
			{
				if (nhw_process[i+j+1]<-14 && nhw_process[i+j+1]<10000)
				{
					if (((-nhw_process[i+j+1])&7)==7) nhw_process[i+j+1]++;
					else if (((-nhw_process[i+j+1])&7)<2 && j<((2*IM_DIM)-2) && nhw_process[i+j+2]<=0) nhw_process[i+j+1]++;
				}
			}
			}
		}
	}

	offsetY(im,ratio);

	if (im->setup->quality_setting>HIGH1)
	{
		im_recons_wavelet_band(im);
		wavelet_synthesis_high_quality_settings(im,enc);
	}

	im->im_nhw=(unsigned char*)calloc(6*IM_SIZE,sizeof(char));
	scan_run=(unsigned char*)im->im_nhw; 

	for (j=0,count=0,stage=0,e=0;j<(IM_DIM<<1);)
	{
		for (i=0;i<IM_DIM;i++)
		{
			scan_run[count]=nhw_process[j];
			scan_run[count+1]=nhw_process[j+1];
			scan_run[count+2]=nhw_process[j+2];
			scan_run[count+3]=nhw_process[j+3];
	
			j+=(2*IM_DIM);

			scan_run[count+4]=nhw_process[j+3];
			scan_run[count+5]=nhw_process[j+2];
			scan_run[count+6]=nhw_process[j+1];
			scan_run[count+7]=nhw_process[j];

			j+=(2*IM_DIM);
			count+=8;
		}

		j-=((4*IM_SIZE)-4);
	}

	free(im->im_process);

	for (i=0;i<4*IM_SIZE-4;i++)
	{
		if (scan_run[i]!=128 && scan_run[i+1]==128)
		{
			if (scan_run[i+2]==128)
			{
				if  (scan_run[i+3]==128)
				{
					if (scan_run[i]==136 && scan_run[i+4]==136) {scan_run[i]=132;scan_run[i+4]=201;i+=4;}
					else if (scan_run[i]==136 && scan_run[i+4]==120) {scan_run[i]=133;scan_run[i+4]=201;i+=4;}
					else if (scan_run[i]==120 && scan_run[i+4]==136) {scan_run[i]=134;scan_run[i+4]=201;i+=4;}
					else if (scan_run[i]==120 && scan_run[i+4]==120) {scan_run[i]=135;scan_run[i+4]=201;i+=4;}
					//else if (scan_run[i]==136 && scan_run[i+4]==112) {scan_run[i]=127;i+=4;}
					//else if (scan_run[i]==112 && scan_run[i+4]==136) {scan_run[i]=126;i+=4;}
					//else if (scan_run[i]==136 && scan_run[i+4]==144) {scan_run[i]=125;i+=4;}
					//else if (scan_run[i]==144 && scan_run[i+4]==136) {scan_run[i]=123;i+=4;}
					//else if (scan_run[i]==120 && scan_run[i+4]==112) {scan_run[i]=121;i+=4;}
					//else if (scan_run[i]==112 && scan_run[i+4]==120) {scan_run[i]=122;i+=4;}
					else i+=3;
				}
				else i+=2;
			}
			else i++;
		}
	}

	scan_run[0]=128;scan_run[1]=128;scan_run[2]=128;scan_run[3]=128;
	scan_run[(4*IM_SIZE)-4]=128;scan_run[(4*IM_SIZE)-3]=128;scan_run[(4*IM_SIZE)-2]=128;scan_run[(4*IM_SIZE)-1]=128;

	for (i=4,enc->nhw_select1=0,enc->nhw_select2=0,count=0,res=0;i<((4*IM_SIZE)-4);i++)
	{
		if (scan_run[i]==136)
		{
			if (scan_run[i+2]==128 && (scan_run[i+1]==120 || scan_run[i+1]==136)  && scan_run[i-1]==128 && scan_run[i-2]==128 && scan_run[i-3]==128 && scan_run[i-4]==128)
			{
				if (scan_run[i+1]==120) scan_run[i+1]=157;
				else scan_run[i+1]=159;

				enc->nhw_select2++;
			}
			else if (scan_run[i-1]==128 && (scan_run[i+1]==120 || scan_run[i+1]==136) && scan_run[i+2]==128 && scan_run[i+3]==128 && scan_run[i+4]==128 && scan_run[i+5]==128)
			{
				if (scan_run[i+1]==120) scan_run[i+1]=157;
				else scan_run[i+1]=159;

				enc->nhw_select2++;
			}
			else if (scan_run[i-1]==128 && scan_run[i-2]==128 && scan_run[i-3]==128 && scan_run[i-4]==128 && scan_run[i+1]==128)
			{
				scan_run[i]=153;enc->nhw_select1++;
			}
			else if (scan_run[i-1]==128 && scan_run[i+1]==128 && scan_run[i+2]==128 && scan_run[i+3]==128 && scan_run[i+4]==128)
			{
				scan_run[i]=153;enc->nhw_select1++;
			}
		}
		else if (scan_run[i]==120)
		{
			if (scan_run[i+2]==128 && (scan_run[i+1]==120 || scan_run[i+1]==136)  && scan_run[i-1]==128 && scan_run[i-2]==128 && scan_run[i-3]==128 && scan_run[i-4]==128)
			{
				if (scan_run[i+1]==120) scan_run[i+1]=157;
				else scan_run[i+1]=159;

				enc->nhw_select2++;
			}
			else if (scan_run[i-1]==128 && (scan_run[i+1]==120 || scan_run[i+1]==136) && scan_run[i+2]==128 && scan_run[i+3]==128 && scan_run[i+4]==128 && scan_run[i+5]==128)
			{
				if (scan_run[i+1]==120) scan_run[i+1]=157;
				else scan_run[i+1]=159;

				enc->nhw_select2++;
			}
			else if (scan_run[i-1]==128 && scan_run[i-2]==128 && scan_run[i-3]==128 && scan_run[i-4]==128 && scan_run[i+1]==128)
			{
				scan_run[i]=155;enc->nhw_select1++;
			}
			else if (scan_run[i-1]==128 && scan_run[i+1]==128 && scan_run[i+2]==128 && scan_run[i+3]==128 && scan_run[i+4]==128)
			{
				scan_run[i]=155;enc->nhw_select1++;
			}
		}
	}


	for (i=0,count=0;i<(4*IM_SIZE);i++)
	{
		while (scan_run[i]==128 && scan_run[i+1]==128)   
		{
			count++;

			if (count>255)
			{
				if (scan_run[i]==153) scan_run[i]=124;
				else if (scan_run[i]==155) scan_run[i]=123;

				if (scan_run[i+1]==153) scan_run[i+1]=124;
				else if (scan_run[i+1]==155) scan_run[i+1]=123;

				if (scan_run[i+2]==153) scan_run[i+2]=124;
				else if (scan_run[i+2]==155) scan_run[i+2]=123;

				if (scan_run[i+3]==153) scan_run[i+3]=124;
				else if (scan_run[i+3]==155) scan_run[i+3]=123;

				i--;count=0;
			}
			else i++;
		}
		 
		if (count>=252)
		{
			if (scan_run[i+1]==153) scan_run[i+1]=124;
			else if (scan_run[i+1]==155) scan_run[i+1]=123;
		}

		count=0;
	}

	
	// U
	im->im_jpeg=(short*)malloc(IM_SIZE*sizeof(short));
	for (i=0;i<IM_SIZE;i++) im->im_jpeg[i]=(unsigned char)im->im_bufferU[i];
	free(im->im_bufferU);

	im->im_process=(short*)malloc(IM_SIZE*sizeof(short));
	nhw_process=(short*)im->im_process;
	
	if (im->setup->quality_setting<=LOW6) pre_processing_UV(im);

	end_transform=0;im->setup->RES_HIGH=0;
	//for (stage=0;stage<wavelet_order;stage++) wavelet_analysis(im,IM_DIM>>stage,end_transform++,0); 

	wavelet_analysis(im,IM_DIM,end_transform++,0);

	res256=(short*)malloc((IM_SIZE>>2)*sizeof(short));

	for (i=0,count=0;i<(IM_SIZE>>1);i+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++) res256[count++]=im->im_jpeg[scan++];
	}
	
	if (im->setup->quality_setting<=LOW4)
	{
		for (i=0;i<(IM_SIZE>>1);i+=(IM_DIM))
		{
			for (scan=i+(IM_DIM>>1),j=(IM_DIM>>1);j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<24) 
				{	
					 nhw_process[scan]=0;	
				}
			}
		}	
			
		for (i=(IM_SIZE>>1);i<(IM_SIZE);i+=(IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<32) 
				{	
					 nhw_process[scan]=0;	
				}
			}

			for (scan=i+(IM_DIM>>1),j=(IM_DIM>>1);j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<48) 
				{	
					 nhw_process[scan]=0;		
				}
			}
		}
	}

	wavelet_analysis(im,IM_DIM>>1,end_transform,0); 

	offsetUV_recons256(im,ratio,1);

	wavelet_synthesis(im,IM_DIM>>1,end_transform-1,0); 

	for (i=0,count=0,Y=0,a=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (e=i,j=0;j<(IM_DIM>>1);j++,count++,e++)
		{
			scan=nhw_process[e]-res256[count];
			//Y+=abs(scan);
	
			if(scan>10) {im->im_jpeg[e]=res256[count]-6;}
			else if(scan>7) {im->im_jpeg[e]=res256[count]-3;}
			else if(scan>4) {im->im_jpeg[e]=res256[count]-2;}
			else if(scan>3) im->im_jpeg[e]=res256[count]-1;
			else if(scan>2 && (nhw_process[e+1]-res256[count+1])>=0) im->im_jpeg[e]=res256[count]-1;
			else if (scan<-10) {im->im_jpeg[e]=res256[count]+6;}
			else if (scan<-7) {im->im_jpeg[e]=res256[count]+3;}
			else if (scan<-4) {im->im_jpeg[e]=res256[count]+2;}
			else if (scan<-3) im->im_jpeg[e]=res256[count]+1;
			else if(scan<-2 && (nhw_process[e+1]-res256[count+1])<=0) im->im_jpeg[e]=res256[count]+1;
			else im->im_jpeg[e]=res256[count];
		}
	}

	//free(res256);

	wavelet_analysis(im,IM_DIM>>1,end_transform,0);

/////////////////////////////////////////////////////////////////////////////////////////////////

	/*offsetUV_recons256(im,ratio,0);

	wavelet_synthesis(im,IM_DIM>>1,end_transform-1,0);

	for (i=0,count=0,Y=0,e=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++) Y+=abs(nhw_process[scan++]-res256[count++]);
	}*/

//////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////

	resIII=(short*)malloc((IM_SIZE>>2)*sizeof(short));

	for (i=0,count=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++)
		{
			resIII[count++]=nhw_process[scan++];
		}
	}

	offsetUV_recons256(im,ratio,0);

	wavelet_synthesis(im,IM_DIM>>1,end_transform-1,0);

	if (im->setup->quality_setting>LOW3) res_uv=4;else res_uv=5;

	if (im->setup->quality_setting>=LOW2)
	{ 
	for (i=0,count=0,Y=0,e=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++,count++)
		{
			if ((nhw_process[scan]-res256[count])>3 && (nhw_process[scan]-res256[count])<7)
			{
				if ((nhw_process[scan+1]-res256[count+1])>2 && (nhw_process[scan+1]-res256[count+1])<7)
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_DIM>>1)]=12400;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)]=12400;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=12400;count++;scan++;j++;continue;}
				}
			}
			else if ((nhw_process[scan]-res256[count])<-3 && (nhw_process[scan]-res256[count])>-7)
			{
				if ((nhw_process[scan+1]-res256[count+1])<-2 && (nhw_process[scan+1]-res256[count+1])>-8)
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_DIM>>1)]=12600;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)]=12600;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=12600;count++;scan++;j++;continue;}
				}
			}
			
			if (abs(nhw_process[scan]-res256[count])>res_uv) 
			{
				if ((nhw_process[scan]-res256[count])>0)
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) nhw_process[scan+(IM_DIM>>1)]=12900;
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) nhw_process[scan+(IM_SIZE>>1)]=12900; 
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=12900; 
				}
				else if ((nhw_process[scan]-res256[count])==-5)
				{
					if ((nhw_process[scan+1]-res256[count+1])<0)
					{
						if (abs(nhw_process[scan+(IM_DIM>>1)])<8) nhw_process[scan+(IM_DIM>>1)]=13000;
						else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) nhw_process[scan+(IM_SIZE>>1)]=13000; 
						else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=13000;
					}
					
				}
				else
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) nhw_process[scan+(IM_DIM>>1)]=13000;
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) nhw_process[scan+(IM_SIZE>>1)]=13000; 
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=13000; 
				}
			}
		}
	}
	}

	free(res256);

	for (i=0,count=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++)
		{
			nhw_process[scan++]=resIII[count++];
		}
	}

	free(resIII);
	
	if (im->setup->quality_setting<=LOW9)
	{
		//if (im->setup->quality_setting==LOW9 || im->setup->quality_setting==LOW10)
		//{
			wvlt_thrx1=2;
			wvlt_thrx2=3;
			wvlt_thrx3=5;
			wvlt_thrx4=8;
		//}
		
		for (i=0,scan=0;i<(IM_SIZE>>2)-(2*IM_DIM);i+=(IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>2)-2;j++,scan++)
			{
				if (abs(nhw_process[scan+1]-nhw_process[scan+(2*IM_DIM)+1])<wvlt_thrx3 && abs(nhw_process[scan+(IM_DIM)]-nhw_process[scan+(IM_DIM)+2])<wvlt_thrx3)
				{
					if (abs(nhw_process[scan+(IM_DIM)+1]-nhw_process[scan+(IM_DIM)])<(wvlt_thrx4-1) && abs(nhw_process[scan+1]-nhw_process[scan+(IM_DIM)+1])<wvlt_thrx4)
					{
						nhw_process[scan+(IM_DIM)+1]=(nhw_process[scan+1]+nhw_process[scan+(2*IM_DIM)+1]+nhw_process[scan+(IM_DIM)]+nhw_process[scan+(IM_DIM)+2]+2)>>2;
					}
				}
			}
		}
		
		for (i=0,scan=0;i<(IM_SIZE>>2)-(2*IM_DIM);i+=(IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>2)-2;j++,scan++)
			{
				if (abs(nhw_process[scan+2]-nhw_process[scan+1])<wvlt_thrx3 && abs(nhw_process[scan+1]-nhw_process[scan])<wvlt_thrx3)
				{
					if (abs(nhw_process[scan]-nhw_process[scan+(IM_DIM)])<wvlt_thrx3 && abs(nhw_process[scan+2]-nhw_process[scan+(IM_DIM)+2])<wvlt_thrx3)
					{
						if (abs(nhw_process[scan+(2*IM_DIM)+1]-nhw_process[scan+(IM_DIM)])<wvlt_thrx3 && abs(nhw_process[scan+(IM_DIM)]-nhw_process[scan+(IM_DIM)+1])<wvlt_thrx4) 
						{
							nhw_process[scan+(IM_DIM)+1]=(nhw_process[scan+1]+nhw_process[scan+(2*IM_DIM)+1]+nhw_process[scan+(IM_DIM)]+nhw_process[scan+(IM_DIM)+2]+1)>>2;
						}
					}
				}
			}
		}
	}

//////////////////////////////////////////////////////////////////////////////////////////////////

	enc->exw_Y[enc->exw_Y_end++]=0;enc->exw_Y[enc->exw_Y_end++]=0;

	for (i=0,a=(IM_SIZE>>2),res=0;i<((IM_SIZE)>>2);i+=(IM_DIM))
	{
		for (j=0;j<((IM_DIM)>>2);j++)
		{
			scan=nhw_process[j+i];

			if (scan>255 && (j>0 || i>0)) 
			{
				enc->exw_Y[enc->exw_Y_end++]=(i>>8);enc->exw_Y[enc->exw_Y_end++]=j+128;
				Y=scan-255;if (Y>255) Y=255;enc->exw_Y[enc->exw_Y_end++]=Y;
				enc->tree1[a]=enc->tree1[a-1];a++;nhw_process[j+i]=0;
			}
			else if (scan<0 && (j>0 || i>0)) 
			{
				enc->exw_Y[enc->exw_Y_end++]=(i>>8);enc->exw_Y[enc->exw_Y_end++]=j;
				if (scan<-255) scan=-255;
				enc->exw_Y[enc->exw_Y_end++]=-scan;enc->tree1[a]=enc->tree1[a-1];a++;nhw_process[j+i]=0;
			}
			else 
			{
				if (scan>255) 
				{
					scan=255;
				}
				else if (scan<0) 
				{
					scan=0;
				}
				enc->tree1[a++]=scan&254;nhw_process[j+i]=0;
			}
		}
	}

	if (im->setup->quality_setting>LOW5) 
	{
		enc->res_U_64=(unsigned char*)malloc((IM_DIM<<1)*sizeof(char));
		scan_run=(unsigned char*)enc->res_U_64;
		ch_comp=(unsigned char*)malloc((16*IM_DIM)*sizeof(char));

		for (i=0,e=0;i<(16*IM_DIM);i+=8)
		{
			ch_comp[i]=((enc->tree1[i+16384])>>1)&1;
			ch_comp[i+1]=((enc->tree1[i+16385])>>1)&1;
			ch_comp[i+2]=((enc->tree1[i+16386])>>1)&1;
			ch_comp[i+3]=((enc->tree1[i+16387])>>1)&1;
			ch_comp[i+4]=((enc->tree1[i+16388])>>1)&1;
			ch_comp[i+5]=((enc->tree1[i+16389])>>1)&1;
			ch_comp[i+6]=((enc->tree1[i+16390])>>1)&1;
			ch_comp[i+7]=((enc->tree1[i+16391])>>1)&1;

			scan_run[e++]=(ch_comp[i]<<7)|(ch_comp[i+1]<<6)|(ch_comp[i+2]<<5)|(ch_comp[i+3]<<4)|(ch_comp[i+4]<<3)|
					  (ch_comp[i+5]<<2)|(ch_comp[i+6]<<1)|ch_comp[i+7];
		}
	}

	offsetUV(im,ratio);
	//quantizationUV(im);

	for (j=0,count=(4*IM_SIZE);j<(IM_DIM);)
	{
		for (i=0;i<(IM_DIM>>1);i++)
		{
			im->im_nhw[count]=nhw_process[j];
			im->im_nhw[count+2]=nhw_process[j+1];
			im->im_nhw[count+4]=nhw_process[j+2];
			im->im_nhw[count+6]=nhw_process[j+3];
			im->im_nhw[count+8]=nhw_process[j+4];
			im->im_nhw[count+10]=nhw_process[j+5];
			im->im_nhw[count+12]=nhw_process[j+6];
			im->im_nhw[count+14]=nhw_process[j+7];
	
			j+=(IM_DIM);
			im->im_nhw[count+16]=nhw_process[j+7];
			im->im_nhw[count+18]=nhw_process[j+6];
			im->im_nhw[count+20]=nhw_process[j+5];
			im->im_nhw[count+22]=nhw_process[j+4];
			im->im_nhw[count+24]=nhw_process[j+3];
			im->im_nhw[count+26]=nhw_process[j+2];
			im->im_nhw[count+28]=nhw_process[j+1];
			im->im_nhw[count+30]=nhw_process[j];

			j+=(IM_DIM);
			count+=32;
		}

		j-=(IM_SIZE-8);
	}

	// V
	for (i=0;i<IM_SIZE;i++) im->im_jpeg[i]=(unsigned char)im->im_bufferV[i];
	free(im->im_bufferV);

	end_transform=0;im->setup->RES_HIGH=0;
	//for (stage=0;stage<wavelet_order;stage++) wavelet_analysis(im,IM_DIM>>stage,end_transform++,0);
	
	if (im->setup->quality_setting<=LOW6) pre_processing_UV(im);

	wavelet_analysis(im,IM_DIM,end_transform++,0);

	res256=(short*)malloc((IM_SIZE>>2)*sizeof(short));

	for (i=0,count=0;i<(IM_SIZE>>1);i+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++) res256[count++]=im->im_jpeg[scan++];
	}
	
	if (im->setup->quality_setting<=LOW4)
	{
		for (i=0;i<(IM_SIZE>>1);i+=(IM_DIM))
		{
			for (scan=i+(IM_DIM>>1),j=(IM_DIM>>1);j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<24) 
				{	
					 nhw_process[scan]=0;	
				}
			}
		}	
			
		for (i=(IM_SIZE>>1);i<(IM_SIZE);i+=(IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<32) 
				{	
					 nhw_process[scan]=0;	
				}
			}

			for (scan=i+(IM_DIM>>1),j=(IM_DIM>>1);j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<48) 
				{	
					 nhw_process[scan]=0;		
				}
			}
		}
	}

	wavelet_analysis(im,IM_DIM>>1,end_transform,0); 

	offsetUV_recons256(im,ratio,1);

	wavelet_synthesis(im,IM_DIM>>1,end_transform-1,0); 

	for (i=0,count=0,a=0,Y=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (e=i,j=0;j<(IM_DIM>>1);j++,count++,e++)
		{
			scan=nhw_process[e]-res256[count];
	
			if(scan>10) {im->im_jpeg[e]=res256[count]-6;}
			else if(scan>7) {im->im_jpeg[e]=res256[count]-3;}
			else if(scan>4) {im->im_jpeg[e]=res256[count]-2;}
			else if(scan>3) im->im_jpeg[e]=res256[count]-1;
			else if(scan>2 && (nhw_process[e+1]-res256[count+1])>0) im->im_jpeg[e]=res256[count]-1;
			else if (scan<-10) {im->im_jpeg[e]=res256[count]+6;}
			else if (scan<-7) {im->im_jpeg[e]=res256[count]+3;}
			else if (scan<-4) {im->im_jpeg[e]=res256[count]+2;}
			else if (scan<-3) im->im_jpeg[e]=res256[count]+1;
			else if(scan<-2 && (nhw_process[e+1]-res256[count+1])<0) im->im_jpeg[e]=res256[count]+1;
			else im->im_jpeg[e]=res256[count];
		}
	}

	//free(res256);

	wavelet_analysis(im,IM_DIM>>1,end_transform,0);

	/////////////////////////////////////////////////////////////////////////////////////////////////

	resIII=(short*)malloc((IM_SIZE>>2)*sizeof(short));

	for (i=0,count=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++)
		{
			resIII[count++]=nhw_process[scan++];
		}
	}

	offsetUV_recons256(im,ratio,0);

	wavelet_synthesis(im,IM_DIM>>1,end_transform-1,0);

	if (im->setup->quality_setting>=LOW2)
	{ 
	for (i=0,count=0,Y=0,e=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++,count++)
		{
			if ((nhw_process[scan]-res256[count])>3 && (nhw_process[scan]-res256[count])<7)
			{
				if ((nhw_process[scan+1]-res256[count+1])>2 && (nhw_process[scan+1]-res256[count+1])<7)
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_DIM>>1)]=12400;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)]=12400;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=12400;count++;scan++;j++;continue;}
				}
			}
			else if ((nhw_process[scan]-res256[count])<-3 && (nhw_process[scan]-res256[count])>-7)
			{
				if ((nhw_process[scan+1]-res256[count+1])<-2 && (nhw_process[scan+1]-res256[count+1])>-8)
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_DIM>>1)]=12600;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)]=12600;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=12600;count++;scan++;j++;continue;}
				}
			}
			
			if (abs(nhw_process[scan]-res256[count])>res_uv) 
			{
				if ((nhw_process[scan]-res256[count])>0)
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) nhw_process[scan+(IM_DIM>>1)]=12900;
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) nhw_process[scan+(IM_SIZE>>1)]=12900; 
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=12900; 
				}
				else if ((nhw_process[scan]-res256[count])==-5)
				{
					if ((nhw_process[scan+1]-res256[count+1])<0)
					{
						if (abs(nhw_process[scan+(IM_DIM>>1)])<8) nhw_process[scan+(IM_DIM>>1)]=13000;
						else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) nhw_process[scan+(IM_SIZE>>1)]=13000; 
						else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=13000; 
					}
					
				}
				else
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) nhw_process[scan+(IM_DIM>>1)]=13000;
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) nhw_process[scan+(IM_SIZE>>1)]=13000; 
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=13000; 
				}
			}
		}
	}
	}

	free(res256);

	for (i=0,count=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++)
		{
			nhw_process[scan++]=resIII[count++];
		}
	}

	free(resIII);

//////////////////////////////////////////////////////////////////////////////////////////////////

	free(im->im_jpeg);
	
	if (im->setup->quality_setting<=LOW9)
	{
		//if (im->setup->quality_setting==LOW9 || im->setup->quality_setting==LOW10)
		//{
			wvlt_thrx1=2;
			wvlt_thrx2=3;
			wvlt_thrx3=5;
			wvlt_thrx4=8;
		//}
		
		for (i=0,scan=0;i<(IM_SIZE>>2)-(2*IM_DIM);i+=(IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>2)-2;j++,scan++)
			{
				if (abs(nhw_process[scan+1]-nhw_process[scan+(2*IM_DIM)+1])<wvlt_thrx3 && abs(nhw_process[scan+(IM_DIM)]-nhw_process[scan+(IM_DIM)+2])<wvlt_thrx3)
				{
					if (abs(nhw_process[scan+(IM_DIM)+1]-nhw_process[scan+(IM_DIM)])<(wvlt_thrx4-1) && abs(nhw_process[scan+1]-nhw_process[scan+(IM_DIM)+1])<wvlt_thrx4)
					{
						nhw_process[scan+(IM_DIM)+1]=(nhw_process[scan+1]+nhw_process[scan+(2*IM_DIM)+1]+nhw_process[scan+(IM_DIM)]+nhw_process[scan+(IM_DIM)+2]+2)>>2;
					}
				}
			}
		}
		
		for (i=0,scan=0;i<(IM_SIZE>>2)-(2*IM_DIM);i+=(IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>2)-2;j++,scan++)
			{
				if (abs(nhw_process[scan+2]-nhw_process[scan+1])<wvlt_thrx3 && abs(nhw_process[scan+1]-nhw_process[scan])<wvlt_thrx3)
				{
					if (abs(nhw_process[scan]-nhw_process[scan+(IM_DIM)])<wvlt_thrx3 && abs(nhw_process[scan+2]-nhw_process[scan+(IM_DIM)+2])<wvlt_thrx3)
					{
						if (abs(nhw_process[scan+(2*IM_DIM)+1]-nhw_process[scan+(IM_DIM)])<wvlt_thrx3 && abs(nhw_process[scan+(IM_DIM)]-nhw_process[scan+(IM_DIM)+1])<wvlt_thrx4) 
						{
							nhw_process[scan+(IM_DIM)+1]=(nhw_process[scan+1]+nhw_process[scan+(2*IM_DIM)+1]+nhw_process[scan+(IM_DIM)]+nhw_process[scan+(IM_DIM)+2]+1)>>2;
						}
					}
				}
			}
		}
	}

	enc->exw_Y[enc->exw_Y_end++]=0;enc->exw_Y[enc->exw_Y_end++]=0;

	for (i=0,a=((IM_SIZE>>2)+(IM_SIZE>>4));i<((IM_SIZE)>>2);i+=(IM_DIM))
	{
		for (j=0;j<((IM_DIM)>>2);j++)
		{
			scan=nhw_process[j+i];

			if (scan>255 && (j>0 || i>0)) 
			{
				enc->exw_Y[enc->exw_Y_end++]=(i>>8);enc->exw_Y[enc->exw_Y_end++]=j+128;
				Y=scan-255;if (Y>255) Y=255;enc->exw_Y[enc->exw_Y_end++]=Y;
				enc->tree1[a]=enc->tree1[a-1];a++;nhw_process[j+i]=0;
			}
			else if (scan<0 && (j>0 || i>0)) 
			{
				enc->exw_Y[enc->exw_Y_end++]=(i>>8);enc->exw_Y[enc->exw_Y_end++]=j;
				if (scan<-255) scan=-255;
				enc->exw_Y[enc->exw_Y_end++]=-scan;enc->tree1[a]=enc->tree1[a-1];a++;nhw_process[j+i]=0;
			}
			else 
			{
				if (scan>255) 
				{
					scan=255;
				}else if (scan<0) 
				{
					scan=0;
				}
				enc->tree1[a++]=scan&254;nhw_process[j+i]=0;
			}
		}
	}

	if (im->setup->quality_setting>LOW5) 
	{
		enc->res_V_64=(unsigned char*)malloc((IM_DIM<<1)*sizeof(char));
		scan_run=(unsigned char*)enc->res_V_64;

		for (i=0,e=0;i<(16*IM_DIM);i+=8)
		{
			ch_comp[i]=((enc->tree1[i+20480])>>1)&1;
			ch_comp[i+1]=((enc->tree1[i+20481])>>1)&1;
			ch_comp[i+2]=((enc->tree1[i+20482])>>1)&1;
			ch_comp[i+3]=((enc->tree1[i+20483])>>1)&1;
			ch_comp[i+4]=((enc->tree1[i+20484])>>1)&1;
			ch_comp[i+5]=((enc->tree1[i+20485])>>1)&1;
			ch_comp[i+6]=((enc->tree1[i+20486])>>1)&1;
			ch_comp[i+7]=((enc->tree1[i+20487])>>1)&1;
	
			scan_run[e++]=(ch_comp[i]<<7)|(ch_comp[i+1]<<6)|(ch_comp[i+2]<<5)|(ch_comp[i+3]<<4)|(ch_comp[i+4]<<3)|
					  (ch_comp[i+5]<<2)|(ch_comp[i+6]<<1)|ch_comp[i+7];
		}

		free(ch_comp);
	}

	offsetUV(im,ratio);

	for (j=0,count=(4*IM_SIZE+1);j<(IM_DIM);)
	{
		for (i=0;i<(IM_DIM>>1);i++)
		{
			im->im_nhw[count]=nhw_process[j];
			im->im_nhw[count+2]=nhw_process[j+1];
			im->im_nhw[count+4]=nhw_process[j+2];
			im->im_nhw[count+6]=nhw_process[j+3];
			im->im_nhw[count+8]=nhw_process[j+4];
			im->im_nhw[count+10]=nhw_process[j+5];
			im->im_nhw[count+12]=nhw_process[j+6];
			im->im_nhw[count+14]=nhw_process[j+7];
	
			j+=(IM_DIM);
			im->im_nhw[count+16]=nhw_process[j+7];
			im->im_nhw[count+18]=nhw_process[j+6];
			im->im_nhw[count+20]=nhw_process[j+5];
			im->im_nhw[count+22]=nhw_process[j+4];
			im->im_nhw[count+24]=nhw_process[j+3];
			im->im_nhw[count+26]=nhw_process[j+2];
			im->im_nhw[count+28]=nhw_process[j+1];
			im->im_nhw[count+30]=nhw_process[j];

			j+=(IM_DIM);
			count+=32;
		}

		j-=(IM_SIZE-8);
	}

	free(im->im_process);

	highres_compression(im,enc);

	wavlts2packet(im,enc);

	free(im->im_nhw);

}

/*
 * get_little_endian_uint
 */
unsigned int get_little_endian_uint(unsigned char *pbuff)
{
    return  (unsigned int)pbuff[0]     +
           ((unsigned int)pbuff[1]<< 8)+
           ((unsigned int)pbuff[2]<<16)+
           ((unsigned int)pbuff[3]<<24);
}

/* copied and fixed... */
unsigned short get_little_endian_ushort(unsigned char *pbuff)
{
    return  (unsigned short)pbuff[0]     +
           ((unsigned short)pbuff[1]<< 8);
}

/*
 * based on information from
 * https://en.wikipedia.org/wiki/BMP_file_format
 */
int header_check(FILE *i_f, int *data_offset, int req_width, int req_height, char *flipd)
{
    int result, bih_size;

    if (i_f == NULL || data_offset == NULL || flipd == NULL)
    {
        result = HEADER_CHECK_ERROR_NULL_POINTER;
    }
    else if (fseek(i_f, 0, SEEK_SET) != 0)
    {
        /* seek failure */
        result = HEADER_CHECK_ERROR_FSEEK;
    }
    else if (fread(bmp_header, 1, HEADER_CHECK_MINIMAL_AMOUNT, i_f) < HEADER_CHECK_MINIMAL_AMOUNT)
    {
        result = HEADER_CHECK_ERROR_NO_DATA;
    }
    else
    {
        /* we have some actual data! */

        /* check for bitmap correctness */
        if (bmp_header[0] == 'B' && bmp_header[1] == 'M')
        {
            /* file header seems OK */

            /* store the offset of real data */
            *data_offset = (int)get_little_endian_uint(&bmp_header[10]);

            /* more checks in the Bitmap Information Header */
            bih_size = (int)get_little_endian_uint(&bmp_header[14]);

            if (
                bih_size == BITMAPCOREHEADER_SIZE
                ||
                bih_size == BITMAPINFOHEADER_SIZE
                ||
                bih_size == BITMAPV2INFOHEADER_SIZE
                ||
                bih_size == BITMAPV3INFOHEADER_SIZE
                ||
                bih_size == BITMAPV4HEADER_SIZE
                ||
                bih_size == BITMAPV5HEADER_SIZE
               )
            {
                /* maybe we're dealing with a valid bih */
                int width, height, compr;
                short planes, bpp;

                if (bih_size == BITMAPCOREHEADER_SIZE)
                {
                    /* this is a very ancient bih! however, let's parse it as well */
                    width = (int)get_little_endian_ushort(&bmp_header[18]);
                    height = (int)get_little_endian_ushort(&bmp_header[20]);
                    planes = (short)get_little_endian_ushort(&bmp_header[22]);
                    bpp = (short)get_little_endian_ushort(&bmp_header[24]);
                    compr = BI_RGB; /* there was no compression field back then */
                }
                else
                {
                    /* this is a bih commonly used by modern apps */
                    width = (int)get_little_endian_uint(&bmp_header[18]);
                    height = (int)get_little_endian_uint(&bmp_header[22]);
                    planes = (short)get_little_endian_ushort(&bmp_header[26]);
                    bpp = (short)get_little_endian_ushort(&bmp_header[28]);
                    compr = (int)get_little_endian_uint(&bmp_header[30]);
                }

                if (planes != 1)
                {
                    /* as planes MUST ALWAYS be 1, this means we're parsing a malformed bitmap file */
                    result = HEADER_CHECK_ERROR_MALF_PLANES;
                }
                else
                {
                    /* now for sure, this is a VALID bitmap file */
                    /* let's see the properties for application suitability */
                    if (
                        width == req_width
                        &&
                        (height == req_height || height == (-1*req_height))
                        &&
                        bpp == BI_BITCOUNT_5
                        &&
                        compr == BI_RGB
                       )
                    {
                        /* image is OK */
                        *flipd = (height < 0);
                        result = HEADER_CHECK_NO_ERROR;
                    }
                    else
                    {
                        /* this image cannot be used by the application */
                        result = HEADER_CHECK_ERROR_WRONG_IMG_FORMAT;
                    }
                }
            }
            else
            {
                result = HEADER_CHECK_ERROR_BIH_NOT_SUPPORTED;
            }
        }
        else
        {
            /* probably not a bitmap image file */
            result = HEADER_CHECK_ERROR_NO_VALID_SIG;
        }
    }

    return result;
}

void swap_lines(unsigned char *buff, unsigned char *tbuff, int length, int line1, int line2)
{
    /* store line1 on temporary buffer */
    memcpy(tbuff,               buff+(length*line1), length);
    /* store line2 on line1 */
    memcpy(buff+(length*line1), buff+(length*line2), length);
    /* store temporary buffer on line2 */
    memcpy(buff+(length*line2), tbuff,               length);
}

/* vertical flip a true color image */
void image_vertical_flip(unsigned char *buff, int width, int height)
{
    int idx, fixed_length;
    unsigned char *tbuff;

    /* line length in bitmap files must be mod4 */
    fixed_length=((24 * width + 31) / 32) * 4;
    tbuff=(unsigned char*)calloc(fixed_length, sizeof(unsigned char));

    if (tbuff != NULL)
    {
        /* we can swap lines only when the temporary buffer has been allocated */
        for (idx=0; idx<height/2; idx++)
        {
            swap_lines(buff, tbuff, fixed_length, idx, height-1-idx);
        }
        free(tbuff);
    }
}

int read_image_bmp(char *file_name, encode_state *os, image_buffer *im, int rate)
{
	FILE *im256;
    char flag_flipped;
    int hd_ck, data_offset;

	// INITS & MEMORY ALLOCATION FOR ENCODING
	//im->setup=(codec_setup*)malloc(sizeof(codec_setup));
	im->setup->colorspace=YUV;
	im->setup->wavelet_type=WVLTS_53;
	im->setup->RES_HIGH=0;
	im->setup->RES_LOW=3;
	im->setup->wvlts_order=2;
	im->im_buffer4=(unsigned char*)calloc(4*3*IM_SIZE,sizeof(char));

	// OPEN IMAGE
	if ((im256 = fopen(file_name,"rb")) == NULL )
	{
		printf ("menu(): Could not open file: %s\n", file_name);
		exit(-1);
	}

	// SKIP BMP HEADER 
    if ((hd_ck=header_check(im256, &data_offset, 512, 512, &flag_flipped)) != HEADER_CHECK_NO_ERROR)
    {
        /* something went wrong when parsing the bitmap file */
        printf ("invalid image file.\n");
        exit(hd_ck);
    }

    // reach the image data
    if (fseek(im256, data_offset, SEEK_SET) != 0)
    {
        /* seek failure */
        printf ("unable to seek to actual data.\n");
        exit(-2);
    }

	// READ IMAGE DATA
	fread(im->im_buffer4,4*3*IM_SIZE,1,im256); 
	fclose(im256);

    if (flag_flipped)
    {
        /* when image height is negative, the data is vertical flipped - that is, start of data encodes the left-top corner */
        image_vertical_flip(im->im_buffer4, 512, 512);
    }

	downsample_YUV420(im,rate);

	return(0);
}

int write_compressed_file(image_buffer *im,encode_state *enc,char *file_name)
{
	FILE *compressed;
//	int len,i;

	compressed = fopen(file_name,"wb");
	if( NULL == compressed )
	{
		printf("Failed to create file: %s\n",file_name);
		return (-1);
	}

	im->setup->RES_HIGH+=im->setup->wavelet_type;

	fwrite(&im->setup->RES_HIGH,1,1,compressed);
	fwrite(&im->setup->quality_setting,1,1,compressed);
	fwrite(&enc->size_tree1,2,1,compressed);
	fwrite(&enc->size_tree2,2,1,compressed);
	fwrite(&enc->size_data1,4,1,compressed);
	fwrite(&enc->size_data2,4,1,compressed);
	fwrite(&enc->tree_end,2,1,compressed);
	fwrite(&enc->exw_Y_end,2,1,compressed);
	if (im->setup->quality_setting>LOW8) fwrite(&enc->nhw_res1_len,2,1,compressed);
	
	if (im->setup->quality_setting>=LOW1)
	{
		fwrite(&enc->nhw_res3_len,2,1,compressed);
		fwrite(&enc->nhw_res3_bit_len,2,1,compressed);
	}
	if (im->setup->quality_setting>LOW3)
	{
		fwrite(&enc->nhw_res4_len,2,1,compressed);
	}
	
	if (im->setup->quality_setting>LOW8) fwrite(&enc->nhw_res1_bit_len,2,1,compressed);

	if (im->setup->quality_setting>=HIGH1)
	{
		fwrite(&enc->nhw_res5_len,2,1,compressed);
		fwrite(&enc->nhw_res5_bit_len,2,1,compressed);
	}

	if (im->setup->quality_setting>HIGH1)
	{
		fwrite(&enc->nhw_res6_len,4,1,compressed);
		fwrite(&enc->nhw_res6_bit_len,2,1,compressed);
		fwrite(&enc->nhw_char_res1_len,2,1,compressed);
	}

	if (im->setup->quality_setting>HIGH2)
	{
		fwrite(&enc->qsetting3_len,2,1,compressed);
	}

	fwrite(&enc->nhw_select1,2,1,compressed);
	fwrite(&enc->nhw_select2,2,1,compressed);
	
	if (im->setup->quality_setting>LOW5)
	{
		fwrite(&enc->highres_comp_len,2,1,compressed);
	}
	
	fwrite(&enc->end_ch_res,2,1,compressed);
	fwrite(enc->tree1,enc->size_tree1,1,compressed);
	fwrite(enc->tree2,enc->size_tree2,1,compressed);
	fwrite(enc->exw_Y,enc->exw_Y_end,1,compressed);
	
	if (im->setup->quality_setting>LOW8)
	{
		fwrite(enc->nhw_res1,enc->nhw_res1_len,1,compressed);
		fwrite(enc->nhw_res1_bit,enc->nhw_res1_bit_len,1,compressed);
		fwrite(enc->nhw_res1_word,enc->nhw_res1_word_len,1,compressed);
	}
	
	if (im->setup->quality_setting>LOW3)
	{
		fwrite(enc->nhw_res4,enc->nhw_res4_len,1,compressed);
	}
	
	if (im->setup->quality_setting>=LOW1)
	{
		fwrite(enc->nhw_res3,enc->nhw_res3_len,1,compressed);
		fwrite(enc->nhw_res3_bit,enc->nhw_res3_bit_len,1,compressed);
		fwrite(enc->nhw_res3_word,enc->nhw_res3_word_len,1,compressed);
	}
	
	if (im->setup->quality_setting>=HIGH1)
	{
		fwrite(enc->nhw_res5,enc->nhw_res5_len,1,compressed);
		fwrite(enc->nhw_res5_bit,enc->nhw_res5_bit_len,1,compressed);
		fwrite(enc->nhw_res5_word,enc->nhw_res5_word_len,1,compressed);
	}

	if (im->setup->quality_setting>HIGH1)
	{
		fwrite(enc->nhw_res6,enc->nhw_res6_len,1,compressed);
		fwrite(enc->nhw_res6_bit,enc->nhw_res6_bit_len,1,compressed);
		fwrite(enc->nhw_res6_word,enc->nhw_res6_word_len,1,compressed);
		fwrite(enc->nhw_char_res1,enc->nhw_char_res1_len,2,compressed);
	}

	if (im->setup->quality_setting>HIGH2)
	{
		fwrite(enc->high_qsetting3,enc->qsetting3_len,4,compressed);

	}

	fwrite(enc->nhw_select_word1,enc->nhw_select1,1,compressed);
	fwrite(enc->nhw_select_word2,enc->nhw_select2,1,compressed);

	if (im->setup->quality_setting>LOW5) 
	{
		fwrite(enc->res_U_64,(IM_DIM<<1),1,compressed);
		fwrite(enc->res_V_64,(IM_DIM<<1),1,compressed);
		fwrite(enc->highres_word,enc->highres_comp_len,1,compressed);
	}
	
	fwrite(enc->ch_res,enc->end_ch_res,1,compressed);
	fwrite(enc->encode,enc->size_data2*4,1,compressed);

	fclose(compressed);

	free(enc->encode);
	
	if (im->setup->quality_setting>LOW3)
	{
		free(enc->nhw_res4);
	}
	
	if (im->setup->quality_setting>LOW8)
	{
	free(enc->nhw_res1);
	free(enc->nhw_res1_bit);
	free(enc->nhw_res1_word);
	}
	
	free(enc->nhw_select_word1);
	free(enc->nhw_select_word2);

	if (im->setup->quality_setting>=LOW1)
	{
		free(enc->nhw_res3);
		free(enc->nhw_res3_bit);
		free(enc->nhw_res3_word);
	}

	if (im->setup->quality_setting>=HIGH1)
	{
		free(enc->nhw_res5);
		free(enc->nhw_res5_bit);
		free(enc->nhw_res5_word);
	}

	if (im->setup->quality_setting>HIGH1)
	{
		free(enc->nhw_res6);
		free(enc->nhw_res6_bit);
		free(enc->nhw_res6_word);
		free(enc->nhw_char_res1);
	}

	if (im->setup->quality_setting>HIGH2)
	{
		free(enc->high_qsetting3);
	}

	free(enc->exw_Y);
	free(enc->ch_res);

	if (im->setup->quality_setting>LOW5)  	
	{
		free(enc->highres_word);
		free(enc->res_U_64);
		free(enc->res_V_64);
	}

	return 0;
}

