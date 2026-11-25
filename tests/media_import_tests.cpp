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

#include "media_import_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/persistence/project_store.hpp"
#include "aether/ui/main_window.hpp"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTest>

#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    const std::string token = id ? *id : std::string("tmp");
    directory = std::filesystem::temp_directory_path() / ("aether-slice1c-" + token);
    media = std::filesystem::temp_directory_path() / ("aether-slice1c-media-" + token);
    std::filesystem::create_directories(media);
  }

  ~Scratch() {
    session.closeProject();
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
    std::filesystem::remove_all(media, ec);
  }

  std::filesystem::path writeMedia(const std::string& name, const std::string& bytes) {
    const auto path = media / name;
    std::ofstream output(path, std::ios::binary);
    output << bytes;
    output.close();
    return path;
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path directory;
  std::filesystem::path media;
};

}  // namespace

void MediaImportTests::rootBinIsCreatedOnDemandAndStartsEmpty() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Bin").ok());
  auto before = scratch.session.listBins();
  QVERIFY(before.ok());
  QCOMPARE(before.value.size(), static_cast<std::size_t>(0));
  auto root = scratch.session.ensureRootBin();
  QVERIFY(root.ok());
  QCOMPARE(root.value.name, std::string("Root"));
  auto again = scratch.session.ensureRootBin();
  QVERIFY(again.ok());
  QCOMPARE(again.value.binId, root.value.binId);
  auto assets = scratch.session.listBinAssets(root.value.binId);
  QVERIFY(assets.ok());
  QVERIFY(assets.value.empty());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
}

void MediaImportTests::importKeepsStableIdsWhenThePathChanges() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Identity").ok());
  const auto picture = scratch.writeMedia("picture.mov", "picture-bytes");
  const auto sound = scratch.writeMedia("sound.wav", "sound-bytes");
  const auto beforeSize = std::filesystem::file_size(picture);
  auto imported = scratch.session.importReferences({picture.string(), sound.string()});
  QVERIFY(imported.ok());
  QCOMPARE(imported.value.size(), static_cast<std::size_t>(2));
  QCOMPARE(std::filesystem::file_size(picture), beforeSize);
  const auto& first = imported.value[0];
  QVERIFY(first.mediaId != first.recordedPath);
  QVERIFY(first.representationId != first.mediaId);
  QVERIFY(first.objectId != first.mediaId);
  QVERIFY(first.representationId != first.objectId);
  QCOMPARE(first.mediaType, std::string("picture"));
  QCOMPARE(imported.value[1].mediaType, std::string("sound"));
  QVERIFY(first.mediaId != imported.value[1].mediaId);

  const auto moved = scratch.writeMedia("moved.mov", "picture-bytes");
  QVERIFY(scratch.session.setRepresentationPath(first.representationId, moved.string()).ok());
  auto assets = scratch.session.listBinAssets(scratch.session.ensureRootBin().value.binId);
  QVERIFY(assets.ok());
  QCOMPARE(assets.value.size(), static_cast<std::size_t>(2));
  bool found = false;
  for (const aether::BinAssetFact& asset : assets.value) {
    if (asset.mediaId != first.mediaId) {
      continue;
    }
    found = true;
    QCOMPARE(asset.representationId, first.representationId);
    QCOMPARE(asset.objectId, first.objectId);
    QCOMPARE(asset.recordedPath, moved.string());
    QCOMPARE(asset.technicalMetadata, std::string("Not probed"));
    QCOMPARE(asset.registrationStatus, std::string("registered"));
    QCOMPARE(asset.presence, std::string("online"));
    QVERIFY(asset.sizeText.find("bytes") != std::string::npos);
    QVERIFY(asset.mtimeUtc.find("T") != std::string::npos);
  }
  QVERIFY(found);
  auto sequences = scratch.session.listSequences();
  QVERIFY(sequences.ok());
  QVERIFY(sequences.value.empty());
}

