if(NOT DEFINED BINARY OR NOT DEFINED PREFIX)
  message(FATAL_ERROR "check_ffmpeg_resolution.cmake requires BINARY and PREFIX")
endif()

execute_process(COMMAND ldd "${BINARY}"
  OUTPUT_VARIABLE _ldd
  ERROR_VARIABLE _ldd_err
  RESULT_VARIABLE _ldd_rc)
if(NOT _ldd_rc EQUAL 0)
  message(FATAL_ERROR "ldd ${BINARY} failed (${_ldd_rc}): ${_ldd_err}")
endif()

set(_names libavutil.so libavcodec.so libavformat.so libswscale.so libswresample.so)
foreach(_name IN LISTS _names)
  string(FIND "${_ldd}" "${_name}" _pos)
  if(_pos LESS 0)
    message(FATAL_ERROR "ldd did not resolve ${_name}\n${_ldd}")
  endif()
endforeach()

string(REPLACE "\n" ";" _lines "${_ldd}")
foreach(_line IN LISTS _lines)
  if(_line MATCHES "libav|libsw")
    if(NOT _line MATCHES "/opt/aether-ffmpeg-9.1/")
      message(FATAL_ERROR "FFmpeg library resolved outside ${PREFIX}: ${_line}")
    endif()
    if(_line MATCHES "/usr/")
      message(FATAL_ERROR "FFmpeg library resolved under /usr: ${_line}")
    endif()
  endif()
endforeach()

if(_ldd MATCHES "libavfilter|libavdevice|libpostproc|libx264|libx265|libfdk|libgst")
  message(FATAL_ERROR "ldd contains a library this build must not use\n${_ldd}")
endif()
if(_ldd MATCHES "61\\.1\\.102|63\\.1\\.102|10\\.1\\.102|7\\.1\\.102")
  message(FATAL_ERROR "ldd resolved the system FFmpeg 9.0.2 libraries\n${_ldd}")
endif()

message(STATUS "FFmpeg libraries for ${BINARY} resolve under ${PREFIX}")
