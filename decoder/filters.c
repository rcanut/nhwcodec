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
*  remark: -code a fast DWT 2D									           *
***************************************************************************/

/* Copyright (C) 2007-2012 NHW Project
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
#include "wavelets.h"

__inline void upfilter53_08(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E1_=E+M-1;

	for (;_X<_E1_;_X++,_RES+=2)
	{ 
		_RES[0]=_X[0];_RES[1]=(_X[1]+_X[0]+1)>>1;
	}

	_X=E;_RES[0]=_X[M-1];_RES[1]=_X[M-1];
}

__inline void upfilter53(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E1_=E+M-2,r,m=0,a;

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

__inline void upfilter53I_I(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E1_=E+M-1;

	for (;_X<_E1_;_X++,_RES+=2)
	{ 
		if (_X[0]==_X[1])
		{
			//_RES[-1]+=(rand()%4)-2;
			_RES[0]=_X[0]+(rand()%2)-1;_RES[1]=((_X[1]+_RES[0]+1)>>1)+(rand()%1);
		}
		else
		{
			_RES[0]=_X[0];_RES[1]=(_X[1]+_X[0]+1)>>1;
		}
	}

	_X=E;_RES[0]=_X[M-1];_RES[1]=_X[M-1];
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

		//_RES[0]-=((_X[1]+_X[0]+2)>>2);_RES[1]+=(6*_X[1]-_X[2]-_X[0]+4)>>3;
	}

	_X=E;_RES[0]-=((_X[M-1]+_X[M-2]+2)>>2);_RES[1]+=(5*_X[M-1]-_X[M-2]+4)>>3;
}

__inline void upfilter53V(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E2_=E+M-2;

	_RES[0]-=(((_X[0]+1)>>1)+((rand()%2))-1);_RES[1]+=(((5*_X[0]-_X[1]+4)>>3)+(rand()%2)-1);_RES+=2;

	for (;_X<_E2_;_X+=2,_RES+=4)
	{
		_RES[0]-=(((_X[1]+_X[0]+2)>>2)+(rand()%2)-1);_RES[1]+=(((6*_X[1]-_X[2]-_X[0]+4)>>3)+(rand()%2)-1);
		_RES[2]-=(((_X[2]+_X[1]+2)>>2)+((rand()%2)-1));_RES[3]+=(((6*_X[2]-_X[3]-_X[1]+4)>>3)+(rand()%2)-1);
	}

	_X=E;_RES[0]-=(((_X[M-1]+_X[M-2]+2)>>2)+((rand()%2))-1);_RES[1]+=(((5*_X[M-1]-_X[M-2]+4)>>3)+(rand()%2)-1);
}

__inline void upfilter53IV(short *_X,int M,short *_RES)
{	
	short *E=_X,*_E2_=E+M-2;

	_RES[0]-=(((_X[0]+1)>>1)+((rand()%3)-1));_RES[1]+=(((5*_X[0]-_X[1]+4)>>3));_RES+=2;

	for (;_X<_E2_;_X+=2,_RES+=4)
	{
		_RES[0]-=(((_X[1]+_X[0]+2)>>2));_RES[1]+=(((6*_X[1]-_X[2]-_X[0]+4)>>3)+((rand()%3)-1));
		_RES[2]-=(((_X[2]+_X[1]+2)>>2)+((rand()%3)-1));_RES[3]+=(((6*_X[2]-_X[3]-_X[1]+4)>>3));
	}

	_X=E;_RES[0]-=(((_X[M-1]+_X[M-2]+2)>>2));_RES[1]+=(((5*_X[M-1]-_X[M-2]+4)>>3)+((rand()%3)-1));
}

void upfilter97(short *_X,int M,int E,short *_RES)
{	
	int j,e=0;

	if (E)
	{
		_RES[e++]=(short)(W976*_X[0]+W978*(_X[1]+_X[M-1])+0.5f);_RES[e++]=(short)(W97S3*(_X[1]+_X[0])+W97S4*(_X[2]+_X[M-1])+0.5f);

		for (j=3;j<M;j++)
		{
			_RES[e++]=(short)(W976*_X[j-2]+W978*(_X[j-1]+_X[j-3])+0.5f);_RES[e++]=(short)(W97S3*(_X[j-1]+_X[j-2])+W97S4*(_X[j]+_X[j-3])+0.5f);
		}

		_RES[e++]=(short)(W976*_X[M-2]+W978*(_X[M-1]+_X[M-3])+0.5f);_RES[e++]=(short)(W97S3*(_X[M-1]+_X[M-2])+W97S4*(_X[0]+_X[M-3])+0.5f);
		_RES[e++]=(short)(W976*_X[M-1]+W978*(_X[0]+_X[M-2])+0.5f);_RES[e++]=(short)(W97S3*(_X[0]+_X[M-1])+W97S4*(_X[1]+_X[M-2])+0.5f);
	}
	else
	{
		_RES[e++]+=(short)(W97S1*(_X[M-1]+_X[0])+W97S2*(_X[M-2]+_X[1])+0.5f);
		_RES[e++]+=(short)(W971*_X[0]+W973*(_X[M-1]+_X[1])+W975*(_X[M-2]+_X[2])+0.5f);
		_RES[e++]+=(short)(W97S1*(_X[0]+_X[1])+W97S2*(_X[M-1]+_X[2])+0.5f);
		_RES[e++]+=(short)(W971*_X[1]+W973*(_X[0]+_X[2])+W975*(_X[M-1]+_X[3])+0.5f);

		for (j=4;j<M;j++)
		{
			_RES[e++]+=(short)(W97S1*(_X[j-3]+_X[j-2])+W97S2*(_X[j-4]+_X[j-1])+0.5f);
			_RES[e++]+=(short)(W971*_X[j-2]+W973*(_X[j-3]+_X[j-1])+W975*(_X[j-4]+_X[j])+0.5f);
		}

		_RES[e++]+=(short)(W97S1*(_X[M-3]+_X[M-2])+W97S2*(_X[M-4]+_X[M-1])+0.5f);
		_RES[e++]+=(short)(W971*_X[M-2]+W973*(_X[M-3]+_X[M-1])+W975*(_X[M-4]+_X[0])+0.5f);
		_RES[e++]+=(short)(W97S1*(_X[M-2]+_X[M-1])+W97S2*(_X[M-3]+_X[0])+0.5f);
		_RES[e++]+=(short)(W971*_X[M-1]+W973*(_X[M-2]+_X[0])+W975*(_X[M-3]+_X[1])+0.5f);
	}
}
