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

#include "media_probe_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/core/project/project_info.hpp"
#include "aether/media/probe.hpp"
#include "aether/ui/main_window.hpp"
#include "aether/ui/probe_controller.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSemaphore>
#include <QToolButton>
#include <QTest>

#include <sqlite3.h>

#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

bool openPanelMenu(QWidget* window, const char* name) {
  auto* button = window->findChild<QToolButton*>(name);
  if (button == nullptr || button->menu() == nullptr) {
    return false;
  }
  if (!button->menu()->isVisible()) {
    button->menu()->popup(button->mapToGlobal(QPoint(0, button->height())));
  }
  return QTest::qWaitFor([button] { return button->menu()->isVisible(); }, 1000);
}

std::filesystem::path fixture(const char* name) {
  return std::filesystem::path(AETHER_SOURCE_DIR) / "validation-fixtures/media-probe-9.1/media" / name;
}

std::string fileBytes(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

int descriptorCount() {
  int count = 0;
  std::error_code error;
  for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd", error)) {
    (void)entry;
    ++count;
  }
  return count;
}

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    directory = std::filesystem::temp_directory_path() / ("aether-slice1e-" + (id ? *id : std::string("tmp")));
    std::filesystem::create_directories(directory);
  }

  ~Scratch() {
    session.closeProject();
    std::error_code error;
    std::filesystem::remove_all(directory, error);
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path directory;
};

class RestoreMode {
 public:
  RestoreMode(std::filesystem::path path, mode_t mode) : path_(std::move(path)), mode_(mode) {}
  ~RestoreMode() { chmod(path_.c_str(), mode_); }

 private:
  std::filesystem::path path_;
  mode_t mode_;
};

}  // namespace

void MediaProbeTests::linkedPrefixIsFfmpeg91AndNotSystem902() {
  const aether::FfmpegLinkIdentity identity = aether::ffmpegLinkIdentity();
  QCOMPARE(identity.version, std::string("9.1-aether-lgpl-candidate"));
  QCOMPARE(identity.license, std::string("LGPL version 2.1 or later"));
  QVERIFY(identity.configuration.find("--prefix=/opt/aether-ffmpeg-9.1") != std::string::npos);
  QVERIFY(identity.configuration.find("--disable-gpl") != std::string::npos);
  QVERIFY(identity.configuration.find("--disable-version3") != std::string::npos);
  QVERIFY(identity.configuration.find("--disable-nonfree") != std::string::npos);
  QVERIFY(identity.configuration.find("--enable-gpl") == std::string::npos);
  QVERIFY(identity.configuration.find("--enable-lib") == std::string::npos);
  const char* needed[] = {"libavutil.so.61.19.100", "libavcodec.so.63.24.100", "libavformat.so.63.16.100",
                          "libswscale.so.10.5.100", "libswresample.so.7.6.100"};
  for (const char* name : needed) {
    bool found = false;
    for (const std::string& loaded : identity.loadedLibraries) {
      QVERIFY(loaded.find("/usr/lib64") == std::string::npos);
      QVERIFY(loaded.find("61.1.102") == std::string::npos);
      QVERIFY(loaded.find("63.1.102") == std::string::npos);
      QVERIFY(loaded.find("10.1.102") == std::string::npos);
      QVERIFY(loaded.find("7.1.102") == std::string::npos);
      QVERIFY(loaded.find("libavfilter") == std::string::npos);
      if (loaded.find("/opt/aether-ffmpeg-9.1/lib/") != std::string::npos && loaded.find(name) != std::string::npos) {
        found = true;
      }
    }
    QVERIFY2(found, name);
  }
}

void MediaProbeTests::publicHeadersDoNotIncludeFfmpeg() {
  const std::filesystem::path root(AETHER_SOURCE_DIR);
  const std::vector<std::filesystem::path> trees = {root / "include", root / "src"};
  for (const std::filesystem::path& tree : trees) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(tree)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const auto path = entry.path();
      if (path.filename() == "ffmpeg_probe.cpp" || path.filename() == "ffmpeg_picture.cpp") {
        continue;
      }
      const std::string suffix = path.extension().string();
      if (suffix != ".hpp" && suffix != ".h" && suffix != ".cpp") {
        continue;
      }
      const std::string text = fileBytes(path);
      QVERIFY2(text.find("libavformat/") == std::string::npos, path.c_str());
      QVERIFY2(text.find("libavcodec/") == std::string::npos, path.c_str());
      QVERIFY2(text.find("libavutil/") == std::string::npos, path.c_str());
      QVERIFY2(text.find("libswscale/") == std::string::npos, path.c_str());
      QVERIFY2(text.find("libswresample/") == std::string::npos, path.c_str());
      QVERIFY2(text.find("libavfilter") == std::string::npos, path.c_str());
    }
  }
}

