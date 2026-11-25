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

#include "aether/persistence/project_store.hpp"

#include "aether/core/project/ids.hpp"
#include "aether/persistence/manifest.hpp"

#include <sqlite3.h>

#include <sys/file.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <utility>

namespace aether {
namespace {

const char* kSchemaSql[] = {
    "CREATE TABLE schema_info ("
    "  singleton INTEGER PRIMARY KEY CHECK (singleton = 1),"
    "  schema_version INTEGER NOT NULL"
    ");",
    "CREATE TABLE project_meta ("
    "  project_id TEXT PRIMARY KEY,"
    "  name TEXT NOT NULL,"
    "  created_utc TEXT NOT NULL,"
    "  modified_utc TEXT NOT NULL"
    ");",
    "CREATE TABLE bins ("
    "  bin_id TEXT PRIMARY KEY,"
    "  project_id TEXT NOT NULL REFERENCES project_meta(project_id),"
    "  name TEXT NOT NULL,"
    "  created_utc TEXT NOT NULL,"
    "  modified_utc TEXT NOT NULL"
    ");",
    "CREATE TABLE editorial_objects ("
    "  object_id TEXT PRIMARY KEY,"
    "  project_id TEXT NOT NULL REFERENCES project_meta(project_id),"
    "  kind TEXT NOT NULL CHECK (kind = 'master_clip'),"
    "  name TEXT NOT NULL,"
    "  metadata_json TEXT NOT NULL,"
    "  created_utc TEXT NOT NULL,"
    "  modified_utc TEXT NOT NULL"
    ");",
    "CREATE TABLE bin_membership ("
    "  bin_id TEXT NOT NULL REFERENCES bins(bin_id),"
    "  object_id TEXT NOT NULL REFERENCES editorial_objects(object_id),"
    "  created_utc TEXT NOT NULL,"
    "  PRIMARY KEY (bin_id, object_id)"
    ");",
    "CREATE TABLE media_assets ("
    "  media_id TEXT PRIMARY KEY,"
    "  project_id TEXT NOT NULL REFERENCES project_meta(project_id),"
    "  media_type TEXT NOT NULL,"
    "  metadata_json TEXT NOT NULL,"
    "  created_utc TEXT NOT NULL,"
    "  modified_utc TEXT NOT NULL"
    ");",
    "CREATE TABLE media_representations ("
    "  representation_id TEXT PRIMARY KEY,"
    "  media_id TEXT NOT NULL REFERENCES media_assets(media_id),"
    "  role TEXT NOT NULL CHECK (role IN ('original', 'proxy', 'transcode', 'consolidated')),"
    "  recorded_path TEXT NOT NULL,"
    "  status TEXT NOT NULL,"
    "  created_utc TEXT NOT NULL,"
    "  modified_utc TEXT NOT NULL"
    ");",
    "CREATE TABLE master_clips ("
    "  object_id TEXT PRIMARY KEY REFERENCES editorial_objects(object_id),"
    "  media_id TEXT NOT NULL REFERENCES media_assets(media_id)"
    ");",
};

// Additive Slice 1A tables. Applied to a new database and, inside one
// transaction, to a schema-version-1 database. No path column. No sample column.
const char* kSequenceSchemaSql[] = {
    "CREATE TABLE sequences ("
    "  sequence_id TEXT PRIMARY KEY,"
    "  project_id TEXT NOT NULL REFERENCES project_meta(project_id),"
    "  name TEXT NOT NULL,"
    "  rate_numerator INTEGER NOT NULL,"
    "  rate_denominator INTEGER NOT NULL,"
    "  created_utc TEXT NOT NULL,"
    "  modified_utc TEXT NOT NULL,"
    "  CHECK (rate_numerator > 0),"
    "  CHECK (rate_denominator > 0)"
    ");",
    "CREATE TABLE tracks ("
    "  track_id TEXT PRIMARY KEY,"
    "  sequence_id TEXT NOT NULL REFERENCES sequences(sequence_id),"
    "  ordinal INTEGER NOT NULL,"
    "  created_utc TEXT NOT NULL,"
    "  modified_utc TEXT NOT NULL,"
    "  CHECK (ordinal >= 0),"
    "  UNIQUE (sequence_id, ordinal)"
    ");",
    "CREATE TABLE timeline_instances ("
    "  instance_id TEXT PRIMARY KEY,"
    "  sequence_id TEXT NOT NULL REFERENCES sequences(sequence_id),"
    "  track_id TEXT NOT NULL REFERENCES tracks(track_id),"
    "  master_object_id TEXT NOT NULL REFERENCES master_clips(object_id),"
    "  media_id TEXT NOT NULL REFERENCES media_assets(media_id),"
    "  source_rate_numerator INTEGER NOT NULL,"
    "  source_rate_denominator INTEGER NOT NULL,"
    "  source_start INTEGER NOT NULL,"
    "  source_end INTEGER NOT NULL,"
    "  record_start INTEGER NOT NULL,"
    "  record_end INTEGER NOT NULL,"
    "  created_utc TEXT NOT NULL,"
    "  modified_utc TEXT NOT NULL,"
    "  CHECK (source_rate_numerator > 0),"
    "  CHECK (source_rate_denominator > 0),"
    "  CHECK (source_start >= 0),"
    "  CHECK (source_end > source_start),"
    "  CHECK (record_start >= 0),"
    "  CHECK (record_end > record_start)"
    ");",
};

bool migrationHookAllows(int found, std::string& detail) {
  if (found == kPrototypeSchemaVersion || found == kSchemaVersionSlice0A) {
    return true;
  }
  detail = "schema migration hook refused version " + std::to_string(found) +
           "; this prototype opens schema version 1 by migrating it to 2, or opens schema version 2, "
           "and does not modify a file at any other version";
  return false;
}

class Statement {
 public:
  ~Statement() {
    if (raw != nullptr) {
      sqlite3_finalize(raw);
    }
  }
  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;
  Statement() = default;

  int prepare(sqlite3* db, const char* sql) {
    return sqlite3_prepare_v2(db, sql, -1, &raw, nullptr);
  }

  sqlite3_stmt* raw = nullptr;
};

bool bindText(sqlite3_stmt* statement, int index, const std::string& value) {
  return sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int index) {
  const unsigned char* text = sqlite3_column_text(statement, index);
  if (text == nullptr) {
    return {};
  }
  return reinterpret_cast<const char*>(text);
}

std::string normaliseMetadata(const std::string& metadata) {
  if (metadata.empty()) {
    return "{}";
  }
  return metadata;
}

bool validMetadata(const std::string& metadata) {
  return metadata.size() <= 65536;
}

bool validToken(const std::string& value, std::size_t max) {
  return !value.empty() && value.size() <= max;
}

}  // namespace

struct ProjectStore::Impl {
  explicit Impl(Logger& log) : logger(log) {}

  Logger& logger;
  sqlite3* db = nullptr;
  int lockFd = -1;
  bool open = false;
  bool failNextMigration = false;
  bool failNextMutation = false;
  ProjectInfo info;

  void unlock() {
    if (lockFd >= 0) {
      ::flock(lockFd, LOCK_UN);
      ::close(lockFd);
      lockFd = -1;
    }
  }

  void closeDb() {
    if (db == nullptr) {
      return;
    }
    const int rc = sqlite3_close(db);
    if (rc != SQLITE_OK) {
      sqlite3_close_v2(db);
    }
    db = nullptr;
  }

  void release() {
    closeDb();
    unlock();
    open = false;
  }

  Error fromSqlite(int rc, const char* context) const {
    const int extended = db == nullptr ? rc : sqlite3_extended_errcode(db);
    std::string message = std::string(context);
    if (db != nullptr && sqlite3_errmsg(db) != nullptr) {
      message += ": ";
      message += sqlite3_errmsg(db);
    }
    if (extended == SQLITE_CONSTRAINT_FOREIGNKEY) {
      return {ErrorCode::ForeignKey, message};
    }
    if (rc == SQLITE_CONSTRAINT || extended == SQLITE_CONSTRAINT_PRIMARYKEY ||
        extended == SQLITE_CONSTRAINT_UNIQUE || extended == SQLITE_CONSTRAINT_CHECK) {
      return {ErrorCode::Constraint, message};
    }
    if (rc == SQLITE_BUSY) {
      return {ErrorCode::LockHeld, message};
    }
    return {ErrorCode::Database, message};
  }

  Result<void> exec(const char* sql) {
    char* message = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &message);
    if (rc != SQLITE_OK) {
      Error error = fromSqlite(rc, "statement failed");
      if (message != nullptr) {
        error.message += " (";
        error.message += message;
        error.message += ")";
        sqlite3_free(message);
      }
      logger.error("database", error.message);
      return Result<void>::failure(error.code, error.message);
    }
    return Result<void>::success();
  }

