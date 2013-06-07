/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: filters.c  										               *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -wavelet filters set										       *
*          -code a fast DWT 2D											   *
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
#include "wavelets.h"

__inline void downfilter53(short *_X,int N,int decalage,short *_RES)
{
	int r,e=0,m=0,a,w_end;
	short *start_line=_X, *end_line=_X+N-2;

	w_end=(N>>1)-1;

	if (decalage)
	{
		for (;;_X+=4)
		{
			a=_X[0]+_X[2];
			if (a&1) m++;
			r=_X[1]-(a>>1);
			if (r>0) _RES[e++]=(r+1)>>1;
			else _RES[e++]=(r>>1);

			if (e==w_end) goto W_SL;

			a=_X[2]+_X[4];
			if ((a&1) && m) a++;
			r=_X[3]-(a>>1);
			if (r>0) _RES[e++]=(r+1)>>1;
			else _RES[e++]=(r>>1);

			m=0;
		}

W_SL:	_X=start_line;
		_RES[e++]=(short)(((_X[N-1]-_X[N-2])+1)>>1);
	}
	else
	{
		//_RES[e++]=(short)(W531*_X[0]-W535*(_X[1])-W532*(_X[2])+0.5f);
		r=(_X[0]*6+_X[1]*4-_X[2]*2);
		if (r>=0) _RES[e++]=(r+8)>>4;
		else _RES[e++]=-((-r+8)>>4);

		for (;_X<end_line-4;_X+=2)
		{
			//_RES[e++]=(short)(W531*_X[2]+W532*(_X[1]+_X[3])+W533*(_X[0]+_X[4])+0.5f);
			r=_X[2]*6+(_X[1]+_X[3])*2-(_X[0]+_X[4]);

			if (r>=0) _RES[e++]=(r+8)>>4;
			//else _RES[e++]=r>>4;
			else _RES[e++]=-((-r+8)>>4);
		}

		_X=start_line;

		r=6*_X[N-4]+2*(_X[N-5]+_X[N-3])-(_X[N-6]+_X[N-2]);
		if (r>=0) _RES[e++]=(r+8)>>4; 
		else _RES[e++]=-((-r+8)>>4);

		r=6*_X[N-2]+2*(_X[N-3]+_X[N-1])-(_X[N-4]+_X[N-2]);
		if (r>=0) _RES[e++]=(r+8)>>4; 
		else _RES[e++]=-((-r+8)>>4);
	}

}

