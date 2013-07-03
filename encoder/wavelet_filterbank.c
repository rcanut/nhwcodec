/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: wavelet_filterbank.c  										       *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -analysis and synthesis filterbanks							   *
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

void wavelet_analysis(image_buffer *im,int norder,int last_stage,int Y)
{
	int i,j,a;
	short *data,*res,*res2;

	for (i=0;i<(norder*IM_DIM);i+=(2*IM_DIM))
	{
		for (a=i,j=0;j<(norder>>1);j++,a++) im->im_process[a]=0; 
	}

	if (Y)
	{

	data=im->im_jpeg;
	res=im->im_process;
	res2=im->im_process + norder/2;

	if (!last_stage)
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}
	/*else if (last_stage>1)
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}*/
	else
	{

		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*(2*IM_DIM)+j]=im->im_process[i+j*(2*IM_DIM)];
	}*/
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=(2*IM_DIM))
	{
		a=i;
		for (j=0;j<norder;j++,a+=(2*IM_DIM)) res[j]=data[a];
	}

	if (im->setup->quality_setting>HIGH1 && !last_stage)
	{
		im->im_quality_setting=(short*)malloc(2*IM_SIZE*sizeof(short));

		for (i=0;i<(2*IM_SIZE);i++) im->im_quality_setting[i]=im->im_jpeg[i];
	}

	data=im->im_jpeg;
	res=im->im_process;
	res2=im->im_process + norder/2;

	if (!im->setup->RES_HIGH)
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53VI(data,norder,0,res);downfilter53VI(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}
	/*else if (last_stage>1)
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}*/
	else
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53II(data,norder,0,res);downfilter53II(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}

	data=im->im_jpeg + norder*((2*IM_DIM)>>1);
	res=im->im_process + norder*((2*IM_DIM)>>1);
	res2=im->im_process + norder*((2*IM_DIM)>>1) + norder/2;

	if (!last_stage)
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}
	/*else if (last_stage>1)
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}*/
	else 
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{// image transposition
		/*for (i=0;i<norder/2;i++)
		{
			for (j=0;j<norder/2;j++) im->im_jpeg[i*(2*IM_DIM)+j]=im->im_process[i+j*(2*IM_DIM)];
		}*/
		res=im->im_jpeg;data=im->im_process;
		for (i=0;i<(norder>>1);i++,res+=(2*IM_DIM))
		{
			a=i;
			for (j=0;j<(norder>>1);j++,a+=(2*IM_DIM)) res[j]=data[a];
		}
	}
	}
	else
	{
	data=im->im_jpeg;
	res=im->im_process;
	res2=im->im_process + norder/2;

	if (!last_stage)
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}
	/*else if (last_stage>1)
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}*/
	else
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*(IM_DIM)+j]=im->im_process[i+j*(IM_DIM)];
	}*/
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=(IM_DIM))
	{
		a=i;
		for (j=0;j<norder;j++,a+=(IM_DIM)) res[j]=data[a];
	}

	data=im->im_jpeg;
	res=im->im_process;
	res2=im->im_process + norder/2;

	if (!im->setup->RES_HIGH)
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53VI(data,norder,0,res);downfilter53VI(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}
	/*else if (last_stage>1)
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}*/
	else
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53II(data,norder,0,res);downfilter53II(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}

	data=im->im_jpeg + norder*(IM_DIM>>1);
	res=im->im_process + norder*(IM_DIM>>1);
	res2=im->im_process + norder*(IM_DIM>>1) + norder/2;

	if (!last_stage)
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}
	/*else if (last_stage>1)
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}*/
	else 
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{// image transposition
		/*for (i=0;i<norder/2;i++)
		{
			for (j=0;j<norder/2;j++) im->im_jpeg[i*(IM_DIM)+j]=im->im_process[i+j*(IM_DIM)];
		}*/
		res=im->im_jpeg;data=im->im_process;
		for (i=0;i<(norder>>1);i++,res+=(IM_DIM))
		{
			a=i;
			for (j=0;j<(norder>>1);j++,a+=(IM_DIM)) res[j]=data[a];
		}
	}

	}
}


