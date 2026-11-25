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

#include "mark_tests.hpp"
#include "media_import_tests.hpp"
#include "media_probe_tests.hpp"
#include "overwrite_tests.hpp"
#include "playback_tests.hpp"
#include "extract_tests.hpp"
#include "lift_tests.hpp"
#include "split_tests.hpp"
#include "project_tests.hpp"
#include "sequence_tests.hpp"
#include "timeline_placement_tests.hpp"
#include "timeline_workspace_tests.hpp"
#include "timing_tests.hpp"
#include "ui_completion_tests.hpp"
#include "ui_integration_tests.hpp"

#include <QApplication>
#include <QTest>

int main(int argc, char** argv) {
  QApplication application(argc, argv);
  int status = 0;
  TimingTests timing;
  status |= QTest::qExec(&timing, argc, argv);
  ProjectTests project;
  status |= QTest::qExec(&project, argc, argv);
  SequenceTests sequence;
  status |= QTest::qExec(&sequence, argc, argv);
  TimelineWorkspaceTests workspace;
  status |= QTest::qExec(&workspace, argc, argv);
  MediaImportTests media;
  status |= QTest::qExec(&media, argc, argv);
  UiIntegrationTests integration;
  status |= QTest::qExec(&integration, argc, argv);
  TimelinePlacementTests placement;
  status |= QTest::qExec(&placement, argc, argv);
  MarkTests marks;
  status |= QTest::qExec(&marks, argc, argv);
  MediaProbeTests probe;
  status |= QTest::qExec(&probe, argc, argv);
  OverwriteTests overwrite;
  status |= QTest::qExec(&overwrite, argc, argv);
  SplitTests split;
  status |= QTest::qExec(&split, argc, argv);
  LiftTests lift;
  status |= QTest::qExec(&lift, argc, argv);
  ExtractTests extract;
  status |= QTest::qExec(&extract, argc, argv);
  UiCompletionTests completion;
  status |= QTest::qExec(&completion, argc, argv);
  PlaybackTests playback;
  status |= QTest::qExec(&playback, argc, argv);
  return status;
}
