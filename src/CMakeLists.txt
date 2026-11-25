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

add_library(aether_timing STATIC
  core/timing/sample_boundary.cpp
)
target_include_directories(aether_timing PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(aether_persistence STATIC
  core/project/ids.cpp
  core/sequence/sequence.cpp
  persistence/logger.cpp
  persistence/manifest.cpp
  persistence/project_store.cpp
)
target_include_directories(aether_persistence PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aether_persistence PUBLIC SQLite3::SQLite3)

add_library(aether_media STATIC
  media/probe_format.cpp
  media/ffmpeg_probe.cpp
  media/ffmpeg_picture.cpp
)
target_include_directories(aether_media PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(aether_media SYSTEM PRIVATE ${AETHER_FFMPEG_INCLUDE})
target_link_libraries(aether_media PUBLIC ${AETHER_FFMPEG_LIBRARIES})

add_library(aether_session STATIC
  app/project_session.cpp
  core/media/import.cpp
)
target_include_directories(aether_session PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aether_session PUBLIC aether_persistence)

set(CMAKE_AUTORCC ON)
add_library(aether_ui STATIC
  ${CMAKE_SOURCE_DIR}/resources/aether.qrc
  ui/main_window.cpp
  ui/probe_controller.cpp
  ui/placement_dialog.cpp
  ui/project_dialog.cpp
  ui/timeline_workspace.cpp
  ${CMAKE_SOURCE_DIR}/include/aether/ui/main_window.hpp
  ${CMAKE_SOURCE_DIR}/include/aether/ui/probe_controller.hpp
  ${CMAKE_SOURCE_DIR}/include/aether/ui/placement_dialog.hpp
  ${CMAKE_SOURCE_DIR}/include/aether/ui/project_dialog.hpp
  ${CMAKE_SOURCE_DIR}/include/aether/ui/timeline_workspace.hpp
)
target_include_directories(aether_ui PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aether_ui PUBLIC aether_session aether_media Qt6::Widgets)

add_executable(aether
  ui/main.cpp
  ui/probe_checkpoint.cpp
)
target_link_libraries(aether PRIVATE aether_ui)
aether_ffmpeg_rpath(aether)