__inline void downfilter53II(short *_X,int N,int decalage,short *_RES)
{
	int r,e=0,m=0,a,w_end;
	short *start_line=_X,*end_line=_X+N-2;

	w_end=(N>>1)-1;

	if (decalage)
	{
		for (;;_X+=4)
		{
			a=_X[0]+_X[2];
			if (a&1) m++;
			r=_X[1]-(a>>1);
			if (r>=0) _RES[e++]=(r+4)>>3;
			//else _RES[e++]=(r>>3)+1;
			else _RES[e++]=-((-r+4)>>3);

			if (e==w_end) goto W_SLII;

			a=_X[2]+_X[4];
			if ((a&1) && m) a++;
			r=_X[3]-(a>>1);
			if (r>=0) _RES[e++]=(r+4)>>3;
			//else _RES[e++]=(r>>3)+1;
			else _RES[e++]=-((-r+4)>>3);

			m=0;
		}

W_SLII:	_X=start_line;
		_RES[e++]=(short)((_X[N-1]-_X[N-2])>>3);
	}
	else
	{
		//_RES[e++]=(short)(W531*_X[0]-W535*(_X[1])-W532*(_X[2])+0.375f);
		r=(_X[0]*6+_X[1]*4-_X[2]*2);

		_RES[e]=r;if (_RES[e]>=0) _RES[e]=(_RES[e]+32)>>6;else _RES[e]=-((-_RES[e]+32)>>6);e++;
		if (r>=0) {r&=63;if (r<32) _RES[e]=(r>>2); else _RES[e]=-((64-r)>>2);}
		else{r=(-r)&63;if (r<32) _RES[e]=-(r>>2); else _RES[e]=((64-r)>>2);}


		for (;_X<end_line-4;_X+=2)
		{
			//_RES[e++]=(short)(W531*_X[2]+W532*(_X[1]+_X[3])+W533*(_X[0]+_X[4])+0.375f);
			r=_X[2]*6+(_X[1]+_X[3])*2-(_X[0]+_X[4]);

			_RES[e]+=r;

			if (_RES[e]>=0) 
			{
				_RES[e]=(_RES[e]+32)>>6;e++;
			}
			else
			{
				_RES[e]=-((-_RES[e]+32)>>6);e++;
			}
			

			if (r>=0)
			{
				r&=63;
				if (r<32) _RES[e]=(r>>2); else _RES[e]=-((64-r)>>2);
			}
			else
			{
				r=(-r)&63;
				if (r<32) _RES[e]=-(r>>2); else _RES[e]=((64-r)>>2);
			}
		
		}

		_X=start_line;

		r=6*_X[N-4]+2*(_X[N-5]+_X[N-3])-(_X[N-6]+_X[N-2]);

		_RES[e]+=r;if (_RES[e]>=0) _RES[e]=(_RES[e]+32)>>6;else _RES[e]=-((-_RES[e]+32)>>6);e++;
		if (r>=0) {r&=63;if (r<32) _RES[e]=(r>>2); else _RES[e]=-((64-r)>>2);}
		else{r=(-r)&63;if (r<32) _RES[e]=-(r>>2); else _RES[e]=((64-r)>>2);}

		r=6*_X[N-2]+2*(_X[N-3]+_X[N-1])-(_X[N-4]+_X[N-2]);
		_RES[e]+=r;if (_RES[e]>=0) _RES[e]=(_RES[e]+32)>>6;else _RES[e]=-((-_RES[e]+32)>>6);
	}

}

__inline void downfilter53VI(short *_X,int N,int decalage,short *_RES)
{
	int r,e=0,m=0,a,w_end;
	short *start_line=_X,*end_line=_X+N-2;

	w_end=(N>>1)-1;

	if (decalage)
	{
		for (;;_X+=4)
		{
			a=_X[0]+_X[2];
			if (a&1) m++;
			r=_X[1]-(a>>1);
			if (r>=0) _RES[e++]=(r+4)>>3;
			//else _RES[e++]=(r>>3);
			else _RES[e++]=-((-r+4)>>3);

			if (e==w_end) goto W_SLII;

			a=_X[2]+_X[4];
			if ((a&1) && m) a++;
			r=_X[3]-(a>>1);
			if (r>=0) _RES[e++]=(r+4)>>3;
			//else _RES[e++]=(r>>3);
			else _RES[e++]=-((-r+4)>>3);

			m=0;
		}

W_SLII:	_X=start_line;
		_RES[e++]=(short)((_X[N-1]-_X[N-2])>>3);
	}
	else
	{
		//_RES[e++]=(short)(W531*_X[0]-W535*(_X[1])-W532*(_X[2])+0.375f);
		r=(_X[0]*6+_X[1]*4-_X[2]*2);

		_RES[e]=r;if (_RES[e]>=0) _RES[e]=(_RES[e]+32)>>6;else _RES[e]=-((-_RES[e]+32)>>6);e++;
		if (r>=0) {r&=63;if (r<32) _RES[e]=(r>>2); else _RES[e]=-((64-r)>>2);}
		else{r=(-r)&63;if (r<32) _RES[e]=-(r>>2); else _RES[e]=((64-r)>>2);}


		for (;_X<end_line-4;_X+=2)
		{
			//_RES[e++]=(short)(W531*_X[2]+W532*(_X[1]+_X[3])+W533*(_X[0]+_X[4])+0.375f);
			r=_X[2]*6+(_X[1]+_X[3])*2-(_X[0]+_X[4]);

			_RES[e]+=r;

			if (_RES[e]>=0) 
			{
				_RES[e]=(_RES[e]+32)>>6;e++;
			}
			else
			{
				_RES[e]=-((-_RES[e]+32)>>6);e++;
			}
			

			if (r>=0)
			{
				r&=63;
				if (r<32) _RES[e]=(r>>2); else _RES[e]=-((64-r)>>2);
			}
			else
			{
				r=(-r)&63;
				if (r<32) _RES[e]=-(r>>2); else _RES[e]=((64-r)>>2);
			}
		}

		_X=start_line;

		r=6*_X[N-4]+2*(_X[N-5]+_X[N-3])-(_X[N-6]+_X[N-2]);

		_RES[e]+=r;if (_RES[e]>=0) _RES[e]=(_RES[e]+32)>>6;else _RES[e]=-((-_RES[e]+32)>>6);e++;
		if (r>=0) {r&=63;if (r<32) _RES[e]=(r>>2); else _RES[e]=-((64-r)>>2);}
		else{r=(-r)&63;if (r<32) _RES[e]=-(r>>2); else _RES[e]=((64-r)>>2);}

		r=6*_X[N-2]+2*(_X[N-3]+_X[N-1])-(_X[N-4]+_X[N-2]);
		_RES[e]+=r;if (_RES[e]>=0) _RES[e]=(_RES[e]+32)>>6;else _RES[e]=-((-_RES[e]+32)>>6);
	}

}