void wavelet_synthesis(image_buffer *im,int norder,int last_stage,int Y)
{
	short *data,*res,*data2;
	int i,j,IM_SYNTH=IM_DIM,a;

	if (Y)
	{
	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}
	else
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}

	data=im->im_jpeg + norder*((2*IM_SYNTH)>>1);
	res=im->im_process + norder*((2*IM_SYNTH)>>1);
	data2=im->im_jpeg + norder*((2*IM_SYNTH)>>1) + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}
	else
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*(2*IM_SYNTH)+j]=im->im_process[i+j*(2*IM_SYNTH)];
	}*/
	//faster version
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=(2*IM_DIM))
	{
		a=i;
		for (j=0;j<norder;j++,a+=(2*IM_DIM)) res[j]=data[a];
	}

	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}
	else
	{
		for (i=0;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53VI(data2,norder/2,res);
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{//image transposition
		/*for (i=0;i<norder;i++)
		{
			for (j=0;j<norder;j++) im->im_jpeg[i*(2*IM_SYNTH)+j]=im->im_process[i+j*(2*IM_SYNTH)];
		}*/
		//faster version
		res=im->im_jpeg;data=im->im_process;
		for (i=0;i<norder;i++,res+=(2*IM_DIM))
		{
			a=i;
			for (j=0;j<norder;j++,a+=(2*IM_DIM)) res[j]=data[a];
		}
	}
	}
	else 
	{
	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			data +=(IM_SYNTH);res +=(IM_SYNTH);data2 +=(IM_SYNTH);
		}
	}
	else
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}

	data=im->im_jpeg + norder*(IM_SYNTH>>1);
	res=im->im_process + norder*(IM_SYNTH>>1);
	data2=im->im_jpeg + norder*(IM_SYNTH>>1) + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}
	else
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*(IM_SYNTH)+j]=im->im_process[i+j*(IM_SYNTH)];
	}*/
	//faster version
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=(IM_DIM))
	{
		a=i;
		for (j=0;j<norder;j++,a+=(IM_DIM)) res[j]=data[a];
	}

	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}
	else
	{
		for (i=0;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53VI(data2,norder/2,res);
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{//image transposition
		/*for (i=0;i<norder;i++)
		{
			for (j=0;j<norder;j++) im->im_jpeg[i*(IM_SYNTH)+j]=im->im_process[i+j*(IM_SYNTH)];
		}*/
		//faster version
		res=im->im_jpeg;data=im->im_process;
		for (i=0;i<norder;i++,res+=(IM_DIM))
		{
			a=i;
			for (j=0;j<norder;j++,a+=(IM_DIM)) res[j]=data[a];
		}
	}
	}
}

