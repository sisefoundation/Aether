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

# The ASWF sample registers its hello tests from src/CMakeLists.txt.
# AETHER keeps this Qt Test executable in tests/.
# FFmpeg reaches it only through aether_media. The test does not add another prefix.

add_executable(aether_slice0a_tests
  main.cpp
  timing_tests.cpp
  project_tests.cpp
  sequence_tests.cpp
  timeline_workspace_tests.cpp
  media_import_tests.cpp
  ui_integration_tests.cpp
  timeline_placement_tests.cpp
  mark_tests.cpp
  media_probe_tests.cpp
  overwrite_tests.cpp
  split_tests.cpp
  lift_tests.cpp
  extract_tests.cpp
  ui_completion_tests.cpp
  playback_tests.cpp
)
target_include_directories(aether_slice0a_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(aether_slice0a_tests PRIVATE aether_session aether_timing aether_ui Qt6::Test)
target_compile_definitions(aether_slice0a_tests PRIVATE AETHER_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
aether_ffmpeg_rpath(aether_slice0a_tests)

add_test(NAME slice0a COMMAND aether_slice0a_tests)
add_test(NAME aether_ffmpeg_resolution
  COMMAND ${CMAKE_COMMAND}
    -DBINARY=$<TARGET_FILE:aether>
    -DPREFIX=/opt/aether-ffmpeg-9.1
    -P ${CMAKE_SOURCE_DIR}/cmake/check_ffmpeg_resolution.cmake)
