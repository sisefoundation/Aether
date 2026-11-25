# Development-only dynamic link against the pinned LGPL FFmpeg 9.1 prefix.
# ADR-008 stays open. This is not distribution approval and not a package.

set(AETHER_FFMPEG_ROOT "/opt/aether-ffmpeg-9.1" CACHE PATH
  "Development-only FFmpeg 9.1 prefix. No other prefix is accepted.")

if(NOT AETHER_FFMPEG_ROOT STREQUAL "/opt/aether-ffmpeg-9.1")
  message(FATAL_ERROR
    "AETHER_FFMPEG_ROOT must be exactly /opt/aether-ffmpeg-9.1.\n"
    "Got '${AETHER_FFMPEG_ROOT}'. This build does not fall back to /usr or to FFmpeg 9.0.2.")
endif()

if(NOT EXISTS "${AETHER_FFMPEG_ROOT}/include/libavutil/ffversion.h")
  message(FATAL_ERROR
    "AETHER_FFMPEG_ROOT=${AETHER_FFMPEG_ROOT} is missing or is not an FFmpeg development prefix.\n"
    "Expected ${AETHER_FFMPEG_ROOT}/include/libavutil/ffversion.h.\n"
    "The system FFmpeg 9.0.2 install is not a substitute.")
endif()

file(READ "${AETHER_FFMPEG_ROOT}/include/libavutil/ffversion.h" _aether_ffversion)
if(NOT _aether_ffversion MATCHES "9\\.1-aether-lgpl-candidate")
  message(FATAL_ERROR
    "The headers in ${AETHER_FFMPEG_ROOT} are not FFmpeg 9.1-aether-lgpl-candidate.\n"
    "Refusing to mix them with another FFmpeg.")
endif()

foreach(_pair IN ITEMS libavutil libavcodec libavformat libswscale libswresample)
  if(EXISTS "${AETHER_FFMPEG_ROOT}/lib/${_pair}.a")
    message(FATAL_ERROR "Refusing static archive ${AETHER_FFMPEG_ROOT}/lib/${_pair}.a")
  endif()
  if(NOT EXISTS "${AETHER_FFMPEG_ROOT}/lib/${_pair}.so")
    message(FATAL_ERROR "Missing shared library ${AETHER_FFMPEG_ROOT}/lib/${_pair}.so")
  endif()
endforeach()

if(EXISTS "${AETHER_FFMPEG_ROOT}/lib/libavfilter.so" OR
   EXISTS "${AETHER_FFMPEG_ROOT}/lib/pkgconfig/libavfilter.pc" OR
   EXISTS "${AETHER_FFMPEG_ROOT}/include/libavfilter")
  message(FATAL_ERROR "libavfilter is in ${AETHER_FFMPEG_ROOT} and is not approved for this link.")
endif()

find_program(AETHER_PKG_CONFIG pkg-config REQUIRED)
set(_module_names libavutil libavcodec libavformat libswscale libswresample)
set(_module_versions 61.19.100 63.24.100 63.16.100 10.5.100 7.6.100)
list(LENGTH _module_names _module_count)
math(EXPR _module_last "${_module_count} - 1")
foreach(_index RANGE ${_module_last})
  list(GET _module_names ${_index} _module)
  list(GET _module_versions ${_index} _expect)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
      PKG_CONFIG_PATH=
      PKG_CONFIG_LIBDIR=${AETHER_FFMPEG_ROOT}/lib/pkgconfig
      ${AETHER_PKG_CONFIG} --modversion ${_module}
    OUTPUT_VARIABLE _got
    ERROR_VARIABLE _err
    RESULT_VARIABLE _rc
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT _rc EQUAL 0 OR NOT _got STREQUAL _expect)
    message(FATAL_ERROR
      "Isolated pkg-config for ${_module} failed or is not ${_expect}.\n"
      "version='${_got}' exit=${_rc}\n${_err}\n"
      "PKG_CONFIG_PATH is cleared. The system 9.0.2 .pc files are not used.")
  endif()
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
      PKG_CONFIG_PATH=
      PKG_CONFIG_LIBDIR=${AETHER_FFMPEG_ROOT}/lib/pkgconfig
      ${AETHER_PKG_CONFIG} --variable=prefix ${_module}
    OUTPUT_VARIABLE _prefix
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT _prefix STREQUAL "${AETHER_FFMPEG_ROOT}")
    message(FATAL_ERROR "pkg-config prefix for ${_module} is '${_prefix}', not ${AETHER_FFMPEG_ROOT}.")
  endif()
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
      PKG_CONFIG_PATH=
      PKG_CONFIG_LIBDIR=${AETHER_FFMPEG_ROOT}/lib/pkgconfig
      ${AETHER_PKG_CONFIG} --cflags --libs ${_module}
    OUTPUT_VARIABLE _flags
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(_flags MATCHES "/usr")
    message(FATAL_ERROR "pkg-config flags for ${_module} point at /usr:\n${_flags}")
  endif()
