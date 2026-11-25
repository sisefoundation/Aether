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

#include <QObject>

class MediaProbeTests : public QObject {
  Q_OBJECT

 private slots:
  void linkedPrefixIsFfmpeg91AndNotSystem902();
  void publicHeadersDoNotIncludeFfmpeg();
  void videoProbeKeepsRationalTimebaseAndDoesNotVerifyRate();
  void audioProbeUsesChannelLayout();
  void bothStreamsAreReported();
  void multipleStreamsIncludeSubtitle();
  void attachmentStreamIsReported();
  void missingDurationStartAndFrameCountStayUnknown();
  void conflictingRatesStayUnverified();
  void malformedTruncatedMissingAndInaccessibleFail();
  void remoteProtocolAndUnsupportedContainerAreRefused();
  void fileHandlesAreReleased();
  void identitySurvivesMoveAndOffline();
  void probeLeavesMarksAndEditorialRowsUntouched();
  void asyncProbeStaysResponsiveAndDropsStaleResults();
  void reprobeAndProjectSwitchDoNotShowAStaleResult();
};
