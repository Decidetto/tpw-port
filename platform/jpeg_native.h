// jpeg_native.h: the port's use of the vendored libjpeg 6b (third_party/libjpeg-6b) with plain host types. The game's own JPEG
// calls (include/lobby.h, include/Postcard.h) pass the Mac's 32-bit structures; platform/jpeg_bridge.cpp turns them into calls
// of this interface, one Context per Mac decompressor or compressor. This file and jpeg_native.cpp see only the library's headers,
// never the game's (the library's names TRUE, FALSE, boolean and INT32 clash with the game's headers).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace tpw {
namespace jpeg {

struct Context;

// Called from the library's error_exit, inside a call below, before anything else is done. A hook that runs the game's own
// handler may leave through Jump(); one that returns lets the library print its message and leave.
typedef void (*ErrorHook)(Context* context, void* user);
// Fills `dest` with up to `size` bytes; the byte count, 0 or less at the end.
typedef long (*ReadSource)(void* user, uint8_t* dest, size_t size);

struct DecodeInfo {
    uint32_t imageWidth;
    uint32_t imageHeight;
    int32_t quantizeColors;
    uint32_t outputWidth;
    uint32_t outputHeight;
    int32_t outputComponents;
    int32_t colorCount;                // actual_number_of_colors
    const uint8_t* colorTable[3];      // colormap rows, null when there is no colormap
    uint32_t outputScanline;
};

// A decompressor or a compressor with an error manager of its own (the library's standard one, its error_exit leaving the call).
Context* NewDecompressor(ErrorHook hook, void* user);
Context* NewCompressor(ErrorHook hook, void* user);
void DeleteContext(Context* context);         // destroys the library's object whatever state it is in

bool Failed(Context* context);                // an error left a call: every later call but DeleteContext does nothing
void PrintMessage(Context* context);          // the library's output_message for the error being handled
bool Jump(Context* context);                  // leaves the call in progress as an error did; false when no call is in progress

// Decompression (jpeg_stdio_src, jpeg_read_header, jpeg_start_decompress, jpeg_read_scanlines, jpeg_finish_decompress).
void SetFileSource(Context* context, std::FILE* file);
void SetReadSource(Context* context, ReadSource read, void* user);
int ReadHeader(Context* context, int requireImage);
int StartDecompress(Context* context, int quantizeColors);   // the quantize_colors the game left in its structure
unsigned ReadScanlines(Context* context, uint8_t** rows, unsigned maxLines);
int FinishDecompress(Context* context);
DecodeInfo GetDecodeInfo(Context* context);

// Compression (jpeg_stdio_dest, jpeg_set_defaults, jpeg_set_quality, jpeg_start_compress, jpeg_write_scanlines,
// jpeg_finish_compress).
void SetFileDestination(Context* context, std::FILE* file);
void SetImage(Context* context, uint32_t width, uint32_t height, int components, int colorSpace);
void SetDefaults(Context* context);
void SetQuality(Context* context, int quality, int forceBaseline);
void StartCompress(Context* context, int writeAllTables);
unsigned WriteScanlines(Context* context, uint8_t** rows, unsigned lines);
void FinishCompress(Context* context);
uint32_t NextScanline(Context* context);

}  // namespace jpeg
}  // namespace tpw
