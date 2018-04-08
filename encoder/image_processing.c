/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: image_processing.c  										       *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -image processing set										   *
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
#include "tree.h"

void quantizationUV(image_buffer *im)
{
	int i,j,low,high;

	low=(0xFF)^((1<<im->setup->RES_LOW)-1);
	high=(0xFF)^((1<<im->setup->RES_HIGH)-1);

	for (i=0;i<(IM_SIZE>>3);i+=IM_DIM)
	{
		for (j=0;j<(IM_DIM>>3);j++)
		{
			if (i<(IM_SIZE>>4) && j<(IM_DIM>>4)) im->im_process[j+i]&=high;
			else im->im_process[j+i]&=low;
		}
	}

	for (i=0;i<(IM_SIZE>>3);i+=IM_DIM)
	{
		for (j=(IM_DIM>>3);j<IM_DIM;j++) im->im_process[j+i]&=low;
	}

	for (i=(IM_SIZE>>3);i<IM_SIZE;i++)
	{
		im->im_process[i]&=low;
	}
}

void quantizationY(image_buffer *im)
{
	int i,j,low,high;

	low=(0xFF)^((1<<im->setup->RES_LOW)-1);
	high=(0xFF)^((1<<im->setup->RES_HIGH)-1);

	for (i=0;i<((4*IM_SIZE)>>3);i+=(2*IM_DIM))
	{
		for (j=0;j<((2*IM_DIM)>>3);j++)
		{
			if (i<((4*IM_SIZE)>>4) && j<((2*IM_DIM)>>4)) im->im_process[j+i]&=high;
			else im->im_process[j+i]&=low;
		}
	}

	for (i=0;i<((4*IM_SIZE)>>3);i+=(2*IM_DIM))
	{
		for (j=((2*IM_DIM)>>3);j<2*IM_DIM;j++) im->im_process[j+i]&=low;
	}

	for (i=((4*IM_SIZE)>>3);i<4*IM_SIZE;i++)
	{
		im->im_process[i]&=low;
	}

}

void offsetUV(image_buffer *im,encode_state *enc,int m2)
{
	int i,j,wavelet_order,exw,a;

	for (i=0;i<IM_SIZE;i++)
	{
		a = im->im_process[i];

		//if (a>10000) {im->im_process[i]=124;continue;}
		//else if (a<-6 && a>-10) {im->im_process[i]=134;continue;}

		if (a>10000)
		{ 
			if (a==12400) {im->im_process[i]=124;continue;} 
			else if (a==12600) {im->im_process[i]=126;continue;} 
			else if (a==12900) {im->im_process[i]=122;continue;}  
			else if (a==13000) {im->im_process[i]=130;continue;}  
		}

		if (a>127) 
		{
			exw = ((a&0xfff8)-128)>>3;
			if (exw>18) exw=18;
			im->im_process[i]=extra_words1[exw];
			continue;
		}
		else if (a<-127) 
		{
			exw = (((-a)&0xfff8)-128)>>3;
			if (exw>18) exw=18;
			im->im_process[i]=extra_words2[exw];
			continue;
		}

		if (a==-7 || a==-8)
		{
			if ((i&255)<(IM_DIM-1) && (im->im_process[i+1]==-7 || im->im_process[i+1]==-8)) 
			{
				im->im_process[i]=120;im->im_process[i+1]=120;i++;continue;
			}
			else goto L_OVER4N;
		}
		else if (a<0)
		{
L_OVER4N:	a = -a;

			if (im->im_process[i+1]<0 && im->im_process[i+1]>-8)
			{
				if ((a&7)<6)
				{
					a&=504;
				}
			}
			else
			{
				if ((a&7)<7)
				{
					a&=504;
				}
			}
			
			a = -a;
		}
		else if (a>6 && (a&7)>=6)
		{
			if ((i&255)<(IM_DIM-1) && im->im_process[i+1]==7) im->im_process[i+1]=8;
		}

		//if (a==7 || a==4) {im->im_process[i]=132;continue;} 
		if (a<m2 && a>-m2) {im->im_process[i]=128;continue;}
		else a += 128;

		im->im_process[i]=a&248;

	}
}

