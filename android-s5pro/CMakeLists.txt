# s5conv - headless superccd core (S3/S5 Pro RAF -> merged DNG)
# Builds on Windows (MSYS2/mingw64) and Android (NDK / Termux) from one source.
cmake_minimum_required(VERSION 3.16)
project(s5conv C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------- options
option(S5CONV_ANDROID "Build for Android (JNI shared library)" OFF)

# ------------------------------------------------------------- third party
set(THIRD ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# ----- zlib (static)
set(ZLIB_SRC
    ${THIRD}/zlib/adler32.c ${THIRD}/zlib/compress.c ${THIRD}/zlib/crc32.c
    ${THIRD}/zlib/deflate.c ${THIRD}/zlib/gzclose.c ${THIRD}/zlib/gzlib.c
    ${THIRD}/zlib/gzread.c ${THIRD}/zlib/gzwrite.c ${THIRD}/zlib/infback.c
    ${THIRD}/zlib/inffast.c ${THIRD}/zlib/inflate.c ${THIRD}/zlib/inftrees.c
    ${THIRD}/zlib/trees.c ${THIRD}/zlib/uncompr.c ${THIRD}/zlib/zutil.c)
add_library(zlib_s STATIC ${ZLIB_SRC})
target_include_directories(zlib_s PUBLIC ${THIRD}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
# zlib needs zconf.h; the release tarball ships a ready-to-use copy
if(EXISTS ${THIRD}/zlib/zconf.h)
    target_include_directories(zlib_s PUBLIC ${THIRD}/zlib)
endif()

# ----- libtiff (static, no codecs beyond the DNG needs)
set(TIFF_DIR ${THIRD}/libtiff)
set(TIFF_SRC
    ${TIFF_DIR}/libtiff/tif_aux.c ${TIFF_DIR}/libtiff/tif_close.c
    ${TIFF_DIR}/libtiff/tif_codec.c ${TIFF_DIR}/libtiff/tif_color.c
    ${TIFF_DIR}/libtiff/tif_compress.c ${TIFF_DIR}/libtiff/tif_dir.c
    ${TIFF_DIR}/libtiff/tif_dirinfo.c ${TIFF_DIR}/libtiff/tif_dirread.c
    ${TIFF_DIR}/libtiff/tif_dirwrite.c ${TIFF_DIR}/libtiff/tif_dumpmode.c
    ${TIFF_DIR}/libtiff/tif_error.c ${TIFF_DIR}/libtiff/tif_extension.c
    ${TIFF_DIR}/libtiff/tif_fax3.c ${TIFF_DIR}/libtiff/tif_fax3sm.c
    ${TIFF_DIR}/libtiff/tif_flush.c ${TIFF_DIR}/libtiff/tif_getimage.c
    ${TIFF_DIR}/libtiff/tif_hash_set.c
    ${TIFF_DIR}/libtiff/tif_luv.c ${TIFF_DIR}/libtiff/tif_lzw.c
    ${TIFF_DIR}/libtiff/tif_next.c ${TIFF_DIR}/libtiff/tif_open.c ${TIFF_DIR}/libtiff/tif_packbits.c
    ${TIFF_DIR}/libtiff/tif_pixarlog.c ${TIFF_DIR}/libtiff/tif_predict.c
    ${TIFF_DIR}/libtiff/tif_print.c ${TIFF_DIR}/libtiff/tif_read.c
    ${TIFF_DIR}/libtiff/tif_strip.c ${TIFF_DIR}/libtiff/tif_swab.c
    ${TIFF_DIR}/libtiff/tif_thunder.c ${TIFF_DIR}/libtiff/tif_tile.c
    ${TIFF_DIR}/libtiff/tif_unix.c ${TIFF_DIR}/libtiff/tif_version.c
    ${TIFF_DIR}/libtiff/tif_warning.c ${TIFF_DIR}/libtiff/tif_write.c
    ${TIFF_DIR}/libtiff/tif_zip.c)
add_library(tiff_s STATIC ${TIFF_SRC})
target_include_directories(tiff_s PUBLIC ${TIFF_DIR}/libtiff ${CMAKE_CURRENT_BINARY_DIR}/tiff)
target_link_libraries(tiff_s PUBLIC zlib_s)

# tif_config.h / tiffconf.h: generate from templates with a fixed feature set
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tiff)
set(TIFF_DEFS
    "/* s5conv minimal tiff config */"
    "#include <stdint.h>"
    "#include <inttypes.h>"
    "#define CCITT_SUPPORT 1"
    "#define CHECK_JPEG_YCBCR_SUBSAMPLING 0"
    "#define CHUNKY_STRIP_READ_SUPPORT 1"
    "#define CXX_SUPPORT 1"
    "#define DEFER_STRILE_LOAD 1"
    "#define HAVE_ASSERT_H 1"
    "#define HAVE_FCNTL_H 1"
    "#define HAVE_STRING_H 1"
    "#define HAVE_SYS_TYPES_H 1"
    "#define HAVE_UNISTD_H 1"
    "#define HAVE_STDINT_H 1"
    "#define HAVE_IEEEFP 1"
    "#define HOST_FILLORDER FILLORDER_LSB2MSB"
    "#define LZW_SUPPORT 1"
    "#define PACKBITS_SUPPORT 1"
    "#define PIXARLOG_SUPPORT 1"
    "#define STRIPCHOP_DEFAULT TIFF_STRIPCHOP"
    "#define STRIP_SIZE_DEFAULT 8192"
    "#define THUNDER_SUPPORT 1"
    "#define NEXT_SUPPORT 1"
    "#define LOGLUV_SUPPORT 1"
    "#define ZIP_SUPPORT 1"
    "#define SIZEOF_SIGNED_LONG 8"
    "#define SIZEOF_UNSIGNED_LONG 8"
    "#define SIZEOF_SIGNED_LONG_LONG 8"
    "#define SIZEOF_UNSIGNED_LONG_LONG 8"
    "#define SIZEOF_VOIDP 8"
    "#define SIZEOF_SIZE_T 8"
    "#define SIZEOF_WORDTYPE 2"
    "#define TIFF_INT8_T signed char"
    "#define TIFF_UINT8_T unsigned char"
    "#define TIFF_INT16_T signed short"
    "#define TIFF_UINT16_T unsigned short"
    "#define TIFF_INT32_T signed int"
    "#define TIFF_UINT32_T unsigned int"
    "#define TIFF_INT64_T signed long long"
    "#define TIFF_UINT64_T unsigned long long"
    "#define TIFF_INT8_MAX 127"
    "#define TIFF_UINT8_MAX 255"
    "#define TIFF_INT16_MAX 32767"
    "#define TIFF_UINT16_MAX 65535"
    "#define TIFF_INT32_MAX 2147483647"
    "#define TIFF_UINT32_MAX 4294967295U"
    "#define TIFF_INT64_MAX 9223372036854775807LL"
    "#define TIFF_UINT64_MAX 18446744073709551615ULL"
    "#define TIFF_MIN_PLATFORM 1"
    "#define TIFF_SSIZE_T signed long long"
    "#define TIFF_SSIZE_FORMAT \"lld\""
    "#define TIFF_UINT64_FORMAT \"llu\""
    "#define TIFF_INT64_FORMAT \"lld\"")
if(MSVC)
    list(APPEND TIFF_DEFS "#define snprintf _snprintf")
endif()
set(TIFFCONF_DEFS "
/* s5conv minimal tiffconf */
#include <stdint.h>
#include <stddef.h>
#define TIFF_SSIZE_T signed long long
#define TIFF_UINT64_T unsigned long long
#define TIFF_INT64_T signed long long
#define CCITT_SUPPORT 1
#define PACKBITS_SUPPORT 1
#define LZW_SUPPORT 1
#define LOGLUV_SUPPORT 1
#define NEXT_SUPPORT 1
#define THUNDER_SUPPORT 1
#define ZIP_SUPPORT 1
#define PIXARLOG_SUPPORT 1
#define STRIPCHOP_SUPPORT 1
#define STRIP_SIZE_SUPPORT 1
#define COLORIMETRY_SUPPORT 1
#define YCBCR_SUPPORT 1
#define CMYK_SUPPORT 1
#define NEEDED_SUPPORT 1
#define DEFER_STRILE_LOAD 1
#define CHECK_JPEG_YCBCR_SUBSAMPLING 0
#define TIFFLIB_VERSION_STR \"LIBTIFF, Version 4.7.0 (s5conv minimal)\"
")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/tiff/tif_config.h "")
foreach(l ${TIFF_DEFS})
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/tiff/tif_config.h "${l}\n")
endforeach()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/tiff/tiffconf.h "${TIFFCONF_DEFS}")

# ----- LibRaw (static)
set(LIBRAW_DIR ${THIRD}/LibRaw)
set(LIBRAW_SRC
    ${LIBRAW_DIR}/src/libraw_c_api.cpp
    ${LIBRAW_DIR}/src/libraw_datastream.cpp)
file(GLOB LIBRAW_DECODERS ${LIBRAW_DIR}/src/decoders/*.cpp)
file(GLOB LIBRAW_DEMOSAIC ${LIBRAW_DIR}/src/demosaic/*.cpp)
file(GLOB LIBRAW_METADATA ${LIBRAW_DIR}/src/metadata/*.cpp)
file(GLOB LIBRAW_POSTPROC ${LIBRAW_DIR}/src/postprocessing/*.cpp)
file(GLOB LIBRAW_PREPROC ${LIBRAW_DIR}/src/preprocessing/*.cpp)
file(GLOB LIBRAW_TABLES ${LIBRAW_DIR}/src/tables/*.cpp)
file(GLOB LIBRAW_UTILS ${LIBRAW_DIR}/src/utils/*.cpp)
file(GLOB LIBRAW_WRITE ${LIBRAW_DIR}/src/write/*.cpp)
file(GLOB LIBRAW_X3F ${LIBRAW_DIR}/src/x3f/*.cpp)
file(GLOB LIBRAW_INTERNAL ${LIBRAW_DIR}/internal/*.cpp)
# LibRaw ships *_ph.cpp placeholder stubs that duplicate real symbols; the
# official buildfiles exclude them.
foreach(_g LIBRAW_DECODERS LIBRAW_DEMOSAIC LIBRAW_METADATA LIBRAW_POSTPROC
           LIBRAW_PREPROC LIBRAW_TABLES LIBRAW_UTILS LIBRAW_WRITE LIBRAW_X3F)
    list(FILTER ${_g} EXCLUDE REGEX "_ph\\.cpp$")
endforeach()
add_library(raw_s STATIC
    ${LIBRAW_SRC} ${LIBRAW_DECODERS} ${LIBRAW_DEMOSAIC} ${LIBRAW_METADATA}
    ${LIBRAW_POSTPROC} ${LIBRAW_PREPROC} ${LIBRAW_TABLES} ${LIBRAW_UTILS}
    ${LIBRAW_WRITE} ${LIBRAW_X3F} ${LIBRAW_INTERNAL})
target_include_directories(raw_s PUBLIC ${LIBRAW_DIR} ${LIBRAW_DIR}/libraw)
if(WIN32)
    target_compile_definitions(raw_s PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(raw_s PUBLIC ws2_32)
endif()

# ------------------------------------------------------------- miniqt shim
set(MINIQT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/miniqt)
set(MINIQT_SOURCES ${MINIQT_DIR}/miniqt.cpp)

# ----------------------------------------------------------------- s5 core
set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core)
set(CORE_SOURCES
    ${CORE_DIR}/SuperCCDProcessor.cpp
    ${CORE_DIR}/DngWriter.cpp
    ${CORE_DIR}/CfaPlaneAlignment.cpp
    ${CORE_DIR}/AmazeDemosaic.cpp
    ${CORE_DIR}/PreviewColorNormalization.cpp
    ${CORE_DIR}/PreviewPixelCorrection.cpp
    ${CORE_DIR}/SuperCCDProcessor.h
    ${CORE_DIR}/DngWriter.h
    ${CORE_DIR}/CfaPlaneAlignment.h
    ${CORE_DIR}/AmazeDemosaic.h
    ${CORE_DIR}/PreviewColorNormalization.h
    ${CORE_DIR}/PreviewPixelCorrection.h)
add_library(s5core STATIC ${CORE_SOURCES} ${MINIQT_SOURCES})
target_include_directories(s5core PUBLIC ${MINIQT_DIR} ${MINIQT_DIR}/include ${CORE_DIR} ${THIRD})
target_compile_definitions(s5core PRIVATE HAVE_LIBTIFF=1)
target_link_libraries(s5core PUBLIC raw_s tiff_s)

# ------------------------------------------------------------------ outputs
if(S5CONV_ANDROID)
    # __ANDROID_API__ is set by the NDK toolchain; Termux builds need it explicit
    add_library(s5dng SHARED ${CMAKE_CURRENT_SOURCE_DIR}/miniqt/s5_jni.cpp)
    target_link_libraries(s5dng PRIVATE s5core)
    if(ANDROID)
        target_link_libraries(s5dng PRIVATE log)
    endif()
else()
    add_executable(s5conv ${MINIQT_DIR}/s5conv_main.cpp)
    target_link_libraries(s5conv PRIVATE s5core)
    if(MINGW)
        # self-contained exe: no libstdc++/libgcc runtime DLLs needed on target machines
        target_link_options(s5conv PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()
