# tpw-port: the playable port. The translation is the tpw-decomp submodule, built as it is except for the files under
# overrides/, which take the place of the submodule's file of the same path (the port's fixes, listed in FIXES.md).
cmake_minimum_required(VERSION 3.20)
project(tpw_port LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter -ffp-contract=off -fwrapv)
add_compile_definitions(TPW_PORT_FIX)

set(TPW_DECOMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tpw-decomp)
if(NOT EXISTS ${TPW_DECOMP_DIR}/CMakeLists.txt)
  message(FATAL_ERROR "tpw-decomp is missing: git submodule update --init")
endif()
# The game's data tables come from your own copy of the game (see tpw-decomp's README); the default is the extracted patch
# next to this repository.
if(NOT TPW_GAME_PROGRAM AND NOT DEFINED ENV{TPW_GAME_PROGRAM})
  set(TPW_GAME_PROGRAM "${CMAKE_CURRENT_SOURCE_DIR}/../extracted/SimTPPatch_204b3/Sim Theme Park Patch/patch 2.04b3/SimTheme Park"
      CACHE FILEPATH "The 2.04b3 SimTheme Park application the data tables are read from")
endif()
set(TPW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# The headers: the submodule's include/ and platform/ headers, with overrides/include and overrides/platform laid over them,
# copied into the build folder at configure time (TPW_OVERLAY_DIR). Every target reads its headers from the copy and never
# from the submodule (the tail of this file rewrites the submodule's include folders in every target), so an overridden
# header is the override however it is included: a quoted include looks beside the including header first, and #pragma
# once is per file, so the submodule's own copy of a header must never be reachable.
set(TPW_OVERLAY_DIR ${CMAKE_CURRENT_BINARY_DIR}/tpw-overlay)
set(TPW_INCLUDE_DIR ${TPW_OVERLAY_DIR}/include)
file(GLOB_RECURSE TPW_DECOMP_HEADERS CONFIGURE_DEPENDS RELATIVE ${TPW_DECOMP_DIR}
     ${TPW_DECOMP_DIR}/include/* ${TPW_DECOMP_DIR}/platform/*.h)
file(GLOB_RECURSE TPW_OVERRIDE_HEADERS CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/overrides
     ${CMAKE_CURRENT_SOURCE_DIR}/overrides/include/* ${CMAKE_CURRENT_SOURCE_DIR}/overrides/platform/*.h)
foreach(header ${TPW_OVERRIDE_HEADERS})
  if(NOT header IN_LIST TPW_DECOMP_HEADERS)
    message(FATAL_ERROR "overrides/${header} has no file in tpw-decomp to replace")
  endif()
endforeach()
foreach(header ${TPW_DECOMP_HEADERS})
  if(header IN_LIST TPW_OVERRIDE_HEADERS)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/overrides/${header} ${TPW_OVERLAY_DIR}/${header} COPYONLY)
  else()
    configure_file(${TPW_DECOMP_DIR}/${header} ${TPW_OVERLAY_DIR}/${header} COPYONLY)
  endif()
endforeach()
file(GLOB_RECURSE TPW_OVERLAY_HEADERS RELATIVE ${TPW_OVERLAY_DIR} ${TPW_OVERLAY_DIR}/*)
foreach(header ${TPW_OVERLAY_HEADERS})
  if(NOT header IN_LIST TPW_DECOMP_HEADERS)
    file(REMOVE ${TPW_OVERLAY_DIR}/${header})   # gone from the submodule since the last configure
  endif()
endforeach()

add_subdirectory(${TPW_DECOMP_DIR} tpw-decomp)
find_package(ZLIB REQUIRED)

# The native launcher (Milestone 1 unit 9): the translated game code, the platform layer
# (platform/: the Elegant Chaos shims and Mac Toolbox calls on SDL3 and OpenGL) and
# generated stand-ins for everything that is not translated or implemented yet
# (launcher/gen_stubs.py, rerun by every build, so a stand-in disappears when its unit
# lands). Skipped when SDL3 or OpenGL is missing: the library and the tests do not need it.
option(TPW_BUILD_LAUNCHER "Build the native launcher (needs SDL3 and OpenGL)" ON)
# The translation's tree: this one here; tpw-port sets it to its tpw-decomp submodule.
if(NOT TPW_DECOMP_DIR)
  set(TPW_DECOMP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()
# Its headers: include/ of that tree; tpw-port sets it to a copy of the submodule's include/ with its overrides laid over it.
if(NOT TPW_INCLUDE_DIR)
  set(TPW_INCLUDE_DIR ${TPW_DECOMP_DIR}/include)
endif()
if(TPW_BUILD_LAUNCHER)
  find_package(SDL3 CONFIG QUIET)
  find_package(OpenGL QUIET)
  find_package(Python3 COMPONENTS Interpreter QUIET)
endif()
if(TPW_BUILD_LAUNCHER AND SDL3_FOUND AND OpenGL_FOUND AND Python3_FOUND)
  file(GLOB TPW_PLATFORM_SOURCES CONFIGURE_DEPENDS platform/*.cpp ${TPW_DECOMP_DIR}/platform/*.cpp)
  list(REMOVE_DUPLICATES TPW_PLATFORM_SOURCES)
  add_library(tpw_platform STATIC ${TPW_PLATFORM_SOURCES})
  target_include_directories(tpw_platform PUBLIC ${TPW_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/platform ${TPW_DECOMP_DIR}/platform)
  target_link_libraries(tpw_platform PUBLIC SDL3::SDL3 OpenGL::GL)
  target_compile_definitions(tpw_platform PRIVATE GL_SILENCE_DEPRECATION)
  if(APPLE)
    # platform/gdi.cpp draws the signs' text with CoreText into a CoreGraphics bitmap.
    target_link_libraries(tpw_platform PUBLIC "-framework CoreText" "-framework CoreGraphics" "-framework CoreFoundation" objc)
  endif()

  # The game's films (QuickTime, Sorenson video) play when FFmpeg's libraries are there
  # (platform/quicktime.cpp); without them the game skips its films.
  option(TPW_MOVIES "Play the game's films with FFmpeg when it is installed" ON)
  set(TPW_MOVIE_SYSLIBS)
  if(TPW_MOVIES)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(TPW_FFMPEG QUIET IMPORTED_TARGET libavformat libavcodec libswscale libavutil)
    endif()
    if(TPW_FFMPEG_FOUND)
      target_compile_definitions(tpw_platform PRIVATE TPW_HAVE_FFMPEG=1)
      target_link_libraries(tpw_platform PUBLIC PkgConfig::TPW_FFMPEG)
      foreach(lib ${TPW_FFMPEG_LINK_LIBRARIES})
        list(APPEND TPW_MOVIE_SYSLIBS --syslib ${lib})
      endforeach()
      message(STATUS "tpw: films play (FFmpeg ${TPW_FFMPEG_libavformat_VERSION})")
    else()
      message(STATUS "tpw: films are skipped (FFmpeg not found)")
    endif()
  endif()

  add_library(tpw_launcher_objects OBJECT launcher/main.cpp launcher/pending_manual.cpp launcher/static_init.cpp)
  target_include_directories(tpw_launcher_objects PRIVATE ${TPW_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/platform ${TPW_DECOMP_DIR}/platform launcher)
  target_link_libraries(tpw_launcher_objects PRIVATE SDL3::SDL3)

  # zlib's inflate (src/lb_zlib.cpp) comes from the host, so the generator must not stand in for it.
  if(APPLE)
    set(TPW_STUB_SYSLIBS --syslib /System/Library/Frameworks/OpenGL.framework/OpenGL --syslib /usr/lib/libz.dylib)
  else()
    set(TPW_STUB_SYSLIBS --syslib libGL.so.1 --syslib libz.so.1)
  endif()
  set(TPW_STUBS ${CMAKE_CURRENT_BINARY_DIR}/stubs_generated.cpp)
  add_custom_command(
    OUTPUT ${TPW_STUBS}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/launcher/gen_stubs.py
            --out ${TPW_STUBS} --include-dir ${TPW_INCLUDE_DIR}
            --nm ${CMAKE_NM} --syslib $<TARGET_FILE:SDL3::SDL3> ${TPW_STUB_SYSLIBS} ${TPW_MOVIE_SYSLIBS}
            $<TARGET_FILE:tpw> $<TARGET_FILE:tpw_platform> $<TARGET_OBJECTS:tpw_launcher_objects>
    DEPENDS tpw tpw_platform $<TARGET_OBJECTS:tpw_launcher_objects> launcher/gen_stubs.py
    COMMAND_EXPAND_LISTS VERBATIM
    COMMENT "Generating stand-ins for untranslated and unimplemented functions")
  add_executable(tpw_launcher $<TARGET_OBJECTS:tpw_launcher_objects> ${TPW_STUBS})
  target_include_directories(tpw_launcher PRIVATE ${TPW_INCLUDE_DIR} launcher)
  target_link_libraries(tpw_launcher PRIVATE tpw tpw_platform)

  # What can be checked of the platform layer without a display (path mapping, key
  # tables, string helpers): ctest runs it.
  enable_testing()
  add_executable(tpw_platform_tests platform/tests/platform_tests.cpp)
  # TestSound drives the real CSprocketsModule (platform/sound_module.cpp) over the real TbSoundSystemModule base; the pipe
  # channel it pulls from is the test program's own fake, so the game library is not linked.
  target_sources(tpw_platform_tests PRIVATE ${TPW_DECOMP_DIR}/src/snd_handles.cpp ${TPW_DECOMP_DIR}/src/TbSoundSystemModule.cpp)
  target_link_libraries(tpw_platform_tests PRIVATE tpw_platform)
  add_test(NAME platform COMMAND tpw_platform_tests)
else()
  message(STATUS "tpw: launcher not built (needs SDL3, OpenGL and Python 3)")
endif()

if(NOT TARGET tpw_launcher)
  message(WARNING "tpw-port: the launcher needs SDL3, OpenGL and Python 3")
endif()

# The fixes, in every target of the port and of the submodule: a source under overrides/ is built in place of the
# submodule's file of the same path, and the submodule's include/ and platform/ folders are replaced by their copies with
# the overrides laid over them (TPW_OVERLAY_DIR, above).
file(GLOB_RECURSE TPW_OVERRIDE_SOURCES CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/overrides
     ${CMAKE_CURRENT_SOURCE_DIR}/overrides/*.cpp)
set(TPW_OVERRIDES_USED)
string(LENGTH "${TPW_DECOMP_DIR}/" prefix_length)
get_property(TPW_DECOMP_TARGETS DIRECTORY ${TPW_DECOMP_DIR} PROPERTY BUILDSYSTEM_TARGETS)
get_property(TPW_PORT_TARGETS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
foreach(target ${TPW_DECOMP_TARGETS} ${TPW_PORT_TARGETS})
  get_target_property(type ${target} TYPE)
  if(type STREQUAL "UTILITY")
    continue()
  endif()
  get_target_property(sources ${target} SOURCES)
  if(sources)
    set(new_sources)
    foreach(source ${sources})
      if(NOT IS_ABSOLUTE "${source}" AND target IN_LIST TPW_DECOMP_TARGETS AND NOT source MATCHES "^\\$<")
        set(source ${TPW_DECOMP_DIR}/${source})
      endif()
      string(SUBSTRING "${source}" 0 ${prefix_length} prefix)
      if(prefix STREQUAL "${TPW_DECOMP_DIR}/")
        string(SUBSTRING "${source}" ${prefix_length} -1 rel)
        if(rel IN_LIST TPW_OVERRIDE_SOURCES)
          set(source ${CMAKE_CURRENT_SOURCE_DIR}/overrides/${rel})
          list(APPEND TPW_OVERRIDES_USED ${rel})
        endif()
      endif()
      list(APPEND new_sources ${source})
    endforeach()
    set_property(TARGET ${target} PROPERTY SOURCES ${new_sources})
  endif()
  set(props INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT type STREQUAL "INTERFACE_LIBRARY")
    list(APPEND props INCLUDE_DIRECTORIES)
  endif()
  foreach(prop ${props})
    get_target_property(dirs ${target} ${prop})
    if(NOT dirs)
      continue()
    endif()
    set(new_dirs)
    foreach(dir ${dirs})
      if(dir STREQUAL "${TPW_DECOMP_DIR}/include")
        set(dir ${TPW_OVERLAY_DIR}/include)
      elseif(dir STREQUAL "${TPW_DECOMP_DIR}/platform")
        set(dir ${TPW_OVERLAY_DIR}/platform)
      endif()
      string(SUBSTRING "${dir}" 0 ${prefix_length} prefix)
      if(dir STREQUAL "${TPW_DECOMP_DIR}" OR prefix STREQUAL "${TPW_DECOMP_DIR}/")
        message(FATAL_ERROR "tpw-port: ${target} reads headers from ${dir}, past the overrides")
      endif()
      list(APPEND new_dirs ${dir})
    endforeach()
    set_property(TARGET ${target} PROPERTY ${prop} ${new_dirs})
  endforeach()
endforeach()
foreach(rel ${TPW_OVERRIDE_SOURCES})
  if(NOT rel IN_LIST TPW_OVERRIDES_USED)
    message(FATAL_ERROR "overrides/${rel} has no file in tpw-decomp to replace")
  endif()
endforeach()
