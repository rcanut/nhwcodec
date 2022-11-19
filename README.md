NHW Image Codec
============

A Next-Generation Free Open-Source Image Compression Codec

The NHW codec is an experimental codec that compresses for now 512x512 bitmap 24bit color images using notably a wavelet transform.

The NHW codec presents some innovations and a unique approach: more image neatness/sharpness, and aims to be competitive with current codecs like for example x265 (HEVC), Google WebP,...

Another advantage of the NHW codec is that it has a high speed, making it suitable for mobile, embedded devices.


How to compile?
============

1) With mingw/gcc

`$ cd decoder && gcc *.c -O3 -o nhw-dec`
or
`$ cd decoder && make -f GNUMakefile`

2) With CMake

`mkdir build && cd build && cmake ../ && make`


To encode an image (512x512 bitmap color image for now): nhw_encoder.exe imagename.bmp

encoder options: quality settings: -q[1..24] {default: 20}

example:
`$ nhw-enc image.bmp image.nhw`
`$ nhw-enc -q 10 image.bmp image.nhw`

To decode: 
`$ nhw-dec image.nhw image.bmp`
