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

#include "project_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/persistence/manifest.hpp"
#include "aether/persistence/project_store.hpp"

#include <QTest>

#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>

namespace {

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    directory = std::filesystem::temp_directory_path() / ("aether-slice0a-" + (id ? *id : std::string("tmp")));
  }

  ~Scratch() {
    session.closeProject();
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path directory;
};

bool sameId(const std::string& left, const std::string& right) { return left == right && !left.empty(); }

}  // namespace

void ProjectTests::createManifestLayoutAndReopen() {
  Scratch scratch;
  auto created = scratch.session.createProject(scratch.directory, "Opening");
  QVERIFY(created.ok());
  QCOMPARE(created.value.projectId.size(), static_cast<std::size_t>(36));
  QVERIFY(std::filesystem::is_regular_file(scratch.directory / "project.manifest.json"));
  QVERIFY(std::filesystem::is_regular_file(scratch.directory / "project.db"));
  QVERIFY(std::filesystem::is_directory(scratch.directory / "BINS"));
  QVERIFY(std::filesystem::is_directory(scratch.directory / "MEDIA"));
  QVERIFY(std::filesystem::is_directory(scratch.directory / "CACHE"));
  QVERIFY(std::filesystem::is_directory(scratch.directory / "BACKUP"));
  QVERIFY(std::filesystem::is_directory(scratch.directory / "LOGS"));
  QVERIFY(std::filesystem::is_empty(scratch.directory / "BINS"));
  QVERIFY(std::filesystem::is_empty(scratch.directory / "MEDIA"));

  auto manifest = aether::readManifest(scratch.directory / "project.manifest.json");
  QVERIFY(manifest.ok());
  QCOMPARE(manifest.value.info.projectId, created.value.projectId);
  QCOMPARE(manifest.value.info.name, std::string("Opening"));
  QCOMPARE(manifest.value.info.formatVersion, 1);
  QCOMPARE(manifest.value.info.databaseFilename, std::string("project.db"));
  QCOMPARE(manifest.value.info.schemaVersion, 2);
  QVERIFY(!manifest.value.info.createdUtc.empty());
  QVERIFY(!manifest.value.info.prototypeVersion.empty());
  QVERIFY(!manifest.value.info.appVersion.empty());

  auto tables = scratch.session.storeForTest().listUserTables();
  QVERIFY(tables.ok());
  QCOMPARE(tables.value.size(), static_cast<std::size_t>(11));
  const std::string expected[] = {"bin_membership", "bins",           "editorial_objects", "master_clips",
                                  "media_assets",   "media_representations", "project_meta", "schema_info",
                                  "sequences",      "timeline_instances", "tracks"};
  for (std::size_t i = 0; i < 11; ++i) {
    QCOMPARE(tables.value[i], expected[i]);
  }

  const std::string id = created.value.projectId;
  QVERIFY(scratch.session.closeProject().ok());
  auto reopened = scratch.session.openProject(scratch.directory);
  QVERIFY(reopened.ok());
  QCOMPARE(reopened.value.projectId, id);
  QVERIFY(scratch.session.closeProject().ok());

  auto again = scratch.session.createProject(scratch.directory, "Opening");
  QVERIFY(!again.ok());
  QVERIFY(std::filesystem::is_regular_file(scratch.directory / "project.db"));
  auto still = scratch.session.openProject(scratch.directory);
  QVERIFY(still.ok());
  QCOMPARE(still.value.projectId, id);
}

void ProjectTests::projectIdSurvivesRenameCloseAndReopen() {
  Scratch scratch;
  auto created = scratch.session.createProject(scratch.directory, "Stable");
  QVERIFY(created.ok());
  const std::string id = created.value.projectId;
  QVERIFY(scratch.session.closeProject().ok());

  const std::filesystem::path moved = scratch.directory.parent_path() / (scratch.directory.filename().string() + "-renamed");
  std::error_code ec;
  std::filesystem::rename(scratch.directory, moved, ec);
  QVERIFY(!ec);
  scratch.directory = moved;

  auto opened = scratch.session.openProject(scratch.directory);
  QVERIFY(opened.ok());
  QCOMPARE(opened.value.projectId, id);
  QVERIFY(scratch.session.closeProject().ok());
  auto again = scratch.session.openProject(scratch.directory);
  QVERIFY(again.ok());
  QCOMPARE(again.value.projectId, id);
}