  Result<void> configureConnection() {
    if (sqlite3_extended_result_codes(db, 1) != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "extended result codes could not be enabled");
    }
    auto keys = exec("PRAGMA foreign_keys = ON;");
    if (!keys.ok()) {
      return keys;
    }
    Statement statement;
    if (statement.prepare(db, "PRAGMA foreign_keys;") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "foreign key pragma could not be read");
    }
    if (sqlite3_step(statement.raw) != SQLITE_ROW || sqlite3_column_int(statement.raw, 0) != 1) {
      logger.error("database", "foreign keys are off for this connection");
      return Result<void>::failure(ErrorCode::Database, "foreign keys are off for this connection");
    }
    auto synchronous = exec("PRAGMA synchronous = FULL;");
    if (!synchronous.ok()) {
      return synchronous;
    }
    return Result<void>::success();
  }

  Result<void> health() {
    Statement integrity;
    if (integrity.prepare(db, "PRAGMA integrity_check;") != SQLITE_OK) {
      logger.error("database", "integrity_check could not be prepared");
      return Result<void>::failure(ErrorCode::Integrity, "integrity_check could not be prepared");
    }
    int rows = 0;
    std::string first;
    while (true) {
      const int rc = sqlite3_step(integrity.raw);
      if (rc == SQLITE_ROW) {
        ++rows;
        if (rows == 1) {
          first = columnText(integrity.raw, 0);
        }
        continue;
      }
      if (rc == SQLITE_DONE) {
        break;
      }
      logger.error("database", "integrity_check failed to run");
      return Result<void>::failure(ErrorCode::Integrity, "integrity_check failed to run");
    }
    if (rows != 1 || first != "ok") {
      logger.error("database", "integrity_check did not return ok");
      return Result<void>::failure(ErrorCode::Integrity, "integrity_check did not return ok");
    }

    Statement foreign;
    if (foreign.prepare(db, "PRAGMA foreign_key_check;") != SQLITE_OK) {
      logger.error("database", "foreign_key_check could not be prepared");
      return Result<void>::failure(ErrorCode::ForeignKey, "foreign_key_check could not be prepared");
    }
    const int foreignRc = sqlite3_step(foreign.raw);
    if (foreignRc == SQLITE_ROW) {
      logger.error("database", "foreign_key_check found a violation");
      return Result<void>::failure(ErrorCode::ForeignKey, "foreign_key_check found a violation");
    }
    if (foreignRc != SQLITE_DONE) {
      logger.error("database", "foreign_key_check failed to run");
      return Result<void>::failure(ErrorCode::ForeignKey, "foreign_key_check failed to run");
    }
    return Result<void>::success();
  }

  Result<int> readSchemaVersion() {
    Statement statement;
    if (statement.prepare(db, "SELECT schema_version FROM schema_info WHERE singleton = 1;") != SQLITE_OK) {
      return Result<int>::failure(ErrorCode::Schema, "schema version could not be read");
    }
    const int rc = sqlite3_step(statement.raw);
    if (rc != SQLITE_ROW) {
      logger.error("schema", "schema version row is missing");
      return Result<int>::failure(ErrorCode::Schema, "schema version row is missing");
    }
    const int version = sqlite3_column_int(statement.raw, 0);
    if (sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<int>::failure(ErrorCode::Schema, "schema version is not a singleton");
    }
    return Result<int>::success(version);
  }

  Result<void> loadProjectRow() {
    Statement statement;
    if (statement.prepare(db,
                          "SELECT project_id, name, created_utc, modified_utc FROM project_meta;") !=
        SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "project row could not be read");
    }
    if (sqlite3_step(statement.raw) != SQLITE_ROW) {
      logger.error("database", "project row is missing");
      return Result<void>::failure(ErrorCode::Database, "project row is missing");
    }
    info.projectId = columnText(statement.raw, 0);
    info.name = columnText(statement.raw, 1);
    info.createdUtc = columnText(statement.raw, 2);
    info.modifiedUtc = columnText(statement.raw, 3);
    if (sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "more than one project row");
    }
    return Result<void>::success();
  }

  Result<void> touchModified(const std::string& stamp) {
    Statement statement;
    if (statement.prepare(db, "UPDATE project_meta SET modified_utc = ? WHERE project_id = ?;") != SQLITE_OK) {
      return Result<void>::failure(fromSqlite(SQLITE_ERROR, "modified stamp could not be prepared").code,
                                   "modified stamp could not be prepared");
    }
    bindText(statement.raw, 1, stamp);
    bindText(statement.raw, 2, info.projectId);
    if (sqlite3_step(statement.raw) != SQLITE_DONE) {
      logger.error("transaction", "modified stamp was not written");
      return Result<void>::failure(ErrorCode::Database, "modified stamp was not written");
    }
    info.modifiedUtc = stamp;
    return Result<void>::success();
  }

  Result<void> migrateSequenceSchema() {
    if (failNextMigration) {
      failNextMigration = false;
      auto started = begin();
      if (!started.ok()) {
        return started;
      }
      rollback();
      logger.error("schema", "schema migration failed and was rolled back");
      return Result<void>::failure(ErrorCode::Database, "schema migration failed and was rolled back");
    }
    auto started = begin();
    if (!started.ok()) {
      return started;
    }
    for (const char* sql : kSequenceSchemaSql) {
      auto created = exec(sql);
      if (!created.ok()) {
        rollback();
        logger.error("schema", "schema migration failed and was rolled back");
        return Result<void>::failure(created.error.code, "schema migration failed and was rolled back");
      }
    }
    Statement update;
    if (update.prepare(db, "UPDATE schema_info SET schema_version = ? WHERE singleton = 1;") != SQLITE_OK ||
        sqlite3_bind_int(update.raw, 1, kPrototypeSchemaVersion) != SQLITE_OK ||
        sqlite3_step(update.raw) != SQLITE_DONE) {
      rollback();
      logger.error("schema", "schema migration failed and was rolled back");
      return Result<void>::failure(ErrorCode::Database, "schema migration failed and was rolled back");
    }
    auto committed = commit();
    if (!committed.ok()) {
      return committed;
    }
    info.schemaVersion = kPrototypeSchemaVersion;
    auto manifest = rewriteManifest();
    if (!manifest.ok()) {
      logger.error("schema", "sequence schema committed; manifest stamp was not replaced");
    }
    return Result<void>::success();
  }

  Result<void> rewriteManifest() {
    info.formatVersion = kPrototypeFormatVersion;
    info.databaseFilename = kDatabaseFilename;
    info.prototypeVersion = kPrototypeVersion;
    info.appVersion = kPrototypeAppVersion;
    const auto written = writeManifestAtomic(std::filesystem::path(info.directory) / kManifestFilename, info);
    if (!written.ok()) {
      logger.error("manifest", written.error.message);
      logger.error("recovery", "database commit is kept; manifest stamp was not replaced");
    }
    return written;
  }

  Result<void> begin() { return exec("BEGIN IMMEDIATE;"); }

  Result<void> commit() {
    auto committed = exec("COMMIT;");
    if (!committed.ok()) {
      logger.error("transaction", "commit failed");
      exec("ROLLBACK;");
    }
    return committed;
  }

  void rollback() {
    auto rolled = exec("ROLLBACK;");
    if (!rolled.ok()) {
      logger.error("transaction", "rollback failed");
    }
  }

  template <typename Body>
  Result<void> mutate(Body body) {
    if (!open || db == nullptr) {
      return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
    }
    auto started = begin();
    if (!started.ok()) {
      logger.error("transaction", started.error.message);
      return started;
    }
    if (failNextMutation) {
      failNextMutation = false;
      rollback();
      return Result<void>::failure(ErrorCode::Database, "write failed and was rolled back");
    }
    auto did = body();
    if (!did.ok()) {
      rollback();
      logger.error("transaction", did.error.message);
      return did;
    }
    auto stamped = touchModified(utcTimestamp());
    if (!stamped.ok()) {
      rollback();
      return stamped;
    }
    auto committed = commit();
    if (!committed.ok()) {
      return committed;
    }
    auto manifest = rewriteManifest();
    if (!manifest.ok()) {
      // The editorial commit already succeeded. Do not ask the caller to repeat it.
      return Result<void>::success();
    }
    return Result<void>::success();
  }

  Result<void> takeLock(const std::filesystem::path& directory) {
    const std::filesystem::path lockPath = directory / "project.lock";
    lockFd = ::open(lockPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (lockFd < 0) {
      logger.error("project", "project lock could not be created");
      return Result<void>::failure(ErrorCode::Io, "project lock could not be created");
    }
    if (::flock(lockFd, LOCK_EX | LOCK_NB) != 0) {
      ::close(lockFd);
      lockFd = -1;
      logger.error("project", "project lock is held");
      return Result<void>::failure(ErrorCode::LockHeld, "project lock is held");
    }
    return Result<void>::success();
  }

  void discardCreated(const std::filesystem::path& directory) {
    release();
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
    if (ec) {
      logger.error("project", "unpublished project directory could not be removed");
    }
  }
};

ProjectStore::ProjectStore(Logger& logger) : impl_(std::make_unique<Impl>(logger)) {}

ProjectStore::~ProjectStore() { impl_->release(); }

bool ProjectStore::isOpen() const { return impl_->open; }

const ProjectInfo& ProjectStore::info() const { return impl_->info; }

Result<ProjectInfo> ProjectStore::create(const std::filesystem::path& directory, const std::string& name) {
  if (impl_->open) {
    return Result<ProjectInfo>::failure(ErrorCode::InvalidArgument, "a project is already open");
  }
  if (!validDisplayName(name)) {
    return Result<ProjectInfo>::failure(ErrorCode::InvalidArgument, "project name is empty or not a single display name");
  }
  std::error_code existsEc;
  if (std::filesystem::exists(directory, existsEc)) {
    impl_->logger.error("project", "project create refused because the directory exists");
    return Result<ProjectInfo>::failure(ErrorCode::Io, "project directory already exists");
  }
  if (!std::filesystem::create_directory(directory, existsEc) || existsEc) {
    impl_->logger.error("project", "project directory could not be created");
    return Result<ProjectInfo>::failure(ErrorCode::Io, "project directory could not be created");
  }

  const char* children[] = {"BINS", "MEDIA", "CACHE", "BACKUP", "LOGS"};
  for (const char* child : children) {
    std::error_code ec;
    if (!std::filesystem::create_directory(directory / child, ec) || ec) {
      impl_->discardCreated(directory);
      impl_->logger.error("project", "project subdirectory could not be created");
      return Result<ProjectInfo>::failure(ErrorCode::Io, "project subdirectory could not be created");
    }
  }

  auto locked = impl_->takeLock(directory);
  if (!locked.ok()) {
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(locked.error.code, locked.error.message);
  }

  const std::filesystem::path dbPath = directory / kDatabaseFilename;
  const int openFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  if (sqlite3_open_v2(dbPath.c_str(), &impl_->db, openFlags, nullptr) != SQLITE_OK) {
    impl_->logger.error("database", "project database could not be created");
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(ErrorCode::Database, "project database could not be created");
  }

  auto configured = impl_->configureConnection();
  if (!configured.ok()) {
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(configured.error.code, configured.error.message);
  }
  auto journal = impl_->exec("PRAGMA journal_mode = DELETE;");
  if (!journal.ok()) {
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(journal.error.code, journal.error.message);
  }

  const auto minted = makeStableId();
  if (!minted) {
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(ErrorCode::Io, "project id could not be minted");
  }
  const std::string stamp = utcTimestamp();
  impl_->info = ProjectInfo{};
  impl_->info.projectId = *minted;
  impl_->info.name = name;
  impl_->info.formatVersion = kPrototypeFormatVersion;
  impl_->info.databaseFilename = kDatabaseFilename;
  impl_->info.schemaVersion = kPrototypeSchemaVersion;
  impl_->info.createdUtc = stamp;
  impl_->info.modifiedUtc = stamp;
  impl_->info.prototypeVersion = kPrototypeVersion;
  impl_->info.appVersion = kPrototypeAppVersion;
  impl_->info.directory = directory.string();

  auto started = impl_->begin();
  if (!started.ok()) {
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(started.error.code, started.error.message);
  }
  for (const char* sql : kSchemaSql) {
    auto created = impl_->exec(sql);
    if (!created.ok()) {
      impl_->discardCreated(directory);
      return Result<ProjectInfo>::failure(created.error.code, created.error.message);
    }
  }
  for (const char* sql : kSequenceSchemaSql) {
    auto created = impl_->exec(sql);
    if (!created.ok()) {
      impl_->discardCreated(directory);
      return Result<ProjectInfo>::failure(created.error.code, created.error.message);
    }
  }
  bool rowsOk = false;
  {
    Statement schemaInsert;
    Statement projectInsert;
    rowsOk = schemaInsert.prepare(impl_->db, "INSERT INTO schema_info (singleton, schema_version) VALUES (1, ?);") ==
                 SQLITE_OK &&
             sqlite3_bind_int(schemaInsert.raw, 1, kPrototypeSchemaVersion) == SQLITE_OK &&
             sqlite3_step(schemaInsert.raw) == SQLITE_DONE &&
             projectInsert.prepare(impl_->db,
                                   "INSERT INTO project_meta (project_id, name, created_utc, modified_utc) "
                                   "VALUES (?, ?, ?, ?);") == SQLITE_OK &&
             bindText(projectInsert.raw, 1, impl_->info.projectId) &&
             bindText(projectInsert.raw, 2, impl_->info.name) && bindText(projectInsert.raw, 3, stamp) &&
             bindText(projectInsert.raw, 4, stamp) && sqlite3_step(projectInsert.raw) == SQLITE_DONE;
  }
  if (!rowsOk) {
    impl_->logger.error("database", "project rows could not be inserted");
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(ErrorCode::Database, "project rows could not be inserted");
  }
  auto committed = impl_->commit();
  if (!committed.ok()) {
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(committed.error.code, committed.error.message);
  }
  auto healthy = impl_->health();
  if (!healthy.ok()) {
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(healthy.error.code, healthy.error.message);
  }
  auto manifest = impl_->rewriteManifest();
  if (!manifest.ok()) {
    impl_->logger.error("project", "project create did not publish a project id");
    impl_->discardCreated(directory);
    return Result<ProjectInfo>::failure(manifest.error.code, manifest.error.message);
  }

  impl_->open = true;
  impl_->logger.setLogDirectory((directory / "LOGS").string());
  impl_->logger.info("project", "created project " + name);
  return Result<ProjectInfo>::success(impl_->info);
}

