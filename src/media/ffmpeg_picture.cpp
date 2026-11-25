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

// The only picture decoder. It reads a local file and returns RGB bytes.
// It does not write the file, does not transcode it, and does not store a probe.

#include "aether/media/picture.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/ffversion.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <utility>

constexpr bool sameLiteral(const char* left, const char* right) {
  while (*left != '\0' && *left == *right) {
    ++left;
    ++right;
  }
  return *left == *right;
}

static_assert(sameLiteral(FFMPEG_VERSION, "9.1-aether-lgpl-candidate"));
static_assert(LIBAVUTIL_VERSION_MAJOR == 61 && LIBAVUTIL_VERSION_MINOR == 19 && LIBAVUTIL_VERSION_MICRO == 100);
static_assert(LIBAVCODEC_VERSION_MAJOR == 63 && LIBAVCODEC_VERSION_MINOR == 24 && LIBAVCODEC_VERSION_MICRO == 100);
static_assert(LIBAVFORMAT_VERSION_MAJOR == 63 && LIBAVFORMAT_VERSION_MINOR == 16 && LIBAVFORMAT_VERSION_MICRO == 100);
static_assert(LIBSWSCALE_VERSION_MAJOR == 10 && LIBSWSCALE_VERSION_MINOR == 5 && LIBSWSCALE_VERSION_MICRO == 100);
static_assert(LIBSWRESAMPLE_VERSION_MAJOR == 7 && LIBSWRESAMPLE_VERSION_MINOR == 6 && LIBSWRESAMPLE_VERSION_MICRO == 100);

namespace aether {
namespace {

std::string avError(int code) {
  char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
  av_strerror(code, buffer, sizeof(buffer));
  return buffer;
}

}  // namespace

struct PictureReader::State {
  AVFormatContext* format = nullptr;
  AVCodecContext* codec = nullptr;
  SwsContext* scaler = nullptr;
  AVPacket* packet = nullptr;
  AVFrame* frame = nullptr;
  int stream = -1;
  int scalerWidth = 0;
  int scalerHeight = 0;
  int scalerFormat = -1;
  AVRational spacing{};
  std::string spacingField;
  std::int64_t nextIndex = 0;
  bool draining = false;
  bool finished = false;
  DisplayPicture cached;

  ~State() { reset(); }

  void reset() {
    if (scaler != nullptr) {
      sws_freeContext(scaler);
      scaler = nullptr;
    }
    if (frame != nullptr) {
      av_frame_free(&frame);
    }
    if (packet != nullptr) {
      av_packet_free(&packet);
    }
    if (codec != nullptr) {
      avcodec_free_context(&codec);
    }
    if (format != nullptr) {
      avformat_close_input(&format);
    }
    stream = -1;
    scalerWidth = 0;
    scalerHeight = 0;
    scalerFormat = -1;
    spacing = AVRational{0, 0};
    spacingField.clear();
    nextIndex = 0;
    draining = false;
    finished = false;
    cached = {};
  }

  void rewind() {
    if (format == nullptr || stream < 0) {
      return;
    }
    av_seek_frame(format, stream, 0, AVSEEK_FLAG_BACKWARD);
    if (codec != nullptr) {
      avcodec_flush_buffers(codec);
    }
    nextIndex = 0;
    draining = false;
    finished = false;
    cached = {};
  }

  bool decodeNext(std::string* note) {
    if (finished) {
      if (note != nullptr) {
        *note = "no further picture";
      }
      return false;
    }
    while (true) {
      const int received = avcodec_receive_frame(codec, frame);
      if (received == 0) {
        DisplayPicture picture;
        picture.width = frame->width;
        picture.height = frame->height;
        picture.mediaIndex = nextIndex;
        const std::int64_t bytes =
            static_cast<std::int64_t>(picture.width) * static_cast<std::int64_t>(picture.height) * 3;
        if (picture.width <= 0 || picture.height <= 0 || bytes <= 0 || bytes > 128LL * 1024LL * 1024LL) {
          if (note != nullptr) {
            *note = "decoded picture size is not usable";
          }
          av_frame_unref(frame);
          return false;
        }
        if (scaler == nullptr || scalerWidth != picture.width || scalerHeight != picture.height ||
            scalerFormat != frame->format) {
          if (scaler != nullptr) {
            sws_freeContext(scaler);
          }
          scaler = sws_getContext(picture.width, picture.height, static_cast<AVPixelFormat>(frame->format),
                                  picture.width, picture.height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr,
                                  nullptr);
          scalerWidth = picture.width;
          scalerHeight = picture.height;
          scalerFormat = frame->format;
        }
        if (scaler == nullptr) {
          const char* name = av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format));
          if (note != nullptr) {
            *note = std::string("approved scaler cannot convert ") + (name != nullptr ? name : "this pixel format");
          }
          av_frame_unref(frame);
          return false;
        }
        picture.rgb.resize(static_cast<std::size_t>(bytes));
        uint8_t* planes[1] = {picture.rgb.data()};
        int stride[1] = {picture.width * 3};
        const int rows = sws_scale(scaler, frame->data, frame->linesize, 0, picture.height, planes, stride);
        av_frame_unref(frame);
        if (rows != picture.height) {
          if (note != nullptr) {
            *note = "scaler did not convert the picture";
          }
          return false;
        }
        picture.present = true;
        cached = std::move(picture);
        ++nextIndex;
        return true;
      }
      if (received != AVERROR(EAGAIN) && received != AVERROR_EOF) {
        if (note != nullptr) {
          *note = "picture decode failed: " + avError(received);
        }
        return false;
      }
      if (draining) {
        finished = true;
        if (note != nullptr) {
          *note = "no further picture";
        }
        return false;
      }
      const int read = av_read_frame(format, packet);
      if (read < 0) {
        avcodec_send_packet(codec, nullptr);
        draining = true;
        continue;
      }
      if (packet->stream_index != stream) {
        av_packet_unref(packet);
        continue;
      }
      const int sent = avcodec_send_packet(codec, packet);
      av_packet_unref(packet);
      if (sent < 0 && sent != AVERROR(EAGAIN)) {
        if (note != nullptr) {
          *note = "picture packet was refused: " + avError(sent);
        }
        return false;
      }
    }
  }
};

