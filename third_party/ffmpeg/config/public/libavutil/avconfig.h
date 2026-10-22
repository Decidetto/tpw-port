/* Written by gen_config.py (in place of FFmpeg's configure); run it again rather than edit this file. */
#ifndef AVUTIL_AVCONFIG_H
#define AVUTIL_AVCONFIG_H
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define AV_HAVE_BIGENDIAN 1
#else
#define AV_HAVE_BIGENDIAN 0
#endif
#define AV_HAVE_FAST_UNALIGNED 0
#endif /* AVUTIL_AVCONFIG_H */
