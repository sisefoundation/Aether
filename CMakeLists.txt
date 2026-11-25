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

# Slice 0A product scaffold, arranged like the ASWF sample project:
# this file, then include/ and src/. Tests stay in tests/.
# C++17 is prototype-only. ADR-001 does not set a language standard.
# The sample project's hello target uses C++14. AETHER does not.
# FFmpeg is the development-only prefix /opt/aether-ffmpeg-9.1.
# The harness under tools/validation/ffmpeg_link_smoke_test/ is not part of this build.
# GStreamer is not searched. ADR-008 stays open.

cmake_minimum_required(VERSION 3.24)

project(aether VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 QUIET)
find_package(Qt6 QUIET COMPONENTS Widgets Test)

set(_aether_missing "")
if(NOT SQLite3_FOUND)
  list(APPEND _aether_missing "sqlite-devel")
endif()
if(NOT Qt6_FOUND)
  list(APPEND _aether_missing "qt6-qtbase-devel")
endif()
if(_aether_missing)
  string(JOIN ", " _aether_missing_text ${_aether_missing})
  message(FATAL_ERROR
    "Slice 0A cannot configure. Missing Fedora development packages: ${_aether_missing_text}\n"
    "sqlite-devel provides the sqlite3.h header and the libsqlite3 link library. "
    "The runtime package sqlite-libs is not sufficient.\n"
    "qt6-qtbase-devel provides Qt 6 Widgets, Qt 6 Test, and Qt6Config.cmake. "
    "The runtime package qt6-qtbase is not sufficient.\n"
    "This build does not vendor Qt or SQLite and does not substitute another UI toolkit, "
    "database, or test framework.")
endif()

include(cmake/AetherFfmpeg.cmake)

# CMake 4.3 FindSQLite3 exports SQLite3::SQLite3. SQLite::SQLite3 is the
# deprecated name. This project does not alias one to the other.
if(NOT TARGET SQLite3::SQLite3)
  message(FATAL_ERROR
    "Slice 0B cannot configure. find_package(SQLite3) did not export the imported target SQLite3::SQLite3.\n"
    "That target is what CMake 4.3 FindSQLite3 creates. This build does not fall back to the deprecated SQLite::SQLite3 name and does not add an alias.")
endif()

message(STATUS "Slice 0A SQLite3 ${SQLite3_VERSION}")
message(STATUS "Slice 0B links imported target SQLite3::SQLite3")
message(STATUS "Slice 0A Qt6 ${Qt6_VERSION}")
message(STATUS "Slice 0A C++ standard is prototype-only C++17")

set(CMAKE_AUTOMOC ON)
add_compile_options(-Wall -Wextra -Wpedantic)

include(CTest)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)