Result<ProjectInfo> ProjectStore::open(const std::filesystem::path& directory) {
  if (impl_->open) {
    return Result<ProjectInfo>::failure(ErrorCode::InvalidArgument, "a project is already open");
  }
  const auto manifest = readManifest(directory / kManifestFilename);
  if (!manifest.ok()) {
    impl_->logger.error("manifest", manifest.error.message);
    return Result<ProjectInfo>::failure(manifest.error.code, manifest.error.message);
  }
  if (manifest.value.info.formatVersion != kPrototypeFormatVersion ||
      manifest.value.info.databaseFilename != kDatabaseFilename) {
    impl_->logger.error("manifest", "manifest package fields are not this prototype");
    return Result<ProjectInfo>::failure(ErrorCode::ManifestInvalid,
                                        "manifest package fields are not this prototype");
  }

  const std::filesystem::path dbPath = directory / kDatabaseFilename;
  std::error_code fileEc;
  if (!std::filesystem::is_regular_file(dbPath, fileEc) || fileEc) {
    impl_->logger.error("database", "project database is missing");
    return Result<ProjectInfo>::failure(ErrorCode::DatabaseMissing,
                                        "project database is missing and was not created");
  }

  auto locked = impl_->takeLock(directory);
  if (!locked.ok()) {
    return Result<ProjectInfo>::failure(locked.error.code, locked.error.message);
  }
  if (sqlite3_open_v2(dbPath.c_str(), &impl_->db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
    const std::string message = impl_->db != nullptr && sqlite3_errmsg(impl_->db) != nullptr
                                    ? sqlite3_errmsg(impl_->db)
                                    : "project database could not be opened";
    impl_->logger.error("database", message);
    impl_->release();
    return Result<ProjectInfo>::failure(ErrorCode::Database, message);
  }
  auto configured = impl_->configureConnection();
  if (!configured.ok()) {
    impl_->release();
    return Result<ProjectInfo>::failure(configured.error.code, configured.error.message);
  }
  auto healthy = impl_->health();
  if (!healthy.ok()) {
    impl_->release();
    return Result<ProjectInfo>::failure(healthy.error.code, healthy.error.message);
  }

  auto schema = impl_->readSchemaVersion();
  if (!schema.ok()) {
    impl_->release();
    return Result<ProjectInfo>::failure(schema.error.code, schema.error.message);
  }
  std::string migrationDetail;
  if (!migrationHookAllows(schema.value, migrationDetail)) {
    impl_->logger.error("schema", migrationDetail);
    impl_->release();
    return Result<ProjectInfo>::failure(ErrorCode::MigrationRefused, migrationDetail);
  }
  if (manifest.value.info.schemaVersion != schema.value) {
    impl_->logger.error("schema", "manifest schema version does not match the database");
    impl_->release();
    return Result<ProjectInfo>::failure(ErrorCode::ManifestMismatch,
                                        "manifest schema version does not match the database");
  }
  if (schema.value == kSchemaVersionSlice0A) {
    impl_->info = manifest.value.info;
    impl_->info.directory = directory.string();
    auto migrated = impl_->migrateSequenceSchema();
    if (!migrated.ok()) {
      impl_->release();
      impl_->info = ProjectInfo{};
      return Result<ProjectInfo>::failure(migrated.error.code, migrated.error.message);
    }
    schema.value = kPrototypeSchemaVersion;
  }

  auto loaded = impl_->loadProjectRow();
  if (!loaded.ok()) {
    impl_->release();
    return Result<ProjectInfo>::failure(loaded.error.code, loaded.error.message);
  }
  if (impl_->info.projectId != manifest.value.info.projectId) {
    impl_->logger.error("manifest", "manifest project id does not match the database");
    impl_->release();
    impl_->info = ProjectInfo{};
    return Result<ProjectInfo>::failure(ErrorCode::ManifestMismatch,
                                        "manifest project id does not match the database");
  }
  if (impl_->info.modifiedUtc != manifest.value.info.modifiedUtc ||
      impl_->info.name != manifest.value.info.name) {
    impl_->logger.error("recovery",
                        "manifest name or modified stamp lags the committed database; manifest was not rewritten");
  }

  impl_->info.formatVersion = manifest.value.info.formatVersion;
  impl_->info.databaseFilename = manifest.value.info.databaseFilename;
  impl_->info.schemaVersion = schema.value;
  impl_->info.prototypeVersion = manifest.value.info.prototypeVersion;
  impl_->info.appVersion = manifest.value.info.appVersion;
  impl_->info.directory = directory.string();
  impl_->open = true;
  impl_->logger.setLogDirectory((directory / "LOGS").string());
  impl_->logger.info("project", "opened project " + impl_->info.name);
  return Result<ProjectInfo>::success(impl_->info);
}

Result<void> ProjectStore::close() {
  if (!impl_->open) {
    return Result<void>::success();
  }
  const std::string name = impl_->info.name;
  impl_->logger.info("project", "closed project " + name);
  impl_->logger.setLogDirectory("");
  impl_->release();
  impl_->info = ProjectInfo{};
  return Result<void>::success();
}

Result<BinRecord> ProjectStore::insertBin(const std::string& name) {
  if (!validDisplayName(name)) {
    return Result<BinRecord>::failure(ErrorCode::InvalidArgument, "bin name is empty or rejected");
  }
  const auto minted = makeStableId();
  if (!minted) {
    return Result<BinRecord>::failure(ErrorCode::Io, "bin id could not be minted");
  }
  BinRecord bin;
  bin.binId = *minted;
  bin.projectId = impl_->info.projectId;
  bin.name = name;
  bin.createdUtc = utcTimestamp();
  bin.modifiedUtc = bin.createdUtc;
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "INSERT INTO bins (bin_id, project_id, name, created_utc, modified_utc) "
                          "VALUES (?, ?, ?, ?, ?);") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "bin insert could not be prepared");
    }
    if (!bindText(statement.raw, 1, bin.binId) || !bindText(statement.raw, 2, bin.projectId) ||
        !bindText(statement.raw, 3, bin.name) || !bindText(statement.raw, 4, bin.createdUtc) ||
        !bindText(statement.raw, 5, bin.modifiedUtc) || sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(impl_->fromSqlite(sqlite3_errcode(impl_->db), "bin insert failed").code,
                                   "bin insert failed");
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<BinRecord>::failure(wrote.error.code, wrote.error.message);
  }
  return Result<BinRecord>::success(bin);
}

Result<std::vector<BinRecord>> ProjectStore::listBins() {
  if (!impl_->open) {
    return Result<std::vector<BinRecord>>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT bin_id, project_id, name, created_utc, modified_utc FROM bins "
                        "ORDER BY created_utc, bin_id;") != SQLITE_OK) {
    return Result<std::vector<BinRecord>>::failure(ErrorCode::Database, "bin list could not be prepared");
  }
  std::vector<BinRecord> bins;
  while (sqlite3_step(statement.raw) == SQLITE_ROW) {
    BinRecord bin;
    bin.binId = columnText(statement.raw, 0);
    bin.projectId = columnText(statement.raw, 1);
    bin.name = columnText(statement.raw, 2);
    bin.createdUtc = columnText(statement.raw, 3);
    bin.modifiedUtc = columnText(statement.raw, 4);
    bins.push_back(std::move(bin));
  }
  return Result<std::vector<BinRecord>>::success(std::move(bins));
}

Result<BinRecord> ProjectStore::readBin(const std::string& binId) {
  if (!impl_->open) {
    return Result<BinRecord>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT bin_id, project_id, name, created_utc, modified_utc FROM bins "
                        "WHERE bin_id = ?;") != SQLITE_OK) {
    return Result<BinRecord>::failure(ErrorCode::Database, "bin read could not be prepared");
  }
  bindText(statement.raw, 1, binId);
  if (sqlite3_step(statement.raw) != SQLITE_ROW) {
    return Result<BinRecord>::failure(ErrorCode::InvalidArgument, "bin was not found");
  }
  BinRecord bin;
  bin.binId = columnText(statement.raw, 0);
  bin.projectId = columnText(statement.raw, 1);
  bin.name = columnText(statement.raw, 2);
  bin.createdUtc = columnText(statement.raw, 3);
  bin.modifiedUtc = columnText(statement.raw, 4);
  return Result<BinRecord>::success(bin);
}

Result<void> ProjectStore::renameBin(const std::string& binId, const std::string& name) {
  if (!validDisplayName(name)) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "bin name is empty or rejected");
  }
  const std::string stamp = utcTimestamp();
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db, "UPDATE bins SET name = ?, modified_utc = ? WHERE bin_id = ?;") !=
        SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "bin rename could not be prepared");
    }
    if (!bindText(statement.raw, 1, name) || !bindText(statement.raw, 2, stamp) ||
        !bindText(statement.raw, 3, binId) || sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "bin rename failed");
    }
    if (sqlite3_changes(impl_->db) != 1) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "bin was not found");
    }
    return Result<void>::success();
  });
}

Result<MediaAsset> ProjectStore::insertAsset(const std::string& mediaType, const std::string& metadata) {
  if (!validToken(mediaType, 64) || !validMetadata(metadata)) {
    return Result<MediaAsset>::failure(ErrorCode::InvalidArgument, "media type or metadata was rejected");
  }
  const auto minted = makeStableId();
  if (!minted) {
    return Result<MediaAsset>::failure(ErrorCode::Io, "media id could not be minted");
  }
  MediaAsset asset;
  asset.mediaId = *minted;
  asset.projectId = impl_->info.projectId;
  asset.mediaType = mediaType;
  asset.metadata = normaliseMetadata(metadata);
  asset.createdUtc = utcTimestamp();
  asset.modifiedUtc = asset.createdUtc;
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "INSERT INTO media_assets "
                          "(media_id, project_id, media_type, metadata_json, created_utc, modified_utc) "
                          "VALUES (?, ?, ?, ?, ?, ?);") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "media insert could not be prepared");
    }
    if (!bindText(statement.raw, 1, asset.mediaId) || !bindText(statement.raw, 2, asset.projectId) ||
        !bindText(statement.raw, 3, asset.mediaType) || !bindText(statement.raw, 4, asset.metadata) ||
        !bindText(statement.raw, 5, asset.createdUtc) || !bindText(statement.raw, 6, asset.modifiedUtc) ||
        sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(impl_->fromSqlite(sqlite3_errcode(impl_->db), "media insert failed").code,
                                   "media insert failed");
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<MediaAsset>::failure(wrote.error.code, wrote.error.message);
  }
  return Result<MediaAsset>::success(asset);
}

Result<MediaAsset> ProjectStore::readAsset(const std::string& mediaId) {
  if (!impl_->open) {
    return Result<MediaAsset>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT media_id, project_id, media_type, metadata_json, created_utc, modified_utc "
                        "FROM media_assets WHERE media_id = ?;") != SQLITE_OK) {
    return Result<MediaAsset>::failure(ErrorCode::Database, "media read could not be prepared");
  }
  bindText(statement.raw, 1, mediaId);
  if (sqlite3_step(statement.raw) != SQLITE_ROW) {
    return Result<MediaAsset>::failure(ErrorCode::InvalidArgument, "media asset was not found");
  }
  MediaAsset asset;
  asset.mediaId = columnText(statement.raw, 0);
  asset.projectId = columnText(statement.raw, 1);
  asset.mediaType = columnText(statement.raw, 2);
  asset.metadata = columnText(statement.raw, 3);
  asset.createdUtc = columnText(statement.raw, 4);
  asset.modifiedUtc = columnText(statement.raw, 5);
  return Result<MediaAsset>::success(asset);
}

Result<Representation> ProjectStore::insertRepresentation(const std::string& mediaId,
                                                         RepresentationRole role,
                                                         const std::string& recordedPath,
                                                         const std::string& status) {
  if (!validToken(recordedPath, 4096) || !validToken(status, 64)) {
    return Result<Representation>::failure(ErrorCode::InvalidArgument, "representation path or status was rejected");
  }
  const auto minted = makeStableId();
  if (!minted) {
    return Result<Representation>::failure(ErrorCode::Io, "representation id could not be minted");
  }
  Representation representation;
  representation.representationId = *minted;
  representation.mediaId = mediaId;
  representation.role = role;
  representation.recordedPath = recordedPath;
  representation.status = status;
  representation.createdUtc = utcTimestamp();
  representation.modifiedUtc = representation.createdUtc;
  const char* roleName = representationRoleName(role);
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "INSERT INTO media_representations "
                          "(representation_id, media_id, role, recorded_path, status, created_utc, modified_utc) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?);") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "representation insert could not be prepared");
    }
    if (!bindText(statement.raw, 1, representation.representationId) ||
        !bindText(statement.raw, 2, representation.mediaId) || !bindText(statement.raw, 3, roleName) ||
        !bindText(statement.raw, 4, representation.recordedPath) ||
        !bindText(statement.raw, 5, representation.status) ||
        !bindText(statement.raw, 6, representation.createdUtc) ||
        !bindText(statement.raw, 7, representation.modifiedUtc)) {
      return Result<void>::failure(ErrorCode::Database, "representation insert could not be bound");
    }
    const int rc = sqlite3_step(statement.raw);
    if (rc != SQLITE_DONE) {
      const Error error = impl_->fromSqlite(rc, "representation insert failed");
      return Result<void>::failure(error.code, error.message);
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<Representation>::failure(wrote.error.code, wrote.error.message);
  }
  return Result<Representation>::success(representation);
}

Result<void> ProjectStore::updateRepresentationPath(const std::string& representationId,
                                                   const std::string& recordedPath) {
  if (!validToken(recordedPath, 4096)) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "representation path was rejected");
  }
  const std::string stamp = utcTimestamp();
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "UPDATE media_representations SET recorded_path = ?, modified_utc = ? "
                          "WHERE representation_id = ?;") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "representation path update could not be prepared");
    }
    if (!bindText(statement.raw, 1, recordedPath) || !bindText(statement.raw, 2, stamp) ||
        !bindText(statement.raw, 3, representationId) || sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "representation path update failed");
    }
    if (sqlite3_changes(impl_->db) != 1) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "representation was not found");
    }
    return Result<void>::success();
  });
}

Result<std::vector<Representation>> ProjectStore::listRepresentations(const std::string& mediaId) {
  if (!impl_->open) {
    return Result<std::vector<Representation>>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT representation_id, media_id, role, recorded_path, status, created_utc, "
                        "modified_utc FROM media_representations WHERE media_id = ? "
                        "ORDER BY created_utc, representation_id;") != SQLITE_OK) {
    return Result<std::vector<Representation>>::failure(ErrorCode::Database,
                                                       "representation list could not be prepared");
  }
  bindText(statement.raw, 1, mediaId);
  std::vector<Representation> rows;
  while (sqlite3_step(statement.raw) == SQLITE_ROW) {
    Representation row;
    row.representationId = columnText(statement.raw, 0);
    row.mediaId = columnText(statement.raw, 1);
    if (!representationRoleFromName(columnText(statement.raw, 2), row.role)) {
      return Result<std::vector<Representation>>::failure(ErrorCode::Database, "representation role is unknown");
    }
    row.recordedPath = columnText(statement.raw, 3);
    row.status = columnText(statement.raw, 4);
    row.createdUtc = columnText(statement.raw, 5);
    row.modifiedUtc = columnText(statement.raw, 6);
    rows.push_back(std::move(row));
  }
  return Result<std::vector<Representation>>::success(std::move(rows));
}