void MediaImportTests::closeAndReopenKeepsTheSameIds() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Reopen").ok());
  const auto picture = scratch.writeMedia("keep.mp4", "keep");
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  const auto saved = imported.value[0];
  QVERIFY(scratch.session.closeProject().ok());
  aether::MemoryLogger otherLog;
  aether::ProjectSession other(otherLog);
  QVERIFY(other.openProject(scratch.directory).ok());
  auto root = other.ensureRootBin();
  QVERIFY(root.ok());
  auto assets = other.listBinAssets(root.value.binId);
  QVERIFY(assets.ok());
  QCOMPARE(assets.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(assets.value[0].mediaId, saved.mediaId);
  QCOMPARE(assets.value[0].representationId, saved.representationId);
  QCOMPARE(assets.value[0].objectId, saved.objectId);
  QCOMPARE(assets.value[0].recordedPath, saved.recordedPath);
  other.closeProject();
}

void MediaImportTests::exactPathIsRefusedAndNothingIsMerged() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Duplicate").ok());
  const auto picture = scratch.writeMedia("same.mov", "same-bytes");
  const auto otherName = scratch.writeMedia("other.mov", "same-bytes");
  QVERIFY(scratch.session.importReferences({picture.string()}).ok());
  const auto lexical = std::filesystem::path(picture.string() + "/../" + picture.filename().string());
  auto duplicate = scratch.session.importReferences({lexical.string()});
  QVERIFY(!duplicate.ok());
  QVERIFY(duplicate.error.message.find("no records were written") != std::string::npos);
  auto firstAssets = scratch.session.listBinAssets(scratch.session.ensureRootBin().value.binId);
  QVERIFY(firstAssets.ok());
  QCOMPARE(firstAssets.value.size(), static_cast<std::size_t>(1));
  auto sameBytes = scratch.session.importReferences({otherName.string()});
  QVERIFY(sameBytes.ok());
  QVERIFY(sameBytes.value[0].mediaId != firstAssets.value[0].mediaId);
  auto assets = scratch.session.listBinAssets(scratch.session.ensureRootBin().value.binId);
  QVERIFY(assets.ok());
  QCOMPARE(assets.value.size(), static_cast<std::size_t>(2));

  const auto extra = scratch.writeMedia("extra.wav", "extra");
  auto batch = scratch.session.importReferences({extra.string(), picture.string()});
  QVERIFY(!batch.ok());
  assets = scratch.session.listBinAssets(scratch.session.ensureRootBin().value.binId);
  QVERIFY(assets.ok());
  QCOMPARE(assets.value.size(), static_cast<std::size_t>(2));
}

void MediaImportTests::failedImportWritesNothing() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Failures").ok());
  auto root = scratch.session.ensureRootBin();
  QVERIFY(root.ok());
  const auto good = scratch.writeMedia("good.mov", "good");
  const auto missing = scratch.media / "missing.mov";
  const auto unknown = scratch.writeMedia("notes.txt", "notes");
  const auto directory = scratch.media / "not-a-file.mov";
  std::filesystem::create_directory(directory);

  auto missingImport = scratch.session.importReferences({good.string(), missing.string()});
  QVERIFY(!missingImport.ok());
  QVERIFY(missingImport.error.message.find("no records were written") != std::string::npos);
  QCOMPARE(scratch.session.listBinAssets(root.value.binId).value.size(), static_cast<std::size_t>(0));

  auto unknownImport = scratch.session.importReferences({unknown.string()});
  QVERIFY(!unknownImport.ok());
  QCOMPARE(unknownImport.error.code, aether::ErrorCode::InvalidArgument);
  QCOMPARE(scratch.session.listBinAssets(root.value.binId).value.size(), static_cast<std::size_t>(0));

  auto invalidImport = scratch.session.importReferences({directory.string()});
  QVERIFY(!invalidImport.ok());
  QCOMPARE(scratch.session.listBinAssets(root.value.binId).value.size(), static_cast<std::size_t>(0));

  auto emptyImport = scratch.session.importReferences({""});
  QVERIFY(!emptyImport.ok());
  QCOMPARE(scratch.session.listBinAssets(root.value.binId).value.size(), static_cast<std::size_t>(0));

  if (::geteuid() != 0) {
    const auto blocked = scratch.writeMedia("blocked.mov", "blocked");
    QVERIFY(::chmod(blocked.c_str(), 0) == 0);
    auto unreadable = scratch.session.importReferences({blocked.string()});
    ::chmod(blocked.c_str(), 0600);
    QVERIFY(!unreadable.ok());
    QCOMPARE(unreadable.error.code, aether::ErrorCode::Io);
    QVERIFY(unreadable.error.message.find("unreadable") != std::string::npos);
    QCOMPARE(scratch.session.listBinAssets(root.value.binId).value.size(), static_cast<std::size_t>(0));
  }

  QVERIFY(scratch.session.importReferences({good.string()}).ok());
  QCOMPARE(scratch.session.listBinAssets(root.value.binId).value.size(), static_cast<std::size_t>(1));
}

