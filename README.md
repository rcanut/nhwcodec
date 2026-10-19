NHW Image Codec
============

A Next-Generation Free Open-Source Image Compression Codec

The NHW codec is an experimental codec that compresses 24bit bitmap color images using notably a wavelet transform.

To work with any image resolution, it uses 512x512 tiling of a NHW core.

-Compression is not optimal for now.-

The NHW codec presents some innovations and a unique approach: more image neatness/sharpness, and aims to be competitive with current codecs like for example x265 (HEVC), Google WebP,...

Another advantage of the NHW codec is that it has a high speed, making it suitable for mobile, embedded devices.


How to compile?
============

1) With mingw/gcc

`$ cd decoder && gcc *.c -O3 -o nhw-dec`

`$ cd encoder && gcc *.c -O3 -o nhw-enc`

To enable multithreading (OpenMP), add the compiler flag:  ` -fopenmp` (gcc/clang) or ` /openmp` (MSVC)

2) With CMake

`$ mkdir build && cd build && cmake ../ && make`


To encode an image (512x512 bitmap color image for now):

encoder options: quality settings: -q[1..23] {default: 20}

example:

`$ nhw-enc image.bmp image.nhw`

`$ nhw-enc -q10 image.bmp image.nhw`

To decode:

`$ nhw-dec image.nhw image.bmp`
