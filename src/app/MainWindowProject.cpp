// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "MainWindow.h"

#include <core/ApplicationSettings.h>
#include <core/IconProvider.h>

#include <QActionGroup>
#include <QCursor>
#include <QDir>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QMenu>
#include <QMessageBox>
#include <QResource>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QtWidgets/QInputDialog>
#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <cmath>
#include <memory>

#include "AbstractRelinker.h"
#include "Application.h"
#include "AutoRemovingFile.h"
#include "BasicImageView.h"
#include "ContentBoxPropagator.h"
#include "DebugImageView.h"
#include "DebugImages.h"
#include "DefaultParamsDialog.h"
#include "ErrorWidget.h"
#include "FilterOptionsWidget.h"
#include "FixDpiDialog.h"
#include "ImageInfo.h"
#include "ImageMetadataLoader.h"
#include "ImageSizeLimit.h"
#include "ImageViewBase.h"
#include "LoadFileTask.h"
#include "LoadFilesStatusDialog.h"
#include "MemoryBudget.h"
#include "NewOpenProjectPanel.h"
#include "OutOfMemoryDialog.h"
#include "OutOfMemoryHandler.h"
#include "PageOrientationPropagator.h"
#include "PageSelectionAccessor.h"
#include "PageSequence.h"
#include "ProcessingIndicationWidget.h"
#include "ProcessingTaskQueue.h"
#include "ProjectCreationContext.h"
#include "ProjectOpeningContext.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectSaveFlow.h"
#include "ProjectWriter.h"
#include "RecentProjects.h"
#include "RelinkingDialog.h"
#include "ScopedIncDec.h"
#include "SettingsDialog.h"
#include "SkinnedButton.h"
#include "SmartFilenameOrdering.h"
#include "StageSequence.h"
#include "SystemLoadWidget.h"
#include "TabbedDebugImages.h"
#include "ThumbnailDprTracker.h"
#include "ThumbnailFactory.h"
#include "UnitsProvider.h"
#include "Utils.h"
#include "WorkerThreadPool.h"
#include "config.h"
#include "filters/deskew/CacheDrivenTask.h"
#include "filters/deskew/Task.h"
#include "filters/fix_orientation/CacheDrivenTask.h"
#include "filters/fix_orientation/Task.h"
#include "filters/output/CacheDrivenTask.h"
#include "filters/output/TabbedImageView.h"
#include "filters/output/Task.h"
#include "filters/page_layout/CacheDrivenTask.h"
#include "filters/page_layout/Task.h"
#include "filters/page_split/CacheDrivenTask.h"
#include "filters/page_split/Task.h"
#include "filters/select_content/CacheDrivenTask.h"
#include "filters/select_content/Task.h"
#include "ui_AboutDialog.h"
#include "ui_BatchProcessingLowerPanel.h"
#include "ui_RemovePagesDialog.h"
#include "version.h"

using namespace core;