void MediaProbeTests::videoProbeKeepsRationalTimebaseAndDoesNotVerifyRate() {
  const auto path = fixture("video.mkv");
  const auto before = std::filesystem::file_size(path);
  const aether::ProbeReport report = aether::probeRegisteredRepresentation(path.string());
  QCOMPARE(std::filesystem::file_size(path), before);
  QCOMPARE(report.completion, aether::ProbeCompletion::Complete);
  QCOMPARE(report.containerName, std::string("matroska,webm"));
  QVERIFY(report.containerLongName.find("Matroska") != std::string::npos);
  QVERIFY(report.fileSizePresent);
  QCOMPARE(report.fileSizeBytes, static_cast<std::int64_t>(before));
  QVERIFY(report.mtimeUtc.find("T") != std::string::npos);
  QVERIFY(report.containerDuration.present);
  QCOMPARE(report.containerDuration.timebase.numerator, static_cast<std::int64_t>(1));
  QCOMPARE(report.containerDuration.timebase.denominator, static_cast<std::int64_t>(1000000));
  QVERIFY(!report.sourceRateVerified);
  QCOMPARE(report.video.size(), static_cast<std::size_t>(1));
  const aether::VideoStream& video = report.video.front();
  QCOMPARE(video.codecName, std::string("ffv1"));
  QVERIFY(video.codecIdPresent);
  QVERIFY(video.widthPresent);
  QCOMPARE(video.width, 160);
  QVERIFY(video.heightPresent);
  QCOMPARE(video.height, 120);
  QCOMPARE(video.pixelFormat, std::string("yuv420p"));
  QVERIFY(video.sampleAspectPresent);
  QCOMPARE(video.sampleAspect.numerator, static_cast<std::int64_t>(1));
  QCOMPARE(video.sampleAspect.denominator, static_cast<std::int64_t>(1));
  QVERIFY(video.displayAspectPresent);
  QCOMPARE(video.displayAspect.numerator, static_cast<std::int64_t>(4));
  QCOMPARE(video.displayAspect.denominator, static_cast<std::int64_t>(3));
  QCOMPARE(video.displayAspectProvenance, aether::Provenance::CalculatedFromReportedExact);
  QVERIFY(video.timebasePresent);
  QCOMPARE(video.timebase.numerator, static_cast<std::int64_t>(1));
  QCOMPARE(video.timebase.denominator, static_cast<std::int64_t>(1000));
  QVERIFY(video.averageFrameRate.present);
  QCOMPARE(video.averageFrameRate.rate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(video.averageFrameRate.rate.denominator, static_cast<std::int64_t>(1));
  QVERIFY(video.nominalFrameRate.present);
  QCOMPARE(video.nominalFrameRate.rate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(video.nominalFrameRate.rate.denominator, static_cast<std::int64_t>(1));
  QVERIFY(!video.ratesConflict);
  QVERIFY(!video.frameCountPresent);
  QVERIFY(video.start.present);
  QCOMPARE(video.start.ticks, static_cast<std::int64_t>(0));
  QVERIFY(!video.duration.present);
  const std::string text = aether::formatProbeReport(report);
  QVERIFY(text.find("not a proven frame rate") != std::string::npos);
  QVERIFY(text.find("not proof of constant frame rate") != std::string::npos);
  QVERIFY(text.find("verified source rate: no") != std::string::npos);
  QVERIFY(text.find("frame count unknown") != std::string::npos);
}

void MediaProbeTests::audioProbeUsesChannelLayout() {
  const aether::ProbeReport report = aether::probeRegisteredRepresentation(fixture("audio.wav").string());
  QCOMPARE(report.completion, aether::ProbeCompletion::Complete);
  QCOMPARE(report.containerName, std::string("wav"));
  QVERIFY(report.video.empty());
  QCOMPARE(report.audio.size(), static_cast<std::size_t>(1));
  const aether::AudioStream& audio = report.audio.front();
  QCOMPARE(audio.codecName, std::string("pcm_s16le"));
  QVERIFY(audio.sampleRatePresent);
  QCOMPARE(audio.sampleRate, 48000);
  QVERIFY(audio.channelCountPresent);
  QCOMPARE(audio.channelCount, 1);
  QVERIFY(audio.channelLayout != "stereo");
  QCOMPARE(audio.sampleFormat, std::string("s16"));
  QVERIFY(audio.timebasePresent);
  QCOMPARE(audio.timebase.numerator, static_cast<std::int64_t>(1));
  QCOMPARE(audio.timebase.denominator, static_cast<std::int64_t>(48000));
  QVERIFY(!audio.start.present);
  QVERIFY(audio.duration.present);
  QCOMPARE(audio.duration.ticks, static_cast<std::int64_t>(24000));
  QCOMPARE(audio.duration.timebase.denominator, static_cast<std::int64_t>(48000));
  QVERIFY(!report.sourceRateVerified);
}

void MediaProbeTests::bothStreamsAreReported() {
  const aether::ProbeReport report = aether::probeRegisteredRepresentation(fixture("both.mkv").string());
  QCOMPARE(report.completion, aether::ProbeCompletion::Complete);
  QCOMPARE(report.video.size(), static_cast<std::size_t>(1));
  QCOMPARE(report.audio.size(), static_cast<std::size_t>(1));
  QCOMPARE(report.video.front().width, 160);
  QCOMPARE(report.audio.front().sampleRate, 48000);
}

void MediaProbeTests::multipleStreamsIncludeSubtitle() {
  const aether::ProbeReport report = aether::probeRegisteredRepresentation(fixture("multi.mkv").string());
  QCOMPARE(report.completion, aether::ProbeCompletion::Complete);
  QCOMPARE(report.video.size(), static_cast<std::size_t>(1));
  QCOMPARE(report.audio.size(), static_cast<std::size_t>(2));
  QCOMPARE(report.audio[0].sampleRate, 48000);
  QCOMPARE(report.audio[1].sampleRate, 44100);
  QCOMPARE(report.other.size(), static_cast<std::size_t>(1));
  QCOMPARE(report.other.front().kind, std::string("subtitle"));
  QCOMPARE(report.other.front().codecName, std::string("subrip"));
}

void MediaProbeTests::attachmentStreamIsReported() {
  const aether::ProbeReport report = aether::probeRegisteredRepresentation(fixture("attached.mkv").string());
  QCOMPARE(report.completion, aether::ProbeCompletion::Complete);
  QCOMPARE(report.video.size(), static_cast<std::size_t>(1));
  bool attachment = false;
  for (const aether::OtherStream& stream : report.other) {
    if (stream.kind == "attachment") {
      attachment = true;
      QCOMPARE(stream.codecName, std::string("png"));
    }
  }
  QVERIFY(attachment);
}

void MediaProbeTests::missingDurationStartAndFrameCountStayUnknown() {
  const aether::ProbeReport report = aether::probeRegisteredRepresentation(fixture("noduration.m2v").string());
  QCOMPARE(report.completion, aether::ProbeCompletion::Complete);
  QVERIFY(!report.containerDuration.present);
  QCOMPARE(report.video.size(), static_cast<std::size_t>(1));
  QVERIFY(!report.video.front().start.present);
  QVERIFY(!report.video.front().duration.present);
  QVERIFY(!report.video.front().frameCountPresent);
  QVERIFY(report.video.front().timebasePresent);
  QCOMPARE(report.video.front().timebase.numerator, static_cast<std::int64_t>(1));
  QCOMPARE(report.video.front().timebase.denominator, static_cast<std::int64_t>(1200000));
  const std::string text = aether::formatProbeReport(report);
  QVERIFY(text.find("container duration unknown") != std::string::npos);
  QVERIFY(text.find("frame count unknown") != std::string::npos);
  QVERIFY(text.find("start unknown") != std::string::npos);
}

void MediaProbeTests::conflictingRatesStayUnverified() {
  const aether::ProbeReport report = aether::probeRegisteredRepresentation(fixture("vfr.mp4").string());
  QCOMPARE(report.completion, aether::ProbeCompletion::Complete);
  QCOMPARE(report.video.size(), static_cast<std::size_t>(1));
  const aether::VideoStream& video = report.video.front();
  QVERIFY(video.averageFrameRate.present);
  QCOMPARE(video.averageFrameRate.rate.numerator, static_cast<std::int64_t>(90));
  QCOMPARE(video.averageFrameRate.rate.denominator, static_cast<std::int64_t>(2903));
  QVERIFY(video.nominalFrameRate.present);
  QCOMPARE(video.nominalFrameRate.rate.numerator, static_cast<std::int64_t>(30));
  QCOMPARE(video.nominalFrameRate.rate.denominator, static_cast<std::int64_t>(1001));
  QVERIFY(video.ratesConflict);
  QVERIFY(video.frameCountPresent);
  QCOMPARE(video.frameCount, static_cast<std::int64_t>(30));
  QCOMPARE(video.frameCountProvenance, aether::Provenance::ReportedByStream);
  QVERIFY(video.timebasePresent);
  QCOMPARE(video.timebase.numerator, static_cast<std::int64_t>(1));
  QCOMPARE(video.timebase.denominator, static_cast<std::int64_t>(15360));
  QVERIFY(!report.sourceRateVerified);
  QVERIFY(aether::formatProbeReport(report).find("frame rate candidates conflict") != std::string::npos);
}

void MediaProbeTests::malformedTruncatedMissingAndInaccessibleFail() {
  const aether::ProbeReport malformed = aether::probeRegisteredRepresentation(fixture("malformed.mov").string());
  QCOMPARE(malformed.completion, aether::ProbeCompletion::Failed);
  QVERIFY(!malformed.errors.empty());
  QVERIFY(malformed.video.empty());

  const aether::ProbeReport truncated = aether::probeRegisteredRepresentation(fixture("truncated.mkv").string());
  QVERIFY(truncated.completion == aether::ProbeCompletion::Failed ||
          truncated.completion == aether::ProbeCompletion::Incomplete);
  QVERIFY(!truncated.errors.empty() || truncated.completion == aether::ProbeCompletion::Incomplete);
  QVERIFY(!truncated.sourceRateVerified);

  const aether::ProbeReport missing =
      aether::probeRegisteredRepresentation("/tmp/aether-slice1e-does-not-exist.mov");
  QCOMPARE(missing.completion, aether::ProbeCompletion::Failed);
  QCOMPARE(missing.errors.front(), std::string("file is missing"));

  const auto blocked = std::filesystem::temp_directory_path() / "aether-slice1e-unreadable.mov";
  {
    std::ofstream output(blocked, std::ios::binary);
    output << "blocked";
  }
  chmod(blocked.c_str(), 0);
  RestoreMode restore(blocked, 0600);
  const aether::ProbeReport denied = aether::probeRegisteredRepresentation(blocked.string());
  QCOMPARE(denied.completion, aether::ProbeCompletion::Failed);
  QCOMPARE(denied.errors.front(), std::string("file is not readable"));
  std::filesystem::remove(blocked);
}

void MediaProbeTests::remoteProtocolAndUnsupportedContainerAreRefused() {
  const aether::ProbeReport remote = aether::probeRegisteredRepresentation("http://example.invalid/clip.mp4");
  QCOMPARE(remote.completion, aether::ProbeCompletion::Failed);
  QVERIFY(remote.errors.front().find("not opened") != std::string::npos);
  QVERIFY(remote.video.empty());

  const aether::ProbeReport relative = aether::probeRegisteredRepresentation("clip.mov");
  QCOMPARE(relative.completion, aether::ProbeCompletion::Failed);
  QVERIFY(relative.errors.front().find("absolute") != std::string::npos);

  const aether::ProbeReport text = aether::probeRegisteredRepresentation(fixture("notes.avi").string());
  QCOMPARE(text.completion, aether::ProbeCompletion::Failed);
  QVERIFY(!text.errors.empty());
}

void MediaProbeTests::fileHandlesAreReleased() {
  const int before = descriptorCount();
  for (int index = 0; index < 12; ++index) {
    const aether::ProbeReport report = aether::probeRegisteredRepresentation(fixture("video.mkv").string());
    QCOMPARE(report.completion, aether::ProbeCompletion::Complete);
  }
  const aether::ProbeReport failed = aether::probeRegisteredRepresentation(fixture("malformed.mov").string());
  QCOMPARE(failed.completion, aether::ProbeCompletion::Failed);
  QVERIFY(descriptorCount() <= before + 2);
}

void MediaProbeTests::identitySurvivesMoveAndOffline() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory / "Identity", "Identity").ok());
  const auto source = fixture("video.mkv");
  const auto online = scratch.directory / "online.mkv";
  const auto moved = scratch.directory / "moved.mkv";
  std::filesystem::copy_file(source, online);
  std::filesystem::copy_file(source, moved);
  auto imported = scratch.session.importReferences({online.string()});
  QVERIFY(imported.ok());
  const auto saved = imported.value.front();
  const aether::ProbeReport first = aether::probeRegisteredRepresentation(saved.recordedPath);
  QCOMPARE(first.completion, aether::ProbeCompletion::Complete);
  auto representations = scratch.session.listRepresentations(saved.mediaId);
  QVERIFY(representations.ok());
  QCOMPARE(representations.value.front().representationId, saved.representationId);
  QCOMPARE(representations.value.front().mediaId, saved.mediaId);
  QVERIFY(scratch.session.setRepresentationPath(saved.representationId, moved.string()).ok());
  representations = scratch.session.listRepresentations(saved.mediaId);
  QCOMPARE(representations.value.front().representationId, saved.representationId);
  QCOMPARE(representations.value.front().recordedPath, moved.string());
  const aether::ProbeReport second = aether::probeRegisteredRepresentation(moved.string());
  QCOMPARE(second.completion, aether::ProbeCompletion::Complete);
  std::filesystem::remove(moved);
  auto assets = scratch.session.listBinAssets(scratch.session.ensureRootBin().value.binId);
  QVERIFY(assets.ok());
  QCOMPARE(assets.value.front().mediaId, saved.mediaId);
  QCOMPARE(assets.value.front().representationId, saved.representationId);
  QCOMPARE(assets.value.front().presence, std::string("offline"));
  QCOMPARE(assets.value.front().technicalMetadata, std::string("Not probed"));
  const aether::ProbeReport offline = aether::probeRegisteredRepresentation(moved.string());
  QCOMPARE(offline.completion, aether::ProbeCompletion::Failed);
  representations = scratch.session.listRepresentations(saved.mediaId);
  QCOMPARE(representations.value.front().representationId, saved.representationId);
  QVERIFY(scratch.session.listSequences().value.empty());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(std::string(aether::kPrototypeVersion), std::string("slice-0a"));
}