PictureReader::PictureReader() = default;

PictureReader::~PictureReader() { close(); }

PictureReader::PictureReader(PictureReader&& other) noexcept : state_(other.state_) { other.state_ = nullptr; }

PictureReader& PictureReader::operator=(PictureReader&& other) noexcept {
  if (this != &other) {
    close();
    state_ = other.state_;
    other.state_ = nullptr;
  }
  return *this;
}

bool PictureReader::open(const std::string& path, std::string* note) {
  close();
  if (path.empty() || path.find("://") != std::string::npos) {
    if (note != nullptr) {
      *note = "remote or non-local protocol is not opened";
    }
    return false;
  }
  auto* state = new State();
  const int opened = avformat_open_input(&state->format, path.c_str(), nullptr, nullptr);
  if (opened < 0) {
    if (note != nullptr) {
      *note = "picture file did not open: " + avError(opened);
    }
    delete state;
    return false;
  }
  const int info = avformat_find_stream_info(state->format, nullptr);
  if (info < 0) {
    if (note != nullptr) {
      *note = "picture streams were not read: " + avError(info);
    }
    delete state;
    return false;
  }
  state->stream = av_find_best_stream(state->format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (state->stream < 0) {
    if (note != nullptr) {
      *note = "no video picture stream";
    }
    delete state;
    return false;
  }
  AVStream* stream = state->format->streams[state->stream];
  const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
  if (decoder == nullptr) {
    if (note != nullptr) {
      *note = std::string("decoder for ") + avcodec_get_name(stream->codecpar->codec_id) +
              " is not in the approved library";
    }
    delete state;
    return false;
  }
  state->codec = avcodec_alloc_context3(decoder);
  if (state->codec == nullptr || avcodec_parameters_to_context(state->codec, stream->codecpar) < 0) {
    if (note != nullptr) {
      *note = "picture decoder context was not created";
    }
    delete state;
    return false;
  }
  const int codecOpen = avcodec_open2(state->codec, decoder, nullptr);
  if (codecOpen < 0) {
    if (note != nullptr) {
      *note = std::string("decoder for ") + decoder->name + " did not open: " + avError(codecOpen);
    }
    delete state;
    return false;
  }
  state->packet = av_packet_alloc();
  state->frame = av_frame_alloc();
  if (state->packet == nullptr || state->frame == nullptr) {
    if (note != nullptr) {
      *note = "picture buffers were not created";
    }
    delete state;
    return false;
  }
  if (stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0) {
    state->spacing = stream->avg_frame_rate;
    state->spacingField = "avg_frame_rate";
  } else if (stream->r_frame_rate.num > 0 && stream->r_frame_rate.den > 0) {
    state->spacing = stream->r_frame_rate;
    state->spacingField = "r_frame_rate";
  }
  state_ = state;
  if (note != nullptr) {
    note->clear();
  }
  return true;
}

void PictureReader::close() {
  delete state_;
  state_ = nullptr;
}

bool PictureReader::isOpen() const { return state_ != nullptr; }

bool PictureReader::reportedDisplayIntervalUs(std::int64_t* microseconds, std::string* note) const {
  if (state_ == nullptr || state_->spacing.num <= 0 || state_->spacing.den <= 0) {
    if (note != nullptr) {
      *note = "the container did not report a display interval";
    }
    return false;
  }
  if (state_->spacing.den > (INT64_C(0x7fffffffffffffff) / 1000000)) {
    if (note != nullptr) {
      *note = "the container display interval does not fit";
    }
    return false;
  }
  const std::int64_t us = (static_cast<std::int64_t>(state_->spacing.den) * 1000000) / state_->spacing.num;
  if (us <= 0) {
    if (note != nullptr) {
      *note = "the container display interval is empty";
    }
    return false;
  }
  if (microseconds != nullptr) {
    *microseconds = us;
  }
  if (note != nullptr) {
    *note = state_->spacingField;
  }
  return true;
}

DisplayPicture PictureReader::pictureAt(std::int64_t index) {
  DisplayPicture missing;
  if (state_ == nullptr) {
    missing.note = "no picture is open";
    return missing;
  }
  if (index < 0) {
    missing.note = "a picture index is not negative";
    return missing;
  }
  if (state_->cached.present && state_->cached.mediaIndex == index) {
    return state_->cached;
  }
  if (index < state_->nextIndex) {
    state_->rewind();
  }
  std::string note;
  while (state_->nextIndex <= index) {
    if (!state_->decodeNext(&note)) {
      missing.mediaIndex = index;
      missing.note = note.empty() ? "no picture at this index" : note;
      return missing;
    }
  }
  return state_->cached;
}

}  // namespace aether