void offsetY(image_buffer *im,encode_state *enc, int m1)
{
	int i,j,wavelet_order,exw,a,r,scan,t1=0;
	short *nhw_process;

	nhw_process=(short*)im->im_process;

	wavelet_order=im->setup->wvlts_order;

	for (i=0;i<(4*IM_SIZE);i++)
	{
		if (i>=(2*IM_SIZE) || (i&511)>=IM_DIM)
		{
		if (nhw_process[i]>7 && nhw_process[i+1]>7 && (i&511)<((2*IM_DIM)-1))
		{
			a=nhw_process[i];

			if (!(a&7) && !(nhw_process[i+1]&7))
			{
				if (a>15)
				{
					if (i>0)
					{
						if (nhw_process[i-1]<=0) 
						{
							nhw_process[i]--;
						}
						else if (nhw_process[i+1]>15)
						{
							if ((i&511)<((2*IM_DIM)-2))
							{
								if (nhw_process[i+2]<=0) 
								{	
									nhw_process[i+1]--;
								}
							}
						}
					}
				}
				else if (nhw_process[i+1]>15)
				{
					if ((i&511)<((2*IM_DIM)-2))
					{
						if (nhw_process[i+2]<=0) 
						{	
							nhw_process[i+1]--;
						}
					}
				}
			}
		}
		}
	}

	for (i=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (a=i+1,j=1;j<(IM_DIM-1);j++,a++)
		{
			if (nhw_process[a]>3 && nhw_process[a]<8)
			{
				if (nhw_process[a-1]>3 && nhw_process[a-1]<=7)
				{
					if (nhw_process[a+1]>3 && nhw_process[a+1]<=7)
					{
						nhw_process[a]=12700;nhw_process[a-1]=10100;j++;a++;//nhw_process[a+1]=10100;
					}
					else if (nhw_process[a+(2*IM_DIM-1)]>3 && nhw_process[a+(2*IM_DIM-1)]<=7)
					{
						if (nhw_process[a+(2*IM_DIM)]>3 && nhw_process[a+(2*IM_DIM)]<=7)
						{
							nhw_process[a-1]=12100;nhw_process[a]=10100;
							nhw_process[a+(2*IM_DIM-1)]=10100;nhw_process[a+(2*IM_DIM)]=10100;
							j++;a++;
						}
					}
				}
			}
			else if (nhw_process[a]<-3 && nhw_process[a]>-8)
			{
				if (nhw_process[a-1]<-3 && nhw_process[a-1]>=-7)
				{
					if (nhw_process[a+1]<-3 && nhw_process[a+1]>=-7)
					{
						nhw_process[a]=12900;nhw_process[a-1]=10100;j++;a++;//nhw_process[a+1]=10100;	 
					}
					else if (nhw_process[a+(2*IM_DIM-1)]<-3 && nhw_process[a+(2*IM_DIM-1)]>=-7)
					{
						if (nhw_process[a+(2*IM_DIM)]<-3 && nhw_process[a+(2*IM_DIM)]>=-7)
						{
							nhw_process[a-1]=12200;nhw_process[a]=10100;
							nhw_process[a+(2*IM_DIM-1)]=10100;nhw_process[a+(2*IM_DIM)]=10100;
							j++;a++;
						}
					}
				}
			}
		}
	}

	for (i=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (a=i,j=0;j<(IM_DIM-1);j++,a++)
		{
			if (nhw_process[a]==5 || nhw_process[a]==6 || nhw_process[a]==7)
			{
				if (nhw_process[a+1]==5 || nhw_process[a+1]==6 || nhw_process[a+1]==7)
				{
					//nhw_process[a]+=3;
					nhw_process[a]=10300;
					j++;a++;
				}
			}
			else if (nhw_process[a]==-5 || nhw_process[a]==-6 || nhw_process[a]==-7)
			{
				if (nhw_process[a+1]==-5 || nhw_process[a+1]==-6 || nhw_process[a+1]==-7)
				{
					//nhw_process[a]-=3;
					nhw_process[a]=10204;
					j++;a++;
				}
			}
		}
	}

	for (i=0;i<(4*IM_SIZE);i++)
	{
		a = nhw_process[i];

		if (a>10000)
		{
			if (a==10100) {nhw_process[i]=128;continue;}
			else if (a==12700) {nhw_process[i]=127;continue;}
			else if (a==12900) {nhw_process[i]=129;continue;}
			else if (a==10204) {nhw_process[i]=125;continue;}
			else if (a==10300) {nhw_process[i]=126;continue;}
			else if (a==12100) {nhw_process[i]=121;continue;}
			else if (a==12200) {nhw_process[i]=122;continue;}
		}

		if (a>127) 
		{
			exw = ((a&0xfff8)-128)>>3;
			if (exw>18) exw=18;
			nhw_process[i]=extra_words1[exw];
			continue;
		}
		else if (a<-127) 
		{
			exw = (((-a)&0xfff8)-128)>>3;
			if (exw>18) exw=18;
			nhw_process[i]=extra_words2[exw];
			continue;
		}

		if (a<-12 && ((-a)&7)==6)
		{
			if ((i&511)<(2*IM_DIM-1) && nhw_process[i+1]==-7) nhw_process[i+1]=-9;
		}
		
		if (a<0)
		{
			if (a==-7 && nhw_process[i+1]==8 && (i&511)<(2*IM_DIM-1)) {nhw_process[i]=-8;a=-8;}

			a = -a;

			if ((a&7)<7)
			{
				a&=504;
			}

			a = -a;
		}
		else if (a==8 && nhw_process[i+1]==-7 && (i&511)<(2*IM_DIM-1)) nhw_process[i+1]=-8;
		else if (a>12 && (a&7)>=6)
		{
			if ((i&511)<(2*IM_DIM-1) && nhw_process[i+1]==7) nhw_process[i+1]=9;
		}

		if (a<m1 && a>-m1)  
		{
			nhw_process[i]=128;continue;
		}
		else a += 128;

		nhw_process[i]=a&248;
	
	}
}