void MediaProbeTests::probeLeavesMarksAndEditorialRowsUntouched() {
  Scratch scratch;
  const auto project = scratch.directory / "Marks";
  QVERIFY(scratch.session.createProject(project, "Marks").ok());
  const auto media = scratch.directory / "picture.mkv";
  std::filesystem::copy_file(fixture("video.mkv"), media);
  auto imported = scratch.session.importReferences({media.string()});
  QVERIFY(imported.ok());
  const auto saved = imported.value.front();
  auto sequence = scratch.session.createSequence("Cut", {24, 1});
  QVERIFY(sequence.ok());
  auto placed = scratch.session.placeInstance(sequence.value.sequenceId, "", saved.objectId, {24, 1}, 0, 8, 0, 8);
  QVERIFY(placed.ok());
  QVERIFY(scratch.session.setSourceMarkIn(saved.representationId, 3).ok());
  QVERIFY(scratch.session.setSourceMarkOut(saved.representationId, 9).ok());
  QVERIFY(scratch.session.setRecordMarkIn(sequence.value.sequenceId, 1).ok());
  QVERIFY(scratch.session.setRecordMarkOut(sequence.value.sequenceId, 4).ok());
  const auto sourceBefore = scratch.session.sourceMarks(saved.representationId).value;
  const auto recordBefore = scratch.session.recordMarks(sequence.value.sequenceId).value;
  const auto instanceBefore = scratch.session.readInstance(placed.value.instanceId).value;
  const std::string databaseBefore = fileBytes(project / "project.db");

  const aether::ProbeReport report = aether::probeRegisteredRepresentation(saved.recordedPath);
  QCOMPARE(report.completion, aether::ProbeCompletion::Complete);
  QVERIFY(!report.sourceRateVerified);

  const auto sourceAfter = scratch.session.sourceMarks(saved.representationId).value;
  const auto recordAfter = scratch.session.recordMarks(sequence.value.sequenceId).value;
  const auto instanceAfter = scratch.session.readInstance(placed.value.instanceId).value;
  QCOMPARE(sourceAfter.markIn, sourceBefore.markIn);
  QCOMPARE(sourceAfter.markOut, sourceBefore.markOut);
  QCOMPARE(recordAfter.markIn, recordBefore.markIn);
  QCOMPARE(recordAfter.markOut, recordBefore.markOut);
  QCOMPARE(instanceAfter.sourceStart, instanceBefore.sourceStart);
  QCOMPARE(instanceAfter.sourceEnd, instanceBefore.sourceEnd);
  QCOMPARE(instanceAfter.recordStart, instanceBefore.recordStart);
  QCOMPARE(instanceAfter.recordEnd, instanceBefore.recordEnd);
  QCOMPARE(instanceAfter.mediaId, saved.mediaId);
  QCOMPARE(instanceAfter.sourceRate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(fileBytes(project / "project.db"), databaseBefore);

  sqlite3* database = nullptr;
  QVERIFY(sqlite3_open_v2((project / "project.db").c_str(), &database, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK);
  std::vector<std::string> tables;
  sqlite3_exec(
      database, "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name",
      [](void* data, int count, char** values, char**) -> int {
        if (count == 1 && values[0] != nullptr) {
          static_cast<std::vector<std::string>*>(data)->push_back(values[0]);
        }
        return 0;
      },
      &tables, nullptr);
  sqlite3_close(database);
  const std::vector<std::string> expected = {"bin_membership", "bins",        "editorial_objects", "master_clips",
                                             "media_assets",   "media_representations", "project_meta", "schema_info",
                                             "sequences",      "timeline_instances", "tracks"};
  QCOMPARE(tables, expected);
  for (const std::string& name : tables) {
    QVERIFY(name.find("mark") == std::string::npos);
    QVERIFY(name.find("probe") == std::string::npos);
  }

  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(project).ok());
  auto reopened = scratch.session.readInstance(placed.value.instanceId);
  QVERIFY(reopened.ok());
  QCOMPARE(reopened.value.recordEnd, static_cast<std::int64_t>(8));
  auto sourceGone = scratch.session.sourceMarks(saved.representationId);
  QVERIFY(sourceGone.ok());
  QVERIFY(!sourceGone.value.markIn.has_value());
  QVERIFY(!sourceGone.value.markOut.has_value());
  auto recordGone = scratch.session.recordMarks(sequence.value.sequenceId);
  QVERIFY(recordGone.ok());
  QVERIFY(!recordGone.value.markIn.has_value());
  QVERIFY(!recordGone.value.markOut.has_value());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
}

void MediaProbeTests::asyncProbeStaysResponsiveAndDropsStaleResults() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory / "Async", "Async").ok());
  const auto first = scratch.directory / "first.mkv";
  const auto second = scratch.directory / "second.wav";
  std::filesystem::copy_file(fixture("video.mkv"), first);
  std::filesystem::copy_file(fixture("audio.wav"), second);
  aether::MainWindow window(scratch.session, scratch.logger);
  window.importPaths({QString::fromStdString(first.string()), QString::fromStdString(second.string())});
  window.show();
  auto* controller = window.findChild<aether::ProbeController*>("probeController");
  QVERIFY(controller != nullptr);
  QSemaphore hold(0);
  controller->setProbeFunctionForTest([&hold](const std::string& path) {
    aether::ProbeReport report;
    report.completion = aether::ProbeCompletion::Complete;
    report.containerName = path;
    report.sourceRateVerified = false;
    hold.acquire();
    return report;
  });
  auto* assets = window.findChild<QListWidget*>("assetList");
  auto* status = window.findChild<QLabel*>("probeStatus");
  auto* report = window.findChild<QPlainTextEdit*>("probeReport");
  auto* probe = window.findChild<QPushButton*>("probeMedia");
  auto* frame = window.findChild<QLineEdit*>("sourceMarkFrame");
  auto* markIn = window.findChild<QPushButton*>("sourceMarkIn");
  auto* readout = window.findChild<QLabel*>("sourceMarkReadout");
  QVERIFY(assets != nullptr && status != nullptr && report != nullptr && probe != nullptr);
  auto selectNamed = [assets](const QString& name) {
    for (int index = 0; index < assets->count(); ++index) {
      if (assets->item(index)->text().contains(name)) {
        assets->setCurrentRow(index);
        return true;
      }
    }
    return false;
  };
  QVERIFY(selectNamed("first.mkv"));
  QVERIFY(openPanelMenu(&window, "projectPanelMenu"));
  QTest::mouseClick(probe, Qt::LeftButton);
  QTRY_COMPARE(status->text(), QString("Probing"));
  frame->setText("6");
  auto* sourceTab = window.findChild<QPushButton*>("viewerSourceTab");
  QVERIFY(sourceTab != nullptr);
  sourceTab->click();
  QVERIFY(openPanelMenu(&window, "viewerPanelMenu"));
  QTest::mouseClick(markIn, Qt::LeftButton);
  QVERIFY(readout->text().contains("Mark In: 6"));
  QCOMPARE(status->text(), QString("Probing"));
  QVERIFY(selectNamed("second.wav"));
  QApplication::processEvents();
  QCOMPARE(status->text(), QString("Not probed"));
  QVERIFY(!report->toPlainText().contains(QString::fromStdString(first.string())));
  hold.release();
  QTest::qWait(100);
  QCOMPARE(status->text(), QString("Not probed"));
  QVERIFY(!report->toPlainText().contains("completion complete"));
  QVERIFY(selectNamed("first.mkv"));
  QTRY_VERIFY(report->toPlainText().contains(QString::fromStdString(first.string())));
  QCOMPARE(status->text(), QString("Probed"));
  QVERIFY(!report->toPlainText().contains(QString::fromStdString(second.string())));
}

