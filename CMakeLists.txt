cmake_minimum_required(VERSION 3.0)

project(NHWCodec C)

# encoder
set(ENC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/codec.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/tree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/wavelets.h
)
set(ENC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/colorspace.c
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/compress_pixel.c
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/filters.c
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/image_processing.c
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder/wavelet_filterbank.c
)
add_library(nhw_enc ${ENC_SOURCES})

# decoder
set(DEC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder/codec.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder/tables.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder/wavelets.h
)
set(DEC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder/compress_pixel.c
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder/filters.c
    ${CMAKE_CURRENT_SOURCE_DIR}/decoder/wavelet_filterbank.c
)
add_library(nhw_dec ${DEC_SOURCES})

# executables
add_executable(dec
               ${DEC_HEADERS}
               ${DEC_SOURCES}
               decoder/nhw_decoder.c)
target_link_libraries(dec nhw_dec)
if (UNIX)
    target_link_libraries(dec m)
endif (UNIX)
install(TARGETS dec
        RUNTIME DESTINATION bin
        DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(enc
               ${ENC_HEADERS}
               ${ENC_SOURCES}
               encoder/nhw_encoder.c)
target_link_libraries(enc nhw_enc)
if (UNIX)
    target_link_libraries(enc m)
endif (UNIX)
install(TARGETS enc
        RUNTIME DESTINATION bin
        DESTINATION ${CMAKE_INSTALL_BINDIR})
