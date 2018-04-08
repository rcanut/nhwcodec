/***************************************************************************
****************************************************************************
*  NHW Image Codec														   *
*  file: compress_pixel.c											       *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  rmk: -mixed entropy coding                        					   * 
***************************************************************************/

/* Copyright (C) 2007-2015 NHW Project
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

int wavlts2packet(image_buffer *im,encode_state *enc)
{
	int i,j,k,e,a,b,c,select,tag,thresh,pos,pack,match,part,p1,p2;
	unsigned long weight[354],weight2[354],l1,l2,huffman_word;
	unsigned short rle_tree[580];
	unsigned char codebook[580],zone_entrance,color,pixel,*nhw_s1,*nhw_s2;
	unsigned char *nhw_comp;
	int rle_buf[256],rle_128[256];

	nhw_comp=(unsigned char*)im->im_nhw;

	enc->encode=(unsigned long*)calloc(80000,sizeof(long));
	enc->tree1=(unsigned char*)calloc(291*2,sizeof(char));
	enc->tree2=(unsigned char*)calloc(291*2,sizeof(char));

	part=0;p1=0;p2=(4*IM_SIZE);a=0;color=im->im_nhw[4*IM_SIZE];im->im_nhw[4*IM_SIZE]=3;

L1:	if (part==0) select=4; else select=3;
	thresh=354;
	memset(rle_buf,0,256*sizeof(int));
	memset(rle_128,0,256*sizeof(int));
	
	/////// FIRST STAGE ////////

	e=0;b=0;c=0;

	for (i=p1;i<p2-1;i++)   
	{
L_RUN1:	if (nhw_comp[i]==128)   
		{
			while (nhw_comp[i+1]==128)
			{
				e++;c=1;
				if (e>255) {e=254;rle_128[254]++;i--;e=0;c=0;goto L_RUN1;}
				else i++;
			}
		}

		if (c) 
		{
			rle_128[e+1]++;
		}
		else 
		{
			rle_buf[nhw_comp[i]]++;
		}
		e=0;c=0;
	}

L_RATIO:
	memset(weight,0,354*sizeof(int));
	memset(weight2,0,354*sizeof(int));

	for (i=0;i<109;i+=2)
	{
		if (rle_buf[i]>0) 
		{
			weight2[i] = rle_buf[i]; 
		}
	}

	if (rle_buf[112]>0) 
	{
		weight2[112] = rle_buf[112]; 
	}

	for (i=120;i<141;i++)
	{
		if (rle_buf[i]>0) 
		{
			weight2[i] = rle_buf[i]; 
		}
	}

	for (i=144;i<256;i+=4)
	{
		if (rle_buf[i]>0) 
		{
			weight2[i] = rle_buf[i]; 
		}
	}

	for (j=2;j<256;j++)
		{
			if (rle_128[j]>0) 
			{
				weight2[128] += j*rle_128[j]; 
			}
		}

	for (j=2;j<select;j++) rle_128[j]=0;

	for(j=select;j<256;j++)
	{
		if (rle_128[j]>0) weight2[128]-=j*rle_128[j];
		//else rle_buf[128][j]=0;
	}

	rle_buf[128]=weight2[128];

	k=0;
	for (j=select;j<256;j++)
	{
		if (rle_128[j]>0)
		{
			rle_tree[k]=(unsigned short)((j<<8)|128);
			weight[k]=rle_128[j];
			k++;
		}
	}

	for (i=0;i<109;i+=2)
	{
		if (rle_buf[i]>0)
		{
			rle_tree[k]=(unsigned short)((1<<8)|i);
			weight[k]=rle_buf[i];
			k++;
		}
	}

	if (rle_buf[112]>0)
	{
		rle_tree[k]=(unsigned short)((1<<8)|112);
		weight[k]=rle_buf[112];
		k++;
	}

	for (i=120;i<141;i++)
	{
		if (rle_buf[i]>0)
		{
			rle_tree[k]=(unsigned short)((1<<8)|i);
			weight[k]=rle_buf[i];
			k++;
		}
	}

	for (i=144;i<256;i+=4)
	{
		if (rle_buf[i]>0)
		{
			rle_tree[k]=(unsigned short)((1<<8)|i);
			weight[k]=rle_buf[i];
			k++;
		}
	}

 	if (k>thresh)   
	{
		select++;
		if (select>=100) exit(-1);
		goto L_RATIO;
	}

	for (i=0;(i<k);i++)
	{
		for (j=1;j<(k-i);j++)
		{
			if (weight[j]>weight[j-1])
			{
				l1=rle_tree[j-1];
				l2=weight[j-1];
				rle_tree[j-1]=rle_tree[j];
				weight[j-1]=weight[j];
				rle_tree[j]=l1;
				weight[j]=l2;
			}
		}
	}

#ifdef NHW_BOOKS
	printf("\nsize= %d\n",k);printf("\nCodeBooks\n");
	for(i=0;i<290;i++) printf("%d %d %d %d\n",i,(unsigned char)rle_tree[i],rle_tree[i]>>8,weight[i]);
#endif

	//////// LAST STAGE ///////

	for (i=0;i<k;i++)
	{
		//l1=(unsigned char)(rle_tree[i]);
		//l2=(unsigned char)(rle_tree[i]>>8);
		if ((rle_tree[i]>>8)==1) rle_buf[(rle_tree[i]&0xFF)]=i;
		else rle_128[(rle_tree[i]>>8)]=i;
	}

	if ((rle_tree[0]==((1<<8)|128))) b=1; else b=0;
	if ((part==0) && (b==0) && (k>290)) exit(-1);
	if ((part==1) && (select!=4) && (k>290)) exit(-1);

	if (select==4 && b==1) zone_entrance=1;else zone_entrance=0;
	if (part==1) zone_entrance=0;

	nhw_s1=(unsigned char*)malloc(enc->nhw_select1*sizeof(char));
	nhw_s2=(unsigned char*)malloc(enc->nhw_select2*sizeof(char));

	e=1;match=0;pack=0;tag=0;c=0,j=0;
	for (i=p1;i<p2-1;i++)   
	{
		pixel=nhw_comp[i];

		if (pixel>=153)
		{
			if (pixel==153) 
			{
				nhw_s1[c++]=0;continue;
			}
			else if (pixel==155) 
			{
				nhw_s1[c++]=1;continue;
			}
			else if (pixel==157) 
			{
				nhw_s2[j++]=0;continue;
			}
			else if (pixel==159) 
			{
				nhw_s2[j++]=1;continue;
			}
		}

		if (pixel!=128 && pixel<136 && pixel>120)
		{
			pos=(unsigned short)rle_buf[pixel];
			if (pixel>131) i+=4;
			goto L_ZE;
		}

		if (pixel==128)   
		{
			while (nhw_comp[i+1]==128)
			{
				e++;
				if (e>255) {e=254;i--;goto L_JUMP;}
				else i++;
			}

			if (e>1) 
			{ 
				if (e<select) {i-=(e-1);tag=e;e=1;}
			}
		}

L_JUMP:	if (e==1) pos=(unsigned short)rle_buf[pixel];
		else pos=(unsigned short)rle_128[e];

L_ZE:		if (pos>=110 && pos<174 && zone_entrance)
		{
			pack += 15;
			if (pack<=32) enc->encode[a] |= ((1<<6)|(pos-110))<<(32-pack); 
			else
			{
				match=pack-32;
				enc->encode[a] |= ((1<<6)|(pos-110))>>match;
				a++;
				enc->encode[a] |= (((1<<6)|(pos-110))&((1<<match)-1))<<(32-match);
				pack=match;
			}
		}
		else
		{
			if (pos>=174) if (zone_entrance) pos -=64;
			pack += len[pos];

			if (pack<=32) enc->encode[a] |= huffman_tree[pos]<<(32-pack); 
			else
			{
				match=pack-32;
				enc->encode[a] |= huffman_tree[pos]>>match;
				a++;
				enc->encode[a] |= (huffman_tree[pos]&((1<<match)-1))<<(32-match);
				pack=match;
			}
		}

L_TAG:	e=1;
		// check the tag, maybe can be elsewhere faster...
		if (tag>0) {tag--;if (tag>0){i++;goto L_JUMP;}} 
	}

	
	if (part==0)
	{
	enc->size_data1=a+1;
	if (select>4 || b==0) im->setup->wavelet_type=4;
	else im->setup->wavelet_type=0;

	b=(c>>3)+1;e=0;
	enc->nhw_select_word1=(unsigned char*)malloc((b<<3)*sizeof(char));

	for (i=0;i<(b<<3);i+=8)
	{
		enc->nhw_select_word1[e++]=((nhw_s1[i]&1)<<7)|((nhw_s1[i+1]&1)<<6)|
								   ((nhw_s1[i+2]&1)<<5)|((nhw_s1[i+3]&1)<<4)|
								   ((nhw_s1[i+4]&1)<<3)|((nhw_s1[i+5]&1)<<2)|
								   ((nhw_s1[i+6]&1)<<1)|((nhw_s1[i+7]&1));	
	}

	free(nhw_s1);

	enc->nhw_select1=e;

	b=(j>>3)+1;e=0;
	enc->nhw_select_word2=(unsigned char*)malloc((b<<3)*sizeof(char));

	for (i=0;i<(b<<3);i+=8)
	{
		enc->nhw_select_word2[e++]=((nhw_s2[i]&1)<<7)|((nhw_s2[i+1]&1)<<6)|
								   ((nhw_s2[i+2]&1)<<5)|((nhw_s2[i+3]&1)<<4)|
								   ((nhw_s2[i+4]&1)<<3)|((nhw_s2[i+5]&1)<<2)|
								   ((nhw_s2[i+6]&1)<<1)|((nhw_s2[i+7]&1));	
	}

	free(nhw_s2);

	enc->nhw_select2=e;

	e=0;b=0;c=0;
	for (i=0;i<k;i++)
	{
		if ((rle_tree[i]>>8)==0x1) enc->tree1[e++]= (unsigned char)((rle_tree[i]&0xff));
		else {enc->tree1[e++]=3;enc->tree1[e++]=(rle_tree[i]>>8);}
	}

	for (i=0;i<e;i+=2) codebook[b++]=enc->tree1[i];
	for (i=1;i<e;i+=2) codebook[b++]=enc->tree1[i];

	for (i=0,b=0;i<e;i++)
	{
L_COD:	if (codebook[i]==3)
		{
			c++;i++;goto L_COD;
		}
		else
		{
			if (c>0) {enc->tree1[b++]=3;enc->tree1[b++]=c;c=0;i--;}
			else {enc->tree1[b++]=codebook[i];}
		}
	}

	//for (i=0;i<b;i++) printf("%d %d\n",i,enc->tree1[i]);

	enc->size_tree1=b;
	}
	else
	{
	enc->size_data2=a+1;

	e=0;b=0;c=0;

	for (i=0;i<k;i++)
	{
		if ((rle_tree[i]>>8)==0x1) enc->tree2[e++]= (unsigned char)((rle_tree[i]&0xff)|1);
		else {enc->tree2[e++]=(rle_tree[i]&0xff);enc->tree2[e++]=(rle_tree[i]>>8);}
	}

	enc->tree_end=e;

	for (i=0;i<e;i+=2) codebook[b++]=enc->tree2[i];
	for (i=1;i<e;i+=2) codebook[b++]=enc->tree2[i];

//	for (i=0;i<e;i++) printf("%d %d\n",i,codebook[i]);

	for (i=0,b=0;i<e;i++)
	{
L_COD2:	if (codebook[i]==128)
		{
			c++;i++;goto L_COD2;
		}
		else
		{
			if (c>0) {enc->tree2[b++]=128;enc->tree2[b++]=c;c=0;i--;}
			else {enc->tree2[b++]=codebook[i];}
		}
	}

	//for (i=0;i<b;i++) printf("%d %d\n",i,enc->tree2[i]);

	enc->size_tree2=b;
	}

	if (part==0) {part=1;a++;p1=4*IM_SIZE;p2=6*IM_SIZE;im->im_nhw[4*IM_SIZE]=color;
											im->im_nhw[6*IM_SIZE-1]=im->im_nhw[6*IM_SIZE-2];goto L1;}
}

void Y_highres_compression(image_buffer *im,encode_state *enc)
{
	int i,j,e,Y,a,res,scan,count,mem;
	unsigned char *highres,*ch_comp,*comp_tmp;

	highres=(unsigned char*)enc->tree1;

	enc->highres_word=(unsigned char*)malloc((IM_SIZE>>2)*sizeof(char));

	//for (i=0;i<300;i++) printf("%d %d\n",i,highres[i]);

	for (i=1,e=0,Y=0,a=0;i<(IM_SIZE>>2);i++)
	{
L11:	if (highres[i]==highres[i-1])
		{
			e++;
			if (e<16) 
			{
				if (e==8) a++;
				i++;goto L11;
			}
			else if (e==16) {Y++;goto L15;}
		}
L15:	e=0;
	}

	a+=Y;

	enc->highres_mem_len=0;
	enc->highres_comp=(unsigned char*)calloc((96*IM_DIM),sizeof(char));
	enc->highres_mem=(unsigned short*)malloc((IM_SIZE>>2)*sizeof(short));
	ch_comp=(unsigned char*)enc->highres_comp;

	ch_comp[0]=highres[0];

	if (Y>299) {im->setup->RES_LOW=2;goto L4;}
	else if (a>199) {im->setup->RES_LOW=1;goto L2;} 
	else im->setup->RES_LOW=0;

	for (i=1,j=1,a=0,res=0,mem=0;i<(IM_SIZE>>2);i++)
	{
		scan=highres[i]-highres[i-1];
		count=highres[i+1]-highres[i];

		if (scan==0 && count==0)
		{
			if (highres[i+a+2]==highres[i+a+1]) a++;

			i+=(a+2);
			ch_comp[j]=(a<<3);
			
			if (highres[i]-highres[i-1]==2) 
			{
				if (highres[i+1]-highres[i]==-2) 
				{
					ch_comp[j]+=2;i++;
				}
				else if (highres[i+1]-highres[i]==0) 
				{
					ch_comp[j]+=3;i++;
				}
				else ch_comp[j]+=1;
			}
			else if (highres[i]-highres[i-1]==-2)
			{
				if (highres[i+1]-highres[i]==2) 
				{
					ch_comp[j]+=4;i++;
				}
				else if (highres[i+1]-highres[i]==0) 
				{
					ch_comp[j]+=5;i++;
				}
				else ch_comp[j]+=6;
				
			}
			else if (highres[i]-highres[i-1]==4) ch_comp[j]+=7;
			else  i--;

			a=0;
			j++;
		}
		else if (abs(scan)<=6 && abs(count)<=8)
		{
			scan+=6;count+=8;
			if (scan==12 || count==16) 
			{
				if (abs(highres[i+2]-highres[i+1])<=32 && i<16382) 
				{
					e=highres[i+2]-highres[i+1]+32;scan+=26;count+=8;goto COMP3;
				}
				else 
				{
					if (im->setup->quality_setting>LOW5)
					{
						ch_comp[j++]=128;
						ch_comp[j++]=128+(highres[i]>>1);
						ch_comp[j++]=128+(highres[i+1]>>1);
						enc->highres_word[mem++]=enc->ch_res[i];
						enc->highres_mem[enc->highres_mem_len++]=i;
						i++;
					}
					else 
					{
						ch_comp[j++]=128;
						ch_comp[j++]=128+(highres[i]>>1);
					}
				}
			}
			else 
			{
				if (scan<8) ch_comp[j++]=32+ (scan<<2) + (count>>1);
				else
				{
					if (scan==8)
					{
						ch_comp[j++]= 16 + (count>>1);
					}
					else
					{
						ch_comp[j++]= 24 + (count>>1);
					}
				}

				i++;
			}
		}
		else if (abs(scan)<=32 && abs(count)<=16 && abs(highres[i+2]-highres[i+1])<=32 && i<16382)
		{
			scan+=32;count+=16;e=highres[i+2]-highres[i+1]+32;
COMP3:		if (scan==64 || count==32 || e==64) 
			{
				if (im->setup->quality_setting>LOW5)
				{
					ch_comp[j++]=128;
					ch_comp[j++]=128+(highres[i]>>1);
					ch_comp[j++]=128+(highres[i+1]>>1);
					enc->highres_word[mem++]=enc->ch_res[i];
					enc->highres_mem[enc->highres_mem_len++]=i;
					i++;
				}
				else 
				{
					ch_comp[j++]=128;
					ch_comp[j++]=128+(highres[i]>>1);
				}
			}
			else 
			{
				count>>=1;
				ch_comp[j++]=64;
				ch_comp[j++]=64 +(scan) + (count>>3);
				ch_comp[j++]=((count&7)<<5) + (e>>1);
	
				i+=2;
			}
		}
		else
		{
			if (im->setup->quality_setting>LOW5)
			{
				ch_comp[j++]=128;
				ch_comp[j++]=128+(highres[i]>>1);
				ch_comp[j++]=128+(highres[i+1]>>1);
				enc->highres_word[mem++]=enc->ch_res[i];
				enc->highres_mem[enc->highres_mem_len++]=i;
				i++;
			}
			else 
			{
				ch_comp[j++]=128;
				ch_comp[j++]=128+(highres[i]>>1);
			}
		}
	}

	goto L3;

L2:	for (i=1,j=1,a=0,mem=0;i<(IM_SIZE>>2);i++)
	{
		scan=highres[i]-highres[i-1];
		count=highres[i+1]-highres[i];

		if (scan==0 && count==0)
		{
RES_COMPR4:	if (highres[i+a+2]==highres[i+a+1])
			{
				a++;
				if (a<7) goto RES_COMPR4;
				if (a==7) goto END_RES4;
			}
END_RES4:	
			i+=(a+2);
			ch_comp[j]=(a<<2);
			
			if (highres[i]-highres[i-1]==2) 
			{
				ch_comp[j]+=1;
			}
			else if (highres[i]-highres[i-1]==-2)
			{
				ch_comp[j]+=2;	
			}
			else if (highres[i]-highres[i-1]==0) ch_comp[j]+=3;
			else i--; 

			a=0;
			j++;
		}
		else if (abs(scan)<=4 && abs(count)<=8)
		{
			scan+=4;count+=8;
			if (scan==8 || count==16) 
			{
				if (abs(highres[i+2]-highres[i+1])<=32 && i<16382) 
				{
					e=highres[i+2]-highres[i+1]+32;scan+=28;count+=8;goto COMP4;
				}
				else
				{
					if (im->setup->quality_setting>LOW5)
					{
						ch_comp[j++]=128;
						ch_comp[j++]=128+(highres[i]>>1);
						ch_comp[j++]=128+(highres[i+1]>>1);
						enc->highres_word[mem++]=enc->ch_res[i];
						enc->highres_mem[enc->highres_mem_len++]=i;
						i++;
					}
					else 
					{
						ch_comp[j++]=128;
						ch_comp[j++]=128+(highres[i]>>1);
					}
				}
			}
			else 
			{
				ch_comp[j++]=32+ (scan<<2) + (count>>1);
				//if (highres[i+2]==highres[i+1] && i<12286) { ch_comp[j-1]+=1;i+=2;}
				//else i++;
				i++;
			}
		}
		else if (abs(scan)<=32 && abs(count)<=16 && abs(highres[i+2]-highres[i+1])<=32 && i<16382)
		{
			scan+=32;count+=16;e=highres[i+2]-highres[i+1]+32;
COMP4:		if (scan==64 || count==32 || e==64) 
			{
				if (im->setup->quality_setting>LOW5)
				{
					ch_comp[j++]=128;
					ch_comp[j++]=128+(highres[i]>>1);
					ch_comp[j++]=128+(highres[i+1]>>1);
					enc->highres_word[mem++]=enc->ch_res[i];
					enc->highres_mem[enc->highres_mem_len++]=i;
					i++;
				}
				else 
				{
					ch_comp[j++]=128;
					ch_comp[j++]=128+(highres[i]>>1);
				}
			}
			else 
			{
				count>>=1;
				ch_comp[j++]=64;
				ch_comp[j++]=64 +(scan) + (count>>3);
				ch_comp[j++]=((count&7)<<5) + (e>>1);
	
				i+=2;
			}
		}
		else
		{
			if (im->setup->quality_setting>LOW5)
			{
				ch_comp[j++]=128;
				ch_comp[j++]=128+(highres[i]>>1);
				ch_comp[j++]=128+(highres[i+1]>>1);
				enc->highres_word[mem++]=enc->ch_res[i];
				enc->highres_mem[enc->highres_mem_len++]=i;
				i++;
			}
			else 
			{
				ch_comp[j++]=128;
				ch_comp[j++]=128+(highres[i]>>1);
			}
		}
	}

	goto L3;

L4:	for (i=1,j=1,a=0,mem=0;i<(IM_SIZE>>2);i++)
	{
		scan=highres[i]-highres[i-1];
		count=highres[i+1]-highres[i];

		if (scan==0 && count==0)
		{
RES_COMPR5:	if (highres[i+a+2]==highres[i+a+1])
			{
				a++;
				if (a<63) goto RES_COMPR5;
				if (a==63) goto END_RES5;
			}
END_RES5:	
			i+=(a+1);
			ch_comp[j++]=a;
			a=0;
		}
		else if (abs(scan)<=32 && abs(count)<=16 && abs(highres[i+2]-highres[i+1])<=32 && i<16382)
		{
			scan+=32;count+=16;e=highres[i+2]-highres[i+1]+32;
		if (scan==64 || count==32 || e==64) 
			{
				if (im->setup->quality_setting>LOW5)
				{
					ch_comp[j++]=128;
					ch_comp[j++]=128+(highres[i]>>1);
					ch_comp[j++]=128+(highres[i+1]>>1);
					enc->highres_word[mem++]=enc->ch_res[i];
					enc->highres_mem[enc->highres_mem_len++]=i;
					i++;
				}
				else 
				{
					ch_comp[j++]=128;
					ch_comp[j++]=128+(highres[i]>>1);
				}
			}
			else 
			{
				count>>=1;
				ch_comp[j++]=64;
				ch_comp[j++]=64 +(scan) + (count>>3);
				ch_comp[j++]=((count&7)<<5) + (e>>1);
	
				i+=2;
			}
		}
		else
		{
			if (im->setup->quality_setting>LOW5)
			{
				ch_comp[j++]=128;
				ch_comp[j++]=128+(highres[i]>>1);
				ch_comp[j++]=128+(highres[i+1]>>1);
				enc->highres_word[mem++]=enc->ch_res[i];
				enc->highres_mem[enc->highres_mem_len++]=i;
				i++;
			}
			else 
			{
				ch_comp[j++]=128;
				ch_comp[j++]=128+(highres[i]>>1);
			}
		}
	}

L3: 
	comp_tmp=(unsigned char*)malloc(j*sizeof(char));

	memcpy(comp_tmp,ch_comp,j*sizeof(char));

	for (i=1,e=1,a=0;i<j-1;i++)
	{
		if (comp_tmp[i]==64)
		{
			ch_comp[e++]=comp_tmp[i+1];
			ch_comp[e++]=comp_tmp[i+2];
			i+=2;
		}
		else if (comp_tmp[i]==128)
		{
			if (im->setup->quality_setting>LOW5)
			{
				i++;
				ch_comp[e++]=comp_tmp[i+1];
				//if (comp_tmp[i+1]<128) ch_comp[e]+=128;
				//e++;

				i++;
			}
			else
			{
				i++;
				ch_comp[e++]=comp_tmp[i];
			}
		}
		else ch_comp[e++]=comp_tmp[i];
	}

	if (i<j) ch_comp[e++]=comp_tmp[j-1];

	free(comp_tmp);

	enc->highres_comp_len=mem;

	enc->Y_res_comp=e;

}

void highres_compression(image_buffer *im,encode_state *enc)
{
	int i,j,e,Y,a,res,scan,count;
	unsigned char *highres,*ch_comp;

	highres=(unsigned char*)enc->tree1;
	ch_comp=(unsigned char*)enc->highres_comp;

	for (i=(IM_SIZE>>2);i<(IM_SIZE>>2)+(IM_SIZE>>3);i++) highres[i]&=252;
	im->setup->RES_HIGH=im->setup->RES_LOW;

	j=enc->Y_res_comp;

	ch_comp[j++]=highres[(IM_SIZE>>2)];

	for (i=(IM_SIZE>>2)+1,a=0,e=0,res=0;i<(IM_SIZE>>2)+(IM_SIZE>>3);i++)
	{
		scan=highres[i]-highres[i-1];
		count=highres[i+1]-highres[i];

		if (scan==0 && count==0)
		{
RES_COMPR3:	if (highres[i+a+2]==highres[i+a+1])
			{
				a++;
				if (a<7) goto RES_COMPR3;
				//if (a==7) goto END_RES3;
				if (a==7 || res==1) 
				{
					res=1;
					if (a<14) goto RES_COMPR3; 
					else goto END_RES3;
				}
			}
END_RES3:	
			i+=(a+1);

			if (res==1)
			{
				ch_comp[j]=64+ (7<<3) + a-7;
			}
			else
			{
				i++;
				ch_comp[j]=64+ (a<<3);
			
				if (highres[i]-highres[i-1]==4) 
				{
					if (highres[i+1]-highres[i]==-4) 
					{
						if (highres[i+2]-highres[i+1]==0) {ch_comp[j]+=3;i+=2;}
						else  {ch_comp[j]+=2;i++;}
					}
					else ch_comp[j]+=1;
				}
				else if (highres[i]-highres[i-1]==-4)
				{
					if (highres[i+1]-highres[i]==4) 
					{
						if (highres[i+2]-highres[i+1]==0) {ch_comp[j]+=4;i+=2;}
						else  {ch_comp[j]+=5;i++;}
					}
					else ch_comp[j]+=6;
				
				}
				else if (highres[i]-highres[i-1]==8) ch_comp[j]+=7;
				else i--;
			}

			a=0;res=0;
			j++;
		}
		else if (abs(scan)<=4 && abs(count)<=4)
		{
			if (!scan && count==4) res=0;
			else if (!scan && count==-4) res=1;
			else if (scan==4 && !count) res=2;
			else if (scan==-4 && !count) res=3;
			else if (scan==4 && count==4) res=4;
			else if (scan==4 && count==-4) res=5;
			else if (scan==-4 && count==4) res=6;
			else if (scan==-4 && count==-4) res=7;

			if (!(highres[i+2]-highres[i+1])) 
			{
				ch_comp[j++]=128 + 64 + (res<<2);i+=2;
			}
			else if ((highres[i+2]-highres[i+1])==4) 
			{
				ch_comp[j++]=128 + 64 + (res<<2) + 1;i+=2;
			}
			else if ((highres[i+2]-highres[i+1])==-4) 
			{
				ch_comp[j++]=128 + 64 + (res<<2) + 2;i+=2;
			}
			else if ((highres[i+2]-highres[i+1])==8) 
			{
				ch_comp[j++]=128 + 64 + (res<<2) + 3;i+=2;
			}
			else 
			{
				scan+=16;count+=16;
				ch_comp[j++]= (scan<<1) + (count>>2);
				i++;
			}

			res=0;
		}
		else if (abs(scan)<=16 && abs(count)<=16)
		{
			scan+=16;count+=16;//e=(highres[i+1]>>2)&1;
			if (scan==32 || count==32) 
			{
				ch_comp[j++]=(1<<7)+(highres[i]>>2);
				//if (highres[i+1]==128) {ch_comp[j-1]+=(1<<6);i++;}
				//else {ch_comp[j++]=(1<<7)+(highres[i+1]>>2);i++;}
			}
			/*else if (((highres[i+2]-highres[i+1])==0 && !e)|| ((highres[i+2]-highres[i+1])==4 && e==1))
			{
				ch_comp[j++]=128 + 64 + (scan<<1) + (count>>2);i+=2;
			}*/
			else 
			{
				ch_comp[j++]= (scan<<1) + (count>>2);
				//if (highres[i+2]==highres[i+1] && i<12286) { ch_comp[j-1]+=1;i+=2;}
				//else i++;
				i++;
			}
		}
		else
		{
			ch_comp[j++]=(1<<7)+(highres[i]>>2);
			//if (highres[i+1]==128) {ch_comp[j-1]+=(1<<6);i++;}
			//else {ch_comp[j++]=(1<<7)+(highres[i+1]>>2);i++;}

		}
	}

	free(enc->tree1);

	enc->ch_res=(unsigned char*)malloc(j*sizeof(char));
	enc->end_ch_res=j;
	memcpy(enc->ch_res,ch_comp,enc->end_ch_res*sizeof(char));
	free(enc->highres_comp);
}


