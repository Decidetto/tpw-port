# libjpeg 6b (vendored)

The Independent JPEG Group's JPEG library, version 6b (27 March 1998), the version the Mac game linked. The port builds it as a
static library (`tpw_libjpeg`, CMakeLists.txt here) and the game's JPEG calls reach it through `platform/jpeg_bridge.cpp`. The
host's own libjpeg is not used: its struct layout and version differ, and the port must not depend on a system library.

This software is based in part on the work of the Independent JPEG Group. The license is in README (the library is free to use,
copy and redistribute with that credit and README kept).

## Origin

- URL: https://www.ijg.org/files/jpegsrc.v6b.tar.gz
- SHA-256: `75c3ec241e9996504fe02a9ed4d12f16b74ade713972f3db9e65ce95cd27e35d` (checked after the download, equal to the widely
  published value)
- Fetched: 2026-10-22
- The archive was unpacked into an empty directory and only read. Its configure script, makefiles and programs were never run.

## Files

Copied unchanged from the archive: README and these library files.

- Compression: jcapimin jcapistd jccoefct jccolor jcdctmgr jchuff jcinit jcmainct jcmarker jcmaster jcomapi jcparam jcphuff
  jcprepct jcsample jctrans (.c)
- Decompression: jdapimin jdapistd jdatadst jdatasrc jdcoefct jdcolor jddctmgr jdhuff jdinput jdmainct jdmarker jdmaster
  jdmerge jdphuff jdpostct jdsample jdtrans (.c)
- Shared: jerror jfdctflt jfdctfst jfdctint jidctflt jidctfst jidctint jidctred jmemmgr jmemnobs jquant1 jquant2 jutils (.c)
- Headers: jchuff.h jdct.h jdhuff.h jerror.h jinclude.h jmemsys.h jmorecfg.h jpegint.h jpeglib.h jversion.h

Not copied: the programs (cjpeg, djpeg, jpegtran, rdjpgcom, wrjpgcom and their helpers), the other memory managers (jmemansi,
jmemname, jmemdos, jmemmac), the configure and makefile machinery, the documents and the test images.

## What is not the IJG's

- `jconfig.h`: written by hand for the host (prototypes, unsigned char and short, standard headers). It also names `XMD_H` and
  gives `INT16` as `short` and `INT32` as `int`, so the library's 32-bit arithmetic is exact on a 64-bit `long` host, as it was on
  the Mac where `long` is 32 bits.
- `CMakeLists.txt`: the static library target. It silences warnings for these old C sources.
- This file.

No IJG source file was edited.

## Struct layout against the translation

The translation declares the Mac's 32-bit structures (`jpeg_decompress_struct` 0x1d0 bytes in include/lobby.h,
`jpeg_compress_struct` 0x174 bytes in include/Postcard.h, the error manager 0x84 bytes). The host's structures hold 64-bit
pointers, so the bridge keeps a native context per Mac structure and copies the fields the game reads and writes (the offsets are
`static_assert`ed in platform/jpeg_bridge.cpp).

How the Mac offsets were checked against this very `jpeglib.h`: it was compiled as 32-bit-pointer code (`clang
--target=arm64_32-apple-darwin`) and the offsets printed. Decompress: image_width 0x1c, image_height 0x20, quantize_colors 0x54,
output_width 0x70, output_height 0x74, output_components 0x7c, actual_number_of_colors 0x84, colormap 0x88, output_scanline
0x8c, size 0x1d0, error manager 0x84. All equal the translation's. The compressor came out 4 bytes larger (0x178, next_scanline
0xe0) because arm64_32 aligns a double to 8 and the Mac's PowerPC ABI aligns it to 4 inside a structure; with that rule the Mac's
0x174 and 0xdc follow.
