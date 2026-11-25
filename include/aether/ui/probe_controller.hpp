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

// Runs a representation probe off the UI thread. This header does not include FFmpeg.

#include "aether/media/probe.hpp"

#include <QObject>
#include <QString>

#include <cstdint>
#include <functional>
#include <map>
#include <string>

class QThread;

namespace aether {

class ProbeController : public QObject {
  Q_OBJECT

 public:
  using Function = std::function<ProbeReport(const std::string&)>;

  explicit ProbeController(QObject* parent = nullptr);
  ~ProbeController() override;

  void setProbeFunctionForTest(Function function);
  void setProject(const std::string& projectId);
  void showRepresentation(const std::string& representationId);
  void probe(const std::string& representationId, const std::string& recordedPath);

  QString statusText() const;
  QString reportText() const;

 signals:
  void updated();

 private:
  struct Request {
    std::uint64_t epoch = 0;
    std::uint64_t generation = 0;
    std::string projectId;
    std::string representationId;
    std::string path;
    Function function;
  };

  void apply(const Request& request, const ProbeReport& report);

  Function function_;
  QThread* thread_ = nullptr;
  QObject* worker_ = nullptr;
  std::uint64_t epoch_ = 0;
  std::uint64_t nextGeneration_ = 0;
  std::string projectId_;
  std::string shown_;
  std::map<std::string, std::uint64_t> inflight_;
  std::map<std::string, ProbeReport> cache_;
  bool stopping_ = false;
};

}  // namespace aether
