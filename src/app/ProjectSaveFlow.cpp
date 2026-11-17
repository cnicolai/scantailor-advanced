// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectSaveFlow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>

#include "OutputFileNameGenerator.h"
#include "ProjectPages.h"
#include "ProjectWriter.h"
#include "RecentProjects.h"
#include "SelectedPage.h"
#include "StageSequence.h"

QString ProjectSaveFlow::saveAs(QWidget* parent,
                                const QString& currentProjectFile,
                                const std::shared_ptr<ProjectPages>& pages,
                                const SelectedPage& selectedPage,
                                const OutputFileNameGenerator& outFileNameGen,
                                const StageSequence& stages) {
  QString projectDir;
  if (!currentProjectFile.isEmpty()) {
    projectDir = QFileInfo(currentProjectFile).absolutePath();
  } else {
    QSettings settings;
    projectDir = settings.value("project/lastDir").toString();
  }

  // The native panel on the Mac: no option here may force Qt's own dialog.
  const QString chosen(
      QFileDialog::getSaveFileName(parent, QString(), projectDir, tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (chosen.isEmpty()) {
    return QString();
  }

  const QString projectFile = withProjectExtension(chosen);
  if (!writeWithFeedback(parent, projectFile, pages, selectedPage, outFileNameGen, stages)) {
    return QString();
  }

  remember(projectFile);
  return projectFile;
}

bool ProjectSaveFlow::writeWithFeedback(QWidget* parent,
                                        const QString& projectFile,
                                        const std::shared_ptr<ProjectPages>& pages,
                                        const SelectedPage& selectedPage,
                                        const OutputFileNameGenerator& outFileNameGen,
                                        const StageSequence& stages) {
  ProjectWriter writer(pages, selectedPage, outFileNameGen);

  if (!writer.write(projectFile, stages.filters())) {
    showSaveError(parent);
    return false;
  }
  return true;
}

void ProjectSaveFlow::showSaveError(QWidget* parent) {
  QMessageBox::warning(parent, tr("Error"), tr("Error saving the project file!"));
}

QString ProjectSaveFlow::withProjectExtension(const QString& path) {
  if (path.endsWith(".ScanTailor", Qt::CaseInsensitive)) {
    return path;
  }
  return path + ".ScanTailor";
}

void ProjectSaveFlow::remember(const QString& projectFile) {
  QSettings settings;
  settings.setValue("project/lastDir", QFileInfo(projectFile).absolutePath());

  RecentProjects rp;
  rp.read();
  rp.setMostRecent(projectFile);
  rp.write();
}
