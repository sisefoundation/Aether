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

#include "aether/media/probe.hpp"

#include <string>

namespace aether {
namespace {

std::string rationalText(const Rational& rational) {
  return std::to_string(rational.numerator) + "/" + std::to_string(rational.denominator);
}

std::string timestampText(const char* label, const Timestamp& stamp) {
  if (!stamp.present) {
    return std::string(label) + " unknown\n";
  }
  std::string line = std::string(label) + " " + std::to_string(stamp.ticks) + " ticks timebase " +
                     rationalText(stamp.timebase) + " " + provenanceLabel(stamp.provenance);
  if (!stamp.note.empty()) {
    line += " (" + stamp.note + ")";
  }
  return line + "\n";
}

void appendRate(std::string& text, const RateCandidate& rate) {
  if (!rate.present) {
    text += rate.role + " unknown\n";
    return;
  }
  text += rate.role + " " + rationalText(rate.rate) + " " + provenanceLabel(rate.provenance);
  if (!rate.caveat.empty()) {
    text += " — " + rate.caveat;
  }
  text += "\n";
}

}  // namespace

const char* provenanceLabel(Provenance provenance) {
  switch (provenance) {
    case Provenance::ReportedByStream:
      return "reported by stream";
    case Provenance::ReportedByContainer:
      return "reported by container";
    case Provenance::CalculatedFromReportedExact:
      return "calculated from reported exact values";
    case Provenance::Estimated:
      return "estimated";
    case Provenance::Unknown:
      return "unknown";
    case Provenance::Conflicting:
      return "conflicting";
    case Provenance::RequiresValidation:
      return "requires validation";
  }
  return "unknown";
}

const char* completionLabel(ProbeCompletion completion) {
  switch (completion) {
    case ProbeCompletion::NotProbed:
      return "not probed";
    case ProbeCompletion::Probing:
      return "probing";
    case ProbeCompletion::Complete:
      return "complete";
    case ProbeCompletion::Incomplete:
      return "incomplete";
    case ProbeCompletion::Failed:
      return "failed";
  }
  return "unknown";
}

std::string formatProbeReport(const ProbeReport& report) {
  std::string text;
  text += "completion ";
  text += completionLabel(report.completion);
  text += "\n";
  text += "container name ";
  text += report.containerName.empty() ? "unknown" : report.containerName;
  text += "\ncontainer long name ";
  text += report.containerLongName.empty() ? "unknown" : report.containerLongName;
  text += "\n";
  text += timestampText("container duration", report.containerDuration);
  if (!report.durationEstimation.empty()) {
    text += "duration estimation method " + report.durationEstimation + "\n";
  }
  if (report.fileSizePresent) {
    text += "file size " + std::to_string(report.fileSizeBytes) + " bytes\n";
  } else {
    text += "file size unknown\n";
  }
  text += "mtime ";
  text += report.mtimeUtc.empty() ? "unknown" : report.mtimeUtc;
  text += "\n";
  text += report.sourceRateVerified ? "verified source rate: yes\n" : "verified source rate: no\n";
  if (!report.sourceRateNote.empty()) {
    text += report.sourceRateNote + "\n";
  }
  if (report.errors.empty()) {
    text += "errors none\n";
  }
  for (const std::string& error : report.errors) {
    text += "error " + error + "\n";
  }
  for (const std::string& discrepancy : report.discrepancies) {
    text += "discrepancy " + discrepancy + "\n";
  }
  for (const VideoStream& stream : report.video) {
    text += "video stream " + std::to_string(stream.index) + "\n";
    text += "codec name " + (stream.codecName.empty() ? std::string("unknown") : stream.codecName) + "\n";
    text += stream.codecIdPresent ? "codec id " + std::to_string(stream.codecId) + "\n" : "codec id unknown\n";
    text += stream.widthPresent ? "width " + std::to_string(stream.width) + "\n" : "width unknown\n";
    text += stream.heightPresent ? "height " + std::to_string(stream.height) + "\n" : "height unknown\n";
    text += "pixel format " + (stream.pixelFormat.empty() ? std::string("unknown") : stream.pixelFormat) + "\n";
    if (stream.sampleAspectPresent) {
      text += "sample aspect ratio " + rationalText(stream.sampleAspect) + " " +
              provenanceLabel(stream.sampleAspectProvenance) + "\n";
    } else {
      text += "sample aspect ratio unknown";
      if (stream.sampleAspectProvenance == Provenance::Conflicting) {
        text += " conflicting";
      }
      text += "\n";
    }
    if (!stream.sampleAspectNote.empty()) {
      text += stream.sampleAspectNote + "\n";
    }
    if (stream.displayAspectPresent) {
      text += "display aspect ratio " + rationalText(stream.displayAspect) + " " +
              provenanceLabel(stream.displayAspectProvenance) + "\n";
    } else {
      text += "display aspect ratio unknown\n";
    }
    if (!stream.displayAspectNote.empty()) {
      text += stream.displayAspectNote + "\n";
    }
    if (stream.timebasePresent) {
      text += "timebase " + rationalText(stream.timebase) + " " + provenanceLabel(stream.timebaseProvenance) + "\n";
    } else {
      text += "timebase unknown\n";
    }
    appendRate(text, stream.averageFrameRate);
    appendRate(text, stream.nominalFrameRate);
    appendRate(text, stream.codecHeaderRate);
    text += stream.ratesConflict ? "frame rate candidates conflict\n" : "frame rate candidates do not conflict\n";
    if (!stream.rateNote.empty()) {
      text += stream.rateNote + "\n";
    }
    text += timestampText("start", stream.start);
    text += timestampText("duration", stream.duration);
    if (stream.frameCountPresent) {
      text += "frame count " + std::to_string(stream.frameCount) + " " +
              provenanceLabel(stream.frameCountProvenance) + "\n";
    } else {
      text += "frame count unknown\n";
    }
  }
  for (const AudioStream& stream : report.audio) {
    text += "audio stream " + std::to_string(stream.index) + "\n";
    text += "codec name " + (stream.codecName.empty() ? std::string("unknown") : stream.codecName) + "\n";
    text += stream.codecIdPresent ? "codec id " + std::to_string(stream.codecId) + "\n" : "codec id unknown\n";
    text += stream.sampleRatePresent ? "sample rate " + std::to_string(stream.sampleRate) + "\n"
                                     : "sample rate unknown\n";
    text += stream.channelCountPresent ? "channel count " + std::to_string(stream.channelCount) + "\n"
                                       : "channel count unknown\n";
    text += "channel layout " + (stream.channelLayout.empty() ? std::string("unknown") : stream.channelLayout) + "\n";
    text += "sample format " + (stream.sampleFormat.empty() ? std::string("unknown") : stream.sampleFormat) + "\n";
    if (stream.timebasePresent) {
      text += "timebase " + rationalText(stream.timebase) + " " + provenanceLabel(stream.timebaseProvenance) + "\n";
    } else {
      text += "timebase unknown\n";
    }
    text += timestampText("start", stream.start);
    text += timestampText("duration", stream.duration);
  }
  for (const OtherStream& stream : report.other) {
    text += stream.kind + " stream " + std::to_string(stream.index) + "\n";
    text += "codec name " + (stream.codecName.empty() ? std::string("unknown") : stream.codecName) + "\n";
    text += stream.codecIdPresent ? "codec id " + std::to_string(stream.codecId) + "\n" : "codec id unknown\n";
    if (stream.timebasePresent) {
      text += "timebase " + rationalText(stream.timebase) + "\n";
    } else {
      text += "timebase unknown\n";
    }
    text += timestampText("start", stream.start);
    text += timestampText("duration", stream.duration);
  }
  if (report.video.empty() && report.audio.empty() && report.other.empty() &&
      report.completion != ProbeCompletion::Failed) {
    text += "streams none reported\n";
  }
  return text;
}

}  // namespace aether
