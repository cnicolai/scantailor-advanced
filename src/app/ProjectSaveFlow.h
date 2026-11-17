// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_PROJECTSAVEFLOW_H_
#define SCANTAILOR_APP_PROJECTSAVEFLOW_H_

#include <QCoreApplication>
#include <QString>
#include <memory>

class OutputFileNameGenerator;
class ProjectPages;
class QWidget;
class SelectedPage;
class StageSequence;

/**
 * The one "Save Project As" flow, used by the main window and by the out-of-memory dialog: the file panel, the
 * ".ScanTailor" extension, the project writer with its warning box, and the remembered folder and recent-projects
 * list. Every function runs on the GUI thread.
 */
class ProjectSaveFlow {
  Q_DECLARE_TR_FUNCTIONS(ProjectSaveFlow)
 public:
  /**
   * Asks for a project file name (the panel starts in the folder of \p currentProjectFile, or in the last used
   * folder when that is empty), writes the project and remembers the file.
   *
   * \return The saved path, or an empty string when the panel was cancelled or the write failed (the failure has
   *         been reported to the user).
   */
  static QString saveAs(QWidget* parent,
                        const QString& currentProjectFile,
                        const std::shared_ptr<ProjectPages>& pages,
                        const SelectedPage& selectedPage,
                        const OutputFileNameGenerator& outFileNameGen,
                        const StageSequence& stages);

  /** Writes the project file; on failure shows the "Error saving the project file!" box and returns false. */
  static bool writeWithFeedback(QWidget* parent,
                                const QString& projectFile,
                                const std::shared_ptr<ProjectPages>& pages,
                                const SelectedPage& selectedPage,
                                const OutputFileNameGenerator& outFileNameGen,
                                const StageSequence& stages);

  /** Shows the "Error saving the project file!" box. */
  static void showSaveError(QWidget* parent);

  /** \p path with ".ScanTailor" appended unless it already ends with that (any letter case). */
  static QString withProjectExtension(const QString& path);

  /** Stores the folder of \p projectFile as "project/lastDir" and moves the file to the top of the recent list. */
  static void remember(const QString& projectFile);
};

#endif  // ifndef SCANTAILOR_APP_PROJECTSAVEFLOW_H_
