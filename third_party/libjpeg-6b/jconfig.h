/* jconfig.h: the configuration of libjpeg 6b for the port's host (written by hand for this build; the IJG's configure
 * script is not used). The sources are the IJG's own. */

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
/* #undef void */
/* #undef const */
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

/* The Mac build's long is 32 bits and the library's INT32 is a long, so its integer arithmetic wraps at 32 bits. The host's
 * long is 64 bits; INT16 and INT32 are given here as exactly 16 and 32 bits (naming XMD_H keeps jmorecfg.h from declaring
 * them again) so that the host computes what the Mac computed. */
#define XMD_H
typedef short INT16;
typedef int INT32;

#ifdef JPEG_INTERNALS
#undef RIGHT_SHIFT_IS_UNSIGNED
#endif
