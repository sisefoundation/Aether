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

#include "aether/persistence/logger.hpp"
#include "aether/ui/main_window.hpp"

#include <QApplication>
#include <QTimer>

#include <iostream>
#include <string>
#include <vector>

namespace aether {
bool runProbeCheckpoint(MainWindow& window, ProjectSession& session, const std::string& projectDir,
                        const std::string& shotDir, const std::vector<std::string>& imports);
}

int main(int argc, char** argv) {
  QApplication application(argc, argv);
  application.setApplicationName("Aether");

  aether::FileLogger logger;
  logger.info("startup", "AETHER start");

  aether::ProjectSession session(logger);
  aether::MainWindow window(session, logger);
  window.show();

  bool shutdownNow = false;
  std::string projectDir;
  std::string shotDir;
  std::vector<std::string> imports;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--shutdown-now") {
      shutdownNow = true;
    } else if (arg == "--checkpoint-project" && i + 1 < argc) {
      projectDir = argv[++i];
    } else if (arg == "--checkpoint-shots" && i + 1 < argc) {
      shotDir = argv[++i];
    } else if (arg == "--import" && i + 1 < argc) {
      imports.push_back(argv[++i]);
    }
  }
  if (shutdownNow) {
    QTimer::singleShot(0, &application, &QApplication::quit);
  } else if (!projectDir.empty() && !shotDir.empty()) {
    if (!aether::runProbeCheckpoint(window, session, projectDir, shotDir, imports)) {
      std::cerr << "probe checkpoint failed\n";
      return 1;
    }
  } else {
    QTimer::singleShot(0, &window, [&window, &session]() {
      if (!session.isOpen()) {
        window.showProjectDialog();
      }
    });
  }

  const int code = application.exec();
  session.closeProject();
  logger.info("shutdown", "AETHER stop");
  return code;
}