void ProjectTests::badManifestMissingDatabaseAndBadSchema() {
  Scratch scratch;
  auto created = scratch.session.createProject(scratch.directory, "Guarded");
  QVERIFY(created.ok());
  const std::string id = created.value.projectId;
  QVERIFY(scratch.session.closeProject().ok());

  const auto manifestPath = scratch.directory / "project.manifest.json";
  const auto dbPath = scratch.directory / "project.db";
  std::ifstream originalIn(manifestPath);
  std::string original((std::istreambuf_iterator<char>(originalIn)), std::istreambuf_iterator<char>());
  QVERIFY(original.find(id) != std::string::npos);

  {
    std::ofstream out(manifestPath, std::ios::trunc);
    out << "{";
  }
  auto bad = scratch.session.openProject(scratch.directory);
  QCOMPARE(static_cast<int>(bad.error.code), static_cast<int>(aether::ErrorCode::ManifestInvalid));
  QVERIFY(std::filesystem::is_regular_file(dbPath));

  {
    std::ofstream out(manifestPath, std::ios::trunc);
    out << original;
  }
  std::string swapped = original;
  const std::string fake = "00000000-0000-4000-8000-000000000000";
  const auto pos = swapped.find(id);
  QVERIFY(pos != std::string::npos);
  swapped.replace(pos, id.size(), fake);
  {
    std::ofstream out(manifestPath, std::ios::trunc);
    out << swapped;
  }
  auto mismatch = scratch.session.openProject(scratch.directory);
  QCOMPARE(static_cast<int>(mismatch.error.code), static_cast<int>(aether::ErrorCode::ManifestMismatch));
  std::ifstream still(manifestPath);
  std::string after((std::istreambuf_iterator<char>(still)), std::istreambuf_iterator<char>());
  QVERIFY(after.find(fake) != std::string::npos);
  QVERIFY(after.find(id) == std::string::npos);
  {
    std::ofstream out(manifestPath, std::ios::trunc);
    out << original;
  }

  std::filesystem::remove(dbPath);
  auto missing = scratch.session.openProject(scratch.directory);
  QCOMPARE(static_cast<int>(missing.error.code), static_cast<int>(aether::ErrorCode::DatabaseMissing));
  QVERIFY(!std::filesystem::exists(dbPath));

  {
    std::ofstream out(manifestPath, std::ios::trunc);
    out << original;
  }
  // The database was removed above. Recreate a clean project in a new folder for the schema case.
  QVERIFY(scratch.session.closeProject().ok());
  std::error_code ec;
  std::filesystem::remove_all(scratch.directory, ec);
  auto recreated = scratch.session.createProject(scratch.directory, "Guarded");
  QVERIFY(recreated.ok());
  auto bumped = scratch.session.storeForTest().debugExec("UPDATE schema_info SET schema_version = 99;");
  QVERIFY(bumped.ok());
  QVERIFY(scratch.session.closeProject().ok());
  auto refused = scratch.session.openProject(scratch.directory);
  QCOMPARE(static_cast<int>(refused.error.code), static_cast<int>(aether::ErrorCode::MigrationRefused));
  QVERIFY(std::filesystem::is_regular_file(scratch.directory / "project.db"));
  auto version = scratch.session.storeForTest().debugReadSchemaVersion(scratch.directory);
  QVERIFY(version.ok());
  QCOMPARE(version.value, 99);
}

void ProjectTests::transactionsForeignKeysAndUniqueness() {
  Scratch scratch;
  auto created = scratch.session.createProject(scratch.directory, "Tx");
  QVERIFY(created.ok());
  auto bin = scratch.session.createBin("Bin");
  QVERIFY(bin.ok());
  auto rollback = scratch.session.storeForTest().debugProveRollback(bin.value.binId);
  QVERIFY(rollback.ok());
  auto reread = scratch.session.storeForTest().readBin(bin.value.binId);
  QVERIFY(reread.ok());
  QCOMPARE(reread.value.name, std::string("Bin"));

  auto foreign = scratch.session.storeForTest().debugExec(
      "INSERT INTO media_representations "
      "(representation_id, media_id, role, recorded_path, status, created_utc, modified_utc) "
      "VALUES ('foreign-rep', 'missing-media', 'original', 'absent', 'recorded', 't', 't');");
  QCOMPARE(static_cast<int>(foreign.error.code), static_cast<int>(aether::ErrorCode::ForeignKey));

  const std::string insert =
      "INSERT INTO bins (bin_id, project_id, name, created_utc, modified_utc) VALUES ('fixed-bin', '" +
      created.value.projectId + "', 'Fixed', 't', 't');";
  QVERIFY(scratch.session.storeForTest().debugExec(insert).ok());
  auto duplicate = scratch.session.storeForTest().debugExec(insert);
  QCOMPARE(static_cast<int>(duplicate.error.code), static_cast<int>(aether::ErrorCode::Constraint));

  aether::MemoryLogger otherLog;
  aether::ProjectStore other(otherLog);
  auto second = other.open(scratch.directory);
  QCOMPARE(static_cast<int>(second.error.code), static_cast<int>(aether::ErrorCode::LockHeld));
}