Result<MasterClip> ProjectStore::insertMasterClip(const std::string& name,
                                                 const std::string& mediaId,
                                                 const std::string& binId,
                                                 const std::string& metadata) {
  if (!validDisplayName(name) || !validMetadata(metadata)) {
    return Result<MasterClip>::failure(ErrorCode::InvalidArgument, "master clip name or metadata was rejected");
  }
  const auto minted = makeStableId();
  if (!minted) {
    return Result<MasterClip>::failure(ErrorCode::Io, "object id could not be minted");
  }
  MasterClip clip;
  clip.objectId = *minted;
  clip.projectId = impl_->info.projectId;
  clip.name = name;
  clip.mediaId = mediaId;
  clip.metadata = normaliseMetadata(metadata);
  clip.createdUtc = utcTimestamp();
  clip.modifiedUtc = clip.createdUtc;
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement objectInsert;
    Statement clipInsert;
    Statement memberInsert;
    if (objectInsert.prepare(impl_->db,
                             "INSERT INTO editorial_objects "
                             "(object_id, project_id, kind, name, metadata_json, created_utc, modified_utc) "
                             "VALUES (?, ?, 'master_clip', ?, ?, ?, ?);") != SQLITE_OK ||
        !bindText(objectInsert.raw, 1, clip.objectId) || !bindText(objectInsert.raw, 2, clip.projectId) ||
        !bindText(objectInsert.raw, 3, clip.name) || !bindText(objectInsert.raw, 4, clip.metadata) ||
        !bindText(objectInsert.raw, 5, clip.createdUtc) || !bindText(objectInsert.raw, 6, clip.modifiedUtc)) {
      return Result<void>::failure(ErrorCode::Database, "master clip object could not be prepared");
    }
    const int objectRc = sqlite3_step(objectInsert.raw);
    if (objectRc != SQLITE_DONE) {
      const Error error = impl_->fromSqlite(objectRc, "master clip object insert failed");
      return Result<void>::failure(error.code, error.message);
    }
    if (clipInsert.prepare(impl_->db, "INSERT INTO master_clips (object_id, media_id) VALUES (?, ?);") !=
            SQLITE_OK ||
        !bindText(clipInsert.raw, 1, clip.objectId) || !bindText(clipInsert.raw, 2, clip.mediaId)) {
      return Result<void>::failure(ErrorCode::Database, "master clip row could not be prepared");
    }
    const int clipRc = sqlite3_step(clipInsert.raw);
    if (clipRc != SQLITE_DONE) {
      const Error error = impl_->fromSqlite(clipRc, "master clip insert failed");
      return Result<void>::failure(error.code, error.message);
    }
    if (memberInsert.prepare(impl_->db,
                             "INSERT INTO bin_membership (bin_id, object_id, created_utc) VALUES (?, ?, ?);") !=
            SQLITE_OK ||
        !bindText(memberInsert.raw, 1, binId) || !bindText(memberInsert.raw, 2, clip.objectId) ||
        !bindText(memberInsert.raw, 3, clip.createdUtc)) {
      return Result<void>::failure(ErrorCode::Database, "master clip membership could not be prepared");
    }
    const int memberRc = sqlite3_step(memberInsert.raw);
    if (memberRc != SQLITE_DONE) {
      const Error error = impl_->fromSqlite(memberRc, "master clip membership insert failed");
      return Result<void>::failure(error.code, error.message);
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<MasterClip>::failure(wrote.error.code, wrote.error.message);
  }
  return Result<MasterClip>::success(clip);
}

Result<MasterClip> ProjectStore::readMasterClip(const std::string& objectId) {
  if (!impl_->open) {
    return Result<MasterClip>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT o.object_id, o.project_id, o.name, m.media_id, o.metadata_json, "
                        "o.created_utc, o.modified_utc "
                        "FROM editorial_objects o JOIN master_clips m ON m.object_id = o.object_id "
                        "WHERE o.object_id = ?;") != SQLITE_OK) {
    return Result<MasterClip>::failure(ErrorCode::Database, "master clip read could not be prepared");
  }
  bindText(statement.raw, 1, objectId);
  if (sqlite3_step(statement.raw) != SQLITE_ROW) {
    return Result<MasterClip>::failure(ErrorCode::InvalidArgument, "master clip was not found");
  }
  MasterClip clip;
  clip.objectId = columnText(statement.raw, 0);
  clip.projectId = columnText(statement.raw, 1);
  clip.name = columnText(statement.raw, 2);
  clip.mediaId = columnText(statement.raw, 3);
  clip.metadata = columnText(statement.raw, 4);
  clip.createdUtc = columnText(statement.raw, 5);
  clip.modifiedUtc = columnText(statement.raw, 6);
  return Result<MasterClip>::success(clip);
}

Result<std::vector<MasterClip>> ProjectStore::listMasterClips(const std::string& binId) {
  if (!impl_->open) {
    return Result<std::vector<MasterClip>>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT o.object_id, o.project_id, o.name, m.media_id, o.metadata_json, "
                        "o.created_utc, o.modified_utc "
                        "FROM bin_membership bm "
                        "JOIN editorial_objects o ON o.object_id = bm.object_id "
                        "JOIN master_clips m ON m.object_id = o.object_id "
                        "WHERE bm.bin_id = ? ORDER BY bm.created_utc, o.object_id;") != SQLITE_OK) {
    return Result<std::vector<MasterClip>>::failure(ErrorCode::Database, "bin object list could not be prepared");
  }
  bindText(statement.raw, 1, binId);
  std::vector<MasterClip> clips;
  while (sqlite3_step(statement.raw) == SQLITE_ROW) {
    MasterClip clip;
    clip.objectId = columnText(statement.raw, 0);
    clip.projectId = columnText(statement.raw, 1);
    clip.name = columnText(statement.raw, 2);
    clip.mediaId = columnText(statement.raw, 3);
    clip.metadata = columnText(statement.raw, 4);
    clip.createdUtc = columnText(statement.raw, 5);
    clip.modifiedUtc = columnText(statement.raw, 6);
    clips.push_back(std::move(clip));
  }
  return Result<std::vector<MasterClip>>::success(std::move(clips));
}

Result<void> ProjectStore::addMembership(const std::string& binId, const std::string& objectId) {
  const std::string stamp = utcTimestamp();
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "INSERT INTO bin_membership (bin_id, object_id, created_utc) VALUES (?, ?, ?);") !=
        SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "membership insert could not be prepared");
    }
    if (!bindText(statement.raw, 1, binId) || !bindText(statement.raw, 2, objectId) ||
        !bindText(statement.raw, 3, stamp)) {
      return Result<void>::failure(ErrorCode::Database, "membership insert could not be bound");
    }
    const int rc = sqlite3_step(statement.raw);
    if (rc != SQLITE_DONE) {
      const Error error = impl_->fromSqlite(rc, "membership insert failed");
      return Result<void>::failure(error.code, error.message);
    }
    return Result<void>::success();
  });
}

Result<void> ProjectStore::removeMembership(const std::string& binId, const std::string& objectId) {
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db, "DELETE FROM bin_membership WHERE bin_id = ? AND object_id = ?;") !=
        SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "membership delete could not be prepared");
    }
    if (!bindText(statement.raw, 1, binId) || !bindText(statement.raw, 2, objectId) ||
        sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "membership delete failed");
    }
    if (sqlite3_changes(impl_->db) != 1) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "membership was not found");
    }
    return Result<void>::success();
  });
}

Result<void> ProjectStore::moveMembership(const std::string& objectId,
                                         const std::string& fromBinId,
                                         const std::string& toBinId) {
  if (fromBinId == toBinId) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "move source and destination are the same bin");
  }
  const std::string stamp = utcTimestamp();
  return impl_->mutate([&]() -> Result<void> {
    Statement remove;
    if (remove.prepare(impl_->db, "DELETE FROM bin_membership WHERE bin_id = ? AND object_id = ?;") != SQLITE_OK ||
        !bindText(remove.raw, 1, fromBinId) || !bindText(remove.raw, 2, objectId) ||
        sqlite3_step(remove.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "move could not leave the source bin");
    }
    if (sqlite3_changes(impl_->db) != 1) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "source membership was not found");
    }
    Statement add;
    if (add.prepare(impl_->db, "INSERT INTO bin_membership (bin_id, object_id, created_utc) VALUES (?, ?, ?);") !=
            SQLITE_OK ||
        !bindText(add.raw, 1, toBinId) || !bindText(add.raw, 2, objectId) || !bindText(add.raw, 3, stamp)) {
      return Result<void>::failure(ErrorCode::Database, "move could not be prepared");
    }
    const int rc = sqlite3_step(add.raw);
    if (rc != SQLITE_DONE) {
      const Error error = impl_->fromSqlite(rc, "move into the destination bin failed");
      return Result<void>::failure(error.code, error.message);
    }
    return Result<void>::success();
  });
}

Result<MasterClip> ProjectStore::duplicateMasterClip(const std::string& objectId, const std::string& binId) {
  auto existing = readMasterClip(objectId);
  if (!existing.ok()) {
    return existing;
  }
  return insertMasterClip(existing.value.name, existing.value.mediaId, binId, existing.value.metadata);
}

Result<void> ProjectStore::setObjectMetadata(const std::string& objectId,
                                            const std::string& name,
                                            const std::string& metadata) {
  if (!validDisplayName(name) || !validMetadata(metadata)) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "object name or metadata was rejected");
  }
  const std::string stored = normaliseMetadata(metadata);
  const std::string stamp = utcTimestamp();
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "UPDATE editorial_objects SET name = ?, metadata_json = ?, modified_utc = ? "
                          "WHERE object_id = ?;") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "metadata update could not be prepared");
    }
    if (!bindText(statement.raw, 1, name) || !bindText(statement.raw, 2, stored) ||
        !bindText(statement.raw, 3, stamp) || !bindText(statement.raw, 4, objectId) ||
        sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "metadata update failed");
    }
    if (sqlite3_changes(impl_->db) != 1) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "editorial object was not found");
    }
    return Result<void>::success();
  });
}

Result<void> ProjectStore::autosave() {
  if (!impl_->open || impl_->db == nullptr) {
    return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
  }
  const std::filesystem::path dbPath = std::filesystem::path(impl_->info.directory) / kDatabaseFilename;
  const int fd = ::open(dbPath.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    impl_->logger.error("autosave", "committed database could not be flushed");
    return Result<void>::failure(ErrorCode::Io, "committed database could not be flushed");
  }
  const int flushed = ::fsync(fd);
  ::close(fd);
  if (flushed != 0) {
    impl_->logger.error("autosave", "committed database could not be flushed");
    return Result<void>::failure(ErrorCode::Io, "committed database could not be flushed");
  }
  auto manifest = impl_->rewriteManifest();
  if (!manifest.ok()) {
    impl_->logger.error("autosave", manifest.error.message);
    return manifest;
  }
  impl_->logger.info("autosave", "flushed committed project " + impl_->info.name);
  return Result<void>::success();
}

Result<std::vector<std::string>> ProjectStore::listUserTables() {
  if (!impl_->open) {
    return Result<std::vector<std::string>>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT name FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%' "
                        "ORDER BY name;") != SQLITE_OK) {
    return Result<std::vector<std::string>>::failure(ErrorCode::Database, "table list could not be prepared");
  }
  std::vector<std::string> names;
  while (sqlite3_step(statement.raw) == SQLITE_ROW) {
    names.push_back(columnText(statement.raw, 0));
  }
  return Result<std::vector<std::string>>::success(std::move(names));
}

namespace {

bool bindInt64(sqlite3_stmt* statement, int index, std::int64_t value) {
  return sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}

TimelineInstance instanceFromRow(sqlite3_stmt* statement) {
  TimelineInstance instance;
  instance.instanceId = columnText(statement, 0);
  instance.sequenceId = columnText(statement, 1);
  instance.trackId = columnText(statement, 2);
  instance.masterObjectId = columnText(statement, 3);
  instance.mediaId = columnText(statement, 4);
  instance.sourceRate.numerator = sqlite3_column_int64(statement, 5);
  instance.sourceRate.denominator = sqlite3_column_int64(statement, 6);
  instance.sourceStart = sqlite3_column_int64(statement, 7);
  instance.sourceEnd = sqlite3_column_int64(statement, 8);
  instance.recordStart = sqlite3_column_int64(statement, 9);
  instance.recordEnd = sqlite3_column_int64(statement, 10);
  instance.createdUtc = columnText(statement, 11);
  instance.modifiedUtc = columnText(statement, 12);
  return instance;
}

const char* kInstanceSelect =
    "SELECT instance_id, sequence_id, track_id, master_object_id, media_id, "
    "source_rate_numerator, source_rate_denominator, source_start, source_end, "
    "record_start, record_end, created_utc, modified_utc FROM timeline_instances ";

}  // namespace

