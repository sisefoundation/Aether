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

#include "aether/ui/probe_controller.hpp"

#include <QThread>

#include <exception>

namespace aether {

ProbeController::ProbeController(QObject* parent) : QObject(parent) {
  setObjectName("probeController");
  function_ = [](const std::string& path) { return probeRegisteredRepresentation(path); };
  worker_ = new QObject;
  thread_ = new QThread(this);
  worker_->moveToThread(thread_);
  thread_->start();
}

ProbeController::~ProbeController() {
  stopping_ = true;
  thread_->quit();
  thread_->wait();
  delete worker_;
  worker_ = nullptr;
}

void ProbeController::setProbeFunctionForTest(Function function) { function_ = std::move(function); }

void ProbeController::setProject(const std::string& projectId) {
  if (projectId == projectId_) {
    return;
  }
  projectId_ = projectId;
  ++epoch_;
  cache_.clear();
  inflight_.clear();
  shown_.clear();
  emit updated();
}

void ProbeController::showRepresentation(const std::string& representationId) {
  shown_ = representationId;
  emit updated();
}

void ProbeController::probe(const std::string& representationId, const std::string& recordedPath) {
  if (stopping_ || projectId_.empty() || representationId.empty() || recordedPath.empty() || !function_) {
    return;
  }
  Request request;
  request.epoch = epoch_;
  request.generation = ++nextGeneration_;
  request.projectId = projectId_;
  request.representationId = representationId;
  request.path = recordedPath;
  request.function = function_;
  inflight_[representationId] = request.generation;
  emit updated();
  QMetaObject::invokeMethod(
      worker_,
      [this, request]() {
        ProbeReport report;
        try {
          report = request.function(request.path);
        } catch (const std::exception& ex) {
          report.completion = ProbeCompletion::Failed;
          report.errors.push_back(ex.what());
        }
        QMetaObject::invokeMethod(
            this, [this, request, report]() { apply(request, report); }, Qt::QueuedConnection);
      },
      Qt::QueuedConnection);
}

QString ProbeController::statusText() const {
  if (shown_.empty()) {
    return QStringLiteral("Not probed");
  }
  if (inflight_.find(shown_) != inflight_.end()) {
    return QStringLiteral("Probing");
  }
  const auto found = cache_.find(shown_);
  if (found == cache_.end()) {
    return QStringLiteral("Not probed");
  }
  switch (found->second.completion) {
    case ProbeCompletion::Complete:
      return QStringLiteral("Probed");
    case ProbeCompletion::Incomplete:
      return QStringLiteral("Incomplete");
    case ProbeCompletion::Failed:
      return QStringLiteral("Failed");
    case ProbeCompletion::Probing:
      return QStringLiteral("Probing");
    case ProbeCompletion::NotProbed:
      return QStringLiteral("Not probed");
  }
  return QStringLiteral("Not probed");
}

QString ProbeController::reportText() const {
  if (shown_.empty()) {
    return QStringLiteral("Not probed. No representation is selected.");
  }
  if (inflight_.find(shown_) != inflight_.end()) {
    return QStringLiteral(
        "Probing this representation. The window stays responsive. "
        "No source rate is filled. Probe metadata is not editorial timing.");
  }
  const auto found = cache_.find(shown_);
  if (found == cache_.end()) {
    return QStringLiteral(
        "Not probed. The file is registered. No container, duration, frame rate, or frame count is known yet. "
        "Placement keeps the explicit source rate.");
  }
  return QString::fromStdString(formatProbeReport(found->second));
}

void ProbeController::apply(const Request& request, const ProbeReport& report) {
  if (stopping_ || request.epoch != epoch_ || request.projectId != projectId_) {
    return;
  }
  const auto inflight = inflight_.find(request.representationId);
  if (inflight == inflight_.end() || inflight->second != request.generation) {
    return;
  }
  inflight_.erase(inflight);
  cache_[request.representationId] = report;
  emit updated();
}

}  // namespace aether