void wavelet_synthesis_high_quality_settings(image_buffer *im,encode_state *enc)
{
	int i,j,e,res,Y,scan,count;
	short *wavelet_half_synthesis,*data,*res_w,*data2;
	unsigned char *nhw_res6I_word,*ch_comp,*highres,*scan_run;

	wavelet_half_synthesis=(short*)malloc(2*IM_SIZE*sizeof(short));

	res_w=(short*)wavelet_half_synthesis;
	data=(short*)im->im_wavelet_first_order;
	data2=(short*)im->im_wavelet_band;
	

	for (i=0;i<IM_DIM;i++)
	{
		upfilter53I(data,IM_DIM,res_w);upfilter53III(data2,IM_DIM,res_w);	
		data +=(IM_DIM);res_w +=(2*IM_DIM);data2 +=(IM_DIM);
	}

	//for (i=3000;i<3300;i++) printf("%d %d %d\n",i,im->im_quality_setting[i],wavelet_half_synthesis[i]);

	for (i=0,count=0,scan=0;i<2*IM_SIZE;i++) 
	{
		if (abs(im->im_quality_setting[i]-wavelet_half_synthesis[i])>36)
		{
			if ((im->im_quality_setting[i]-wavelet_half_synthesis[i])>0) 
			{
				wavelet_half_synthesis[i]=30000;count++;
			}
			else 
			{
				wavelet_half_synthesis[i]=31000;count++;
			}
		}
	}

	free(im->im_quality_setting);
	free(im->im_wavelet_first_order);
	free(im->im_wavelet_band);

	highres=(unsigned char*)malloc((count+(2*IM_DIM))*sizeof(char));
	nhw_res6I_word=(unsigned char*)malloc(count*sizeof(char));

	enc->nhw_char_res1=(unsigned short*)malloc(256*sizeof(short));

	for (i=0,count=0,e=0,res=0;i<(2*IM_SIZE);i+=(2*IM_DIM)) 
	{
		for (scan=i,j=0;j<(2*IM_DIM);j++,scan++)
		{
			if (j==(IM_DIM-2) || j==((2*IM_DIM)-2))
			{
				highres[count++]=IM_DIM-2;

				if (j==(IM_DIM-2))
				{
					if (wavelet_half_synthesis[scan]==30000)
					{
						enc->nhw_char_res1[res++]=(i>>1);
					}
					else if (wavelet_half_synthesis[scan]==31000)
					{
						enc->nhw_char_res1[res++]=(i>>1)+1;
					}
					
					if (wavelet_half_synthesis[scan+1]==30000)
					{
						enc->nhw_char_res1[res++]=(i>>1)+2;
					}
					else if (wavelet_half_synthesis[scan+1]==31000)
					{
						enc->nhw_char_res1[res++]=(i>>1)+3;
					}
				}

				j++;scan++;
			}
			else if (wavelet_half_synthesis[scan]==30000)
			{
				highres[count++]=(j&255);nhw_res6I_word[e++]=0;
			}
			else if (wavelet_half_synthesis[scan]==31000)
			{
				highres[count++]=(j&255);nhw_res6I_word[e++]=1;
			}
		}
	}

	free(wavelet_half_synthesis);

	enc->nhw_char_res1_len=res;

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

	enc->nhw_res6_len=res;
	enc->nhw_res6_word_len=e;
	enc->nhw_res6=(unsigned char*)malloc((enc->nhw_res6_len)*sizeof(char));

	for (i=0;i<enc->nhw_res6_len;i++) enc->nhw_res6[i]=highres[i];

	scan_run=(unsigned char*)malloc((enc->nhw_res6_len+8)*sizeof(char));

	for (i=0;i<enc->nhw_res6_len;i++) scan_run[i]=enc->nhw_res6[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->nhw_res6_len-1;i++)
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

	for (i=0,scan=0;i<enc->nhw_res6_len;i++) 
	{
		if (enc->nhw_res6[i]!=(IM_DIM-2)) scan_run[scan++]=enc->nhw_res6[i];
	}

	for (i=scan;i<scan+8;i++) scan_run[i]=0;

	enc->nhw_res6_bit_len=((scan>>3)+1);

	enc->nhw_res6_bit=(unsigned char*)malloc(enc->nhw_res6_bit_len*sizeof(char));

	Y=scan>>3;

	for (i=0,scan=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res6_bit[scan++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}

	enc->nhw_res6_len=count;

	Y=enc->nhw_res6_word_len>>3;
	free(scan_run);
	scan_run=(unsigned char*)nhw_res6I_word;
	enc->nhw_res6_word=(unsigned char*)malloc((enc->nhw_res6_bit_len<<1)*sizeof(char));

	for (i=0,scan=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res6_word[scan++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}

	enc->nhw_res6_word_len=scan;

	for (i=0;i<count;i++) enc->nhw_res6[i]=highres[i];

	
	free(nhw_res6I_word);

}