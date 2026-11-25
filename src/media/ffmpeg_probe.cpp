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

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/ffversion.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <thread>

constexpr bool sameLiteral(const char* left, const char* right) {
  while (*left != '\0' && *left == *right) {
    ++left;
    ++right;
  }
  return *left == *right;
}

static_assert(sameLiteral(FFMPEG_VERSION, "9.1-aether-lgpl-candidate"));

#include <sys/stat.h>
#include <unistd.h>

static_assert(LIBAVUTIL_VERSION_MAJOR == 61 && LIBAVUTIL_VERSION_MINOR == 19 && LIBAVUTIL_VERSION_MICRO == 100);
static_assert(LIBAVCODEC_VERSION_MAJOR == 63 && LIBAVCODEC_VERSION_MINOR == 24 && LIBAVCODEC_VERSION_MICRO == 100);
static_assert(LIBAVFORMAT_VERSION_MAJOR == 63 && LIBAVFORMAT_VERSION_MINOR == 16 && LIBAVFORMAT_VERSION_MICRO == 100);
static_assert(LIBSWSCALE_VERSION_MAJOR == 10 && LIBSWSCALE_VERSION_MINOR == 5 && LIBSWSCALE_VERSION_MICRO == 100);
static_assert(LIBSWRESAMPLE_VERSION_MAJOR == 7 && LIBSWRESAMPLE_VERSION_MINOR == 6 && LIBSWRESAMPLE_VERSION_MICRO == 100);

