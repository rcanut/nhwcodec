NHW Image Codec
============

A Next-Generation Free Open-Source Image Compression Codec

The NHW codec is an experimental codec that compresses for now 512x512 bitmap 24bit color images using notably a wavelet transform.

The codec has a good image neatness and is very fast.



How to compile?
============

For Windows: gcc *.c -O3 -o nhw_en/decoder.exe

For Linux:   gcc *.c -O3 -lm -o nhw_en/decoder.exe