endforeach()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E env
    PKG_CONFIG_PATH=
    PKG_CONFIG_LIBDIR=${AETHER_FFMPEG_ROOT}/lib/pkgconfig
    ${AETHER_PKG_CONFIG} --exists libavfilter
  RESULT_VARIABLE _avfilter_rc)
if(_avfilter_rc EQUAL 0)
  message(FATAL_ERROR "Isolated pkg-config found libavfilter. It is not linked.")
endif()

set(AETHER_FFMPEG_INCLUDE "${AETHER_FFMPEG_ROOT}/include")
set(AETHER_FFMPEG_LIBRARIES
  "${AETHER_FFMPEG_ROOT}/lib/libavformat.so"
  "${AETHER_FFMPEG_ROOT}/lib/libavcodec.so"
  "${AETHER_FFMPEG_ROOT}/lib/libswscale.so"
  "${AETHER_FFMPEG_ROOT}/lib/libswresample.so"
  "${AETHER_FFMPEG_ROOT}/lib/libavutil.so")

try_run(AETHER_FFMPEG_RUN AETHER_FFMPEG_COMPILE
  "${CMAKE_BINARY_DIR}/CMakeFiles/ffmpeg-prefix-check"
  "${CMAKE_SOURCE_DIR}/cmake/ffmpeg_prefix_check.cpp"
  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${AETHER_FFMPEG_INCLUDE}"
  LINK_OPTIONS "LINKER:--disable-new-dtags" "LINKER:-rpath,${AETHER_FFMPEG_ROOT}/lib"
  LINK_LIBRARIES ${AETHER_FFMPEG_LIBRARIES}
  COMPILE_OUTPUT_VARIABLE AETHER_FFMPEG_COMPILE_LOG
  RUN_OUTPUT_VARIABLE AETHER_FFMPEG_RUN_LOG)
if(NOT AETHER_FFMPEG_COMPILE)
  message(FATAL_ERROR
    "The FFmpeg 9.1 prefix check did not compile against ${AETHER_FFMPEG_ROOT}.\n"
    "${AETHER_FFMPEG_COMPILE_LOG}")
endif()
if(NOT AETHER_FFMPEG_RUN EQUAL 0)
  message(FATAL_ERROR
    "The FFmpeg 9.1 prefix check failed at runtime (exit ${AETHER_FFMPEG_RUN}).\n"
    "${AETHER_FFMPEG_RUN_LOG}\n"
    "No Aether binary is linked when this check fails.")
endif()

function(aether_ffmpeg_rpath target)
  set_target_properties(${target} PROPERTIES
    BUILD_RPATH "${AETHER_FFMPEG_ROOT}/lib"
    INSTALL_RPATH "")
  target_link_options(${target} PRIVATE "LINKER:--disable-new-dtags")
endfunction()

message(STATUS "Development-only FFmpeg ${AETHER_FFMPEG_ROOT} (9.1-aether-lgpl-candidate, LGPL 2.1 or later)")
message(STATUS "ADR-008 remains open. This link is not distribution approval. GStreamer is not searched.")