namespace aether {
namespace {

constexpr const char* kSourceRateNote =
    "No verified source rate. avg_frame_rate is not proof of constant frame rate. "
    "r_frame_rate is not a proven frame rate. Probe metadata is not editorial timing. "
    "Placement keeps the explicit source rate.";

struct InputGuard {
  AVFormatContext* context = nullptr;
  ~InputGuard() {
    if (context != nullptr) {
      avformat_close_input(&context);
    }
  }
};

struct DictionaryGuard {
  AVDictionary* dictionary = nullptr;
  ~DictionaryGuard() { av_dict_free(&dictionary); }
};

std::string ffmpegError(int err) {
  char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
  av_strerror(err, buffer, sizeof(buffer));
  return buffer;
}

bool multiply(std::int64_t left, std::int64_t right, std::int64_t* out) {
  if (left == 0 || right == 0) {
    *out = 0;
    return true;
  }
  if (left < 0 || right < 0) {
    return false;
  }
  if (left > std::numeric_limits<std::int64_t>::max() / right) {
    return false;
  }
  *out = left * right;
  return true;
}

std::int64_t gcd(std::int64_t left, std::int64_t right) {
  if (left < 0) {
    left = -left;
  }
  if (right < 0) {
    right = -right;
  }
  while (right != 0) {
    const std::int64_t next = left % right;
    left = right;
    right = next;
  }
  return left == 0 ? 1 : left;
}

Rational reduced(std::int64_t numerator, std::int64_t denominator) {
  const std::int64_t divisor = gcd(numerator, denominator);
  return Rational{numerator / divisor, denominator / divisor};
}

bool sameReduced(Rational left, Rational right) {
  std::int64_t a = 0;
  std::int64_t b = 0;
  if (!multiply(left.numerator, right.denominator, &a) || !multiply(right.numerator, left.denominator, &b)) {
    return false;
  }
  return a == b;
}

bool positiveRatio(AVRational ratio) { return ratio.num > 0 && ratio.den > 0; }

Rational fromAv(AVRational ratio) { return Rational{ratio.num, ratio.den}; }

void waitForCheckpointGate() {
  const char* gate = std::getenv("AETHER_PROBE_GATE");
  if (gate == nullptr || gate[0] == '\0') {
    return;
  }
  for (;;) {
    std::ifstream input(gate);
    std::string line;
    std::getline(input, line);
    if (line == "go") {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }
}

void readFileFacts(const std::string& path, ProbeReport* report) {
  struct stat fileStat {};
  if (stat(path.c_str(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
    return;
  }
  report->fileSizePresent = true;
  report->fileSizeBytes = static_cast<std::int64_t>(fileStat.st_size);
  std::tm utc {};
  if (gmtime_r(&fileStat.st_mtime, &utc) != nullptr) {
    char buffer[32];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc) > 0) {
      report->mtimeUtc = buffer;
    }
  }
}

bool acceptLocalFile(const std::string& path, ProbeReport* report) {
  if (path.empty()) {
    report->errors.push_back("recorded path is empty");
    return false;
  }
  if (path.find("://") != std::string::npos) {
    report->errors.push_back("remote or non-local protocol is not opened");
    return false;
  }
  if (path.front() != '/') {
    report->errors.push_back("path is not a local absolute path");
    return false;
  }
  struct stat fileStat {};
  if (stat(path.c_str(), &fileStat) != 0) {
    report->errors.push_back(errno == ENOENT ? "file is missing" : "file is not readable");
    return false;
  }
  if (!S_ISREG(fileStat.st_mode)) {
    report->errors.push_back("path is not a regular file");
    return false;
  }
  if (access(path.c_str(), R_OK) != 0) {
    report->errors.push_back("file is not readable");
    return false;
  }
  return true;
}

Timestamp timestampFrom(std::int64_t ticks, AVRational timebase, Provenance provenance, const std::string& note) {
  Timestamp stamp;
  if (ticks == AV_NOPTS_VALUE || timebase.den == 0) {
    return stamp;
  }
  stamp.present = true;
  stamp.ticks = ticks;
  stamp.timebase = fromAv(timebase);
  stamp.provenance = provenance;
  stamp.note = note;
  return stamp;
}

RateCandidate rateFrom(AVRational ratio, const char* role, Provenance provenance, const char* caveat) {
  RateCandidate candidate;
  candidate.role = role;
  candidate.caveat = caveat;
  if (!positiveRatio(ratio)) {
    return candidate;
  }
  candidate.present = true;
  candidate.rate = fromAv(ratio);
  candidate.provenance = provenance;
  return candidate;
}

void fillVideo(AVStream* stream, AVCodecParameters* parameters, VideoStream* video) {
  video->index = stream->index;
  const char* codecName = avcodec_get_name(parameters->codec_id);
  if (codecName != nullptr) {
    video->codecName = codecName;
  }
  video->codecIdPresent = true;
  video->codecId = static_cast<int>(parameters->codec_id);
  if (parameters->width > 0) {
    video->widthPresent = true;
    video->width = parameters->width;
  }
  if (parameters->height > 0) {
    video->heightPresent = true;
    video->height = parameters->height;
  }
  if (parameters->format >= 0) {
    const char* pixel = av_get_pix_fmt_name(static_cast<AVPixelFormat>(parameters->format));
    if (pixel != nullptr) {
      video->pixelFormat = pixel;
    }
  }
  const bool streamSar = positiveRatio(stream->sample_aspect_ratio);
  const bool parameterSar = positiveRatio(parameters->sample_aspect_ratio);
  if (streamSar && parameterSar &&
      !sameReduced(fromAv(stream->sample_aspect_ratio), fromAv(parameters->sample_aspect_ratio))) {
    video->sampleAspectProvenance = Provenance::Conflicting;
    video->sampleAspectNote = "sample aspect ratio conflicts: stream " +
                              std::to_string(stream->sample_aspect_ratio.num) + "/" +
                              std::to_string(stream->sample_aspect_ratio.den) + " and codec parameters " +
                              std::to_string(parameters->sample_aspect_ratio.num) + "/" +
                              std::to_string(parameters->sample_aspect_ratio.den) +
                              ". Display aspect ratio was not derived.";
  } else if (streamSar || parameterSar) {
    const AVRational chosen = streamSar ? stream->sample_aspect_ratio : parameters->sample_aspect_ratio;
    video->sampleAspectPresent = true;
    video->sampleAspect = fromAv(chosen);
    video->sampleAspectProvenance = Provenance::ReportedByStream;
    if (video->widthPresent && video->heightPresent) {
      std::int64_t numerator = 0;
      std::int64_t denominator = 0;
      if (multiply(chosen.num, video->width, &numerator) && multiply(chosen.den, video->height, &denominator) &&
          numerator > 0 && denominator > 0) {
        video->displayAspectPresent = true;
        video->displayAspect = reduced(numerator, denominator);
        video->displayAspectProvenance = Provenance::CalculatedFromReportedExact;
        video->displayAspectNote =
            "display aspect ratio calculated from the reported sample aspect ratio and the reported pixel width and "
            "height";
      } else {
        video->displayAspectNote = "display aspect ratio was not derived because the exact product does not fit";
      }
    }
  }

  if (stream->time_base.den != 0) {
    video->timebasePresent = true;
    video->timebase = fromAv(stream->time_base);
    video->timebaseProvenance = Provenance::ReportedByStream;
  }
  video->averageFrameRate =
      rateFrom(stream->avg_frame_rate, "average frame rate candidate", Provenance::ReportedByStream,
               "avg_frame_rate is not proof of constant frame rate and is not a verified source rate");
  video->nominalFrameRate =
      rateFrom(stream->r_frame_rate, "nominal/base rate candidate", Provenance::RequiresValidation,
               "r_frame_rate is not a proven frame rate and is not a verified source rate");
  video->codecHeaderRate = rateFrom(parameters->framerate, "codec header rate candidate", Provenance::ReportedByStream,
                                    "codec header framerate is not a verified source rate");
  const RateCandidate* present[3] = {};
  int count = 0;
  for (const RateCandidate* candidate : {&video->averageFrameRate, &video->nominalFrameRate, &video->codecHeaderRate}) {
    if (candidate->present) {
      present[count++] = candidate;
    }
  }
  for (int i = 0; i < count && !video->ratesConflict; ++i) {
    for (int j = i + 1; j < count; ++j) {
      if (!sameReduced(present[i]->rate, present[j]->rate)) {
        video->ratesConflict = true;
        video->rateNote = "frame rate candidates disagree. None is a verified source rate.";
      }
    }
  }
  video->start = timestampFrom(stream->start_time, stream->time_base, Provenance::ReportedByStream, "");
  video->duration = timestampFrom(stream->duration, stream->time_base, Provenance::ReportedByStream,
                                  "stream duration is not an editorial frame count");
  if (stream->nb_frames > 0) {
    video->frameCountPresent = true;
    video->frameCount = stream->nb_frames;
    video->frameCountProvenance = Provenance::ReportedByStream;
  }
}

void fillAudio(AVStream* stream, AVCodecParameters* parameters, AudioStream* audio) {
  audio->index = stream->index;
  const char* codecName = avcodec_get_name(parameters->codec_id);
  if (codecName != nullptr) {
    audio->codecName = codecName;
  }
  audio->codecIdPresent = true;
  audio->codecId = static_cast<int>(parameters->codec_id);
  if (parameters->sample_rate > 0) {
    audio->sampleRatePresent = true;
    audio->sampleRate = parameters->sample_rate;
  }
  if (av_channel_layout_check(&parameters->ch_layout)) {
    if (parameters->ch_layout.nb_channels > 0) {
      audio->channelCountPresent = true;
      audio->channelCount = parameters->ch_layout.nb_channels;
    }
    char layout[256] = {};
    if (av_channel_layout_describe(&parameters->ch_layout, layout, sizeof(layout)) >= 0 && layout[0] != '\0') {
      audio->channelLayout = layout;
    }
  }
  if (parameters->format >= 0) {
    const char* sample = av_get_sample_fmt_name(static_cast<AVSampleFormat>(parameters->format));
    if (sample != nullptr) {
      audio->sampleFormat = sample;
    }
  }
  if (stream->time_base.den != 0) {
    audio->timebasePresent = true;
    audio->timebase = fromAv(stream->time_base);
    audio->timebaseProvenance = Provenance::ReportedByStream;
  }
  audio->start = timestampFrom(stream->start_time, stream->time_base, Provenance::ReportedByStream, "");
  audio->duration = timestampFrom(stream->duration, stream->time_base, Provenance::ReportedByStream, "");
}

void fillOther(AVStream* stream, AVCodecParameters* parameters, const char* kind, OtherStream* other) {
  other->index = stream->index;
  other->kind = kind;
  const char* codecName = avcodec_get_name(parameters->codec_id);
  if (codecName != nullptr) {
    other->codecName = codecName;
  }
  other->codecIdPresent = true;
  other->codecId = static_cast<int>(parameters->codec_id);
  if (stream->time_base.den != 0) {
    other->timebasePresent = true;
    other->timebase = fromAv(stream->time_base);
  }
  other->start = timestampFrom(stream->start_time, stream->time_base, Provenance::ReportedByStream, "");
  other->duration = timestampFrom(stream->duration, stream->time_base, Provenance::ReportedByStream, "");
}

void noteDurationDisagreement(const Timestamp& container, const Timestamp& stream, int index, ProbeReport* report) {
  if (!container.present || !stream.present || container.timebase.denominator == 0 || stream.timebase.denominator == 0) {
    return;
  }
  std::int64_t left = 0;
  std::int64_t right = 0;
  const bool leftOk = multiply(stream.ticks, stream.timebase.numerator, &left) &&
                      multiply(left, container.timebase.denominator, &left);
  const bool rightOk = multiply(container.ticks, container.timebase.numerator, &right) &&
                       multiply(right, stream.timebase.denominator, &right);
  if (!leftOk || !rightOk) {
    report->discrepancies.push_back("stream " + std::to_string(index) +
                                    " duration and the container duration could not be compared exactly. Both values "
                                    "stay visible. Neither becomes editorial time.");
    return;
  }
  if (left != right) {
    report->discrepancies.push_back(
        "stream " + std::to_string(index) + " duration " + std::to_string(stream.ticks) + " ticks of " +
        std::to_string(stream.timebase.numerator) + "/" + std::to_string(stream.timebase.denominator) +
        " differs from container duration " + std::to_string(container.ticks) + " ticks of " +
        std::to_string(container.timebase.numerator) + "/" + std::to_string(container.timebase.denominator) +
        ". Neither value is a verified source duration.");
  }
}

std::vector<std::string> mappedLibraries() {
  std::vector<std::string> libraries;
  std::ifstream maps("/proc/self/maps");
  std::string line;
  while (std::getline(maps, line)) {
    const bool ffmpeg = line.find("libavutil.so") != std::string::npos ||
                        line.find("libavcodec.so") != std::string::npos ||
                        line.find("libavformat.so") != std::string::npos ||
                        line.find("libswscale.so") != std::string::npos ||
                        line.find("libswresample.so") != std::string::npos ||
                        line.find("libavfilter.so") != std::string::npos;
    if (!ffmpeg) {
      continue;
    }
    const auto slash = line.rfind(' ');
    if (slash == std::string::npos) {
      continue;
    }
    const std::string path = line.substr(slash + 1);
    if (path.empty()) {
      continue;
    }
    bool seen = false;
    for (const std::string& earlier : libraries) {
      if (earlier == path) {
        seen = true;
      }
    }
    if (!seen) {
      libraries.push_back(path);
    }
  }
  return libraries;
}

}  // namespace

ProbeReport probeRegisteredRepresentation(const std::string& recordedPath) {
  waitForCheckpointGate();
  ProbeReport report;
  report.sourceRateVerified = false;
  report.sourceRateNote = kSourceRateNote;
  readFileFacts(recordedPath, &report);
  if (!acceptLocalFile(recordedPath, &report)) {
    report.completion = ProbeCompletion::Failed;
    return report;
  }

  DictionaryGuard options;
  if (av_dict_set(&options.dictionary, "protocol_whitelist", "file", 0) < 0) {
    report.completion = ProbeCompletion::Failed;
    report.errors.push_back("protocol whitelist could not be set");
    return report;
  }
  AVFormatContext* context = nullptr;
  const int opened = avformat_open_input(&context, recordedPath.c_str(), nullptr, &options.dictionary);
  const bool whitelistIgnored = av_dict_get(options.dictionary, "protocol_whitelist", nullptr, 0) != nullptr;
  if (opened < 0) {
    report.completion = ProbeCompletion::Failed;
    report.errors.push_back(ffmpegError(opened));
    return report;
  }
  InputGuard guard{context};
  if (whitelistIgnored) {
    report.completion = ProbeCompletion::Failed;
    report.errors.push_back("protocol whitelist was not applied; the file was closed");
    return report;
  }
  const char* url = context->url != nullptr ? context->url : recordedPath.c_str();
  const char* protocol = avio_find_protocol_name(url);
  if (protocol == nullptr || std::strcmp(protocol, "file") != 0) {
    report.completion = ProbeCompletion::Failed;
    report.errors.push_back(std::string("refusing protocol ") + (protocol == nullptr ? "unknown" : protocol));
    return report;
  }

  if (context->iformat != nullptr) {
    if (context->iformat->name != nullptr) {
      report.containerName = context->iformat->name;
    }
    if (context->iformat->long_name != nullptr) {
      report.containerLongName = context->iformat->long_name;
    }
  }
  const int info = avformat_find_stream_info(context, nullptr);
  report.completion = info < 0 ? ProbeCompletion::Incomplete : ProbeCompletion::Complete;
  if (info < 0) {
    report.errors.push_back(ffmpegError(info));
  }

  if (context->duration == AV_NOPTS_VALUE) {
    report.containerDuration.provenance = Provenance::Unknown;
  } else {
    Provenance provenance = Provenance::ReportedByContainer;
    std::string note = "container duration in AV_TIME_BASE ticks. Not editorial time.";
    if (context->duration_estimation_method == AVFMT_DURATION_FROM_BITRATE) {
      provenance = Provenance::Estimated;
      note = "AVFMT_DURATION_FROM_BITRATE. Estimated. Not editorial time.";
      report.durationEstimation = "AVFMT_DURATION_FROM_BITRATE";
    } else if (context->duration_estimation_method == AVFMT_DURATION_FROM_PTS) {
      provenance = Provenance::CalculatedFromReportedExact;
      note = "AVFMT_DURATION_FROM_PTS. Not a verified editorial duration.";
      report.durationEstimation = "AVFMT_DURATION_FROM_PTS";
    } else {
      note = "AVFMT_DURATION_FROM_STREAM. Not a verified editorial duration.";
      report.durationEstimation = "AVFMT_DURATION_FROM_STREAM";
    }
    report.containerDuration =
        timestampFrom(context->duration, AVRational{1, AV_TIME_BASE}, provenance, note);
  }

  for (unsigned index = 0; index < context->nb_streams; ++index) {
    AVStream* stream = context->streams[index];
    AVCodecParameters* parameters = stream->codecpar;
    if (parameters == nullptr) {
      continue;
    }
    if ((stream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
      OtherStream other;
      fillOther(stream, parameters, "attachment", &other);
      report.other.push_back(std::move(other));
    } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      VideoStream video;
      fillVideo(stream, parameters, &video);
      if (context->duration_estimation_method == AVFMT_DURATION_FROM_BITRATE && video.duration.present) {
        video.duration.provenance = Provenance::Estimated;
        video.duration.note = "stream duration may be estimated from bitrate. Not an editorial frame count.";
      }
      noteDurationDisagreement(report.containerDuration, video.duration, video.index, &report);
      report.video.push_back(std::move(video));
    } else if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
      AudioStream audio;
      fillAudio(stream, parameters, &audio);
      if (context->duration_estimation_method == AVFMT_DURATION_FROM_BITRATE && audio.duration.present) {
        audio.duration.provenance = Provenance::Estimated;
        audio.duration.note = "stream duration may be estimated from bitrate.";
      }
      noteDurationDisagreement(report.containerDuration, audio.duration, audio.index, &report);
      report.audio.push_back(std::move(audio));
    } else if (parameters->codec_type == AVMEDIA_TYPE_SUBTITLE) {
      OtherStream other;
      fillOther(stream, parameters, "subtitle", &other);
      report.other.push_back(std::move(other));
    } else if (parameters->codec_type == AVMEDIA_TYPE_DATA) {
      OtherStream other;
      fillOther(stream, parameters, "data", &other);
      report.other.push_back(std::move(other));
    } else if (parameters->codec_type == AVMEDIA_TYPE_ATTACHMENT) {
      OtherStream other;
      fillOther(stream, parameters, "attachment", &other);
      report.other.push_back(std::move(other));
    } else {
      OtherStream other;
      fillOther(stream, parameters, "unknown", &other);
      report.other.push_back(std::move(other));
    }
  }
  return report;
}

FfmpegLinkIdentity ffmpegLinkIdentity() {
  FfmpegLinkIdentity identity;
  const char* version = av_version_info();
  identity.version = version == nullptr ? "" : version;
  const char* license = avutil_license();
  identity.license = license == nullptr ? "" : license;
  const char* configuration = avutil_configuration();
  identity.configuration = configuration == nullptr ? "" : configuration;
  avcodec_version();
  avformat_version();
  swscale_version();
  swresample_version();
  identity.loadedLibraries = mappedLibraries();
  return identity;
}

}  // namespace aether
