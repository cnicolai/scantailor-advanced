// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutOfMemoryDialog.h"

#include <utility>

#include "ProjectSaveFlow.h"

OutOfMemoryDialog::OutOfMemoryDialog(QWidget* parent) : QDialog(parent) {
  ui.setupUi(this);
  if (sizeof(void*) > 4) {
    ui.only_32bit_1->hide();
    ui.only_32bit_2->hide();
  }

  ui.topLevelStack->setCurrentWidget(ui.mainPage);

  connect(ui.saveProjectBtn, SIGNAL(clicked()), SLOT(saveProject()));
  connect(ui.saveProjectAsBtn, SIGNAL(clicked()), SLOT(saveProjectAs()));
  connect(ui.dontSaveBtn, SIGNAL(clicked()), SLOT(reject()));
}

void OutOfMemoryDialog::setParams(const QString& projectFile,
                                  std::shared_ptr<StageSequence> stages,
                                  std::shared_ptr<ProjectPages> pages,
                                  const SelectedPage& selectedPage,
                                  const OutputFileNameGenerator& outFileNameGen) {
  m_projectFile = projectFile;
  m_stages = std::move(stages);
  m_pages = std::move(pages);
  m_selectedPage = selectedPage;
  m_outFileNameGen = outFileNameGen;

  ui.saveProjectBtn->setVisible(!projectFile.isEmpty());
}

void OutOfMemoryDialog::saveProject() {
  if (m_projectFile.isEmpty()) {
    saveProjectAs();
  } else if (ProjectSaveFlow::writeWithFeedback(this, m_projectFile, m_pages, m_selectedPage, m_outFileNameGen,
                                                *m_stages)) {
    showSaveSuccessScreen();
  }
}

void OutOfMemoryDialog::saveProjectAs() {
  const QString projectFile
      = ProjectSaveFlow::saveAs(this, m_projectFile, m_pages, m_selectedPage, m_outFileNameGen, *m_stages);
  if (projectFile.isEmpty()) {
    return;
  }

  m_projectFile = projectFile;
  showSaveSuccessScreen();
}

void OutOfMemoryDialog::showSaveSuccessScreen() {
  ui.topLevelStack->setCurrentWidget(ui.saveSuccessPage);
}
