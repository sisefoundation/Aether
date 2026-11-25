// Copyright 2026 Society of Indian Screen Editors Foundation
//
// AETHER
// Architect: Subhajit Singha, SISE
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

// Framework-independent probe facts. This header does not include FFmpeg.
// A probe result is not editorial time and is not a verified source rate.

#include <cstdint>
#include <string>
#include <vector>

namespace aether {

enum class Provenance {
  ReportedByStream,
  ReportedByContainer,
  CalculatedFromReportedExact,
  Estimated,
  Unknown,
  Conflicting,
  RequiresValidation
};

enum class ProbeCompletion {
  NotProbed,
  Probing,
  Complete,
  Incomplete,
  Failed
};

struct Rational {
  std::int64_t numerator = 0;
  std::int64_t denominator = 0;
};

struct Timestamp {
  bool present = false;
  std::int64_t ticks = 0;
  Rational timebase{};
  Provenance provenance = Provenance::Unknown;
  std::string note;
};

struct RateCandidate {
  bool present = false;
  Rational rate{};
  Provenance provenance = Provenance::Unknown;
  std::string role;
  std::string caveat;
};

struct VideoStream {
  int index = -1;
  std::string codecName;
  bool codecIdPresent = false;
  int codecId = 0;
  bool widthPresent = false;
  int width = 0;
  bool heightPresent = false;
  int height = 0;
  std::string pixelFormat;
  bool sampleAspectPresent = false;
  Rational sampleAspect{};
  Provenance sampleAspectProvenance = Provenance::Unknown;
  std::string sampleAspectNote;
  bool displayAspectPresent = false;
  Rational displayAspect{};
  Provenance displayAspectProvenance = Provenance::Unknown;
  std::string displayAspectNote;
  bool timebasePresent = false;
  Rational timebase{};
  Provenance timebaseProvenance = Provenance::Unknown;
  RateCandidate averageFrameRate;
  RateCandidate nominalFrameRate;
  RateCandidate codecHeaderRate;
  bool ratesConflict = false;
  std::string rateNote;
  Timestamp start;
  Timestamp duration;
  bool frameCountPresent = false;
  std::int64_t frameCount = 0;
  Provenance frameCountProvenance = Provenance::Unknown;
};

struct AudioStream {
  int index = -1;
  std::string codecName;
  bool codecIdPresent = false;
  int codecId = 0;
  bool sampleRatePresent = false;
  int sampleRate = 0;
  bool channelCountPresent = false;
  int channelCount = 0;
  std::string channelLayout;
  std::string sampleFormat;
  bool timebasePresent = false;
  Rational timebase{};
  Provenance timebaseProvenance = Provenance::Unknown;
  Timestamp start;
  Timestamp duration;
};

struct OtherStream {
  int index = -1;
  std::string kind;
  std::string codecName;
  bool codecIdPresent = false;
  int codecId = 0;
  bool timebasePresent = false;
  Rational timebase{};
  Timestamp start;
  Timestamp duration;
};

struct ProbeReport {
  ProbeCompletion completion = ProbeCompletion::NotProbed;
  std::string containerName;
  std::string containerLongName;
  Timestamp containerDuration;
  std::string durationEstimation;
  bool fileSizePresent = false;
  std::int64_t fileSizeBytes = 0;
  std::string mtimeUtc;
  std::vector<std::string> errors;
  std::vector<std::string> discrepancies;
  std::vector<VideoStream> video;
  std::vector<AudioStream> audio;
  std::vector<OtherStream> other;
  bool sourceRateVerified = false;
  std::string sourceRateNote;
};

struct FfmpegLinkIdentity {
  std::string version;
  std::string license;
  std::string configuration;
  std::vector<std::string> loadedLibraries;
};

const char* provenanceLabel(Provenance provenance);
const char* completionLabel(ProbeCompletion completion);
std::string formatProbeReport(const ProbeReport& report);

// Reads one local regular file. Does not write the file, the database, or a source rate.
ProbeReport probeRegisteredRepresentation(const std::string& recordedPath);

// Runtime identity of the linked FFmpeg libraries in this process.
FfmpegLinkIdentity ffmpegLinkIdentity();

}  // namespace aether