Result<SequenceRecord> ProjectStore::insertSequence(const std::string& name, FrameRate rate) {
  if (!validDisplayName(name)) {
    return Result<SequenceRecord>::failure(ErrorCode::InvalidArgument, "sequence name is empty or rejected");
  }
  if (!isAcceptedFrameRate(rate)) {
    return Result<SequenceRecord>::failure(ErrorCode::InvalidArgument, "sequence rate is not an accepted reduced rate");
  }
  const auto minted = makeStableId();
  if (!minted) {
    return Result<SequenceRecord>::failure(ErrorCode::Io, "sequence id could not be minted");
  }
  SequenceRecord sequence;
  sequence.sequenceId = *minted;
  sequence.projectId = impl_->info.projectId;
  sequence.name = name;
  sequence.rate = rate;
  sequence.createdUtc = utcTimestamp();
  sequence.modifiedUtc = sequence.createdUtc;
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "INSERT INTO sequences (sequence_id, project_id, name, rate_numerator, "
                          "rate_denominator, created_utc, modified_utc) VALUES (?, ?, ?, ?, ?, ?, ?);") !=
        SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "sequence insert could not be prepared");
    }
    if (!bindText(statement.raw, 1, sequence.sequenceId) || !bindText(statement.raw, 2, sequence.projectId) ||
        !bindText(statement.raw, 3, sequence.name) || !bindInt64(statement.raw, 4, rate.numerator) ||
        !bindInt64(statement.raw, 5, rate.denominator) || !bindText(statement.raw, 6, sequence.createdUtc) ||
        !bindText(statement.raw, 7, sequence.modifiedUtc) || sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "sequence insert failed");
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<SequenceRecord>::failure(wrote.error.code, wrote.error.message);
  }
  return Result<SequenceRecord>::success(sequence);
}

Result<SequenceRecord> ProjectStore::reinsertSequence(const SequenceRecord& sequence) {
  if (!validDisplayName(sequence.name) || !isAcceptedFrameRate(sequence.rate) || sequence.sequenceId.empty()) {
    return Result<SequenceRecord>::failure(ErrorCode::InvalidArgument, "sequence restore is invalid");
  }
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "INSERT INTO sequences (sequence_id, project_id, name, rate_numerator, "
                          "rate_denominator, created_utc, modified_utc) VALUES (?, ?, ?, ?, ?, ?, ?);") !=
        SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "sequence restore could not be prepared");
    }
    if (!bindText(statement.raw, 1, sequence.sequenceId) || !bindText(statement.raw, 2, impl_->info.projectId) ||
        !bindText(statement.raw, 3, sequence.name) || !bindInt64(statement.raw, 4, sequence.rate.numerator) ||
        !bindInt64(statement.raw, 5, sequence.rate.denominator) || !bindText(statement.raw, 6, sequence.createdUtc) ||
        !bindText(statement.raw, 7, utcTimestamp()) || sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "sequence restore failed");
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<SequenceRecord>::failure(wrote.error.code, wrote.error.message);
  }
  return readSequence(sequence.sequenceId);
}

Result<SequenceRecord> ProjectStore::readSequence(const std::string& sequenceId) {
  if (!impl_->open) {
    return Result<SequenceRecord>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT sequence_id, project_id, name, rate_numerator, rate_denominator, "
                        "created_utc, modified_utc FROM sequences WHERE sequence_id = ?;") != SQLITE_OK ||
      !bindText(statement.raw, 1, sequenceId)) {
    return Result<SequenceRecord>::failure(ErrorCode::Database, "sequence could not be read");
  }
  if (sqlite3_step(statement.raw) != SQLITE_ROW) {
    return Result<SequenceRecord>::failure(ErrorCode::InvalidArgument, "sequence was not found");
  }
  SequenceRecord sequence;
  sequence.sequenceId = columnText(statement.raw, 0);
  sequence.projectId = columnText(statement.raw, 1);
  sequence.name = columnText(statement.raw, 2);
  sequence.rate.numerator = sqlite3_column_int64(statement.raw, 3);
  sequence.rate.denominator = sqlite3_column_int64(statement.raw, 4);
  sequence.createdUtc = columnText(statement.raw, 5);
  sequence.modifiedUtc = columnText(statement.raw, 6);
  return Result<SequenceRecord>::success(sequence);
}

Result<std::vector<SequenceRecord>> ProjectStore::listSequences() {
  if (!impl_->open) {
    return Result<std::vector<SequenceRecord>>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT sequence_id, project_id, name, rate_numerator, rate_denominator, "
                        "created_utc, modified_utc FROM sequences ORDER BY created_utc, sequence_id;") !=
      SQLITE_OK) {
    return Result<std::vector<SequenceRecord>>::failure(ErrorCode::Database, "sequence list could not be prepared");
  }
  std::vector<SequenceRecord> sequences;
  while (sqlite3_step(statement.raw) == SQLITE_ROW) {
    SequenceRecord sequence;
    sequence.sequenceId = columnText(statement.raw, 0);
    sequence.projectId = columnText(statement.raw, 1);
    sequence.name = columnText(statement.raw, 2);
    sequence.rate.numerator = sqlite3_column_int64(statement.raw, 3);
    sequence.rate.denominator = sqlite3_column_int64(statement.raw, 4);
    sequence.createdUtc = columnText(statement.raw, 5);
    sequence.modifiedUtc = columnText(statement.raw, 6);
    sequences.push_back(std::move(sequence));
  }
  return Result<std::vector<SequenceRecord>>::success(std::move(sequences));
}

Result<void> ProjectStore::renameSequence(const std::string& sequenceId, const std::string& name) {
  if (!validDisplayName(name)) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "sequence name is empty or rejected");
  }
  auto existing = readSequence(sequenceId);
  if (!existing.ok()) {
    return Result<void>::failure(existing.error.code, existing.error.message);
  }
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "UPDATE sequences SET name = ?, modified_utc = ? WHERE sequence_id = ?;") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "sequence rename could not be prepared");
    }
    const std::string stamp = utcTimestamp();
    if (!bindText(statement.raw, 1, name) || !bindText(statement.raw, 2, stamp) ||
        !bindText(statement.raw, 3, sequenceId) || sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "sequence rename failed");
    }
    return Result<void>::success();
  });
}

Result<void> ProjectStore::deleteSequenceIfEmpty(const std::string& sequenceId) {
  auto tracks = listTracks(sequenceId);
  if (!tracks.ok()) {
    return Result<void>::failure(tracks.error.code, tracks.error.message);
  }
  if (!tracks.value.empty()) {
    return Result<void>::failure(ErrorCode::Constraint, "sequence still has tracks");
  }
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db, "DELETE FROM sequences WHERE sequence_id = ?;") != SQLITE_OK ||
        !bindText(statement.raw, 1, sequenceId) || sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "sequence delete failed");
    }
    if (sqlite3_changes(impl_->db) != 1) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "sequence was not found");
    }
    return Result<void>::success();
  });
}

Result<TrackRecord> ProjectStore::insertTrack(const std::string& sequenceId) {
  auto sequence = readSequence(sequenceId);
  if (!sequence.ok()) {
    return Result<TrackRecord>::failure(sequence.error.code, sequence.error.message);
  }
  auto existing = listTracks(sequenceId);
  if (!existing.ok()) {
    return Result<TrackRecord>::failure(existing.error.code, existing.error.message);
  }
  const auto minted = makeStableId();
  if (!minted) {
    return Result<TrackRecord>::failure(ErrorCode::Io, "track id could not be minted");
  }
  TrackRecord track;
  track.trackId = *minted;
  track.sequenceId = sequenceId;
  track.ordinal = static_cast<int>(existing.value.size());
  track.createdUtc = utcTimestamp();
  track.modifiedUtc = track.createdUtc;
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "INSERT INTO tracks (track_id, sequence_id, ordinal, created_utc, modified_utc) "
                          "VALUES (?, ?, ?, ?, ?);") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "track insert could not be prepared");
    }
    if (!bindText(statement.raw, 1, track.trackId) || !bindText(statement.raw, 2, track.sequenceId) ||
        sqlite3_bind_int(statement.raw, 3, track.ordinal) != SQLITE_OK ||
        !bindText(statement.raw, 4, track.createdUtc) || !bindText(statement.raw, 5, track.modifiedUtc) ||
        sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "track insert failed");
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<TrackRecord>::failure(wrote.error.code, wrote.error.message);
  }
  return Result<TrackRecord>::success(track);
}

Result<TrackRecord> ProjectStore::reinsertTrack(const TrackRecord& track) {
  if (track.trackId.empty() || track.ordinal < 0) {
    return Result<TrackRecord>::failure(ErrorCode::InvalidArgument, "track restore is invalid");
  }
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "INSERT INTO tracks (track_id, sequence_id, ordinal, created_utc, modified_utc) "
                          "VALUES (?, ?, ?, ?, ?);") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "track restore could not be prepared");
    }
    if (!bindText(statement.raw, 1, track.trackId) || !bindText(statement.raw, 2, track.sequenceId) ||
        sqlite3_bind_int(statement.raw, 3, track.ordinal) != SQLITE_OK ||
        !bindText(statement.raw, 4, track.createdUtc) || !bindText(statement.raw, 5, utcTimestamp()) ||
        sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "track restore failed");
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<TrackRecord>::failure(wrote.error.code, wrote.error.message);
  }
  return Result<TrackRecord>::success(track);
}

Result<std::vector<TrackRecord>> ProjectStore::listTracks(const std::string& sequenceId) {
  if (!impl_->open) {
    return Result<std::vector<TrackRecord>>::failure(ErrorCode::NotOpen, "project is not open");
  }
  Statement statement;
  if (statement.prepare(impl_->db,
                        "SELECT track_id, sequence_id, ordinal, created_utc, modified_utc FROM tracks "
                        "WHERE sequence_id = ? ORDER BY ordinal, track_id;") != SQLITE_OK ||
      !bindText(statement.raw, 1, sequenceId)) {
    return Result<std::vector<TrackRecord>>::failure(ErrorCode::Database, "track list could not be prepared");
  }
  std::vector<TrackRecord> tracks;
  while (sqlite3_step(statement.raw) == SQLITE_ROW) {
    TrackRecord track;
    track.trackId = columnText(statement.raw, 0);
    track.sequenceId = columnText(statement.raw, 1);
    track.ordinal = sqlite3_column_int(statement.raw, 2);
    track.createdUtc = columnText(statement.raw, 3);
    track.modifiedUtc = columnText(statement.raw, 4);
    tracks.push_back(std::move(track));
  }
  return Result<std::vector<TrackRecord>>::success(std::move(tracks));
}

Result<void> ProjectStore::moveTrack(const std::string& trackId, int ordinal) {
  if (ordinal < 0) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "track ordinal is negative");
  }
  std::string sequenceId;
  {
    Statement statement;
    if (!impl_->open ||
        statement.prepare(impl_->db, "SELECT sequence_id FROM tracks WHERE track_id = ?;") != SQLITE_OK ||
        !bindText(statement.raw, 1, trackId) || sqlite3_step(statement.raw) != SQLITE_ROW) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "track was not found");
    }
    sequenceId = columnText(statement.raw, 0);
  }
  auto tracks = listTracks(sequenceId);
  if (!tracks.ok()) {
    return Result<void>::failure(tracks.error.code, tracks.error.message);
  }
  std::vector<TrackRecord> order = tracks.value;
  std::size_t from = order.size();
  for (std::size_t i = 0; i < order.size(); ++i) {
    if (order[i].trackId == trackId) {
      from = i;
    }
  }
  if (from == order.size() || ordinal >= static_cast<int>(order.size())) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "track ordinal is outside the sequence");
  }
  TrackRecord moved = order[from];
  order.erase(order.begin() + static_cast<std::vector<TrackRecord>::difference_type>(from));
  order.insert(order.begin() + ordinal, moved);
  return impl_->mutate([&]() -> Result<void> {
    Statement park;
    if (park.prepare(impl_->db, "UPDATE tracks SET ordinal = ordinal + 1000000 WHERE sequence_id = ?;") !=
            SQLITE_OK ||
        !bindText(park.raw, 1, sequenceId) || sqlite3_step(park.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "track order could not be parked");
    }
    for (int index = 0; index < static_cast<int>(order.size()); ++index) {
      Statement statement;
      if (statement.prepare(impl_->db, "UPDATE tracks SET ordinal = ?, modified_utc = ? WHERE track_id = ?;") !=
              SQLITE_OK ||
          sqlite3_bind_int(statement.raw, 1, index) != SQLITE_OK || !bindText(statement.raw, 2, utcTimestamp()) ||
          !bindText(statement.raw, 3, order[static_cast<std::size_t>(index)].trackId) ||
          sqlite3_step(statement.raw) != SQLITE_DONE) {
        return Result<void>::failure(ErrorCode::Database, "track order could not be written");
      }
    }
    return Result<void>::success();
  });
}

