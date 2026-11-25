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

// Slice 0A prototype types. Not a permanent error catalogue.

#include <string>
#include <utility>

namespace aether {

enum class ErrorCode {
  Ok = 0,
  InvalidArgument,
  Io,
  LockHeld,
  ManifestInvalid,
  ManifestMismatch,
  DatabaseMissing,
  Database,
  Schema,
  Integrity,
  ForeignKey,
  Constraint,
  NotOpen,
  NothingToUndo,
  NothingToRedo,
  MigrationRefused,
};

struct Error {
  ErrorCode code = ErrorCode::Ok;
  std::string message;

  bool ok() const { return code == ErrorCode::Ok; }
};

template <typename T>
struct Result {
  T value{};
  Error error;

  bool ok() const { return error.ok(); }

  static Result success(T v) {
    Result out;
    out.value = std::move(v);
    return out;
  }

  static Result failure(ErrorCode code, std::string message) {
    Result out;
    out.error.code = code;
    out.error.message = std::move(message);
    return out;
  }
};

template <>
struct Result<void> {
  Error error;

  bool ok() const { return error.ok(); }

  static Result success() { return Result{}; }

  static Result failure(ErrorCode code, std::string message) {
    Result out;
    out.error.code = code;
    out.error.message = std::move(message);
    return out;
  }
};

inline const char* errorCodeName(ErrorCode code) {
  switch (code) {
    case ErrorCode::Ok:
      return "ok";
    case ErrorCode::InvalidArgument:
      return "invalid_argument";
    case ErrorCode::Io:
      return "io";
    case ErrorCode::LockHeld:
      return "lock_held";
    case ErrorCode::ManifestInvalid:
      return "manifest_invalid";
    case ErrorCode::ManifestMismatch:
      return "manifest_mismatch";
    case ErrorCode::DatabaseMissing:
      return "database_missing";
    case ErrorCode::Database:
      return "database";
    case ErrorCode::Schema:
      return "schema";
    case ErrorCode::Integrity:
      return "integrity";
    case ErrorCode::ForeignKey:
      return "foreign_key";
    case ErrorCode::Constraint:
      return "constraint";
    case ErrorCode::NotOpen:
      return "not_open";
    case ErrorCode::NothingToUndo:
      return "nothing_to_undo";
    case ErrorCode::NothingToRedo:
      return "nothing_to_redo";
    case ErrorCode::MigrationRefused:
      return "migration_refused";
  }
  return "unknown";
}

}  // namespace aether