__inline void downfilter53II1(short *_X,int N,int decalage,short *_RES)
{
	int r,e=0,m=0,a,w_end;
	short *start_line=_X,*end_line=_X+N-2;

	w_end=(N>>1)-1;

	if (decalage)
	{
		for (;;_X+=4)
		{
			a=_X[0]+_X[2];
			if (a&1) m++;
			r=_X[1]-(a>>1);
			if (r>=0) _RES[e++]=(r+4)>>3;
			//else _RES[e++]=(r>>3)+1;
			else _RES[e++]=-((-r+4)>>3)+1;

			if (e==w_end) goto W_SLII;

			a=_X[2]+_X[4];
			if ((a&1) && m) a++;
			r=_X[3]-(a>>1);
			if (r>=0) _RES[e++]=(r+4)>>3;
			//else _RES[e++]=(r>>3)+1;
			else _RES[e++]=-((-r+4)>>3)+1;

			m=0;
		}

W_SLII:	_X=start_line;
		_RES[e++]=(short)((_X[N-1]-_X[N-2])>>3);
	}
	else
	{
		//_RES[e++]=(short)(W531*_X[0]-W535*(_X[1])-W532*(_X[2])+0.375f);
		r=(_X[0]*6+_X[1]*4-_X[2]*2);
		if (r>=0) _RES[e++]=(r+32)>>6;else _RES[e++]=r>>6;

		for (;_X<end_line-4;_X+=2)
		{
			//_RES[e++]=(short)(W531*_X[2]+W532*(_X[1]+_X[3])+W533*(_X[0]+_X[4])+0.375f);
			r=_X[2]*6+(_X[1]+_X[3])*2-(_X[0]+_X[4]);
			if (r>0) _RES[e++]=(r+32)>>6;else _RES[e++]=r>>6;
		}

		_X=start_line;

		r=6*_X[N-4]+2*(_X[N-5]+_X[N-3])-(_X[N-6]+_X[N-2]);
		if (r>=0) _RES[e++]=(r+32)>>6;else _RES[e++]=r>>6;

		r=6*_X[N-2]+2*(_X[N-3]+_X[N-1])-(_X[N-4]+_X[N-2]);
		if (r>=0) _RES[e++]=(r+32)>>6;else _RES[e++]=r>>6;
	}

}