Result<void> ProjectStore::deleteTrackIfEmpty(const std::string& trackId) {
  Statement find;
  if (!impl_->open || find.prepare(impl_->db, "SELECT sequence_id FROM tracks WHERE track_id = ?;") != SQLITE_OK ||
      !bindText(find.raw, 1, trackId) || sqlite3_step(find.raw) != SQLITE_ROW) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "track was not found");
  }
  const std::string sequenceId = columnText(find.raw, 0);
  Statement count;
  if (count.prepare(impl_->db, "SELECT COUNT(*) FROM timeline_instances WHERE track_id = ?;") != SQLITE_OK ||
      !bindText(count.raw, 1, trackId) || sqlite3_step(count.raw) != SQLITE_ROW) {
    return Result<void>::failure(ErrorCode::Database, "track occupancy could not be read");
  }
  if (sqlite3_column_int(count.raw, 0) != 0) {
    return Result<void>::failure(ErrorCode::Constraint, "track still has instances");
  }
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db, "DELETE FROM tracks WHERE track_id = ?;") != SQLITE_OK ||
        !bindText(statement.raw, 1, trackId) || sqlite3_step(statement.raw) != SQLITE_DONE ||
        sqlite3_changes(impl_->db) != 1) {
      return Result<void>::failure(ErrorCode::Database, "track delete failed");
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return wrote;
  }
  auto remaining = listTracks(sequenceId);
  if (!remaining.ok()) {
    return Result<void>::failure(remaining.error.code, remaining.error.message);
  }
  for (int index = 0; index < static_cast<int>(remaining.value.size()); ++index) {
    if (remaining.value[static_cast<std::size_t>(index)].ordinal == index) {
      continue;
    }
    auto moved = moveTrack(remaining.value[static_cast<std::size_t>(index)].trackId, index);
    if (!moved.ok()) {
      return moved;
    }
  }
  return Result<void>::success();
}

Result<std::vector<TimelineInstance>> ProjectStore::listInstances(const std::string& trackId) {
  if (!impl_->open) {
    return Result<std::vector<TimelineInstance>>::failure(ErrorCode::NotOpen, "project is not open");
  }
  const std::string sql = std::string(kInstanceSelect) + "WHERE track_id = ? ORDER BY record_start, instance_id;";
  Statement statement;
  if (statement.prepare(impl_->db, sql.c_str()) != SQLITE_OK || !bindText(statement.raw, 1, trackId)) {
    return Result<std::vector<TimelineInstance>>::failure(ErrorCode::Database, "instance list could not be prepared");
  }
  std::vector<TimelineInstance> instances;
  while (sqlite3_step(statement.raw) == SQLITE_ROW) {
    instances.push_back(instanceFromRow(statement.raw));
  }
  return Result<std::vector<TimelineInstance>>::success(std::move(instances));
}

Result<TimelineInstance> ProjectStore::readInstance(const std::string& instanceId) {
  if (!impl_->open) {
    return Result<TimelineInstance>::failure(ErrorCode::NotOpen, "project is not open");
  }
  const std::string sql = std::string(kInstanceSelect) + "WHERE instance_id = ?;";
  Statement statement;
  if (statement.prepare(impl_->db, sql.c_str()) != SQLITE_OK || !bindText(statement.raw, 1, instanceId)) {
    return Result<TimelineInstance>::failure(ErrorCode::Database, "instance could not be read");
  }
  if (sqlite3_step(statement.raw) != SQLITE_ROW) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "instance was not found");
  }
  return Result<TimelineInstance>::success(instanceFromRow(statement.raw));
}

Result<TimelineInstance> ProjectStore::insertInstance(const TimelineInstance& requested) {
  TimelineInstance instance = requested;
  if (instance.instanceId.empty()) {
    const auto minted = makeStableId();
    if (!minted) {
      return Result<TimelineInstance>::failure(ErrorCode::Io, "instance id could not be minted");
    }
    instance.instanceId = *minted;
  }
  auto sequence = readSequence(instance.sequenceId);
  if (!sequence.ok()) {
    return Result<TimelineInstance>::failure(sequence.error.code, sequence.error.message);
  }
  auto master = readMasterClip(instance.masterObjectId);
  if (!master.ok()) {
    return Result<TimelineInstance>::failure(master.error.code, master.error.message);
  }
  if (!instance.mediaId.empty() && instance.mediaId != master.value.mediaId) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "instance media id does not match the master clip");
  }
  instance.mediaId = master.value.mediaId;
  Statement track;
  if (track.prepare(impl_->db, "SELECT sequence_id FROM tracks WHERE track_id = ?;") != SQLITE_OK ||
      !bindText(track.raw, 1, instance.trackId) || sqlite3_step(track.raw) != SQLITE_ROW ||
      columnText(track.raw, 0) != instance.sequenceId) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "track is not in the sequence");
  }
  auto siblings = listInstances(instance.trackId);
  if (!siblings.ok()) {
    return Result<TimelineInstance>::failure(siblings.error.code, siblings.error.message);
  }
  auto valid = validateInstancePlacement(instance, sequence.value.rate, siblings.value, "");
  if (!valid.ok()) {
    return Result<TimelineInstance>::failure(valid.error.code, valid.error.message);
  }
  instance.createdUtc = utcTimestamp();
  instance.modifiedUtc = instance.createdUtc;
  auto wrote = impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "INSERT INTO timeline_instances (instance_id, sequence_id, track_id, master_object_id, "
                          "media_id, source_rate_numerator, source_rate_denominator, source_start, source_end, "
                          "record_start, record_end, created_utc, modified_utc) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "instance insert could not be prepared");
    }
    if (!bindText(statement.raw, 1, instance.instanceId) || !bindText(statement.raw, 2, instance.sequenceId) ||
        !bindText(statement.raw, 3, instance.trackId) || !bindText(statement.raw, 4, instance.masterObjectId) ||
        !bindText(statement.raw, 5, instance.mediaId) ||
        !bindInt64(statement.raw, 6, instance.sourceRate.numerator) ||
        !bindInt64(statement.raw, 7, instance.sourceRate.denominator) ||
        !bindInt64(statement.raw, 8, instance.sourceStart) || !bindInt64(statement.raw, 9, instance.sourceEnd) ||
        !bindInt64(statement.raw, 10, instance.recordStart) || !bindInt64(statement.raw, 11, instance.recordEnd) ||
        !bindText(statement.raw, 12, instance.createdUtc) || !bindText(statement.raw, 13, instance.modifiedUtc) ||
        sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "instance insert failed");
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<TimelineInstance>::failure(wrote.error.code, wrote.error.message);
  }
  return Result<TimelineInstance>::success(instance);
}

Result<TimelineInstance> ProjectStore::reinsertInstance(const TimelineInstance& instance) {
  return insertInstance(instance);
}

Result<void> ProjectStore::moveInstance(const std::string& instanceId, std::int64_t recordStart) {
  auto current = readInstance(instanceId);
  if (!current.ok()) {
    return Result<void>::failure(current.error.code, current.error.message);
  }
  if (recordStart < 0) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "record start is negative");
  }
  const std::int64_t duration = current.value.recordEnd - current.value.recordStart;
  if (duration <= 0 || recordStart > INT64_MAX - duration) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "moved record range does not fit");
  }
  TimelineInstance next = current.value;
  next.recordStart = recordStart;
  next.recordEnd = recordStart + duration;
  auto sequence = readSequence(next.sequenceId);
  if (!sequence.ok()) {
    return Result<void>::failure(sequence.error.code, sequence.error.message);
  }
  auto siblings = listInstances(next.trackId);
  if (!siblings.ok()) {
    return Result<void>::failure(siblings.error.code, siblings.error.message);
  }
  auto valid = validateInstancePlacement(next, sequence.value.rate, siblings.value, instanceId);
  if (!valid.ok()) {
    return valid;
  }
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "UPDATE timeline_instances SET record_start = ?, record_end = ?, modified_utc = ? "
                          "WHERE instance_id = ?;") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "instance move could not be prepared");
    }
    if (!bindInt64(statement.raw, 1, next.recordStart) || !bindInt64(statement.raw, 2, next.recordEnd) ||
        !bindText(statement.raw, 3, utcTimestamp()) || !bindText(statement.raw, 4, instanceId) ||
        sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "instance move failed");
    }
    return Result<void>::success();
  });
}

Result<void> ProjectStore::trimInstance(const std::string& instanceId,
                                       std::int64_t sourceStart,
                                       std::int64_t sourceEnd,
                                       std::int64_t recordStart,
                                       std::int64_t recordEnd) {
  auto current = readInstance(instanceId);
  if (!current.ok()) {
    return Result<void>::failure(current.error.code, current.error.message);
  }
  TimelineInstance next = current.value;
  next.sourceStart = sourceStart;
  next.sourceEnd = sourceEnd;
  next.recordStart = recordStart;
  next.recordEnd = recordEnd;
  auto sequence = readSequence(next.sequenceId);
  if (!sequence.ok()) {
    return Result<void>::failure(sequence.error.code, sequence.error.message);
  }
  auto siblings = listInstances(next.trackId);
  if (!siblings.ok()) {
    return Result<void>::failure(siblings.error.code, siblings.error.message);
  }
  auto valid = validateInstancePlacement(next, sequence.value.rate, siblings.value, instanceId);
  if (!valid.ok()) {
    return valid;
  }
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db,
                          "UPDATE timeline_instances SET source_start = ?, source_end = ?, record_start = ?, "
                          "record_end = ?, modified_utc = ? WHERE instance_id = ?;") != SQLITE_OK) {
      return Result<void>::failure(ErrorCode::Database, "instance trim could not be prepared");
    }
    if (!bindInt64(statement.raw, 1, sourceStart) || !bindInt64(statement.raw, 2, sourceEnd) ||
        !bindInt64(statement.raw, 3, recordStart) || !bindInt64(statement.raw, 4, recordEnd) ||
        !bindText(statement.raw, 5, utcTimestamp()) || !bindText(statement.raw, 6, instanceId) ||
        sqlite3_step(statement.raw) != SQLITE_DONE) {
      return Result<void>::failure(ErrorCode::Database, "instance trim failed");
    }
    return Result<void>::success();
  });
}

Result<void> ProjectStore::deleteInstance(const std::string& instanceId) {
  auto current = readInstance(instanceId);
  if (!current.ok()) {
    return Result<void>::failure(current.error.code, current.error.message);
  }
  return impl_->mutate([&]() -> Result<void> {
    Statement statement;
    if (statement.prepare(impl_->db, "DELETE FROM timeline_instances WHERE instance_id = ?;") != SQLITE_OK ||
        !bindText(statement.raw, 1, instanceId) || sqlite3_step(statement.raw) != SQLITE_DONE ||
        sqlite3_changes(impl_->db) != 1) {
      return Result<void>::failure(ErrorCode::Database, "instance delete failed");
    }
    return Result<void>::success();
  });
}

Result<void> ProjectStore::debugExec(const std::string& sql) {
  if (!impl_->open) {
    return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
  }
  char* message = nullptr;
  const int rc = sqlite3_exec(impl_->db, sql.c_str(), nullptr, nullptr, &message);
  if (rc != SQLITE_OK) {
    Error error = impl_->fromSqlite(rc, "debug statement failed");
    sqlite3_free(message);
    impl_->logger.error("database", "debug statement failed");
    return Result<void>::failure(error.code, error.message);
  }
  return Result<void>::success();
}

Result<int> ProjectStore::debugReadSchemaVersion(const std::filesystem::path& directory) {
  if (impl_->open) {
    return Result<int>::failure(ErrorCode::InvalidArgument, "close the project before a raw schema read");
  }
  const std::filesystem::path dbPath = directory / kDatabaseFilename;
  sqlite3* db = nullptr;
  if (sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
    if (db != nullptr) {
      sqlite3_close(db);
    }
    return Result<int>::failure(ErrorCode::Database, "schema version could not be inspected");
  }
  sqlite3_stmt* statement = nullptr;
  int version = -1;
  const int prepared =
      sqlite3_prepare_v2(db, "SELECT schema_version FROM schema_info WHERE singleton = 1;", -1, &statement, nullptr);
  if (prepared == SQLITE_OK && sqlite3_step(statement) == SQLITE_ROW) {
    version = sqlite3_column_int(statement, 0);
  }
  sqlite3_finalize(statement);
  sqlite3_close(db);
  if (version < 0) {
    return Result<int>::failure(ErrorCode::Schema, "schema version could not be inspected");
  }
  return Result<int>::success(version);
}

Result<void> ProjectStore::debugProveRollback(const std::string& binId) {
  if (!impl_->open) {
    return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
  }
  auto original = readBin(binId);
  if (!original.ok()) {
    return Result<void>::failure(original.error.code, original.error.message);
  }
  auto started = impl_->begin();
  if (!started.ok()) {
    return started;
  }
  {
    Statement rename;
    if (rename.prepare(impl_->db, "UPDATE bins SET name = ? WHERE bin_id = ?;") != SQLITE_OK ||
        !bindText(rename.raw, 1, "rollback-probe") || !bindText(rename.raw, 2, binId) ||
        sqlite3_step(rename.raw) != SQLITE_DONE) {
      impl_->rollback();
      return Result<void>::failure(ErrorCode::Database, "rollback probe could not rename");
    }
  }
  int rc = SQLITE_OK;
  Error error;
  {
    Statement bad;
    const char* sql =
        "INSERT INTO media_representations "
        "(representation_id, media_id, role, recorded_path, status, created_utc, modified_utc) "
        "VALUES ('rollback-rep', 'missing-media', 'original', 'missing', 'recorded', 't', 't');";
    if (bad.prepare(impl_->db, sql) != SQLITE_OK) {
      impl_->rollback();
      return Result<void>::failure(ErrorCode::Database, "rollback probe could not be prepared");
    }
    rc = sqlite3_step(bad.raw);
    if (rc != SQLITE_DONE) {
      error = impl_->fromSqlite(rc, "rollback probe");
    }
  }
  if (rc == SQLITE_DONE) {
    impl_->rollback();
    return Result<void>::failure(ErrorCode::Database, "rollback probe insert was accepted");
  }
  impl_->rollback();
  if (error.code != ErrorCode::ForeignKey) {
    return Result<void>::failure(error.code, error.message);
  }
  auto after = readBin(binId);
  if (!after.ok()) {
    return Result<void>::failure(after.error.code, after.error.message);
  }
  if (after.value.name != original.value.name) {
    impl_->logger.error("transaction", "rollback did not restore the bin name");
    return Result<void>::failure(ErrorCode::Database, "rollback did not restore the bin name");
  }
  return Result<void>::success();
}

