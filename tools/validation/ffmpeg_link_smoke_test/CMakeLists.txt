# Copyright 2026 Society of Indian Screen Editors Foundation
#
# AETHER
# Architect: Subhajit Singha, SISE
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Isolated FFmpeg link smoke test. This is not the AETHER application.
# The product build does not add this directory.
# Approved link set only: libavutil, libavcodec, libavformat, libswscale, libswresample.
# libavfilter and libavdevice are intentionally not searched and not linked.

cmake_minimum_required(VERSION 3.16)
project(aether_ffmpeg_link_smoke_test LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AETHER_FFMPEG_SMOKE REQUIRED IMPORTED_TARGET
    libavutil
    libavcodec
    libavformat
    libswscale
    libswresample)

add_executable(ffmpeg_link_smoke_test main.c)
target_link_libraries(ffmpeg_link_smoke_test PRIVATE PkgConfig::AETHER_FFMPEG_SMOKE)
target_compile_features(ffmpeg_link_smoke_test PRIVATE c_std_11)

# Refuse a static archive if the .pc files ever point at one. AETHER's
# accepted direction is dynamic linking of this set.
get_target_property(_smoke_libs PkgConfig::AETHER_FFMPEG_SMOKE INTERFACE_LINK_LIBRARIES)
if(_smoke_libs MATCHES "\\.a([; ]|$)")
    message(FATAL_ERROR "ffmpeg_link_smoke_test refuses a static FFmpeg link")
endif()