__inline void downfilter53IV(short *_X,int N,int decalage,short *_RES)
{
	int r,e=0,m=0,a,w_end;
	short *start_line=_X,*end_line=_X+N-2;

	w_end=(N>>1)-1;

	if (decalage)
	{
		for (;;_X+=4)
		{
			_RES[e++]=(_X[1]<<1)-(_X[0]+_X[2]);

			if (e==w_end) goto W_SL3;

			_RES[e++]=(_X[3]<<1)-(_X[2]+_X[4]);
		}

W_SL3:	_X=start_line;
		_RES[e++]=(short)((_X[N-1]-_X[N-2])<<1);
	}
	else
	{
		m=0;
		//_RES[e++]=(short)(W531*_X[0]-W535*(_X[1])-W532*(_X[2])+0.375f);
		_RES[e++]=(_X[0]*6+_X[1]*4-_X[2]*2);

		for (;_X<end_line-4;_X+=2)
		{
			//_RES[e++]=(short)(W531*_X[2]+W532*(_X[1]+_X[3])+W533*(_X[0]+_X[4])+0.375f);
			_RES[e++]=_X[2]*6+(_X[1]+_X[3])*2-(_X[0]+_X[4]);
		}

		_X=start_line;

		_RES[e++]=6*_X[N-4]+2*(_X[N-5]+_X[N-3])-(_X[N-6]+_X[N-2]);

		_RES[e++]=6*_X[N-2]+2*(_X[N-3]+_X[N-1])-(_X[N-4]+_X[N-2]);
	}

}

void downfilter97(short *_X,int N,int decalage,short *_RES)
{
	short __FIR[9];
	int k,e=0;

	if (decalage)
	{
		__FIR[0]=_X[N-2];__FIR[1]=_X[N-1];__FIR[2]=_X[0];
		__FIR[3]=_X[1];__FIR[4]=_X[2];__FIR[5]=_X[3];__FIR[6]=_X[4];

		for (k=5;k<N-1;k+=2)
		{
			_RES[e++]=floor(W976*__FIR[3]+W977*(__FIR[2]+__FIR[4])+W978*(__FIR[1]+__FIR[5])+W979*(__FIR[0]+__FIR[6])+0.5f);
			__FIR[0]=__FIR[2];__FIR[1]=__FIR[3];__FIR[2]=__FIR[4];
			__FIR[3]=__FIR[5];__FIR[4]=__FIR[6];__FIR[5]=_X[k];__FIR[6]=_X[k+1];
		}

		_RES[e++]=floor(W976*__FIR[3]+W977*(__FIR[2]+__FIR[4])+W978*(__FIR[1]+__FIR[5])+W979*(__FIR[0]+__FIR[6])+0.5f);
		__FIR[0]=__FIR[2];__FIR[1]=__FIR[3];__FIR[2]=__FIR[4];
		__FIR[3]=__FIR[5];__FIR[4]=__FIR[6];__FIR[5]=_X[N-1];__FIR[6]=_X[0];

		_RES[e++]=floor(W976*__FIR[3]+W977*(__FIR[2]+__FIR[4])+W978*(__FIR[1]+__FIR[5])+W979*(__FIR[0]+__FIR[6])+0.5f);
		__FIR[0]=__FIR[2];__FIR[1]=__FIR[3];__FIR[2]=__FIR[4];
		__FIR[3]=__FIR[5];__FIR[4]=__FIR[6];__FIR[5]=_X[1];__FIR[6]=_X[2];

		_RES[e++]=floor(W976*__FIR[3]+W977*(__FIR[2]+__FIR[4])+W978*(__FIR[1]+__FIR[5])+W979*(__FIR[0]+__FIR[6])+0.5f);
	}
	else
	{
	
		__FIR[0]=_X[N-4];__FIR[1]=_X[N-3];__FIR[2]=_X[N-2];__FIR[3]=_X[N-1];
		__FIR[4]=_X[0];__FIR[5]=_X[1];__FIR[6]=_X[2];__FIR[7]=_X[3];__FIR[8]=_X[4];

		for (k=5;k<N-1;k+=2)
		{
			_RES[e++]=floor(W971*__FIR[4]+W972*(__FIR[3]+__FIR[5])+W973*(__FIR[2]+__FIR[6])+W974*(__FIR[1]+__FIR[7])
								+W975*(__FIR[0]+__FIR[8])+0.5f);	
			__FIR[0]=__FIR[2];__FIR[1]=__FIR[3];__FIR[2]=__FIR[4];__FIR[3]=__FIR[5];
			__FIR[4]=__FIR[6];__FIR[5]=__FIR[7];__FIR[6]=__FIR[8];__FIR[7]=_X[k];__FIR[8]=_X[k+1];
		}

		_RES[e++]=floor(W971*__FIR[4]+W972*(__FIR[3]+__FIR[5])+W973*(__FIR[2]+__FIR[6])+W974*(__FIR[1]+__FIR[7])
								+W975*(__FIR[0]+__FIR[8])+0.5f);
		__FIR[0]=__FIR[2];__FIR[1]=__FIR[3];__FIR[2]=__FIR[4];__FIR[3]=__FIR[5];
		__FIR[4]=__FIR[6];__FIR[5]=__FIR[7];__FIR[6]=__FIR[8];__FIR[7]=_X[N-1];__FIR[8]=_X[0];

		_RES[e++]=floor(W971*__FIR[4]+W972*(__FIR[3]+__FIR[5])+W973*(__FIR[2]+__FIR[6])+W974*(__FIR[1]+__FIR[7])
								+W975*(__FIR[0]+__FIR[8])+0.5f);	
		__FIR[0]=__FIR[2];__FIR[1]=__FIR[3];__FIR[2]=__FIR[4];__FIR[3]=__FIR[5];
		__FIR[4]=__FIR[6];__FIR[5]=__FIR[7];__FIR[6]=__FIR[8];__FIR[7]=_X[1];__FIR[8]=_X[2];

		_RES[e++]=floor(W971*__FIR[4]+W972*(__FIR[3]+__FIR[5])+W973*(__FIR[2]+__FIR[6])+W974*(__FIR[1]+__FIR[7])
								+W975*(__FIR[0]+__FIR[8])+0.5f);	
	}
}

