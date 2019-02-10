NHW Image Codec
============

A Next-Generation Free Open-Source Image Compression Codec

The NHW codec is an experimental codec that compresses for now 512x512 bitmap 24bit color images using notably a wavelet transform.

The NHW codec presents some innovations and a unique approach: more image neatness/sharpness, and aims to be competitive with current codecs like for example x265 (HEVC), Google WebP,...

Another advantage of the NHW codec is that it has a high speed, making it suitable for mobile, embedded devices.


How to compile?
============

For Windows: gcc *.c -O3 -o nhw_en/decoder.exe

For Linux: gcc *.c -O3 -lm -o nhw_en/decoder.exe

To encode an image (512x512 bitmap color image for now): nhw_encoder.exe imagename.bmp

encoder options: quality settings: -h1..3 or -l1..18

example: nhw_encoder.exe imagename.bmp -l3
                 
To decode: nhw_decoder.exe imagename.nhw
