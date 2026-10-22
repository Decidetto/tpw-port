# libjpeg 6b, the Independent JPEG Group's library, as the game linked it (see NOTE.md). Built as a static library of the
# port: the game's JPEG calls (platform/jpeg_bridge.cpp) go to it, not to a library of the host.
enable_language(C)
set(TPW_LIBJPEG_SOURCES
  jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c
  jcparam.c jcphuff.c jcprepct.c jcsample.c jctrans.c
  jdapimin.c jdapistd.c jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c
  jdmaster.c jdmerge.c jdphuff.c jdpostct.c jdsample.c jdtrans.c
  jerror.c jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c
  jmemmgr.c jmemnobs.c jquant1.c jquant2.c jutils.c)
list(TRANSFORM TPW_LIBJPEG_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
add_library(tpw_libjpeg STATIC ${TPW_LIBJPEG_SOURCES})
target_include_directories(tpw_libjpeg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# The IJG's code is old C and is kept as it is: no warnings are asked of it.
set_target_properties(tpw_libjpeg PROPERTIES C_STANDARD 99 POSITION_INDEPENDENT_CODE ON)
target_compile_options(tpw_libjpeg PRIVATE -w)