__inline void upfilter53(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E1_=E+M-2,r=0,m=0,a;

	for (;;_X+=2,_RES+=4)
	{
		_RES[0]=_X[0];
		a=_X[1]+_X[0];
		if (a&1) 
		{
			if (a>0) _RES[1]=(a+1)>>1;
			else _RES[1]=a>>1;
			
			m++;
		}
		else _RES[1]=a>>1;

		if (_X==_E1_)  goto W_SE;

		_RES[2]=_X[1];
		a=_X[2]+_X[1];
		if (a&1) 
		{
			if (a>0) _RES[3]=(a+1)>>1;
			else _RES[3]=a>>1;
			
			m++;
		}
		else 
		{
			_RES[3]=a>>1;m=0;
		}

		_X+=2;_RES+=4;

		_RES[0]=_X[0];
		a=_X[1]+_X[0];
		if (a&1) 
		{
			if (a>0) _RES[1]=(a+1)>>1;
			else _RES[1]=a>>1;
			
			m++;
		}
		else _RES[1]=a>>1;

		if (m==3) 
		{
			if (_RES[-1]>=0) _RES[-1]--;else _RES[-1]++;
			m=0;
		}

		if (_X==_E1_)  goto W_SE;

		_RES[2]=_X[1];
		a=_X[2]+_X[1];
		if (a&1) 
		{
			if (a>0) _RES[3]=(a+1)>>1;
			else _RES[3]=a>>1;
			
			m++;
		}
		else _RES[3]=a>>1;

		if (m==3) 
		{
			if (_RES[1]>=0) _RES[1]--;else _RES[1]++;
		}

		m=0;

	}

W_SE: _X=E;_RES[2]=_X[M-1];_RES[3]=_X[M-1];
}