Result<void> ProjectStore::abandonUncommittedInsert() {
  if (!impl_->open || impl_->db == nullptr) {
    return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
  }
  auto started = impl_->exec("BEGIN IMMEDIATE;");
  if (!started.ok()) {
    return started;
  }
  bool inserted = false;
  {
    Statement statement;
    inserted = statement.prepare(impl_->db,
                                 "INSERT INTO bins (bin_id, project_id, name, created_utc, modified_utc) "
                                 "VALUES ('interrupted-uncommitted', ?, 'nope', 't', 't');") == SQLITE_OK &&
               bindText(statement.raw, 1, impl_->info.projectId) && sqlite3_step(statement.raw) == SQLITE_DONE;
  }
  if (!inserted) {
    impl_->rollback();
    impl_->logger.error("transaction", "interrupted insert could not be started");
    return Result<void>::failure(ErrorCode::Database, "interrupted insert could not be started");
  }
  // The statement is finalized and the transaction is still open.
  // Closing the connection rolls the insert back. The file is not deleted.
  impl_->logger.error("transaction", "uncommitted insert was abandoned without commit");
  impl_->logger.setLogDirectory("");
  impl_->closeDb();
  impl_->unlock();
  impl_->open = false;
  impl_->info = ProjectInfo{};
  return Result<void>::success();
}

Result<void> ProjectStore::debugDowngradeEmptySequenceSchema() {
  if (!impl_->open) {
    return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
  }
  {
    Statement count;
    if (count.prepare(impl_->db,
                      "SELECT (SELECT COUNT(*) FROM sequences) + (SELECT COUNT(*) FROM tracks) + "
                      "(SELECT COUNT(*) FROM timeline_instances);") != SQLITE_OK ||
        sqlite3_step(count.raw) != SQLITE_ROW) {
      return Result<void>::failure(ErrorCode::Database, "sequence schema could not be counted");
    }
    if (sqlite3_column_int(count.raw, 0) != 0) {
      return Result<void>::failure(ErrorCode::Constraint, "sequence schema is not empty");
    }
  }
  auto wrote = impl_->mutate([&]() -> Result<void> {
    if (impl_->exec("DROP TABLE timeline_instances;").ok() && impl_->exec("DROP TABLE tracks;").ok() &&
        impl_->exec("DROP TABLE sequences;").ok()) {
      Statement statement;
      if (statement.prepare(impl_->db, "UPDATE schema_info SET schema_version = 1 WHERE singleton = 1;") ==
              SQLITE_OK &&
          sqlite3_step(statement.raw) == SQLITE_DONE) {
        impl_->info.schemaVersion = kSchemaVersionSlice0A;
        return Result<void>::success();
      }
    }
    return Result<void>::failure(ErrorCode::Database,
                                 std::string("schema could not be returned to version 1: ") +
                                     (impl_->db != nullptr && sqlite3_errmsg(impl_->db) != nullptr ? sqlite3_errmsg(impl_->db)
                                                                                                  : "unknown"));
  });
  return wrote;
}

void ProjectStore::debugArmMigrationFailure() {
  impl_->failNextMigration = true;
}

void ProjectStore::debugArmNextMutationFailure() {
  impl_->failNextMutation = true;
}