void MainWindow::switchToNewProject(const std::shared_ptr<ProjectPages>& pages,
                                    const QString& outDir,
                                    const QString& projectFilePath,
                                    const ProjectReader* projectReader) {
  stopBatchProcessing(CLEAR_MAIN_AREA);
  // The cap belongs to the pages of the previous project, whatever ended its batch.
  m_workerThreadPool->setMemoryCap(MemoryBudget::kNoCap);
  m_systemLoadWidget->setMemoryCap(MemoryBudget::kNoCap);
  m_interactiveQueue->cancelAndClear();

  if (!outDir.isEmpty()) {
    Utils::maybeCreateCacheDir(outDir);
  }
  m_pages = pages;
  m_projectFile = projectFilePath;

  if (projectReader) {
    m_selectedPage = projectReader->selectedPage();
  }

  std::shared_ptr<FileNameDisambiguator> disambiguator;
  if (projectReader) {
    disambiguator = projectReader->namingDisambiguator();
  } else {
    disambiguator.reset(new FileNameDisambiguator);
  }

  m_outFileNameGen = OutputFileNameGenerator(disambiguator, outDir, pages->layoutDirection());
  // These two need to go in this order.
  updateDisambiguationRecords(pages->toPageSequence(IMAGE_VIEW));

  // Recreate the stages and load their state.
  m_stages = std::make_shared<StageSequence>(pages, newPageSelectionAccessor());
  if (projectReader) {
    projectReader->readFilterSettings(m_stages->filters());
  }

  // Connect the filter list model to the view and select
  // the first item.
  {
    ScopedIncDec<int> guard(m_ignoreSelectionChanges);
    filterList->setStages(m_stages);
    filterList->selectRow(0);
    m_curFilter = 0;
    // Setting a data model also implicitly sets a new
    // selection model, so we have to reconnect to it.
    connect(filterList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
            SLOT(filterSelectionChanged(const QItemSelection&)));
  }

  updateSortOptions();

  m_contentBoxPropagator = std::make_unique<ContentBoxPropagator>(
      m_stages->pageLayoutFilter(), createCompositeCacheDrivenTask(m_stages->selectContentFilterIdx()));

  m_pageOrientationPropagator = std::make_unique<PageOrientationPropagator>(
      m_stages->pageSplitFilter(), createCompositeCacheDrivenTask(m_stages->fixOrientationFilterIdx()));

  // Thumbnails are stored relative to the output directory,
  // so recreate the thumbnail cache.
  if (outDir.isEmpty()) {
    m_thumbnailCache.reset();
  } else {
    m_thumbnailCache = Utils::createThumbnailCache(m_outFileNameGen.outDir(), thumbView->devicePixelRatioF());
  }
  m_thumbDprTracker->setCache(m_thumbnailCache);
  resetThumbSequence(currentPageOrderProvider());

  removeFilterOptionsWidget();
  updateProjectActions();
  updateWindowTitle();
  updateMainArea();

  if (!QDir(outDir).exists()) {
    showRelinkingDialog();
  }
}  // MainWindow::switchToNewProject

void MainWindow::showNewOpenProjectPanel() {
  auto outerWidget = std::make_unique<QWidget>();
  auto* layout = new QGridLayout(outerWidget.get());
  outerWidget->setLayout(layout);

  auto* nop = new NewOpenProjectPanel(outerWidget.get());
  // We use asynchronous connections because otherwise we
  // would be deleting a widget from its event handler, which
  // Qt doesn't like.
  connect(nop, SIGNAL(newProject()), this, SLOT(newProject()), Qt::QueuedConnection);
  connect(nop, SIGNAL(openProject()), this, SLOT(openProject()), Qt::QueuedConnection);
  connect(nop, SIGNAL(openRecentProject(const QString&)), this, SLOT(openProject(const QString&)),
          Qt::QueuedConnection);

  layout->addWidget(nop, 1, 1);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(2, 1);
  layout->setRowStretch(0, 1);
  layout->setRowStretch(2, 1);
  setImageWidget(outerWidget.release(), TRANSFER_OWNERSHIP);

  filterList->setBatchProcessingPossible(false);
}  // MainWindow::showNewOpenProjectPanel