void im_recons_wavelet_band(image_buffer *im)
{
	int i,j,a,r,scan,count;

	im->im_wavelet_band=(short*)calloc(IM_SIZE,sizeof(short));
		
	for (i=0,r=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{ 
		for (scan=i+IM_DIM,j=0;j<IM_DIM;j++,scan++)
		{
			a = im->im_process[scan];

			if (a==128) {r++;continue;}
			else if (a==127) {im->im_wavelet_band[r-1]=5;im->im_wavelet_band[r]=6;im->im_wavelet_band[r+1]=5;r+=2;j++;scan++;}
			else if (a==129) {im->im_wavelet_band[r-1]=-5;im->im_wavelet_band[r]=-7;im->im_wavelet_band[r+1]=-5;r+=2;j++;scan++;}
			else if ((a&7)!=0)
			{
				if (extra_table[a]>0) 
				{
					im->im_wavelet_band[r++]=WVLT_ENERGY_NHW+(extra_table[a]<<3);
				}
				else 
				{
					im->im_wavelet_band[r++]=(extra_table[a]<<3)-WVLT_ENERGY_NHW;
				}
			}
			else 
			{
				if (a>0x80) im->im_wavelet_band[r++]=(a-125);
				else im->im_wavelet_band[r++]=(a-131);
			}	
		}
	}
}

void pre_processing(image_buffer *im)
{
	int i,j,scan,res,count,e=0,a=0,sharpness;
	short *nhw_process;
	char lower_quality_setting_on;

	nhw_process=(short*)im->im_process;
	memcpy(im->im_process,im->im_jpeg,4*IM_SIZE*sizeof(short));

	if (im->setup->quality_setting<LOW2) lower_quality_setting_on=1;
	else lower_quality_setting_on=0;

	if (im->setup->quality_setting==LOW4) sharpness=82;else sharpness=77;

	for (i=(2*IM_DIM);i<((4*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i+1,j=1;j<((2*IM_DIM)-2);j++,scan++)
		{   
			res	   =   (nhw_process[scan]<<3) -
						nhw_process[scan-1]-nhw_process[scan+1]-
						nhw_process[scan-(2*IM_DIM)]-nhw_process[scan+(2*IM_DIM)]-
						nhw_process[scan-(2*IM_DIM+1)]-nhw_process[scan+(2*IM_DIM-1)]-
						nhw_process[scan-(2*IM_DIM-1)]-nhw_process[scan+(2*IM_DIM+1)];

			j++;scan++;

			count   =  (nhw_process[scan]<<3) -
						nhw_process[scan-1]-nhw_process[scan+1]-
						nhw_process[scan-(2*IM_DIM)]-nhw_process[scan+(2*IM_DIM)]-
						nhw_process[scan-(2*IM_DIM+1)]-nhw_process[scan+(2*IM_DIM-1)]-
						nhw_process[scan-(2*IM_DIM-1)]-nhw_process[scan+(2*IM_DIM+1)];


			/*if (lower_quality_setting_on)
			{
				if (abs(count)>4 && abs(count)<65 && abs(res)<30)
				{
					if (abs(nhw_process[scan-(2*IM_DIM)]-nhw_process[scan-1])<9 && abs(nhw_process[scan-1]-nhw_process[scan+(2*IM_DIM)])<9 && abs(nhw_process[scan+(2*IM_DIM)]-nhw_process[scan+1])<9 && abs(nhw_process[scan+1]-nhw_process[scan-(2*IM_DIM)])<9)
					{
						im->im_jpeg[scan]=( (nhw_process[scan]<<2) +
											 nhw_process[scan-1]+nhw_process[scan+1]+
											 nhw_process[scan-(2*IM_DIM)]+nhw_process[scan+(2*IM_DIM)]+4 )>>3;

					}
				}
			}*/

			if (im->setup->quality_setting>LOW4) 
			{
			if (res>201) {im->im_jpeg[scan-1]-=2;e=4;}
			else if (res<-201) {im->im_jpeg[scan-1]+=2;e=3;}
			else if (res>176) {im->im_jpeg[scan-1]--;e=2;}
			else if (res<-176) {im->im_jpeg[scan-1]++;e=1;}
			else e=0;

			if (count>201)
			{
				if (!e || e==3) im->im_jpeg[scan]-=2;
				else if (e!=4) im->im_jpeg[scan]--;
			}
			else if (count<-201) 
			{
				if (!e || e==4) im->im_jpeg[scan]+=2;
				else if (e!=3) im->im_jpeg[scan]++;
			}
			else if (count>176) 
			{
				if (e!=4) im->im_jpeg[scan]--;
			}
			else if (count<-176) 
			{
				if (e!=3) im->im_jpeg[scan]++;
			}
			}
			else
			{
				if (abs(res)>sharpness)
				{
					if (res>0) im->im_jpeg[scan-1]++;else im->im_jpeg[scan-1]--;
				}

				if (abs(count)>sharpness)
				{
					if (count>0) im->im_jpeg[scan]++;else im->im_jpeg[scan]--;
				}

			}

			if (res<32 && res>10) 
			{
				if (abs(count)>=23) 
				{
					if (res<16)
					{
						if (count>0 && count<32 && res>11) im->im_jpeg[scan]++;
						im->im_jpeg[scan-1]++;
						a=0;continue;
					}
					else 
					{
						if (!a) im->im_jpeg[scan-1]+=2;else im->im_jpeg[scan-1]++;
						a=0;continue;
					}
				}
			}
			else if (res>-32 && res<-10) 
			{
				if (abs(count)>=23) 
				{
					if (res>-16)
					{
						if (count<0 && count>-32 && res<-11) im->im_jpeg[scan]--;
						im->im_jpeg[scan-1]--;
						a=0;continue;
					}
					else 
					{
						if (!a) im->im_jpeg[scan-1]-=2;else im->im_jpeg[scan-1]--; 
						a=0;continue;
					}
				}
			}
			
			a=0;

			if (count<32 && count>10) 
			{
				if (abs(res)>=23)
				{
					if (count<16)
					{
						if (res>0 && res<32 && count>11) im->im_jpeg[scan-1]++;
						im->im_jpeg[scan]++;
					}
					else {im->im_jpeg[scan]+=2;a=1;}
				}
			}
			else if (count>-32 && count<-10) 
			{
				if (abs(res)>=23)
				{
					if (count>-16)
					{
						if (res<0 && res>-32 && count<-11) im->im_jpeg[scan-1]--;
						im->im_jpeg[scan]--;
					}
					else {im->im_jpeg[scan]-=2;a=1;}
				}
			}
		}
	}
}

void block_variance_avg(image_buffer *im)
{
	int i,j,e,a,t1,scan,count,avg,variance;
	short *nhw_process,*nhw_process2;
	unsigned char *block_var;

	memcpy(im->im_process,im->im_jpeg,4*IM_SIZE*sizeof(short));

	nhw_process=(short*)im->im_jpeg;
	nhw_process2=(short*)im->im_process;

	block_var=(unsigned char*)calloc(((4*IM_SIZE)>>6),sizeof(char));

	for (i=0,a=0;i<(4*IM_SIZE);i+=(8*2*IM_DIM))
	{
		for (scan=i,j=0;j<(2*IM_DIM);j+=8,scan+=8)
		{
			avg=0;variance=0;count=0;

			for (e=0;e<(8*2*IM_DIM);e+=(2*IM_DIM))
			{
				avg+=nhw_process[scan+e];
				avg+=nhw_process[scan+1+e];
				avg+=nhw_process[scan+2+e];
				avg+=nhw_process[scan+3+e];
				avg+=nhw_process[scan+4+e];
				avg+=nhw_process[scan+5+e];
				avg+=nhw_process[scan+6+e];
				avg+=nhw_process[scan+7+e];
			}

			avg=(avg+32)>>6;

			for (e=0;e<(8*2*IM_DIM);e+=(2*IM_DIM))
			{
				count=(nhw_process[scan+e]-avg);variance+=count*count;
				count=(nhw_process[scan+1+e]-avg);variance+=count*count;
				count=(nhw_process[scan+2+e]-avg);variance+=count*count;
				count=(nhw_process[scan+3+e]-avg);variance+=count*count;
				count=(nhw_process[scan+4+e]-avg);variance+=count*count;
				count=(nhw_process[scan+5+e]-avg);variance+=count*count;
				count=(nhw_process[scan+6+e]-avg);variance+=count*count;
				count=(nhw_process[scan+7+e]-avg);variance+=count*count;
			}

			if (variance<1500)
			{
				block_var[a++]=1;

				for (e=(2*IM_DIM);e<(7*2*IM_DIM);e+=(2*IM_DIM))
				{
					for (t1=0;t1<6;t1++)
					{
						scan++;

						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-(2*IM_DIM)]+nhw_process2[scan+e+(2*IM_DIM)]+
											nhw_process2[scan+e-1-(2*IM_DIM)]+nhw_process2[scan+e+1-(2*IM_DIM)]+
											nhw_process2[scan+e-1+(2*IM_DIM)]+nhw_process2[scan+e+1+(2*IM_DIM)]+8)>>4;
					}

					scan-=6;
				}
			}
			else a++;
		}
	}

	for (i=0;i<((4*IM_SIZE)>>6)-(IM_DIM>>2);i+=(IM_DIM>>2))
	{
		for (count=i,j=0;j<(IM_DIM>>2)-1;j++,count++)
		{
			if (block_var[count])
			{
				if (block_var[count+1])
				{
					scan=(i*8*8)+(j*8);

					for (e=(2*IM_DIM);e<(7*2*IM_DIM);e+=(2*IM_DIM))
					{
						scan+=7;

						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-(2*IM_DIM)]+nhw_process2[scan+e+(2*IM_DIM)]+
											nhw_process2[scan+e-1-(2*IM_DIM)]+nhw_process2[scan+e+1-(2*IM_DIM)]+
											nhw_process2[scan+e-1+(2*IM_DIM)]+nhw_process2[scan+e+1+(2*IM_DIM)]+8)>>4;

						scan++;

						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-(2*IM_DIM)]+nhw_process2[scan+e+(2*IM_DIM)]+
											nhw_process2[scan+e-1-(2*IM_DIM)]+nhw_process2[scan+e+1-(2*IM_DIM)]+
											nhw_process2[scan+e-1+(2*IM_DIM)]+nhw_process2[scan+e+1+(2*IM_DIM)]+8)>>4;

						scan-=8;
					}
				}

				if (block_var[count+(IM_DIM>>2)])
				{
					scan=(i*8*8)+(j*8);

					scan+=(7*2*IM_DIM);

					for (e=1;e<7;e++)
					{
						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-(2*IM_DIM)]+nhw_process2[scan+e+(2*IM_DIM)]+
											nhw_process2[scan+e-1-(2*IM_DIM)]+nhw_process2[scan+e+1-(2*IM_DIM)]+
											nhw_process2[scan+e-1+(2*IM_DIM)]+nhw_process2[scan+e+1+(2*IM_DIM)]+8)>>4;
					}

					scan+=(2*IM_DIM);

					for (e=1;e<7;e++)
					{
						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-(2*IM_DIM)]+nhw_process2[scan+e+(2*IM_DIM)]+
											nhw_process2[scan+e-1-(2*IM_DIM)]+nhw_process2[scan+e+1-(2*IM_DIM)]+
											nhw_process2[scan+e-1+(2*IM_DIM)]+nhw_process2[scan+e+1+(2*IM_DIM)]+8)>>4;
					}
				}
			}
		}
	}

	free(block_var);
}