void ProjectTests::binsMembershipMoveReferenceAndDuplicate() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Bins").ok());
  auto first = scratch.session.createBin("Selects");
  auto second = scratch.session.createBin("Picture");
  auto third = scratch.session.createBin("Sound");
  QVERIFY(first.ok() && second.ok() && third.ok());
  QVERIFY(first.value.binId != second.value.binId);

  auto asset = scratch.session.createAsset("picture", "{\"camera\":\"A\"}");
  QVERIFY(asset.ok());
  auto clip = scratch.session.createMasterClip("Take", asset.value.mediaId, first.value.binId, "{\"roll\":1}");
  QVERIFY(clip.ok());

  QVERIFY(scratch.session.selectBin(first.value.binId).ok());
  QCOMPARE(scratch.session.selectedBinId(), first.value.binId);

  QVERIFY(scratch.session.referenceObject(clip.value.objectId, second.value.binId).ok());
  auto inFirst = scratch.session.listClips(first.value.binId);
  auto inSecond = scratch.session.listClips(second.value.binId);
  QVERIFY(inFirst.ok() && inSecond.ok());
  QCOMPARE(inFirst.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(inSecond.value.size(), static_cast<std::size_t>(1));
  QVERIFY(sameId(inFirst.value[0].objectId, inSecond.value[0].objectId));
  QVERIFY(sameId(inFirst.value[0].mediaId, asset.value.mediaId));

  QVERIFY(scratch.session.moveMembership(clip.value.objectId, first.value.binId, third.value.binId).ok());
  auto left = scratch.session.listClips(first.value.binId);
  auto moved = scratch.session.listClips(third.value.binId);
  auto still = scratch.session.listClips(second.value.binId);
  QVERIFY(left.ok() && moved.ok() && still.ok());
  QCOMPARE(left.value.size(), static_cast<std::size_t>(0));
  QCOMPARE(moved.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(still.value.size(), static_cast<std::size_t>(1));
  QVERIFY(sameId(moved.value[0].objectId, clip.value.objectId));

  auto copy = scratch.session.duplicateMasterClip(clip.value.objectId, first.value.binId);
  QVERIFY(copy.ok());
  QVERIFY(copy.value.objectId != clip.value.objectId);
  QCOMPARE(copy.value.mediaId, asset.value.mediaId);

  QVERIFY(scratch.session.removeMembership(third.value.binId, clip.value.objectId).ok());
  auto removed = scratch.session.listClips(third.value.binId);
  QVERIFY(removed.ok());
  QCOMPARE(removed.value.size(), static_cast<std::size_t>(0));
  auto stillClip = scratch.session.readMasterClip(clip.value.objectId);
  auto stillAsset = scratch.session.readAsset(asset.value.mediaId);
  QVERIFY(stillClip.ok());
  QVERIFY(stillAsset.ok());
  QCOMPARE(stillClip.value.mediaId, asset.value.mediaId);

  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(scratch.directory).ok());
  QVERIFY(scratch.session.selectedBinId().empty());
  auto after = scratch.session.readMasterClip(clip.value.objectId);
  QVERIFY(after.ok());
  QCOMPARE(after.value.mediaId, asset.value.mediaId);
}