__inline void upfilter53I(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E1_=E+M-1;

	for (;_X<_E1_;_X++,_RES+=2)
	{ 
		_RES[0]=_X[0]<<3;
		_RES[1]=(_X[1]+_X[0])<<2;
	}

	_X=E;_RES[0]=_X[M-1]<<3;_RES[1]=_X[M-1]<<3;
}

__inline void upfilter53III(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E2_=E+M-2,r;

	_RES[0]-=(_X[0]<<2);_RES[1]+=(5*_X[0]-_X[1]);_RES+=2;

	for (;_X<_E2_;_X++,_RES+=2)
	{
		_RES[0]-=(_X[1]+_X[0])<<1;
		_RES[1]+=6*_X[1]-_X[2]-_X[0];
	}

	_X=E;_RES[0]-=(_X[M-1]+_X[M-2])<<1;_RES[1]+=5*_X[M-1]-_X[M-2];
}

__inline void upfilter53VI(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E2_=E+M-2,r;

	_RES[0]-=(_X[0]<<2);_RES[1]+=5*_X[0]-_X[1];
	if (_RES[0]>0) _RES[0]+=32;_RES[0]>>=6;
	if (_RES[1]>0) _RES[1]+=32;_RES[1]>>=6;
	_RES+=2;

	for (;_X<_E2_;_X++,_RES+=2)
	{
		_RES[0]-=(_X[1]+_X[0])<<1;
		if (_RES[0]>0) _RES[0]+=32; 
		_RES[0]>>=6;

		_RES[1]+=6*_X[1]-_X[2]-_X[0];
		if (_RES[1]>0) _RES[1]+=32; 
		_RES[1]>>=6;

	}

	_X=E;_RES[0]-=(_X[M-1]+_X[M-2])<<1;if (_RES[0]>0) _RES[0]+=32;_RES[0]>>=6;
	_RES[1]+=5*_X[M-1]-_X[M-2];if (_RES[1]>0) _RES[1]+=32;_RES[1]>>=6;
}

__inline void upfilter53VI_II(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E2_=E+M-2,r,m;

	_RES[0]-=(_X[0]<<2);_RES[1]+=5*_X[0]-_X[1];
	if (_RES[0]>0) _RES[0]+=32;_RES[0]>>=6;
	if (_RES[1]>0) _RES[1]+=32;_RES[1]>>=6;
	_RES+=2;

	for (;_X<_E2_;_X++,_RES+=2)
	{
		_RES[0]-=(_X[1]+_X[0])<<1;
		_RES[1]+=6*_X[1]-_X[2]-_X[0];

		if ((_RES[1]-_RES[0])>0 && (_RES[1]-_RES[0])<4480) _RES[0]+=32;
		else if ((_RES[1]-_RES[0])<0 && (_RES[1]-_RES[0])>-4480) _RES[1]+=32;

		if (_RES[0]>0) _RES[0]+=32; 
		_RES[0]>>=6;
		if (_RES[1]>0) _RES[1]+=32; 
		_RES[1]>>=6;

	}

	_X=E;_RES[0]-=(_X[M-1]+_X[M-2])<<1;if (_RES[0]>0) _RES[0]+=32;_RES[0]>>=6;
	_RES[1]+=5*_X[M-1]-_X[M-2];if (_RES[1]>0) _RES[1]+=32;_RES[1]>>=6;
}

__inline void upfilter53II(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E2_=E+M-2,r;

	_RES[0]-=((_X[0]+1)>>1);_RES[1]+=(5*_X[0]-_X[1]+4)>>3;_RES+=2;

	for (;_X<_E2_;_X++,_RES+=2)
	{
		r=_X[1]+_X[0];
		if (r>=0) _RES[0]-=(r+2)>>2;
		else _RES[0]+=(-r+2)>>2;

		r=6*_X[1]-_X[2]-_X[0];
		if (r>=0) _RES[1]+=(r+4)>>3;
		else _RES[1]-=(-r+4)>>3;
	}

	_X=E;_RES[0]-=((_X[M-1]+_X[M-2]+2)>>2);_RES[1]+=(5*_X[M-1]-_X[M-2]+4)>>3;
}

