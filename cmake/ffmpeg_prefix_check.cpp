#include <cstdio>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/ffversion.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace {

bool hasToken(const char* config, const char* token) {
  const std::size_t n = std::strlen(token);
  for (const char* p = config; (p = std::strstr(p, token)) != nullptr; ++p) {
    const bool left = p == config || p[-1] == ' ';
    const bool right = p[n] == '\0' || p[n] == ' ';
    if (left && right) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  if (std::strcmp(FFMPEG_VERSION, "9.1-aether-lgpl-candidate") != 0) {
    std::fprintf(stderr, "version string %s\n", FFMPEG_VERSION);
    return 2;
  }
  if (avutil_version() != AV_VERSION_INT(61, 19, 100) || avcodec_version() != AV_VERSION_INT(63, 24, 100) ||
      avformat_version() != AV_VERSION_INT(63, 16, 100) || swscale_version() != AV_VERSION_INT(10, 5, 100) ||
      swresample_version() != AV_VERSION_INT(7, 6, 100)) {
    std::fprintf(stderr, "runtime library version mismatch\n");
    return 3;
  }
  if (std::strcmp(avutil_license(), "LGPL version 2.1 or later") != 0) {
    std::fprintf(stderr, "licence %s\n", avutil_license());
    return 4;
  }
  const char* config = avutil_configuration();
  if (!hasToken(config, "--prefix=/opt/aether-ffmpeg-9.1") || !hasToken(config, "--disable-gpl") ||
      !hasToken(config, "--disable-version3") || !hasToken(config, "--disable-nonfree") ||
      !hasToken(config, "--disable-avfilter")) {
    std::fprintf(stderr, "configuration missing a required token\n%s\n", config);
    return 5;
  }
  if (hasToken(config, "--enable-gpl") || hasToken(config, "--enable-version3") ||
      hasToken(config, "--enable-nonfree") || hasToken(config, "--enable-avfilter") ||
      std::strstr(config, "--prefix=/usr") != nullptr || std::strstr(config, "--enable-lib") != nullptr) {
    std::fprintf(stderr, "configuration is not the LGPL development prefix\n%s\n", config);
    return 6;
  }
  std::printf("ffmpeg-prefix-ok %s\n", FFMPEG_VERSION);
  return 0;
}