Result<std::vector<RegisteredReference>> ProjectStore::importOriginalReferences(
    const std::string& binId,
    const std::vector<ImportCheck>& files) {
  if (!impl_->open) {
    return Result<std::vector<RegisteredReference>>::failure(ErrorCode::NotOpen, "project is not open");
  }
  if (files.empty()) {
    return Result<std::vector<RegisteredReference>>::failure(ErrorCode::InvalidArgument, "no files were requested");
  }
  auto bin = readBin(binId);
  if (!bin.ok()) {
    return Result<std::vector<RegisteredReference>>::failure(bin.error.code, bin.error.message);
  }
  std::vector<RegisteredReference> registered;
  registered.reserve(files.size());
  for (const ImportCheck& file : files) {
    if (file.kind != ImportCheckKind::Ok || file.recordedPath.empty() || file.mediaType.empty()) {
      return Result<std::vector<RegisteredReference>>::failure(ErrorCode::InvalidArgument,
                                                               "import was given a file that did not pass inspection");
    }
    const auto mediaId = makeStableId();
    const auto representationId = makeStableId();
    const auto objectId = makeStableId();
    if (!mediaId || !representationId || !objectId) {
      return Result<std::vector<RegisteredReference>>::failure(ErrorCode::Io, "import id could not be minted");
    }
    RegisteredReference row;
    row.objectId = *objectId;
    row.mediaId = *mediaId;
    row.representationId = *representationId;
    row.recordedPath = file.recordedPath;
    row.fileName = file.fileName;
    row.mediaType = file.mediaType;
    registered.push_back(std::move(row));
  }
  auto wrote = impl_->mutate([&]() -> Result<void> {
    for (const RegisteredReference& row : registered) {
      Statement existing;
      if (existing.prepare(impl_->db,
                           "SELECT representation_id FROM media_representations WHERE recorded_path = ? LIMIT 1;") !=
              SQLITE_OK ||
          !bindText(existing.raw, 1, row.recordedPath)) {
        return Result<void>::failure(ErrorCode::Database, "recorded path could not be checked");
      }
      const int stepped = sqlite3_step(existing.raw);
      if (stepped == SQLITE_ROW) {
        return Result<void>::failure(
            ErrorCode::Constraint,
            "exact recorded path is already registered; no policy merges it or creates another Media ID; no records "
            "were written: " +
                row.recordedPath);
      }
      if (stepped != SQLITE_DONE) {
        return Result<void>::failure(ErrorCode::Database, "recorded path could not be checked");
      }
      const std::string stamp = utcTimestamp();
      Statement asset;
      if (asset.prepare(impl_->db,
                        "INSERT INTO media_assets "
                        "(media_id, project_id, media_type, metadata_json, created_utc, modified_utc) "
                        "VALUES (?, ?, ?, '{}', ?, ?);") != SQLITE_OK ||
          !bindText(asset.raw, 1, row.mediaId) || !bindText(asset.raw, 2, impl_->info.projectId) ||
          !bindText(asset.raw, 3, row.mediaType) || !bindText(asset.raw, 4, stamp) ||
          !bindText(asset.raw, 5, stamp) || sqlite3_step(asset.raw) != SQLITE_DONE) {
        return Result<void>::failure(ErrorCode::Database, "media insert failed and was rolled back");
      }
      Statement representation;
      if (representation.prepare(impl_->db,
                                 "INSERT INTO media_representations "
                                 "(representation_id, media_id, role, recorded_path, status, created_utc, "
                                 "modified_utc) VALUES (?, ?, 'original', ?, 'registered', ?, ?);") != SQLITE_OK ||
          !bindText(representation.raw, 1, row.representationId) || !bindText(representation.raw, 2, row.mediaId) ||
          !bindText(representation.raw, 3, row.recordedPath) || !bindText(representation.raw, 4, stamp) ||
          !bindText(representation.raw, 5, stamp) || sqlite3_step(representation.raw) != SQLITE_DONE) {
        return Result<void>::failure(ErrorCode::Database, "representation insert failed and was rolled back");
      }
      Statement object;
      if (object.prepare(impl_->db,
                         "INSERT INTO editorial_objects "
                         "(object_id, project_id, kind, name, metadata_json, created_utc, modified_utc) "
                         "VALUES (?, ?, 'master_clip', ?, '{}', ?, ?);") != SQLITE_OK ||
          !bindText(object.raw, 1, row.objectId) || !bindText(object.raw, 2, impl_->info.projectId) ||
          !bindText(object.raw, 3, row.fileName) || !bindText(object.raw, 4, stamp) ||
          !bindText(object.raw, 5, stamp) || sqlite3_step(object.raw) != SQLITE_DONE) {
        return Result<void>::failure(ErrorCode::Database, "bin object insert failed and was rolled back");
      }
      Statement clip;
      if (clip.prepare(impl_->db, "INSERT INTO master_clips (object_id, media_id) VALUES (?, ?);") != SQLITE_OK ||
          !bindText(clip.raw, 1, row.objectId) || !bindText(clip.raw, 2, row.mediaId) ||
          sqlite3_step(clip.raw) != SQLITE_DONE) {
        return Result<void>::failure(ErrorCode::Database, "master clip insert failed and was rolled back");
      }
      Statement member;
      if (member.prepare(impl_->db,
                         "INSERT INTO bin_membership (bin_id, object_id, created_utc) VALUES (?, ?, ?);") !=
              SQLITE_OK ||
          !bindText(member.raw, 1, binId) || !bindText(member.raw, 2, row.objectId) ||
          !bindText(member.raw, 3, stamp) || sqlite3_step(member.raw) != SQLITE_DONE) {
        return Result<void>::failure(ErrorCode::Database, "bin membership insert failed and was rolled back");
      }
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<std::vector<RegisteredReference>>::failure(wrote.error.code, wrote.error.message);
  }
  return Result<std::vector<RegisteredReference>>::success(std::move(registered));
}

[[noreturn]] void ProjectStore::crashMidTransactionForTest() {
  if (impl_->db == nullptr) {
    _exit(2);
  }
  char* message = nullptr;
  if (sqlite3_exec(impl_->db, "BEGIN IMMEDIATE;", nullptr, nullptr, &message) != SQLITE_OK) {
    sqlite3_free(message);
    _exit(3);
  }
  sqlite3_stmt* statement = nullptr;
  const char* sql =
      "INSERT INTO bins (bin_id, project_id, name, created_utc, modified_utc) "
      "VALUES ('interrupted-crash', ?, 'crash', 't', 't');";
  if (sqlite3_prepare_v2(impl_->db, sql, -1, &statement, nullptr) != SQLITE_OK ||
      sqlite3_bind_text(statement, 1, impl_->info.projectId.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
      sqlite3_step(statement) != SQLITE_DONE) {
    _exit(4);
  }
  _exit(42);
}

Result<ProjectStore::InstanceEditResult> ProjectStore::commitInstanceEdits(
    const std::string& trackId,
    const std::vector<std::string>& deleteIds,
    const std::vector<TimelineInstance>& updates,
    const std::vector<TimelineInstance>& inserts) {
  if (!impl_->open) {
    return Result<InstanceEditResult>::failure(ErrorCode::NotOpen, "project is not open");
  }
  if (trackId.empty()) {
    return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "choose a track");
  }
  Statement track;
  if (track.prepare(impl_->db, "SELECT sequence_id FROM tracks WHERE track_id = ?;") != SQLITE_OK ||
      !bindText(track.raw, 1, trackId) || sqlite3_step(track.raw) != SQLITE_ROW) {
    return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "track is not in the sequence");
  }
  const std::string sequenceId = columnText(track.raw, 0);
  auto sequence = readSequence(sequenceId);
  if (!sequence.ok()) {
    return Result<InstanceEditResult>::failure(sequence.error.code, sequence.error.message);
  }
  auto current = listInstances(trackId);
  if (!current.ok()) {
    return Result<InstanceEditResult>::failure(current.error.code, current.error.message);
  }
  auto listed = [&](const std::string& instanceId) {
    for (const TimelineInstance& row : current.value) {
      if (row.instanceId == instanceId) {
        return true;
      }
    }
    return false;
  };
  std::vector<TimelineInstance> finalRows;
  for (const TimelineInstance& row : current.value) {
    bool dropped = false;
    for (const std::string& instanceId : deleteIds) {
      if (instanceId == row.instanceId) {
        dropped = true;
      }
    }
    if (dropped) {
      continue;
    }
    bool replaced = false;
    for (const TimelineInstance& update : updates) {
      if (update.instanceId == row.instanceId) {
        finalRows.push_back(update);
        replaced = true;
      }
    }
    if (!replaced) {
      finalRows.push_back(row);
    }
  }
  for (const std::string& instanceId : deleteIds) {
    if (!listed(instanceId)) {
      return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "instance was not found");
    }
  }
  for (const TimelineInstance& update : updates) {
    if (update.trackId != trackId || update.sequenceId != sequenceId || !listed(update.instanceId)) {
      return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "track is not in the sequence");
    }
    bool seen = false;
    for (const TimelineInstance& row : finalRows) {
      if (row.instanceId == update.instanceId) {
        seen = true;
      }
    }
    if (!seen) {
      return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "instance was not found");
    }
  }
  for (const TimelineInstance& insert : inserts) {
    if (insert.trackId != trackId || insert.sequenceId != sequenceId || insert.instanceId.empty()) {
      return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "track is not in the sequence");
    }
    if (listed(insert.instanceId)) {
      return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "instance id is already on the track");
    }
    finalRows.push_back(insert);
  }
  for (const TimelineInstance& row : finalRows) {
    auto valid = validateInstancePlacement(row, sequence.value.rate, finalRows, row.instanceId);
    if (!valid.ok()) {
      return Result<InstanceEditResult>::failure(valid.error.code, valid.error.message);
    }
  }
  const std::string stamp = utcTimestamp();
  std::vector<TimelineInstance> writtenUpdates;
  std::vector<TimelineInstance> writtenInserts;
  auto wrote = impl_->mutate([&]() -> Result<void> {
    for (const std::string& instanceId : deleteIds) {
      Statement statement;
      if (statement.prepare(impl_->db, "DELETE FROM timeline_instances WHERE instance_id = ?;") != SQLITE_OK ||
          !bindText(statement.raw, 1, instanceId) || sqlite3_step(statement.raw) != SQLITE_DONE ||
          sqlite3_changes(impl_->db) != 1) {
        return Result<void>::failure(ErrorCode::Database, "instance delete failed");
      }
    }
    for (const TimelineInstance& update : updates) {
      const std::string modified = update.modifiedUtc.empty() ? stamp : update.modifiedUtc;
      Statement statement;
      if (statement.prepare(impl_->db,
                            "UPDATE timeline_instances SET source_start = ?, source_end = ?, record_start = ?, "
                            "record_end = ?, modified_utc = ? WHERE instance_id = ?;") != SQLITE_OK ||
          !bindInt64(statement.raw, 1, update.sourceStart) || !bindInt64(statement.raw, 2, update.sourceEnd) ||
          !bindInt64(statement.raw, 3, update.recordStart) || !bindInt64(statement.raw, 4, update.recordEnd) ||
          !bindText(statement.raw, 5, modified) || !bindText(statement.raw, 6, update.instanceId) ||
          sqlite3_step(statement.raw) != SQLITE_DONE || sqlite3_changes(impl_->db) != 1) {
        return Result<void>::failure(ErrorCode::Database, "instance update failed");
      }
      TimelineInstance stored = update;
      stored.modifiedUtc = modified;
      writtenUpdates.push_back(stored);
    }
    for (const TimelineInstance& insert : inserts) {
      const std::string created = insert.createdUtc.empty() ? stamp : insert.createdUtc;
      const std::string modified = insert.modifiedUtc.empty() ? stamp : insert.modifiedUtc;
      Statement statement;
      if (statement.prepare(impl_->db,
                            "INSERT INTO timeline_instances (instance_id, sequence_id, track_id, master_object_id, "
                            "media_id, source_rate_numerator, source_rate_denominator, source_start, source_end, "
                            "record_start, record_end, created_utc, modified_utc) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);") != SQLITE_OK) {
        return Result<void>::failure(ErrorCode::Database, "instance insert could not be prepared");
      }
      if (!bindText(statement.raw, 1, insert.instanceId) || !bindText(statement.raw, 2, insert.sequenceId) ||
          !bindText(statement.raw, 3, insert.trackId) || !bindText(statement.raw, 4, insert.masterObjectId) ||
          !bindText(statement.raw, 5, insert.mediaId) ||
          !bindInt64(statement.raw, 6, insert.sourceRate.numerator) ||
          !bindInt64(statement.raw, 7, insert.sourceRate.denominator) ||
          !bindInt64(statement.raw, 8, insert.sourceStart) || !bindInt64(statement.raw, 9, insert.sourceEnd) ||
          !bindInt64(statement.raw, 10, insert.recordStart) || !bindInt64(statement.raw, 11, insert.recordEnd) ||
          !bindText(statement.raw, 12, created) || !bindText(statement.raw, 13, modified) ||
          sqlite3_step(statement.raw) != SQLITE_DONE) {
        return Result<void>::failure(ErrorCode::Database, "instance insert failed");
      }
      TimelineInstance stored = insert;
      stored.createdUtc = created;
      stored.modifiedUtc = modified;
      writtenInserts.push_back(stored);
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<InstanceEditResult>::failure(wrote.error.code, wrote.error.message);
  }
  InstanceEditResult result;
  result.updated = std::move(writtenUpdates);
  result.inserted = std::move(writtenInserts);
  return Result<InstanceEditResult>::success(std::move(result));
}

Result<ProjectStore::InstanceEditResult> ProjectStore::commitMultiTrackEdits(
    const std::vector<InstanceTrackEdit>& edits) {
  if (!impl_->open) {
    return Result<InstanceEditResult>::failure(ErrorCode::NotOpen, "project is not open");
  }
  struct Prepared {
    InstanceTrackEdit edit;
    std::string sequenceId;
    FrameRate rate;
    std::vector<TimelineInstance> current;
  };
  std::vector<Prepared> prepared;
  std::vector<std::string> minted;
  for (const InstanceTrackEdit& edit : edits) {
    if (edit.trackId.empty()) {
      return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "choose a track");
    }
    for (const Prepared& earlier : prepared) {
      if (earlier.edit.trackId == edit.trackId) {
        return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument,
                                                   "track " + edit.trackId + " is named more than once");
      }
    }
    Statement track;
    if (track.prepare(impl_->db, "SELECT sequence_id FROM tracks WHERE track_id = ?;") != SQLITE_OK ||
        !bindText(track.raw, 1, edit.trackId) || sqlite3_step(track.raw) != SQLITE_ROW) {
      return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "track is not in the sequence");
    }
    Prepared row;
    row.edit = edit;
    row.sequenceId = columnText(track.raw, 0);
    auto sequence = readSequence(row.sequenceId);
    if (!sequence.ok()) {
      return Result<InstanceEditResult>::failure(sequence.error.code, sequence.error.message);
    }
    row.rate = sequence.value.rate;
    auto current = listInstances(edit.trackId);
    if (!current.ok()) {
      return Result<InstanceEditResult>::failure(current.error.code, current.error.message);
    }
    row.current = std::move(current.value);
    auto listed = [&](const std::string& instanceId) {
      for (const TimelineInstance& stored : row.current) {
        if (stored.instanceId == instanceId) {
          return true;
        }
      }
      return false;
    };
    std::vector<TimelineInstance> finalRows;
    for (const TimelineInstance& stored : row.current) {
      bool dropped = false;
      for (const std::string& instanceId : edit.deleteIds) {
        if (instanceId == stored.instanceId) {
          dropped = true;
        }
      }
      if (dropped) {
        continue;
      }
      bool replaced = false;
      for (const TimelineInstance& update : edit.updates) {
        if (update.instanceId == stored.instanceId) {
          finalRows.push_back(update);
          replaced = true;
        }
      }
      if (!replaced) {
        finalRows.push_back(stored);
      }
    }
    for (const std::string& instanceId : edit.deleteIds) {
      if (!listed(instanceId)) {
        return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "instance was not found");
      }
    }
    for (const TimelineInstance& update : edit.updates) {
      if (update.trackId != edit.trackId || update.sequenceId != row.sequenceId || !listed(update.instanceId)) {
        return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "track is not in the sequence");
      }
    }
    for (const TimelineInstance& insert : edit.inserts) {
      if (insert.trackId != edit.trackId || insert.sequenceId != row.sequenceId || insert.instanceId.empty()) {
        return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "track is not in the sequence");
      }
      if (listed(insert.instanceId)) {
        return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument, "instance id is already on the track");
      }
      for (const std::string& earlier : minted) {
        if (earlier == insert.instanceId) {
          return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument,
                                                     "instance id " + insert.instanceId + " collides with an existing row");
        }
      }
      Statement existing;
      if (existing.prepare(impl_->db, "SELECT instance_id FROM timeline_instances WHERE instance_id = ?;") !=
              SQLITE_OK ||
          !bindText(existing.raw, 1, insert.instanceId)) {
        return Result<InstanceEditResult>::failure(ErrorCode::Database, "instance id could not be checked");
      }
      const int step = sqlite3_step(existing.raw);
      if (step == SQLITE_ROW) {
        return Result<InstanceEditResult>::failure(ErrorCode::InvalidArgument,
                                                   "instance id " + insert.instanceId + " collides with an existing row");
      }
      if (step != SQLITE_DONE) {
        return Result<InstanceEditResult>::failure(ErrorCode::Database, "instance id could not be checked");
      }
      minted.push_back(insert.instanceId);
      finalRows.push_back(insert);
    }
    for (const TimelineInstance& stored : finalRows) {
      auto valid = validateInstancePlacement(stored, row.rate, finalRows, stored.instanceId);
      if (!valid.ok()) {
        return Result<InstanceEditResult>::failure(valid.error.code, valid.error.message);
      }
    }
    prepared.push_back(std::move(row));
  }
  const std::string stamp = utcTimestamp();
  std::vector<TimelineInstance> writtenUpdates;
  std::vector<TimelineInstance> writtenInserts;
  auto wrote = impl_->mutate([&]() -> Result<void> {
    for (const Prepared& row : prepared) {
      for (const std::string& instanceId : row.edit.deleteIds) {
        Statement statement;
        if (statement.prepare(impl_->db, "DELETE FROM timeline_instances WHERE instance_id = ?;") != SQLITE_OK ||
            !bindText(statement.raw, 1, instanceId) || sqlite3_step(statement.raw) != SQLITE_DONE ||
            sqlite3_changes(impl_->db) != 1) {
          return Result<void>::failure(ErrorCode::Database, "instance delete failed");
        }
      }
      for (const TimelineInstance& update : row.edit.updates) {
        const std::string modified = update.modifiedUtc.empty() ? stamp : update.modifiedUtc;
        Statement statement;
        if (statement.prepare(impl_->db,
                              "UPDATE timeline_instances SET source_start = ?, source_end = ?, record_start = ?, "
                              "record_end = ?, modified_utc = ? WHERE instance_id = ?;") != SQLITE_OK ||
            !bindInt64(statement.raw, 1, update.sourceStart) || !bindInt64(statement.raw, 2, update.sourceEnd) ||
            !bindInt64(statement.raw, 3, update.recordStart) || !bindInt64(statement.raw, 4, update.recordEnd) ||
            !bindText(statement.raw, 5, modified) || !bindText(statement.raw, 6, update.instanceId) ||
            sqlite3_step(statement.raw) != SQLITE_DONE || sqlite3_changes(impl_->db) != 1) {
          return Result<void>::failure(ErrorCode::Database, "instance update failed");
        }
        TimelineInstance stored = update;
        stored.modifiedUtc = modified;
        writtenUpdates.push_back(stored);
      }
      for (const TimelineInstance& insert : row.edit.inserts) {
        const std::string created = insert.createdUtc.empty() ? stamp : insert.createdUtc;
        const std::string modified = insert.modifiedUtc.empty() ? stamp : insert.modifiedUtc;
        Statement statement;
        if (statement.prepare(impl_->db,
                              "INSERT INTO timeline_instances (instance_id, sequence_id, track_id, master_object_id, "
                              "media_id, source_rate_numerator, source_rate_denominator, source_start, source_end, "
                              "record_start, record_end, created_utc, modified_utc) "
                              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);") != SQLITE_OK) {
          return Result<void>::failure(ErrorCode::Database, "instance insert could not be prepared");
        }
        if (!bindText(statement.raw, 1, insert.instanceId) || !bindText(statement.raw, 2, insert.sequenceId) ||
            !bindText(statement.raw, 3, insert.trackId) || !bindText(statement.raw, 4, insert.masterObjectId) ||
            !bindText(statement.raw, 5, insert.mediaId) ||
            !bindInt64(statement.raw, 6, insert.sourceRate.numerator) ||
            !bindInt64(statement.raw, 7, insert.sourceRate.denominator) ||
            !bindInt64(statement.raw, 8, insert.sourceStart) || !bindInt64(statement.raw, 9, insert.sourceEnd) ||
            !bindInt64(statement.raw, 10, insert.recordStart) || !bindInt64(statement.raw, 11, insert.recordEnd) ||
            !bindText(statement.raw, 12, created) || !bindText(statement.raw, 13, modified) ||
            sqlite3_step(statement.raw) != SQLITE_DONE) {
          return Result<void>::failure(ErrorCode::Database, "instance insert failed");
        }
        TimelineInstance stored = insert;
        stored.createdUtc = created;
        stored.modifiedUtc = modified;
        writtenInserts.push_back(stored);
      }
    }
    return Result<void>::success();
  });
  if (!wrote.ok()) {
    return Result<InstanceEditResult>::failure(wrote.error.code, wrote.error.message);
  }
  InstanceEditResult result;
  result.updated = std::move(writtenUpdates);
  result.inserted = std::move(writtenInserts);
  return Result<InstanceEditResult>::success(std::move(result));
}

}  // namespace aether