void MediaProbeTests::reprobeAndProjectSwitchDoNotShowAStaleResult() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory / "Switch", "Switch").ok());
  const auto media = scratch.directory / "switch.mkv";
  std::filesystem::copy_file(fixture("video.mkv"), media);
  aether::MainWindow window(scratch.session, scratch.logger);
  window.importPaths({QString::fromStdString(media.string())});
  window.show();
  auto* controller = window.findChild<aether::ProbeController*>("probeController");
  auto* status = window.findChild<QLabel*>("probeStatus");
  auto* report = window.findChild<QPlainTextEdit*>("probeReport");
  auto* probe = window.findChild<QPushButton*>("reprobeMedia");
  QVERIFY(controller != nullptr);
  int calls = 0;
  QSemaphore hold(0);
  controller->setProbeFunctionForTest([&calls, &hold](const std::string&) {
    aether::ProbeReport result;
    result.completion = aether::ProbeCompletion::Complete;
    result.containerName = calls == 0 ? "first-result" : "second-result";
    ++calls;
    result.sourceRateVerified = false;
    hold.acquire();
    return result;
  });
  QVERIFY(openPanelMenu(&window, "projectPanelMenu"));
  QTest::mouseClick(probe, Qt::LeftButton);
  QTRY_COMPARE(status->text(), QString("Probing"));
  QVERIFY(openPanelMenu(&window, "projectPanelMenu"));
  QTest::mouseClick(probe, Qt::LeftButton);
  hold.release();
  hold.release();
  QTRY_VERIFY(report->toPlainText().contains("second-result"));
  QVERIFY(!report->toPlainText().contains("first-result"));

  controller->setProbeFunctionForTest([](const std::string&) {
    aether::ProbeReport result;
    result.completion = aether::ProbeCompletion::Complete;
    result.containerName = "stale-project";
    result.sourceRateVerified = false;
    return result;
  });
  QVERIFY(openPanelMenu(&window, "projectPanelMenu"));
  QTest::mouseClick(window.findChild<QPushButton*>("probeMedia"), Qt::LeftButton);
  QTRY_COMPARE(status->text(), QString("Probing"));
  controller->setProject("other-project");
  QTRY_COMPARE(status->text(), QString("Not probed"));
  QTest::qWait(50);
  QVERIFY(!report->toPlainText().contains("stale-project"));
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(std::string(aether::kPrototypeVersion), std::string("slice-0a"));
}