void MediaImportTests::databaseFailureRollsBackTheBatch() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Rollback").ok());
  auto root = scratch.session.ensureRootBin();
  QVERIFY(root.ok());
  const auto first = scratch.writeMedia("one.mov", "one");
  const auto second = scratch.writeMedia("two.wav", "two");
  scratch.session.storeForTest().debugArmNextMutationFailure();
  auto failed = scratch.session.importReferences({first.string(), second.string()});
  QVERIFY(!failed.ok());
  QCOMPARE(failed.error.code, aether::ErrorCode::Database);
  auto assets = scratch.session.listBinAssets(root.value.binId);
  QVERIFY(assets.ok());
  QVERIFY(assets.value.empty());
  auto bins = scratch.session.listBins();
  QVERIFY(bins.ok());
  QCOMPARE(bins.value.size(), static_cast<std::size_t>(1));
  QVERIFY(scratch.session.importReferences({first.string()}).ok());
  QCOMPARE(scratch.session.listBinAssets(root.value.binId).value.size(), static_cast<std::size_t>(1));
}

void MediaImportTests::offlineKeepsTheSameIds() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Offline").ok());
  const auto picture = scratch.writeMedia("online.mov", "online-bytes");
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  std::filesystem::remove(picture);
  auto assets = scratch.session.listBinAssets(scratch.session.ensureRootBin().value.binId);
  QVERIFY(assets.ok());
  QCOMPARE(assets.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(assets.value[0].mediaId, imported.value[0].mediaId);
  QCOMPARE(assets.value[0].representationId, imported.value[0].representationId);
  QCOMPARE(assets.value[0].objectId, imported.value[0].objectId);
  QCOMPARE(assets.value[0].recordedPath, imported.value[0].recordedPath);
  QCOMPARE(assets.value[0].presence, std::string("offline"));
  QCOMPARE(assets.value[0].registrationStatus, std::string("registered"));
  QCOMPARE(assets.value[0].sizeText, std::string("unavailable"));
  QCOMPARE(assets.value[0].technicalMetadata, std::string("Not probed"));
}

void MediaImportTests::windowShowsEmptyBinThenImportedInspector() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Window").ok());
  aether::MainWindow empty(scratch.session, scratch.logger);
  empty.show();
  QVERIFY(QTest::qWaitForWindowExposed(&empty));
  auto* emptyLabel = empty.findChild<QLabel*>("emptyBin");
  auto* assetList = empty.findChild<QListWidget*>("assetList");
  auto* importButton = empty.findChild<QPushButton*>("importMedia");
  QVERIFY(emptyLabel != nullptr);
  QVERIFY(assetList != nullptr);
  QVERIFY(importButton != nullptr);
  QCOMPARE(importButton->text(), QString("Import Media"));
  QCOMPARE(emptyLabel->text(), QString("This bin has no media."));
  QCOMPARE(assetList->count(), 0);

  const auto picture = scratch.writeMedia("visible.mov", "visible-bytes");
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  aether::MainWindow filled(scratch.session, scratch.logger);
  filled.show();
  QVERIFY(QTest::qWaitForWindowExposed(&filled));
  auto* filledList = filled.findChild<QListWidget*>("assetList");
  auto* inspector = filled.findChild<QLabel*>("assetInspector");
  QVERIFY(filledList != nullptr);
  QVERIFY(inspector != nullptr);
  QCOMPARE(filledList->count(), 1);
  const QString text = inspector->text();
  QVERIFY(text.contains("visible.mov"));
  QVERIFY(text.contains(QString::fromStdString(imported.value[0].mediaId)));
  QVERIFY(text.contains(QString::fromStdString(imported.value[0].representationId)));
  QVERIFY(text.contains("online"));
  QVERIFY(text.contains("registered"));
  QVERIFY(text.contains("Not probed"));
  if (const char* shot = std::getenv("AETHER_SLICE1C_SHOT")) {
    QVERIFY(filled.grab().save(QString::fromUtf8(shot)));
  }
  filled.close();
  empty.close();
}
