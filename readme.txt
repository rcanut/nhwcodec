--- NHW Image Compression Codec --- 
               
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

-----------------------------------------------------------------------------------------

last version : 0.1.3 08102015 nhw

This codec allows to compress a bitmap color image (with 512x512 size for the moment) using notably a wavelet transform.

-----------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------

encoder option:  nhw_encoder.exe filename.bmp 
		     nhw_encoder.exe filename.bmp -h2
		     nhw_encoder.exe filename.bmp -h1
		     nhw_encoder.exe filename.bmp -l1
		     nhw_encoder.exe filename.bmp -l2
		     nhw_encoder.exe filename.bmp -l3


-----------------------------------------------------------------------------------------

decoder:	     nhw_decoder.exe filename.nhw

-----------------------------------------------------------------------------------------

- To encode and decode an image (512x512 bitmap color) it is faster on this version to just make "slide" the image or compressed .nhw file on the encoder or decoder executable. -

---

