/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: image_processing.c  										       *
*  version: 0.3.0-rc76 						     		     			   *
*  last update: $ 07172025 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -image processing set										   *
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

void offsetUV(image_buffer *im,int m2)
{
	int i,exw,a;

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

void offsetY(image_buffer *im,int m1)
{
	int i,j,wavelet_order,exw,a,r1=0,r2=0,quant=0,quant2,quant3,quant4=0,quant5=0,quant6=0;
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
	
	if (im->setup->quality_setting>LOW4)
	{
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
	}

	for (i=0;i<(4*IM_SIZE);i++)
	{
		if (!(i&511)) {quant=0;quant6=0;}
		
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
			
			if (a>14 && (a&7)==7 && nhw_process[i+1]>0 && nhw_process[i+1]<8) a-=2;  
			
			if (im->setup->quality_setting<=LOW4) 
			{
				if (a==15)
				{
					if (!quant)
					{
						a&=504;
						
						quant=1;
					}
					else if (quant==1)
					{
						quant=2;
					}
					else if (quant==2)
					{
						quant=3;
					}
					else if (quant==3)
					{
						quant=4;
					}
					else if (quant==4)
					{
						quant=5;
					}
					else if (quant==5)
					{
						quant=0;
					}
				}
				else if (a>22 && (a&7)==7)
				{
					if (!quant6)
					{
						a&=504;
						
						quant6=1;
					}
					else if (quant6==1)
					{
						quant6=2;
					}
					else if (quant6==2)
					{
						quant6=3;
					}
					else if (quant6==3)
					{
						quant6=0;
					}
				}
				else a&=504;
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
		else if (a==8 && nhw_process[i+1]==-7 && (i&511)<(2*IM_DIM-1)) nhw_process[i+1]=-8;
		else if (a>12 && (a&7)>=6)
		{
			if ((i&511)<(2*IM_DIM-1) && nhw_process[i+1]==7) nhw_process[i+1]=9;
		}
		
		if (a>=14 && nhw_process[i+1]>=14 && im->setup->quality_setting<=LOW4)
		{
			if (i>=(2*IM_SIZE) || (i&511)>=IM_DIM) 
			{
				quant2 = (a&510);
				quant3 = (nhw_process[i+1]&510);
				
				if(((quant2)&7)==6 && ((quant3)&7)==6 && ((a&1)==1 || (nhw_process[i+1]&1)==1))
				{
					if ((i&511)>0 && (i&511)<((2*IM_DIM)-2))
					{
						if (nhw_process[i-1]<-2 && nhw_process[i-1]>-8)
						{
							r1=1;
						}
						else if (nhw_process[i-1]<-7)
						{
							if (((-nhw_process[i-1])&7)<6) r1=0;
							else r1=1;
						}
						else r1=0;
					
						if (nhw_process[i+2]<-2 && nhw_process[i+2]>-8)
						{
							r2=1;
						}
						else if (nhw_process[i+2]<-7)
						{
							if (((-nhw_process[i+2])&7)<6) r2=0;
							else r2=1;
						}
						else r2=0;
					}
					else 
					{
						r1=0;r2=0;
					}
				
					if (!quant4)
					{
						if ((a&504)==(nhw_process[i+1]&504))
						{
							if (a>=nhw_process[i+1])
							{
								if (!r1) {a+=2;nhw_process[i+1]-=2;quant5=0;}
								else quant5=1;
							}
							else 
							{
								if (!r2) {nhw_process[i+1]+=2;quant5=0;}
								else quant5=1;
							}
						}
						else if (a<=nhw_process[i+1])
						{
							if (!r1) {a+=2;nhw_process[i+1]-=2;quant5=0;}
							else quant5=1;
						}
						else 
						{
							if (!r2) {nhw_process[i+1]+=2;quant5=0;}
							else quant5=1;
						}
						
						//if (!quant5) quant4=1;
						//else quant4=2;
						
						quant4=1;
					}
					else if (quant4==1)
					{
						quant4=2;
					}
					else if (quant4==2)
					{
						quant4=0;
					}
					/*else if (quant4==3)
					{
						quant4=0;
					}*/
				}
			}
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
	int i,j,a,r,scan;

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
    int i,j,scan,res,res2,res3,res4,count,e=0,f=0,a=0,sharpness=0,sharpn2=0,n1,t,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15,t16,t17,t18,t19,t20,t21,t22,t23,t24,t25,t26,t27,t28,t29,t30,t31,t32,t33;
    int nps,w1,w2,w3,w4,w5,w6,w7,w8;
	short *nhw_process, *nhw_kernel;
	char lower_quality_setting_on, *nhw_sharp_on;

	nhw_process=(short*)im->im_process;
	memcpy(im->im_process,im->im_jpeg,4*IM_SIZE*sizeof(short));
	
	nhw_kernel=(short*)malloc(4*IM_SIZE*sizeof(short));

	if (im->setup->quality_setting<=LOW6) lower_quality_setting_on=1;
	else lower_quality_setting_on=0;

	if (im->setup->quality_setting==LOW4) sharpness=59;
	else if (im->setup->quality_setting==LOW5) sharpness=54;
	else if (im->setup->quality_setting==LOW6) sharpness=49;
	else if (im->setup->quality_setting==LOW7) sharpness=44;
	else if (im->setup->quality_setting==LOW8) sharpness=41;
	else if (im->setup->quality_setting==LOW9) sharpness=35;
	else if (im->setup->quality_setting==LOW10) sharpness=17;
	else if (im->setup->quality_setting==LOW11) sharpness=1;
	else if (im->setup->quality_setting==LOW12) sharpness=0;
	else if (im->setup->quality_setting==LOW13) sharpness=0;
	else if (im->setup->quality_setting==LOW14) sharpness=0;
	else if (im->setup->quality_setting==LOW15 || im->setup->quality_setting==LOW16) sharpness=24;
	else if (im->setup->quality_setting==LOW17) sharpness=36;
	else if (im->setup->quality_setting==LOW18) sharpness=45;
	else if (im->setup->quality_setting==LOW19) sharpness=48;
	
	if (sharpness<10) sharpn2=10;else sharpn2=sharpness;
	
	if (im->setup->quality_setting>LOW11) n1=36;
	else if (im->setup->quality_setting==LOW11) n1=24;
	else if (im->setup->quality_setting==LOW12) n1=10;
	else if (im->setup->quality_setting==LOW13) n1=6;
	else if (im->setup->quality_setting==LOW14) n1=36;
	else if (im->setup->quality_setting<=LOW15 && im->setup->quality_setting>=LOW17) n1=36;
	else if (im->setup->quality_setting==LOW18) n1=56;
	else if (im->setup->quality_setting==LOW19) n1=60;
	

	for (i=(2*IM_DIM),res3=0,res4=0,a=0,t1=0,t2=0,t3=0,t4=0,t5=0,t6=0,t7=0;i<((4*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i+1,j=1;j<((2*IM_DIM)-1);j++,scan++)
		{ 
            nps = nhw_process[scan];
            
            w1 = nps-nhw_process[scan-1];
            w2 = nps-nhw_process[scan+1];
            w3 = nps-nhw_process[scan-(2*IM_DIM)];
            w4 = nps-nhw_process[scan+(2*IM_DIM)];
            w5 = nps-nhw_process[scan-(2*IM_DIM-1)];
            w6 = nps-nhw_process[scan-(2*IM_DIM+1)];
            w7 = nps-nhw_process[scan+(2*IM_DIM-1)];
            w8 = nps-nhw_process[scan+(2*IM_DIM+1)];
            
            res    =  w1 + w2 + w3 + w4 + w5 + w6 + w7 + w8;
            
            count = abs(w1) + abs(w2) + abs(w3) + abs(w4) + abs(w5) + abs(w6) + abs(w7) + abs(w8);
					
			if (res<0) 
			{
				res4 = (15*abs(res))+count+((res4+2)>>2);
				
				res2 = -(res4>>4);
                
				res4 &= 15;
				
				if (res2==-sharpn2 && im->setup->quality_setting<=LOW4)
				{
					if (t7<3)
					{
						res2 = -sharpn2-1;
						
						t7++;
					}
				}
				
				if (abs(res)<=sharpn2 && abs(res2)>sharpn2 && abs(res2)<=(sharpn2+20) && im->setup->quality_setting<=LOW4) 
				{
					if (j>1 && abs(nhw_kernel[scan-1])<=(sharpness>>1)) res3 = 0;
					
					if (!res3)
					{
						nhw_kernel[scan] = -20000; // - (sharpn2+21);
						
                        res3 = 1;
					}
					else 
					{
						nhw_kernel[scan] = res2;
						
						if (!t1)
						{
							res3 = 0;
							t1 = 1;
						}
						else 
						{
							if (res3==1) res3 = 2;
							else 
							{
								res3 = 0;
								if (t1==1) t1 = 2;
								else if (t1==2) t1 = 3;
								else t1 = 0;
							}
						}
					}
				}
				else nhw_kernel[scan] = res2;
			}
			else if (res>0)
			{
				res4 = (15*res)+count+((res4+2)>>2);
                
				res2 = res4>>4;
              
			    res4 &= 15;
				
				if (res<=sharpn2 && res2>sharpn2 && res2<=(sharpn2+20) && im->setup->quality_setting<=LOW4)
				{					
					if (j>1 && abs(nhw_kernel[scan-1])<=(sharpness>>1)) a = 0;
					else if (j>1 && (abs(nhw_kernel[scan-1])>10000 || nhw_kernel[scan-1]==(sharpn2+21))) 
					{
						if (!t4)
						{
							a = 0;
							if (!t2) t2 = 1;
							
							t4 = 1;
						}
						else t4 = 0;
					}
					else if (j>1 && nhw_kernel[scan-1]== -(sharpn2+21)) 
					{
						if (!t5)
						{
							t5 = 1;
						}
						else 
						{
							if (!t4)
							{
								a = 0;
								if (!t2) t2 = 1;
							
								t4 = 1;
							}
							else t4 = 0;
							
							if (t5==1) t5 = 2;
							else t5 = 0;
						}
					}
					else if (j>1 && nhw_kernel[scan-1]==(sharpn2+22)) 
					{
						nhw_kernel[scan-1] = 7000;
					}
					
					if (!a)
					{
						nhw_kernel[scan] = 20000; // sharpn2+21;
						
						a = 1;
					}
					else 
					{
						nhw_kernel[scan] = res2;
						
						if (!t2)
						{
							a = 0;
							t2 = 1;
						}
						else
						{
							if (a==1) a = 2;
							else 
							{
								a = 0;
								if (t2==1) t2 = 2;
								else if (t2==2) t2 = 3;
								else t2 = 0;
							}
						}
					}
				}
				else if (res2==(sharpn2+21) && im->setup->quality_setting<=LOW4) 
				{
					if (!t6) nhw_kernel[scan] = 7000;
					else nhw_kernel[scan] = res2;
					
					t6++;
				}
				else nhw_kernel[scan] = res2;
			}
			else 
            {
                nhw_kernel[scan]=0;
                
                res4 = 0;
            }
		}
	}
	
	a = 0;
	
	if (im->setup->quality_setting<=LOW4) nhw_sharp_on=(char*)calloc(4*IM_SIZE,sizeof(char));
						
	for (i=(2*IM_DIM),t1=0,t2=0,t3=0,t4=0,t5=0,t6=8,t7=0,t8=0,t9=0,t10=10,t11=15,t12=0,t13=0,t14=0,t15=0,t16=0,t17=0,t18=8,t19=0,t20=0,t21=0,t22=0,t23=0,t24=0,t25=0,t26=0,t27=0,t28=0,t29=0,t30=20,t31=0,t32=0,t33=0;i<((4*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i+1,j=1;j<((2*IM_DIM)-2);j++,scan++)
		{   
			res= nhw_kernel[scan];
			
			j++;scan++;
			
			count= nhw_kernel[scan];
			
			if (lower_quality_setting_on)
			{
				if (abs(res)>4 && abs(res)<n1)
				{
					scan--;
					
					if (abs(nhw_process[scan-(2*IM_DIM)]-nhw_process[scan-1])<4 && abs(nhw_process[scan-1]-nhw_process[scan+(2*IM_DIM)])<4 && abs(nhw_process[scan+(2*IM_DIM)]-nhw_process[scan+1])<4 && abs(nhw_process[scan+1]-nhw_process[scan-(2*IM_DIM)])<4)
					{
						im->im_jpeg[scan]=( (nhw_process[scan]<<2) +
											 nhw_process[scan-1]+nhw_process[scan+1]+
											 nhw_process[scan-(2*IM_DIM)]+nhw_process[scan+(2*IM_DIM)]+4 )>>3;

					}
					
					scan++;
				}
				
				if (abs(count)>4 && abs(count)<n1)
				{
					if (abs(nhw_process[scan-(2*IM_DIM)]-nhw_process[scan-1])<4 && abs(nhw_process[scan-1]-nhw_process[scan+(2*IM_DIM)])<4 && abs(nhw_process[scan+(2*IM_DIM)]-nhw_process[scan+1])<4 && abs(nhw_process[scan+1]-nhw_process[scan-(2*IM_DIM)])<4)
					{
						im->im_jpeg[scan]=( (nhw_process[scan]<<2) +
											 nhw_process[scan-1]+nhw_process[scan+1]+
											 nhw_process[scan-(2*IM_DIM)]+nhw_process[scan+(2*IM_DIM)]+4 )>>3;
	
					}
				}
			}
		

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
				if (!t1)
				{	
                    t2 = 0;
                    
					if (abs(res)>sharpness)
					{
						if (res>0) im->im_jpeg[scan-1]+=2;else im->im_jpeg[scan-1]-=2;
						
						if (abs(count)>sharpn2 || t8==1) 
						{
							nhw_kernel[scan-1] = 0;
							
							if ((t19<(4*IM_SIZE) || (t20>=3 && t20<(4*IM_SIZE))) && abs(res)>(sharpness+96) && t6>0 && i>(4*IM_DIM))
							{
                                if (t20>=3 && t19>=(8*IM_SIZE))
                                {
									t6 = 7000000;
										
									t20 = (8*IM_SIZE);
                                }
                                
								if (t19>0 && t19<(4*IM_SIZE))
								{
									if (t20>2 || (t20==2 && t6>3 && !t23) || (t20==2 && t6>14 && t23>0))
									{
										if (t23==1) t6 = 5000000;
										
										t23++;
                                    
										t21++;
									
										if (t21>=2)  t19 = (8*IM_SIZE);
									}
								}
								
								if (!t19) 
								{
									t6++;
									
									t20 = 1;
								}
								
								t19++;
							}
						}
                        
                        t2 = 1;
					}
					
					if (abs(count)>sharpness)
					{
                        if ((t2==1 || t12==1) && (!t14 || t14==4 || t14==5))
                        {
                            if (!t3 && t2==1)
                            {
								if (abs(res)>3000)
								{
									if (res>0) res = sharpn2 + 5;
									else res = -sharpn2 - 5;
								}
								
								if (abs(count)>3000)
								{
									if (count>0) count = sharpn2 + 22;
									else count = -sharpn2 - 22;
								}
								
								if (abs(res)<((abs(count))>>2))
								{
									if (res>0) im->im_jpeg[scan-1]--;else im->im_jpeg[scan-1]++;
						
									nhw_kernel[scan-1]=res;
						
									if (count>0) im->im_jpeg[scan]+=2;else im->im_jpeg[scan]-=2;
                                
									if (abs(res)>sharpn2) nhw_kernel[scan]=0;
								}
								else
								{
									if (count>0) im->im_jpeg[scan]++;else im->im_jpeg[scan]--;
                                }
								
                                t3 = 1;
                            }
                            else
                            {
                                if (count>0) im->im_jpeg[scan]+=2;else im->im_jpeg[scan]-=2;
                                
                                if (abs(res)>sharpn2) nhw_kernel[scan]=0;
                                
                                if (t3==1) t3 = 2;
                                else if (t3==2) t3 = 3;
                                else t3 = 0;
                            }
                        }
						else
						{
                           if (count>0) im->im_jpeg[scan]+=2;else im->im_jpeg[scan]-=2;
                           
                           if (abs(res)>sharpn2) nhw_kernel[scan]=0;
						}
						
						if (t14==2) 
						{
							t14 = 1;
							
							t26 = 3;
							
							if (t25>0) t25++;
						}
						
						if (t14==1)
						{
							if (t26<4)
							{
								//t14 = 0;
								
								t26++;
							}
							else
							{
								t14 = 2;
								
								t26 = 0;
							}
						}
					}
					
					if (abs(res)>sharpness || abs(count)>sharpness) t13 = 1;
					
					if (t14==1 || t14==2) t27++;
					else t27 = 0;
					
					if (t27>2) t14 = 1;
                    
                    if (t14==1)
                    {
						t14 = 4;
						
						if (!t25) 
						{						
							t15++;
							
							t25 = 1;
						}
						else
						{	
							t25++;
							
							if (t25>3) t25 = 0;
						}
                    }
					
					t1 = 1;
				}
				else
				{
					if (abs(res)>sharpness)
					{
						if (res>0) im->im_jpeg[scan-1]++;else im->im_jpeg[scan-1]--;
						
                        t1++;t4++;
					}
				
					if (abs(count)>sharpness)
					{
						if (count>0) im->im_jpeg[scan]++;else im->im_jpeg[scan]--;
						
                        t1++;t4++;
					}
					
					if (t4<10)
					{
						if (t4==t10 && t1==t11) t17 = 1;
						else t17 = 0;
					}
					else 
					{
						if (t4>10 || t1!=15)
						{
							if (!t18)
							{
								t17 = 1;
								
								t18 = 1;
							}
							else
							{
								t17 = 0; 
								
								t18++;
								
								if (t18>15) t18 = 0;
							}
						}
						else if (t4==t10 && t1==t11)  t17 = 1;
						else t17 = 0;
					}
					
					if (t6>6000000)
					{
						t6 = 0;
						
						t22 = 0;
					}
					else if (t6>4000000)
					{
						t6 = 0;
						
						if (t21==1) t22 = 1;
						else t22 = 0;
					}
					
					if (t17==1) 
					{
						if (!t6)
						{
							t6 = 1;
                            
                            t14 = 0;
							
							if (!t22) t7++;
							
							if (t22==1) t22 = 0;
						}
						else 
						{
							t6++;
							
							t1++;
                            
                            if (!t15)
                            {
                                t14 = 1;
                                
                                t15 = 1;
                            }
                            else
                            {
                                t14 = 0;
                                
                                t15++;
                                
                                if (t15>9) t15 = 0;
                            }
							
							if (t6>15 && t7<4) 
							{
								t6 = 0;
								
								if (t19>0) t20++;
							}
						}
						
						t4 = 0;
						
						t8 = 0;
                        
                        t5 = 0;
                        
                        t12 = 0;
						
						if (t7==3)
						{
							if (!t6)
							{
								t10 = 10;
							
								t11 = 15;
							}
							else
							{
								t10 = 8;
							
								t11 = 12;
							}
						}
						else if (t7==1)
						{
							if (t9<2)
							{
								t10 = 10;
							
								t11 = 15;
								
								t9++;
							}
							else
							{
								t10 = 8;
							
								t11 = 12;
								
								t9++;
								
								if (t9>=3) t9 = 0;
							}
						}
						else if (t7==2)
						{
							t10 = 8;
							
							t11 = 12;
						}
						else
						{
							if ((t6==10 || t6==11) && !t7)
							{
								t10 = 6;
								
								t11 = 9;
							}
							else if (t7>=4)
							{			
								if (!t16)
								{
									t10 = 10;
							
									t11 = 15;
									
									t16 = 1;
								}
								else if (t16==1)
								{
									t10 = 8;
							
									t11 = 12;
									
									t16 = 2;
								}
								else if (t16==2)
								{
									t10 = 10;
							
									t11 = 15;
									
									t16 = 3;
								}
								else if (t16==3)
								{
									t10 = 8;
							
									t11 = 12;
									
									t16 = 4;
								}
								else if (t16==4)
								{
									t10 = 10;
							
									t11 = 15;
									
									t16 = 5;
								}
								else if (t16==5)
								{
									t10 = 10;
							
									t11 = 15;
									
									t16 = 6;
								}
								else if (t16==6)
								{
									t10 = 8;
							
									t11 = 12;
									
									t16 = 7;
								}
								else if (t16==7)
								{
									t10 = 8;
							
									t11 = 12;
									
									t16 = 8;
								}
								else if (t16==8)
								{
									t10 = 8;
							
									t11 = 12;
									
									if (!t24)
									{
										 t16 = 1;
										 
										 t24 = 1;
									}
									else if (t24==1)
									{
										 t16 = 2;
										 
										 t24 = 2;
									}
									else if (t24==2)
									{
										 t16 = 1;
										 
										 t24 = 3;
									}
									else if (t24==3)
									{
										 t16 = 2;
										 
										 t24 = 4;
									}
									else if (t24==4)
									{
										 t16 = 1;
										 
										 t24 = 5;
									}
									else if (t24==5)
									{
										 t16 = 0;
										 
										 t24 = 6;
									}
									else if (t24==6)
									{
										 t16 = 3;
										 
										 t24 = 7;
									}
									else if (t24==7)
									{
										 t16 = 4;
										 
										 t24 = 8;
									}
									else if (t24==8)
									{
										 t16 = 1;
										 
										 t24 = 9;
									}
									else if (t24==9)
									{
										 t16 = 8;
										 
										 t24 = 10;
									}
									else if (t24==10)
									{
										 t16 = 1;
										 
										 t24 = 11;
									}
									else if (t24==11)
									{
										 t16 = 0;
										 
										 t24 = 12;
									}
									else if (t24==12)
									{
										 t16 = 1;
										 
										 t24 = 13;
									}
									else if (t24==13)
									{
										 t16 = 0;
										 
										 t24 = 14;
									}
									else if (t24==14)
									{
										 t16 = 1;
										 
										 t24 = 15;
									}
									else if (t24==15)
									{
										 t16 = 0;
										 
										 t24 = 12;
									}
								}
							}
							else
							{
								if (t10==8) t10 = 10;else t10 = 8;
						
								if (t11==12) t11 = 15;else t11 = 12;
							}
						}
					}
                    else if (t1>=15)
                    {
                        if (!t4) t8++;
                        else 
                        {
                            t8 = 0;
                            
                            t5 = 0;
                            
                            t12 = 0;
                        }
                        
                        t1++;
						
						if (t4<2 && t29>0 && t14==4)
						{
							if (!t31)
							{
								t14 = 3;
								
								t31++;
							}
                            else if (t31==1)
                            {
                                t14 = 3;
                                
                                t31++;
                            }
							else if (t31==2)
                            {
                                t14 = 0;
								
								t15 = 0;
                                
                                t31++;
                            }
						}
                    }
                    else 
					{
						t1++;
						
						if (t4<2 && t29>0 && (t14==4 || t14==5)) 
						{			
							if (t1==15 && !t28 && (t14==4 || (t14==5 && t32>2)))
							{
								if (t32==0 || t32==2 || t32==3) t14 = 5;
								
								t32++;
							}
							
							if (t28<4 && t1>7)
							{
								if (t14==5 && t30>92 && !t28) 
								{
									t30 = 53;
									
									t33++;
								}
								else t30++;
								
								if (!t28 && t30>100 && t33>0 && t14==4)
								{
									t14 = 3;
									
									t15 += 6;
									
									t28++;
								}
								else if (t28==1 && t30>220 && t14==4)
								{
									t15 = 0;
									
									t28++;
								}
								else if (t28==2 && t31==3)
								{
									t15 = 15;
									
									t33 = t30;
									
									t28++;
								}
								else if (t28==3 && t30>(t33+3) && t31==3)
								{									
									t15 = 0;
									
									t28++;
								}
							}
						}
					}
					
					if (t8>6 && !t4 && t1>1 && t1<15)
					{
                        t5++;
                        
                        if (t5<35)
                        {
                            t1 = 0;
                            
                            if (!t13)
                            {
                                t12 = 1;
                                
                                t13 = 1;
                            }
                            else
                            {
                                t12 = 0;
                                
                                t13++;
                                
                                if (t13>3) t13 = 0;
                            }
                            
                        }
                        else t12 = 0;
					}
					
					if (t1>15)
                    {
                        t1 = 0;
                        
                        t4 = 0;
						
						t29++;
                    }
				}
				
				if (abs(res)>sharpness && abs(res)<=(sharpness+20) && abs(count)>sharpness && abs(count)<=(sharpness+20) )
				{
					if (res>0 && count<0) 
					{
						im->im_jpeg[scan-1]++;im->im_jpeg[scan]--;
						nhw_sharp_on[scan-1]=2;nhw_sharp_on[scan]=3;
					}
					else if (res<0 && count>0) 
					{
						im->im_jpeg[scan-1]--;im->im_jpeg[scan]++;
						nhw_sharp_on[scan-1]=3;nhw_sharp_on[scan]=2;
					}
				}
			}

			if (im->setup->quality_setting>LOW6 || (im->setup->quality_setting<=LOW10 && im->setup->quality_setting>LOW13)) 
			{
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
	
	if (im->setup->quality_setting<=LOW4) 
	{
		for (i=(2*IM_DIM),t1=0,t2=0,t3=0,t4=0,t5=0,t6=0;i<((4*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
		{
			for (scan=i+1,j=1,e=0,t=0,f=0;j<((2*IM_DIM)-3);j++,scan++)
			{ 
				res= nhw_kernel[scan];
			
				j++;scan++;
			
				count= nhw_kernel[scan];
				
				if (abs(res)>6000)
				{
					if (res==20000) 
					{
						if (!t3) 
						{
							nhw_kernel[scan-1]=0;
							
							t3 = 1;
						}
						else
						{
							nhw_kernel[scan-1]= 5000; //sharpn2+21;
							
							if (t3==1) t3 = 2;
							else t3 = 0;
						}
					}
					else if (res== -20000)
					{
						if (!t4) 
						{
							nhw_kernel[scan-1]=0;
							
							t4 = 1;
						}
						else
						{
							nhw_kernel[scan-1]= -5000; //-(sharpn2+21);
							
							if (t4==1) t4 = 2;
							else t4 = 0;
						}
					}
					else if (res==7000) nhw_kernel[scan-1]=sharpn2+22;
					
					if(!t2)
					{
						if (count==20000) 
						{
							if (!t5) 
							{
								nhw_kernel[scan]=0;
							
								t5 = 1;
							}
							else
							{
								nhw_kernel[scan]= 5000; //sharpn2+21;
							
								if (t5==1) t5 = 2;
								else t5 = 0;
							}
						}
						else if (count== -20000)
						{
							if (!t6) 
							{
								nhw_kernel[scan]=0;
							
								t6 = 1;
							}
							else
							{
								nhw_kernel[scan]= -5000; //-(sharpn2+21);
							
								if (t6==1) t6 = 2;
								else t6 = 0;
							}
						}
						else if (count==7000) nhw_kernel[scan]=sharpn2+22;
						
						t2 = 1;
					}
					else t2 = 0;
					
					if (!t1)
					{
						t1 = 1;
						
						continue;
					}
					else t1 = 0;
				}
				else if (abs(count)>6000)
				{
					if (count==20000)
					{
						if (!t5) 
						{
							nhw_kernel[scan]=0;
							
							t5 = 1;
						}
						else
						{
							nhw_kernel[scan]= 5000; //sharpn2+21;
							
							if (t5==1) t5 = 2;
							else t5 = 0;
						}
					}
					else if (count== -20000) 
					{
						if (!t6) 
						{
							nhw_kernel[scan]=0;
							
							t6 = 1;
						}
						else
						{
							nhw_kernel[scan]= -5000; //-(sharpn2+21);
							
							if (t6==1) t6 = 2;
							else t6 = 0;
						}
					}
					else if (count==7000) nhw_kernel[scan]=sharpn2+22;
					
					continue;
				}
				
				if (abs(res)>(sharpness+20) && abs(count)>(sharpness>>1) && abs(count)<=sharpn2)
				{				
					if (res>0)
					{
						im->im_jpeg[scan-1]++;nhw_sharp_on[scan-1]=1;
						
						if (count>0) {im->im_jpeg[scan]+=2;nhw_sharp_on[scan]=1;}
						
						if (scan>=((4*IM_DIM)+2))
						{
							scan-=(2*IM_DIM);
						
							res2	   =   nhw_kernel[scan];
						
							if (res2>4) {im->im_jpeg[scan]++;nhw_sharp_on[scan]=1;}
						
							scan--;
						
							res3	   =   nhw_kernel[scan];
						
							if (res3>4) {im->im_jpeg[scan]++;nhw_sharp_on[scan]=1;}
								 
							if (res2<-24 && !t) {im->im_jpeg[scan+1]--;nhw_sharp_on[scan+1]=1;}
							if (res3<-24 && !t) {im->im_jpeg[scan]--;nhw_sharp_on[scan]=1;}
								
							scan++;

							scan+=(2*IM_DIM);
							
						}
						
						e=0;f=0;
					}
					else if (res<0)  
					{	
						im->im_jpeg[scan-1]--;nhw_sharp_on[scan-1]=1;
						
						if (count<0) {im->im_jpeg[scan]-=2;nhw_sharp_on[scan]=1;}
						
						if (scan>=((4*IM_DIM)+2))
						{
							scan-=(2*IM_DIM);
							
							res2	   =   nhw_kernel[scan];
							
							if (res2<-4) {im->im_jpeg[scan]--;nhw_sharp_on[scan]=1;}

							scan--;
							
							res3	   =   nhw_kernel[scan];
							
							if (res3<-4) {im->im_jpeg[scan]--;nhw_sharp_on[scan]=1;}
								
							if (res2>24 && !t) {im->im_jpeg[scan+1]++;nhw_sharp_on[scan+1]=1;}
							if (res3>24 && !t) {im->im_jpeg[scan]++;nhw_sharp_on[scan]=1;}
								
							scan++;

						    scan+=(2*IM_DIM);
						
						}
						
						e=0;f=0;
					}
					
					if (t==1)
					{
						j++;scan++;t=0;
					}
					else if (t==2)
					{
						j+=3;scan+=3;t=0;
					}
				}
				else if (abs(count)>(sharpness+20) && abs(res)>(sharpness>>1) && abs(res)<=sharpn2)
				{					
					if (count>0)
					{
						im->im_jpeg[scan]++;nhw_sharp_on[scan]=1;
						
						if (res>0) {im->im_jpeg[scan-1]+=2;nhw_sharp_on[scan-1]=1;}
						
						if (scan>=((4*IM_DIM)+2))
						{
						
							scan-=(2*IM_DIM+1);
							
							res2	   =   nhw_kernel[scan];
							
							if (res2>4) {im->im_jpeg[scan]++;nhw_sharp_on[scan]=1;}

							scan++;
							
							res3	   =   nhw_kernel[scan]; 
							
							if (res3>4) {im->im_jpeg[scan]++;nhw_sharp_on[scan]=1;}
								
							if (res2<-24 && !t) {im->im_jpeg[scan-1]--;nhw_sharp_on[scan-1]=1;}
							if (res3<-24 && !t) {im->im_jpeg[scan]--;nhw_sharp_on[scan]=1;}
								
							scan--;

							scan+=(2*IM_DIM+1);
						
						}
						
						e=0;f=0;
					}
					else if (count<0)  
					{
						im->im_jpeg[scan]--;nhw_sharp_on[scan]=1;
						
						if (res<0) {im->im_jpeg[scan-1]-=2;nhw_sharp_on[scan-1]=1;}
						
						if (scan>=((4*IM_DIM)+2))
						{
						
							scan-=(2*IM_DIM+1);
							
							res2	   =   nhw_kernel[scan];
							
							if (res2<-4) {im->im_jpeg[scan]--;nhw_sharp_on[scan]=1;}

							scan++;
							
							res3	   =   nhw_kernel[scan];
							
							if (res3<-4) {im->im_jpeg[scan]--;nhw_sharp_on[scan]=1;}
								
							if (res2>24 && !t) {im->im_jpeg[scan-1]++;nhw_sharp_on[scan-1]=1;}
							if (res3>24 && !t) {im->im_jpeg[scan]++;nhw_sharp_on[scan]=1;}
								
							scan--;

							scan+=(2*IM_DIM+1);
						
						}
						
						e=0;f=0;
					} 
					
					if (t==1)
					{
						j++;scan++;t=0;
					}
					else if (t==2)
					{
						j+=3;scan+=3;t=0;
					}
				}
				else
				{
					e++;
					if (!t) f++;
					
					if (e==2)
					{
						j-=3;scan-=3;
						
						e=0;t=1;
					}
					else if (t==1)
					{
						j++;scan++;t=0;e=0;
						
						if (f==4)
						{
							if(abs(nhw_kernel[scan-5])<=sharpn2 || abs(nhw_kernel[scan-2])<=sharpn2)
							{
								j-=5;scan-=5;t=2;
							}
							
							f=0;
						}
					}
					else if (t==2)
					{
						j+=3;scan+=3;t=0;e=0;f=0;
					}
				}
			}
		}
		
		for (i=(2*IM_DIM);i<((4*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
		{
			for (scan=i+1,j=1;j<((2*IM_DIM)-2);j++,scan++)
			{   
				res= nhw_kernel[scan];
			
				j++;scan++;
			
				count= nhw_kernel[scan];
				
				if (abs(res)>4000 || abs(count)>4000) continue;
				
				if (abs(res)>sharpness && abs(res)<=(sharpness+20) && abs(count)>sharpness && abs(count)<=(sharpness+20) )
				{
					if (nhw_sharp_on[scan-1]!=1 && nhw_sharp_on[scan]!=1)
					{
						if (res>0 && count>0) 
						{
							if (res>=count) 
							{
								if (nhw_sharp_on[scan-1]!=2) im->im_jpeg[scan-1]++;
								else if (nhw_sharp_on[scan]!=2) im->im_jpeg[scan]++;
							}
							else 
							{
								if (nhw_sharp_on[scan]!=2) im->im_jpeg[scan]++;
								else if (nhw_sharp_on[scan-1]!=2) im->im_jpeg[scan-1]++;
							}
						}
						else if (res<0 && count<0) 
						{
							if (res<=count) 
							{
								if (nhw_sharp_on[scan-1]!=3) im->im_jpeg[scan-1]--;
								else if (nhw_sharp_on[scan]!=3) im->im_jpeg[scan]--;
							}
							else 
							{
								if (nhw_sharp_on[scan]!=3) im->im_jpeg[scan]--;
								else if (nhw_sharp_on[scan-1]!=3) im->im_jpeg[scan-1]--;
							}
						}
						else if (j<((2*IM_DIM)-4) && abs(nhw_kernel[scan+1])>sharpness && abs(nhw_kernel[scan+1])<=(sharpness+20)) 
						{
							if ((count>0 && nhw_kernel[scan+1]>0) || (count<0 && nhw_kernel[scan+1]<0)) {j--;scan--;}
						}
					}
					else if (j<((2*IM_DIM)-4) && abs(nhw_kernel[scan+1])>sharpness && abs(nhw_kernel[scan+1])<=(sharpness+20)) 
					{
						if ((count>0 && nhw_kernel[scan+1]>0) || (count<0 && nhw_kernel[scan+1]<0)) {j--;scan--;}
					}
				}
				else if (abs(res)>(sharpness+56) && abs(count)>(sharpness+56))
				{
					if (!nhw_sharp_on[scan-1] && !nhw_sharp_on[scan]) 
					{
						if (res>0 && count<0) {im->im_jpeg[scan-1]++;im->im_jpeg[scan]--;}
						else if (res<0 && count>0) {im->im_jpeg[scan-1]--;im->im_jpeg[scan]++;}
						else if (abs(res)>(sharpness+96) && abs(count)>(sharpness+96))
						{
							if (res>0 && count>0)
							{
								if (res>count) im->im_jpeg[scan-1]++;else im->im_jpeg[scan]++;
							}
							else if (res<0 && count<0)
							{
								if (res<count) im->im_jpeg[scan-1]--;else im->im_jpeg[scan]--;
							}
						}
					}
				}
				else if (abs(res)>(sharpness+160) && abs(count)>sharpn2 && abs(count)<=(sharpn2+20))
				{
					if (!nhw_sharp_on[scan-1] && !nhw_sharp_on[scan]) 
					{
						if (res>0 && count>0) im->im_jpeg[scan]--;
						else if (res<0 && count<0) im->im_jpeg[scan]++;
						else if (j<((2*IM_DIM)-6) && abs(nhw_kernel[scan+1])>(sharpness+160) && abs(nhw_kernel[scan+2])<=sharpn2) 
						{
							j--;scan--;
						}
					}
					else if (j<((2*IM_DIM)-6) && abs(nhw_kernel[scan+1])>(sharpness+160) && abs(nhw_kernel[scan+2])>(sharpn2+20)) 
					{
						j--;scan--;
					}
				}
				else if (abs(count)>(sharpness+160) && abs(res)>sharpn2 && abs(res)<=(sharpn2+20))
				{
					if (!nhw_sharp_on[scan-1] && !nhw_sharp_on[scan]) 
					{
						if (res>0 && count>0) im->im_jpeg[scan-1]--;
						else if (res<0 && count<0) im->im_jpeg[scan-1]++;
						else if (j<((2*IM_DIM)-4) && abs(nhw_kernel[scan+1])>sharpn2 && abs(nhw_kernel[scan+1])<=(sharpn2+20)) 
						{
							j--;scan--;
						}
					}
					else
					{
						j--;scan--;
					}
				}
				else
				{
					j--;scan--;
				}
			}
		}
		
		free(nhw_sharp_on);
	}
	
	free(nhw_kernel);
}

void pre_processing_UV(image_buffer *im)
{
	int i,j,scan,res;
	short *nhw_process;

	nhw_process=(short*)im->im_process;
	memcpy(im->im_process,im->im_jpeg,IM_SIZE*sizeof(short));

	for (i=(IM_DIM);i<((IM_SIZE)-(IM_DIM));i+=(IM_DIM))
	{
		for (scan=i+1,j=1;j<((IM_DIM)-1);j++,scan++)
		{   
			res	   =   (nhw_process[scan]<<3) -
						nhw_process[scan-1]-nhw_process[scan+1]-
						nhw_process[scan-(IM_DIM)]-nhw_process[scan+(IM_DIM)]-
						nhw_process[scan-(IM_DIM+1)]-nhw_process[scan+(IM_DIM-1)]-
						nhw_process[scan-(IM_DIM-1)]-nhw_process[scan+(IM_DIM+1)];
				
			if (im->setup->quality_setting<LOW6) 
			{
				if (abs(res)>=14)
				{
					if (res>0) im->im_jpeg[scan]-=2;else im->im_jpeg[scan]+=2;
				}
				else if (abs(res)>5)
				{
					if (res>0) im->im_jpeg[scan]--;else im->im_jpeg[scan]++;
				}
			}
			else
			{
				if (res>5) im->im_jpeg[scan]--;
				else if (res<-5) im->im_jpeg[scan]++;
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
	int i,j,wavelet_order,a,e,t,quant=0,quant6=0;
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

	
	/*if (!part)
	{
		if (im->setup->quality_setting<LOW6)
		{
			for (i=IM_SIZE;i<(2*IM_SIZE);i+=(2*IM_DIM))
			{
				for (a=i,j=0;j<(IM_DIM);j++,a++)
				{
					if (abs(im->im_process[a])>=8 &&  abs(im->im_process[a])<9) 
					{	
						im->im_process[a]=0;
						//if (nhw_process[scan]>0) nhw_process[scan]=7;else nhw_process[scan]=-7;
					}
				}
			}
		}
	}*/

	
	if (im->setup->quality_setting>LOW4)
	{
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
	}

	for (i=0;i<(IM_SIZE);i+=(2*IM_DIM))
	{
		for (j=(IM_DIM>>1),quant=0,quant6=0;j<IM_DIM;j++) 
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

				if (im->setup->quality_setting<=LOW4) 
				{
					if (a==15)
					{
						if (!quant)
						{
							a&=65528;
						
							quant=1;
						}
						else if (quant==1)
						{
							quant=2;
						}
						else if (quant==2)
						{
							quant=3;
						}
						else if (quant==3)
						{
							quant=4;
						}
						else if (quant==4)
						{
							quant=5;
						}
						else if (quant==5)
						{
							quant=0;
						}
					}
					else if (a>22 && (a&7)==7)
					{	
						if (!quant6)
						{
							a&=65528;quant6=1;
						}
						else if (quant6==1)
						{
							quant6=2;
						}
						else if (quant6==2)
						{
							quant6=3;
						}
						else if (quant6==3)
						{
							quant6=0;
						}
					}				
					else a&=65528;
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

	for (i=(IM_SIZE);i<((4*IM_SIZE)>>1);i+=(2*IM_DIM))
	{
		for (j=0,quant=0,quant6=0;j<IM_DIM;j++) 
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

				if (im->setup->quality_setting<=LOW4) 
				{
					if (a==15)
					{
						if (!quant)
						{
							a&=65528;
						
							quant=1;
						}
						else if (quant==1)
						{
							quant=2;
						}
						else if (quant==2)
						{
							quant=3;
						}
						else if (quant==3)
						{
							quant=4;
						}
						else if (quant==4)
						{
							quant=5;
						}
						else if (quant==5)
						{
							quant=0;
						}
					}
					else if (a>22 && (a&7)==7)
					{	
						if (!quant6)
						{
							a&=65528;quant6=1;
						}
						else if (quant6==1)
						{
							quant6=2;
						}
						else if (quant6==2)
						{
							quant6=3;
						}
						else if (quant6==3)
						{
							quant6=0;
						}
					}				
					else a&=65528;
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
		if (im->setup->quality_setting<=LOW4) 
		{
			for (i=(2*IM_DIM);i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
			{
				for (e=i+1,j=1;j<IM_DIM-1;j++,e++) 
				{
					if (abs(im->im_jpeg[e])>=8)
					{
						if (abs(im->im_jpeg[e-(2*IM_DIM+1)])>=16) continue;
						if (abs(im->im_jpeg[e-(2*IM_DIM)])>=8) continue;
						if (abs(im->im_jpeg[e-(2*IM_DIM-1)])>=16) continue;
						if (abs(im->im_jpeg[e-1])>=8) continue;
						if (abs(im->im_jpeg[e+1])>=8) continue;
						if (abs(im->im_jpeg[e+(2*IM_DIM-1)])>=16) continue;
						if (abs(im->im_jpeg[e+(2*IM_DIM)])>=8) continue;
						if (abs(im->im_jpeg[e+(2*IM_DIM+1)])>=16) continue;

						if (i>=IM_SIZE || j>=(IM_DIM>>1))
						{
							if (im->im_jpeg[e]>0) im->im_jpeg[e]--;
							else im->im_jpeg[e]++;
						}
					}
				}
			}
		}
		else
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
}

void offsetUV_recons256(image_buffer *im, int m1, int comp)
{
	int i,j,wavelet_order,a;

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