MainWindow::SavePromptResult MainWindow::promptProjectSave() {
  QMessageBox msgBox(QMessageBox::Question, tr("Save Project"), tr("Save the project?"),
                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
  msgBox.setDefaultButton(QMessageBox::Yes);

  switch (msgBox.exec()) {
    case QMessageBox::Yes:
      return SAVE;
    case QMessageBox::No:
      return DONT_SAVE;
    default:
      return CANCEL;
  }
}

void MainWindow::autoSaveProject() {
  if (m_projectFile.isEmpty()) {
    return;
  }
  if (!ApplicationSettings::getInstance().isAutoSaveProjectEnabled()) {
    return;
  }

  saveProjectWithFeedback(m_projectFile);
}

bool MainWindow::saveProjectTriggered() {
  if (m_projectFile.isEmpty()) {
    return saveProjectAsTriggered();
  }

  if (!saveProjectWithFeedback(m_projectFile)) {
    return false;
  }
  updateWindowTitle();
  return true;
}

bool MainWindow::saveProjectAsTriggered() {
  const QString projectFile
      = ProjectSaveFlow::saveAs(this, m_projectFile, m_pages, m_selectedPage, m_outFileNameGen, *m_stages);
  if (projectFile.isEmpty()) {
    return false;
  }

  m_projectFile = projectFile;
  updateWindowTitle();
  return true;
}

void MainWindow::newProject() {
  if (!closeProjectInteractive()) {
    return;
  }

  // It will delete itself when it's done.
  auto* context = new ProjectCreationContext(this);
  connect(context, SIGNAL(done(ProjectCreationContext*)), this, SLOT(newProjectCreated(ProjectCreationContext*)));
}

void MainWindow::newProjectCreated(ProjectCreationContext* context) {
  auto pages = std::make_shared<ProjectPages>(context->files(), ProjectPages::AUTO_PAGES, context->layoutDirection());
  switchToNewProject(pages, context->outDir());
}

void MainWindow::openProject() {
  if (!closeProjectInteractive()) {
    return;
  }

  const QString projectDir(QSettings().value("project/lastDir").toString());
  const QString projectFile(QFileDialog::getOpenFileName(this, tr("Open Project"), projectDir,
                                                         tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (projectFile.isEmpty()) {
    // Cancelled by user.
    return;
  }

  openProject(projectFile);
}

void MainWindow::openProject(const QString& projectFile) {
  QFile file(projectFile);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, tr("Error"), tr("Unable to open the project file."));
    return;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    QMessageBox::warning(this, tr("Error"), tr("The project file is broken."));
    return;
  }

  file.close();

  auto* context = new ProjectOpeningContext(this, projectFile, doc);
  connect(context, SIGNAL(done(ProjectOpeningContext*)), SLOT(projectOpened(ProjectOpeningContext*)));
  context->proceed();
}

void MainWindow::projectOpened(ProjectOpeningContext* context) {
  RecentProjects rp;
  rp.read();
  rp.setMostRecent(context->projectFile());
  rp.write();

  QSettings().setValue("project/lastDir", QFileInfo(context->projectFile()).absolutePath());

  switchToNewProject(context->projectReader()->pages(), context->projectReader()->outputDirectory(),
                     context->projectFile(), context->projectReader());
}

void MainWindow::closeProject() {
  closeProjectInteractive();
}

/**
 * \brief Closes the currently project, prompting to save it if necessary.
 *
 * \return true if the project was closed, false if the user cancelled the process.
 */
bool MainWindow::closeProjectInteractive() {
  if (!isProjectLoaded()) {
    return true;
  }

  if (m_projectFile.isEmpty()) {
    switch (promptProjectSave()) {
      case SAVE:
        if (!saveProjectTriggered()) {
          return false;  // Save As cancelled or the write failed: stay in the project, nothing is lost.
        }
        break;
      case DONT_SAVE:
        break;
      case CANCEL:
        return false;
    }
    closeProjectWithoutSaving();
    return true;
  }

  const QFileInfo projectFile(m_projectFile);
  const QFileInfo backupFile(projectFile.absoluteDir(), QString::fromLatin1("Backup.") + projectFile.fileName());
  const QString backupFilePath(backupFile.absoluteFilePath());

  ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen);

  if (!writer.write(backupFilePath, m_stages->filters())) {
    // Backup file could not be written???
    QFile::remove(backupFilePath);
    switch (promptProjectSave()) {
      case SAVE:
        if (!saveProjectTriggered()) {
          return false;  // Save As cancelled or the write failed: stay in the project, nothing is lost.
        }
        break;
      case DONT_SAVE:
        break;
      case CANCEL:
        return false;
    }
    closeProjectWithoutSaving();
    return true;
  }

  if (compareFiles(m_projectFile, backupFilePath)) {
    // The project hasn't really changed.
    QFile::remove(backupFilePath);
    closeProjectWithoutSaving();
    return true;
  }

  switch (promptProjectSave()) {
    case SAVE:
      if (!Utils::overwritingRename(backupFilePath, m_projectFile)) {
        ProjectSaveFlow::showSaveError(this);
        return false;
      }
      // fall through
    case DONT_SAVE:
      QFile::remove(backupFilePath);
      break;
    case CANCEL:
      return false;
  }

  closeProjectWithoutSaving();
  return true;
}  // MainWindow::closeProjectInteractive

void MainWindow::closeProjectWithoutSaving() {
  auto pages = std::make_shared<ProjectPages>();
  switchToNewProject(pages, QString());
}

bool MainWindow::saveProjectWithFeedback(const QString& projectFile) {
  return ProjectSaveFlow::writeWithFeedback(this, projectFile, m_pages, m_selectedPage, m_outFileNameGen, *m_stages);
}