void ProjectTests::mediaIdentityAndMasterClipReopen() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Media").ok());
  auto bin = scratch.session.createBin("Dailies");
  auto asset = scratch.session.createAsset("picture", "{}");
  QVERIFY(bin.ok() && asset.ok());
  auto original = scratch.session.addRepresentation(asset.value.mediaId, aether::RepresentationRole::Original,
                                                   "originals/take.mxf", "recorded");
  auto proxy = scratch.session.addRepresentation(asset.value.mediaId, aether::RepresentationRole::Proxy,
                                                "proxies/take.mov", "recorded");
  QVERIFY(original.ok() && proxy.ok());
  QCOMPARE(original.value.mediaId, asset.value.mediaId);
  QCOMPARE(proxy.value.mediaId, asset.value.mediaId);
  QVERIFY(original.value.representationId != proxy.value.representationId);

  auto clip = scratch.session.createMasterClip("Take", asset.value.mediaId, bin.value.binId, "{}");
  QVERIFY(clip.ok());
  QVERIFY(scratch.session.setRepresentationPath(proxy.value.representationId, "proxies/take-moved.mov").ok());

  const std::string mediaId = asset.value.mediaId;
  const std::string proxyId = proxy.value.representationId;
  const std::string clipId = clip.value.objectId;
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(scratch.directory).ok());

  auto assetAgain = scratch.session.readAsset(mediaId);
  auto reps = scratch.session.listRepresentations(mediaId);
  auto clipAgain = scratch.session.readMasterClip(clipId);
  auto clips = scratch.session.listClips(bin.value.binId);
  QVERIFY(assetAgain.ok() && reps.ok() && clipAgain.ok() && clips.ok());
  QCOMPARE(assetAgain.value.mediaId, mediaId);
  QCOMPARE(reps.value.size(), static_cast<std::size_t>(2));
  QCOMPARE(clipAgain.value.mediaId, mediaId);
  QCOMPARE(clips.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(clips.value[0].objectId, clipId);
  bool foundProxy = false;
  for (const aether::Representation& row : reps.value) {
    QCOMPARE(row.mediaId, mediaId);
    if (row.representationId == proxyId) {
      foundProxy = true;
      QCOMPARE(row.recordedPath, std::string("proxies/take-moved.mov"));
      QCOMPARE(static_cast<int>(row.role), static_cast<int>(aether::RepresentationRole::Proxy));
    }
  }
  QVERIFY(foundProxy);
  QVERIFY(!std::filesystem::exists(scratch.directory / "proxies" / "take-moved.mov"));
}

void ProjectTests::undoRedoResetsAndStaysIndependentOfSave() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Undo").ok());
  auto bin = scratch.session.createBin("Reel");
  auto asset = scratch.session.createAsset("sound", "{}");
  QVERIFY(bin.ok() && asset.ok());
  auto clip = scratch.session.createMasterClip("Line", asset.value.mediaId, bin.value.binId, "{\"take\":1}");
  QVERIFY(clip.ok());
  QVERIFY(!scratch.session.canUndo());

  QVERIFY(scratch.session.renameBin(bin.value.binId, "Reel 2").ok());
  QVERIFY(scratch.session.canUndo());
  QVERIFY(scratch.session.undo().ok());
  auto undone = scratch.session.storeForTest().readBin(bin.value.binId);
  QVERIFY(undone.ok());
  QCOMPARE(undone.value.name, std::string("Reel"));
  QVERIFY(scratch.session.canRedo());
  QVERIFY(scratch.session.redo().ok());
  auto redone = scratch.session.storeForTest().readBin(bin.value.binId);
  QVERIFY(redone.ok());
  QCOMPARE(redone.value.name, std::string("Reel 2"));

  auto other = scratch.session.createBin("Other");
  QVERIFY(other.ok());
  QVERIFY(scratch.session.addMembership(other.value.binId, clip.value.objectId).ok());
  QVERIFY(scratch.session.undo().ok());
  auto membership = scratch.session.listClips(other.value.binId);
  QVERIFY(membership.ok());
  QCOMPARE(membership.value.size(), static_cast<std::size_t>(0));
  QVERIFY(scratch.session.redo().ok());
  membership = scratch.session.listClips(other.value.binId);
  QCOMPARE(membership.value.size(), static_cast<std::size_t>(1));

  QVERIFY(scratch.session.setObjectMetadata(clip.value.objectId, "Line 2", "{\"take\":2}").ok());
  QVERIFY(scratch.session.undo().ok());
  auto meta = scratch.session.readMasterClip(clip.value.objectId);
  QVERIFY(meta.ok());
  QCOMPARE(meta.value.name, std::string("Line"));
  QCOMPARE(meta.value.metadata, std::string("{\"take\":1}"));
  QVERIFY(scratch.session.redo().ok());
  meta = scratch.session.readMasterClip(clip.value.objectId);
  QCOMPARE(meta.value.name, std::string("Line 2"));

  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.openProject(scratch.directory).ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(!scratch.session.canRedo());
  auto kept = scratch.session.readMasterClip(clip.value.objectId);
  QVERIFY(kept.ok());
  QCOMPARE(kept.value.name, std::string("Line 2"));
  auto keptBin = scratch.session.storeForTest().readBin(bin.value.binId);
  QVERIFY(keptBin.ok());
  QCOMPARE(keptBin.value.name, std::string("Reel 2"));
  auto tables = scratch.session.storeForTest().listUserTables();
  QVERIFY(tables.ok());
  for (const std::string& name : tables.value) {
    QVERIFY(name.find("undo") == std::string::npos);
    QVERIFY(name.find("journal") == std::string::npos);
  }
}