void offsetY_recons256(image_buffer *im, encode_state *enc, int m1, int part)
{
	int i,j,wavelet_order,a,e,t;
	short *nhw1,*highres_tmp;

	nhw1=(short*)im->im_process;
	wavelet_order=im->setup->wvlts_order;

	if (im->setup->quality_setting>LOW3)
	{
	if (!part)
	{
		for (i=0,a=0,e=0;i<(IM_SIZE);i+=(2*IM_DIM))
		{
			for (a=i,j=0;j<((IM_DIM>>1)-3);j++,a++)
			{
				if ((nhw1[a]&1)==1 && (nhw1[a+1]&1)==1 && (nhw1[a+2]&1)==1 && (nhw1[a+3]&1)==1 && abs(nhw1[a]-nhw1[a+3])>1)
				{
					nhw1[a]+=16000;nhw1[a+1]+=16000;nhw1[a+2]+=16000;nhw1[a+3]+=16000;
					j+=3;a+=3;
				}
			}
		}
	}
	else
	{
		for (i=0,a=0,e=0;i<(IM_SIZE);i+=(2*IM_DIM))
		{
			for (a=i,j=0;j<((IM_DIM>>1)-3);j++,a++)
			{
				if ((nhw1[a]&1)==1 && (nhw1[a+1]&1)==1 && (nhw1[a+2]&1)==1 && (nhw1[a+3]&1)==1 && abs(nhw1[a]-nhw1[a+3])>1)
				{
					nhw1[a]+=16000;nhw1[a+2]+=16000;
					j+=3;a+=3;
				}
			}
		}
	}
	}

	for (i=0,a=0,e=0,t=0;i<(IM_SIZE);i+=(2*IM_DIM))
	{
		for (a=i,j=0;j<(IM_DIM>>1);j++,a++) 
		{
			if (nhw1[a]>10000) 
			{
				if (!part) im->im_jpeg[a]=nhw1[a];
				else 
				{
					nhw1[a]-=16000;im->im_jpeg[a]=nhw1[a];
					if (nhw1[a+1]>0 && nhw1[a+1]<256) im->im_jpeg[a+1]=(nhw1[a+1]&65534);
					else im->im_jpeg[a+1]=nhw1[a+1];
					j++;a++;
				} 

				continue;
			}
			/*else if (!part && im->setup->quality_setting<=LOW3 && j>0 && j<((IM_DIM>>1)-1) && abs(nhw1[a-1]-nhw1[a+1])<1 && abs(nhw1[a-1]-nhw1[a])<=3)
		 	{
				nhw1[a]=nhw1[a-1];//nhw1[a+1]=nhw1[a-1];
			}*/
			else if ((nhw1[a]&1)==1 && a>i && (nhw1[a+1]&1)==1 /*&& !(nhw1[a-1]&1)*/)
			{
				if (j<((IM_DIM>>1)-2) && (nhw1[a+2]&1)==1 /*&& !(im->im_process[a+3]&1)*/)
				{
					if (abs(nhw1[a]-nhw1[a+2])>1 && im->setup->quality_setting>LOW3) im->im_process[a+1]++;
				}
				/*else if (j<((IM_DIM>>1)-4) && (im->im_process[a+2]&1)==1 && (im->im_process[a+3]&1)==1
						&& !(im->im_process[a+4]&1))
				{
					im->im_process[a+2]++;
				}*/
				else if (i<(IM_SIZE-(2*IM_DIM)-2) && (im->im_process[a+(2*IM_DIM)]&1)==1 
							&& (im->im_process[a+(2*IM_DIM+1)]&1)==1 && !(im->im_process[a+(2*IM_DIM+2)]&1))
				{
					if (im->im_process[a+(2*IM_DIM)]<10000 && im->setup->quality_setting>LOW3) im->im_process[a+(2*IM_DIM)]++;
				}
			}
			else if ((nhw1[a]&1)==1 && i>=(2*IM_DIM) && i<(IM_SIZE-(6*IM_DIM)))
			{
				if ((im->im_process[a+(2*IM_DIM)]&1)==1 && (im->im_process[a+(2*IM_DIM+1)]&1)==1)
				{
					if ((im->im_process[a+(4*IM_DIM)]&1)==1 && !(im->im_process[a+(6*IM_DIM)]&1)) 
					{
						if (im->im_process[a+(2*IM_DIM)]<10000 && im->setup->quality_setting>LOW3) im->im_process[a+(2*IM_DIM)]++;
					}
				}
			}

			if (part) 
			{
				if (nhw1[a]>0 && nhw1[a]<256) im->im_jpeg[a]=(nhw1[a]&65534);
				else im->im_jpeg[a]=nhw1[a];
			}
		}
	}

	if (!part)
	{
		highres_tmp=(short*)malloc((IM_SIZE>>2)*sizeof(short));

		for (i=0,t=0;i<(IM_SIZE);i+=(2*IM_DIM))
		{
			for (a=i,j=0;j<(IM_DIM>>1);j++,a++) 
			{
				if (nhw1[a]<10000)
				{
					highres_tmp[t++]=nhw1[a];
					if (nhw1[a]>=0 && nhw1[a]<256) im->im_jpeg[a]=(nhw1[a]&65534);
					else im->im_jpeg[a]=nhw1[a]; 
				}
				else
				{
					nhw1[a]-=16000;
					highres_tmp[t++]=nhw1[a];
					im->im_jpeg[a]=nhw1[a];
				}
			}
		}
	}

	
	if (im->setup->quality_setting>LOW5)
	{
		if (!part)
		{
			for (i=0;i<enc->highres_mem_len;i++)
			{
				j=(enc->highres_mem[i]>>7);
				a=enc->highres_mem[i]&127;

				im->im_jpeg[(j<<9)+a]=highres_tmp[enc->highres_mem[i]];
			}

			free(highres_tmp);
			free(enc->highres_mem);
		}
	}

	/*if (part)
	{
		for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
		{
			for (a=i+1,j=1;j<(2*IM_DIM-1);j++,a++) 
			{
				if (im->im_process[a]==m1)
				{
					if (abs(im->im_process[a-1])>=4 || abs(im->im_process[a+1])>=4) im->im_process[a]=8;
					else im->im_process[a]=0;
				}
				else if (im->im_process[a]==-m1)
				{
					if (abs(im->im_process[a-1])>=4 || abs(im->im_process[a+1])>=4) im->im_process[a]=-8;
					else im->im_process[a]=0;
				}
			}
		}
	}*/

	for (i=0;i<(IM_SIZE);i+=(2*IM_DIM))
	{
		for (a=i+(IM_DIM>>1)+1,j=(IM_DIM>>1)+1;j<(IM_DIM-1);j++,a++)
		{
				if (im->im_process[a]>3 && im->im_process[a]<8)
				{
					if (im->im_process[a-1]>3 && im->im_process[a-1]<=7)
					{
						if (im->im_process[a+1]>3 && im->im_process[a+1]<=7)
						{
							im->im_process[a-1]=15300;im->im_process[a]=0;im->im_jpeg[a]=5;im->im_jpeg[a+1]=5;j++;a++;
						}
						else if (im->im_process[a+(2*IM_DIM-1)]>3 && im->im_process[a+(2*IM_DIM-1)]<=7)
						{
							if (im->im_process[a+(2*IM_DIM)]>3 && im->im_process[a+(2*IM_DIM)]<=7)
							{
								im->im_process[a-1]=15500;im->im_jpeg[a]=5;
								im->im_process[a+(2*IM_DIM-1)]=15500;im->im_jpeg[a+(2*IM_DIM)]=5;
								im->im_process[a+(2*IM_DIM)]=0;
								j++;a++;
							}
						}
					}
				}
				else if (im->im_process[a]<-3 && im->im_process[a]>-8)
				{
					if (im->im_process[a-1]<-3 && im->im_process[a-1]>=-7)
					{
						if (im->im_process[a+1]<-3 && im->im_process[a+1]>=-7)
						{
							im->im_process[a-1]=15400;im->im_process[a]=0;im->im_jpeg[a]=-6;im->im_jpeg[a+1]=-5;j++;a++;	 
						}
						else if (im->im_process[a+(2*IM_DIM-1)]<-3 && im->im_process[a+(2*IM_DIM-1)]>=-7)
						{
							if (im->im_process[a+(2*IM_DIM)]<-3 && im->im_process[a+(2*IM_DIM)]>=-7)
							{
								im->im_process[a-1]=15600;im->im_jpeg[a]=-5;
								im->im_process[a+(2*IM_DIM-1)]=15600;im->im_jpeg[a+(2*IM_DIM)]=-5;
								im->im_process[a+(2*IM_DIM)]=0;
								j++;a++;
							}
						}
					}
				}
		}
	}

	for (i=(IM_SIZE);i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (a=i+1,j=1;j<(IM_DIM-1);j++,a++)
		{
				if (im->im_process[a]>3 && im->im_process[a]<8)
				{
					if (im->im_process[a-1]>3 && im->im_process[a-1]<=7)
					{
						if (im->im_process[a+1]>3 && im->im_process[a+1]<=7)
						{
							im->im_process[a-1]=15300;im->im_process[a]=0;im->im_jpeg[a]=5;im->im_jpeg[a+1]=5;j++;a++;
						}
						else if (im->im_process[a+(2*IM_DIM-1)]>3 && im->im_process[a+(2*IM_DIM-1)]<=7)
						{
							if (im->im_process[a+(2*IM_DIM)]>3 && im->im_process[a+(2*IM_DIM)]<=7)
							{
								im->im_process[a-1]=15500;im->im_jpeg[a]=5;
								im->im_process[a+(2*IM_DIM-1)]=15500;im->im_jpeg[a+(2*IM_DIM)]=5;
								im->im_process[a+(2*IM_DIM)]=0;
								j++;a++;
							}
						}
					}
				}
				else if (im->im_process[a]<-3 && im->im_process[a]>-8)
				{
					if (im->im_process[a-1]<-3 && im->im_process[a-1]>=-7)
					{
						if (im->im_process[a+1]<-3 && im->im_process[a+1]>=-7)
						{
							im->im_process[a-1]=15400;im->im_process[a]=0;im->im_jpeg[a]=-6;im->im_jpeg[a+1]=-5;j++;a++;	 
						}
						else if (im->im_process[a+(2*IM_DIM-1)]<-3 && im->im_process[a+(2*IM_DIM-1)]>=-7)
						{
							if (im->im_process[a+(2*IM_DIM)]<-3 && im->im_process[a+(2*IM_DIM)]>=-7)
							{
								im->im_process[a-1]=15600;im->im_jpeg[a]=-5;
								im->im_process[a+(2*IM_DIM-1)]=15600;im->im_jpeg[a+(2*IM_DIM)]=-5;
								im->im_process[a+(2*IM_DIM)]=0;
								j++;a++;
							}
						}
					}
				}
		}
	}

	if (!part)
	{
		for (i=0;i<(IM_SIZE);i+=(2*IM_DIM))
		{
			for (a=i+(IM_DIM>>1),j=(IM_DIM>>1);j<(IM_DIM-1);j++,a++)
			{
				if (im->im_process[a]==5 || im->im_process[a]==6 || im->im_process[a]==7)
				{
					if (im->im_process[a+1]==5 || im->im_process[a+1]==6 || im->im_process[a+1]==7)
					{
						//im->im_process[a]+=3;
						im->im_process[a]=15700;
						j++;a++;
					}
				}
				else if (im->im_process[a]==-5 || im->im_process[a]==-6 || im->im_process[a]==-7)
				{
					if (im->im_process[a+1]==-5 || im->im_process[a+1]==-6 || im->im_process[a+1]==-7)
					{
						//im->im_process[a]+=3;
						im->im_process[a]=15800;
						j++;a++;
					}
				}
			}
		}

		for (i=IM_SIZE;i<(2*IM_SIZE);i+=(2*IM_DIM))
		{
			for (a=i,j=0;j<(IM_DIM-1);j++,a++)
			{
				if (im->im_process[a]==5 || im->im_process[a]==6 || im->im_process[a]==7)
				{
					if (im->im_process[a+1]==5 || im->im_process[a+1]==6 || im->im_process[a+1]==7)
					{
						//im->im_process[a]+=3;
						im->im_process[a]=15700;
						j++;a++;
					}
				}
				else if (im->im_process[a]==-5 || im->im_process[a]==-6 || im->im_process[a]==-7)
				{
					if (im->im_process[a+1]==-5 || im->im_process[a+1]==-6 || im->im_process[a+1]==-7)
					{
						//im->im_process[a]-=3;
						im->im_process[a]=15800;
						j++;a++;
					}
				}
			}
		}
	}

	for (i=0;i<(IM_SIZE);i+=(2*IM_DIM))
	{
		for (j=(IM_DIM>>1);j<IM_DIM;j++) 
		{
			a = im->im_process[i+j];

			if (a>15000)
			{
				if (a==15300) {im->im_jpeg[i+j]=5;j+=2;}
				else if (a==15400) {im->im_jpeg[i+j]=-5;j+=2;}
				else if (a==15500) {im->im_jpeg[i+j]=5;j++;}
				else if (a==15600) {im->im_jpeg[i+j]=-5;j++;}
				else if (a==15700) {im->im_jpeg[i+j]=6;im->im_jpeg[i+j+1]=6;j++;}
				else if (a==15800) {im->im_jpeg[i+j]=-6;im->im_jpeg[i+j+1]=-6;j++;}

				continue;
			}

			if (a<-12 /*&& !part*/ && ((-a)&7)==6)
			{
				if (j<(IM_DIM-1) && im->im_process[i+j+1]==-7) im->im_process[i+j+1]=-8;
			}

			if (a<0)
			{
				if (a==-7 && j<(IM_DIM-1) && im->im_process[i+j+1]==8) {im->im_process[i+j]=-8;a=-8;}

				a = -a;

				if ((a&7)<7)
				{
					a&=65528;
				}

				a = -a;
	
			}
			else if (a==8 && j<(IM_DIM-1) && im->im_process[i+j+1]==-7) im->im_process[i+j+1]=-8;
			else if (a>12 && !part && (a&7)>=6)
			{
				if (j<(IM_DIM-1) && im->im_process[i+j+1]==7) im->im_process[i+j+1]=8;
			}

			if (a<m1 && a>-m1) 
			{
				im->im_jpeg[i+j]=0;continue;
			}
			else a += 128;

			if (a<0) a= -((-a)&65528);
			else a&=65528;

			if (a>128) im->im_jpeg[i+j]=a-125;
			else im->im_jpeg[i+j]=a-131;

		}
	}

	for (i=(IM_SIZE);i<(4*IM_SIZE>>1);i+=(2*IM_DIM))
	{
		for (j=0;j<IM_DIM;j++) 
		{
			a = im->im_process[i+j];

			if (a>15000)
			{
				if (a==15300) {im->im_jpeg[i+j]=5;j+=2;}
				else if (a==15400) {im->im_jpeg[i+j]=-5;j+=2;}
				else if (a==15500) {im->im_jpeg[i+j]=5;j++;}
				else if (a==15600) {im->im_jpeg[i+j]=-5;j++;}
				else if (a==15700) {im->im_jpeg[i+j]=6;im->im_jpeg[i+j+1]=6;j++;}
				else if (a==15800) {im->im_jpeg[i+j]=-6;im->im_jpeg[i+j+1]=-6;j++;}

				continue;
			}

			if (a<-12 /*&& !part*/ && ((-a)&7)==6)
			{
				if (j<(IM_DIM-1) && im->im_process[i+j+1]==-7) im->im_process[i+j+1]=-8;
			}

			if (a<0)
			{
				if (a==-7 && j<(IM_DIM-1) && im->im_process[i+j+1]==8) {im->im_process[i+j]=-8;a=-8;}

				a = -a;

				if ((a&7)<7)
				{
					a&=65528;
				}

				a = -a;
	
			}
			else if (a==8 && j<(IM_DIM-1) && im->im_process[i+j+1]==-7) im->im_process[i+j+1]=-8;
			else if (a>12 && !part && (a&7)>=6)
			{
				if (j<(IM_DIM-1) && im->im_process[i+j+1]==7) im->im_process[i+j+1]=8;
			}

			if (a<m1 && a>-(m1)) 
			{
				im->im_jpeg[i+j]=0;continue;
			}
			else a += 128;

			if (a<0) a= -((-a)&65528);
			else a&=65528;

			if (a>128) im->im_jpeg[i+j]=a-125;
			else im->im_jpeg[i+j]=a-131;

		}
	}

	if (!part)
	{
		for (i=(2*IM_DIM);i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
		{
			for (e=i+1,j=1;j<IM_DIM-1;j++,e++) 
			{
				if (abs(im->im_jpeg[e])>=8)
				{
					if (abs(im->im_jpeg[e-(2*IM_DIM+1)])>=8) continue;
					if (abs(im->im_jpeg[e-(2*IM_DIM)])>=8) continue;
					if (abs(im->im_jpeg[e-(2*IM_DIM-1)])>=8) continue;
					if (abs(im->im_jpeg[e-1])>=8) continue;
					if (abs(im->im_jpeg[e+1])>=8) continue;
					if (abs(im->im_jpeg[e+(2*IM_DIM-1)])>=8) continue;
					if (abs(im->im_jpeg[e+(2*IM_DIM)])>=8) continue;
					if (abs(im->im_jpeg[e+(2*IM_DIM+1)])>=8) continue;

					if (i>=IM_SIZE || j>=(IM_DIM>>1))
					{
						if (im->im_jpeg[e]>0) im->im_jpeg[e]--;
						else im->im_jpeg[e]++;
					}
				}
			}
		}
	}
}

void offsetUV_recons256(image_buffer *im, int m1, int comp)
{
	int i,j,wavelet_order,a,e=0;

	wavelet_order=im->setup->wvlts_order;

	if (comp)
	{	
		if (im->setup->quality_setting>LOW5) 
		{
			for (i=0;i<(IM_SIZE>>2);i++)
			{
				if ((i&255)<(IM_DIM>>2))
				{
					if (!(i>>8))
					{
						im->im_jpeg[i]=(im->im_process[i]);
						im->im_jpeg[i+1]=(im->im_process[i+1]&65534);
					}
					else
					{
						im->im_jpeg[i]=(im->im_process[i]&65534);
						im->im_jpeg[i+1]=(im->im_process[i+1]);
					}

					i++;
				}
			}
		}
		else
		{
			for (i=0;i<(IM_SIZE>>2);i++)
			{
				if ((i&255)<(IM_DIM>>2))
				{
					im->im_jpeg[i]=(im->im_process[i]&65532)+1;
				}
			}	
		}
	}
	else
	{
		for (i=0;i<(IM_SIZE>>2);i++)
		{
			if ((i&255)<(IM_DIM>>2))
			{
				if (im->im_process[i]>0 && im->im_process[i]<256) im->im_jpeg[i]=(im->im_process[i]&65534);
				else im->im_jpeg[i]=im->im_process[i];
			}
		}
	}

	for (i=0;i<(IM_SIZE>>2);i+=(IM_DIM))
	{
		for (j=(IM_DIM>>2);j<(IM_DIM>>1);j++) 
		{
			a = im->im_process[i+j];

			//if (a>10000) {im->im_jpeg[i+j]=7;im->im_jpeg[i+j+1]=7;continue;}
			//else if (a<-6 && a>-10) {im->im_jpeg[i+j]=-8;continue;}

			if ((a==-7 || a==-8) && !comp)
			{
				if (j<((IM_DIM>>1)-1) && (im->im_process[i+j+1]==-7 || im->im_process[i+j+1]==-8)) 
				{
					im->im_jpeg[i+j]=-11;im->im_jpeg[i+j+1]=-11;j++;continue;
				}
			}

			if (a<0)
			{
				a = -a;

				if (im->im_process[i+j+1]<0 && im->im_process[i+j+1]>-8)
				{
					if ((a&7)<6)
					{
						a&=65528;
					}
				}
				else 
				{
					if ((a&7)<7)
					{
						a&=65528;
					}
				}

				a = -a;
	
			}

			if (a<m1 && a>(-m1)) 
			{
				im->im_jpeg[i+j]=0;continue;
			}
			else a += 128;

			if (a<0) a= -((-a)&65528);
			else a&=65528;

			if (a>128) im->im_jpeg[i+j]=a-125;
			else im->im_jpeg[i+j]=a-131;

		}
	}

	for (i=(IM_SIZE>>2);i<(IM_SIZE>>1);i+=(IM_DIM))
	{
		for (j=0;j<(IM_DIM>>1);j++) 
		{
			a = im->im_process[i+j];

			//if (a>10000) {im->im_jpeg[i+j]=7;im->im_jpeg[i+j+1]=7;continue;}
			//else if (a<-6 && a>-10) {im->im_jpeg[i+j]=-8;continue;}

			if ((a==-7 || a==-8) && !comp)
			{
				if (j<((IM_DIM>>1)-1) && (im->im_process[i+j+1]==-7 || im->im_process[i+j+1]==-8)) 
				{
					im->im_jpeg[i+j]=-11;im->im_jpeg[i+j+1]=-11;j++;continue;
				}
			}

			if (a<0)
			{
				a = -a;
				
				if (im->im_process[i+j+1]<0 && im->im_process[i+j+1]>-8)
				{
					if ((a&7)<6)
					{
						a&=65528;
					}
				}
				else 
				{
					if ((a&7)<7)
					{
						a&=65528;
					}
				}

				a = -a;
	
			}

			if (a<m1 && a>(-m1)) 
			{
				im->im_jpeg[i+j]=0;continue;
			}
			else a += 128;

			if (a<0) a= -((-a)&65528);
			else a&=65528;

			if (a>128) im->im_jpeg[i+j]=a-125;
			else im->im_jpeg[i+j]=a-131;

		}
	}
}