__inline void upfilter53IV(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E2_=E+M-2;

	/*_RES[0]-=((_X[0]+1)>>1);_RES[1]+=(5*_X[0]-_X[1]+4)>>3;_RES+=2;

	for (;_X<_E2_;_X+=2,_RES+=4)
	{
		_RES[0]-=((_X[1]+_X[0]+2)>>2);_RES[1]+=(6*_X[1]-_X[2]-_X[0]+4)>>3;
		_RES[2]-=((_X[2]+_X[1]+2)>>2);_RES[3]+=(6*_X[2]-_X[3]-_X[1]+4)>>3;
	}

	_X=E;_RES[0]-=((_X[M-1]+_X[M-2]+2)>>2);_RES[1]+=(5*_X[M-1]-_X[M-2]+4)>>3;*/
}

void upfilter97(short *_X,int M,int E,short *_RES)
{	
	int j,e=0;

	if (E)
	{
		_RES[e++]=floor(W976*_X[0]+W978*(_X[1]+_X[M-1])+0.5f);_RES[e++]=floor(W97S3*(_X[1]+_X[0])+W97S4*(_X[2]+_X[M-1])+0.5f);

		for (j=3;j<M;j++)
		{
			_RES[e++]=floor(W976*_X[j-2]+W978*(_X[j-1]+_X[j-3])+0.5f);_RES[e++]=floor(W97S3*(_X[j-1]+_X[j-2])+W97S4*(_X[j]+_X[j-3])+0.5f);
		}

		_RES[e++]=floor(W976*_X[M-2]+W978*(_X[M-1]+_X[M-3])+0.5f);_RES[e++]=floor(W97S3*(_X[M-1]+_X[M-2])+W97S4*(_X[0]+_X[M-3])+0.5f);
		_RES[e++]=floor(W976*_X[M-1]+W978*(_X[0]+_X[M-2])+0.5f);_RES[e++]=floor(W97S3*(_X[0]+_X[M-1])+W97S4*(_X[1]+_X[M-2])+0.5f);
	}
	else
	{
		_RES[e++]+=floor(W97S1*(_X[M-1]+_X[0])+W97S2*(_X[M-2]+_X[1])+0.5f);
		_RES[e++]+=floor(W971*_X[0]+W973*(_X[M-1]+_X[1])+W975*(_X[M-2]+_X[2])+0.5f);
		_RES[e++]+=floor(W97S1*(_X[0]+_X[1])+W97S2*(_X[M-1]+_X[2])+0.5f);
		_RES[e++]+=floor(W971*_X[1]+W973*(_X[0]+_X[2])+W975*(_X[M-1]+_X[3])+0.5f);

		for (j=4;j<M;j++)
		{
			_RES[e++]+=floor(W97S1*(_X[j-3]+_X[j-2])+W97S2*(_X[j-4]+_X[j-1])+0.5f);
			_RES[e++]+=floor(W971*_X[j-2]+W973*(_X[j-3]+_X[j-1])+W975*(_X[j-4]+_X[j])+0.5f);
		}

		_RES[e++]+=floor(W97S1*(_X[M-3]+_X[M-2])+W97S2*(_X[M-4]+_X[M-1])+0.5f);
		_RES[e++]+=floor(W971*_X[M-2]+W973*(_X[M-3]+_X[M-1])+W975*(_X[M-4]+_X[0])+0.5f);
		_RES[e++]+=floor(W97S1*(_X[M-2]+_X[M-1])+W97S2*(_X[M-3]+_X[0])+0.5f);
		_RES[e++]+=floor(W971*_X[M-1]+W973*(_X[M-2]+_X[0])+W975*(_X[M-3]+_X[1])+0.5f);
	}
}