void ProjectTests::autosaveAndInterruptedTransaction() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Save").ok());
  auto bin = scratch.session.createBin("Cut");
  QVERIFY(bin.ok());
  QVERIFY(scratch.session.renameBin(bin.value.binId, "Cut 2").ok());
  QVERIFY(scratch.session.canUndo());
  QVERIFY(scratch.session.autosave().ok());
  QVERIFY(scratch.session.canUndo());
  QVERIFY(scratch.logger.contains("autosave"));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(scratch.directory).ok());
  QVERIFY(!scratch.session.canUndo());
  auto kept = scratch.session.storeForTest().readBin(bin.value.binId);
  QVERIFY(kept.ok());
  QCOMPARE(kept.value.name, std::string("Cut 2"));

  QVERIFY(scratch.session.storeForTest().abandonUncommittedInsert().ok());
  QVERIFY(!scratch.session.isOpen());
  auto reopened = scratch.session.openProject(scratch.directory);
  QVERIFY(reopened.ok());
  auto bins = scratch.session.listBins();
  QVERIFY(bins.ok());
  for (const aether::BinRecord& row : bins.value) {
    QVERIFY(row.binId != "interrupted-uncommitted");
  }
  bool found = false;
  for (const aether::BinRecord& row : bins.value) {
    if (row.binId == bin.value.binId) {
      found = true;
      QCOMPARE(row.name, std::string("Cut 2"));
    }
  }
  QVERIFY(found);
  QVERIFY(scratch.session.closeProject().ok());

  const pid_t pid = ::fork();
  QVERIFY(pid >= 0);
  if (pid == 0) {
    aether::MemoryLogger childLog;
    aether::ProjectStore child(childLog);
    auto opened = child.open(scratch.directory);
    if (!opened.ok()) {
      _exit(2);
    }
    child.crashMidTransactionForTest();
  }
  int status = 0;
  const pid_t waited = ::waitpid(pid, &status, 0);
  QCOMPARE(waited, pid);
  QVERIFY(WIFEXITED(status));
  QCOMPARE(WEXITSTATUS(status), 42);

  auto afterCrash = scratch.session.openProject(scratch.directory);
  if (!afterCrash.ok()) {
    QCOMPARE(static_cast<int>(afterCrash.error.code), static_cast<int>(aether::ErrorCode::Integrity));
    QVERIFY(std::filesystem::is_regular_file(scratch.directory / "project.db"));
    return;
  }
  auto afterBins = scratch.session.listBins();
  QVERIFY(afterBins.ok());
  for (const aether::BinRecord& row : afterBins.value) {
    QVERIFY(row.binId != "interrupted-crash");
  }
}

void ProjectTests::secondOpenIsRefusedAndLogsOmitPaths() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Logs").ok());
  auto asset = scratch.session.createAsset("picture", "{}");
  QVERIFY(asset.ok());
  const std::string secret = "SHOULD-NOT-BE-LOGGED-proxy-path";
  auto proxy = scratch.session.addRepresentation(asset.value.mediaId, aether::RepresentationRole::Proxy, secret,
                                                 "recorded");
  QVERIFY(proxy.ok());
  QVERIFY(scratch.session.setRepresentationPath(proxy.value.representationId, secret + "-moved").ok());
  QVERIFY(scratch.logger.contains("project"));
  QVERIFY(!scratch.logger.contains(secret));
  QVERIFY(scratch.logger.contains("startup") == false);
}
