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

void MainWindow::createBatchProcessingWidget() {
  m_batchProcessingWidget = std::make_unique<QWidget>();
  auto* layout = new QGridLayout(m_batchProcessingWidget.get());
  m_batchProcessingWidget->setLayout(layout);

  const auto& iconProvider = IconProvider::getInstance();
  auto* stopBtn = new SkinnedButton(iconProvider.getIcon("stop"), iconProvider.getIcon("stop-hovered"),
                                    iconProvider.getIcon("stop-pressed"), m_batchProcessingWidget.get());
  stopBtn->setIconSize({128, 128});
  stopBtn->setStatusTip(tr("Stop batch processing"));

  class LowerPanel : public QWidget {
   public:
    explicit LowerPanel(QWidget* parent = 0) : QWidget(parent) { ui.setupUi(this); }

    Ui::BatchProcessingLowerPanel ui;
  };


  auto* lowerPanel = new LowerPanel(m_batchProcessingWidget.get());
  m_checkBeepWhenFinished = [lowerPanel]() { return lowerPanel->ui.beepWhenFinished->isChecked(); };
  m_systemLoadWidget = lowerPanel->ui.systemLoadWidget;

  int row = 0;  // Row 0 is reserved.
  layout->addWidget(stopBtn, ++row, 1, Qt::AlignCenter);
  layout->addWidget(lowerPanel, ++row, 0, 1, 3, Qt::AlignHCenter | Qt::AlignTop);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(2, 1);
  layout->setRowStretch(0, 1);
  layout->setRowStretch(row, 1);

  connect(stopBtn, SIGNAL(clicked()), SLOT(stopBatchProcessing()));
}  // MainWindow::createBatchProcessingWidget

void MainWindow::startBatchProcessing() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  m_interactiveQueue->cancelAndClear();

  m_batchQueue = std::make_unique<ProcessingTaskQueue>();
  // Use full page sequence in current display order so all pages are processed
  // (fixes batch missing pages when e.g. order is "decreasing deviation" and
  // selection was not at the first page).
  const PageSequence sequence(m_thumbSequence->toPageSequence());
  for (const PageInfo& page : sequence) {
    for (int i = 0; i < m_stages->count(); i++) {
      m_stages->filterAt(i)->loadDefaultSettings(page);
    }
    m_batchQueue->addProcessingTask(page, createCompositeTask(page, m_curFilter, /*batch=*/true, m_debug));
  }

  // Cap the parallel pages by physical memory and the largest page before the first task is submitted.
  const int memoryCap = MemoryBudget::batchMemoryCap(sequence);
  m_workerThreadPool->setMemoryCap(memoryCap);
  m_systemLoadWidget->setMemoryCap(memoryCap);

  focusButton->setChecked(true);

  removeFilterOptionsWidget();
  filterList->setBatchProcessingInProgress(true);
  filterList->setEnabled(false);

  BackgroundTaskPtr task(m_batchQueue->takeForProcessing());
  if (task) {
    do {
      m_workerThreadPool->submitTask(task);
      if (!m_workerThreadPool->hasSpareCapacity()) {
        break;
      }
    } while ((task = m_batchQueue->takeForProcessing()));
  } else {
    stopBatchProcessing();
  }

  PageInfo selectedPage(m_batchQueue->selectedPage());
  if (!selectedPage.isNull()) {
    m_thumbSequence->setSelection(selectedPage.id());
  }
  // Display the batch processing screen.
  updateMainArea();
}  // MainWindow::startBatchProcessing

void MainWindow::stopBatchProcessing(MainAreaAction mainArea) {
  if (!isBatchProcessingInProgress()) {
    return;
  }

  const PageInfo page(m_batchQueue->selectedPage());
  if (!page.isNull()) {
    m_thumbSequence->setSelection(page.id());
  }

  m_batchQueue->cancelAndClear();
  m_batchQueue.reset();

  filterList->setBatchProcessingInProgress(false);
  filterList->setEnabled(true);

  switch (mainArea) {
    case UPDATE_MAIN_AREA:
      updateMainArea();
      break;
    case CLEAR_MAIN_AREA:
      removeImageWidget();
      break;
  }

  resetThumbSequence(currentPageOrderProvider());
}

void MainWindow::filterResult(const BackgroundTaskPtr& task, const FilterResultPtr& result) {
  // Cancelled or not, we must mark it as finished.
  m_interactiveQueue->processingFinished(task);
  if (m_batchQueue) {
    m_batchQueue->processingFinished(task);
  }

  if (task->isCancelled()) {
    return;
  }

  if (!isBatchProcessingInProgress()) {
    if (!result->filter()) {
      // Error loading file.  No special action is necessary.
    } else if (result->filter() != m_stages->filterAt(m_curFilter)) {
      // Error from one of the previous filters.
      const int idx = m_stages->findFilter(result->filter());
      assert(idx >= 0);
      m_curFilter = idx;

      ScopedIncDec<int> selectionGuard(m_ignoreSelectionChanges);
      filterList->selectRow(idx);
    }
  }

  // This needs to be done even if batch processing is taking place,
  // for instance because thumbnail invalidation is done from here.
  result->updateUI(this);

  if (isBatchProcessingInProgress()) {
    if (m_batchQueue->allProcessed()) {
      stopBatchProcessing();

      QApplication::alert(this);  // Request the user's attention (the Dock icon on macOS).
      if (m_checkBeepWhenFinished()) {
        // The system alert sound. No shell command is run; an alarm command stored by an earlier version is no longer
        // read and stays in the settings file.
        QApplication::beep();
      }

      if (m_selectedPage.get(getCurrentView()) == m_thumbSequence->lastPage().id()) {
        // If batch processing finished at the last page, jump to the first one.
        goFirstPage();
      }
      return;
    }

    do {
      const BackgroundTaskPtr task(m_batchQueue->takeForProcessing());
      if (!task) {
        break;
      }
      m_workerThreadPool->submitTask(task);
    } while (m_workerThreadPool->hasSpareCapacity());

    const PageInfo page(m_batchQueue->selectedPage());
    if (!page.isNull()) {
      m_thumbSequence->setSelection(page.id());
    }
  }
}  // MainWindow::filterResult

bool MainWindow::isBatchProcessingInProgress() const {
  return m_batchQueue != nullptr;
}

BackgroundTaskPtr MainWindow::createCompositeTask(const PageInfo& page,
                                                  const int lastFilterIdx,
                                                  const bool batch,
                                                  bool debug) {
  std::shared_ptr<fix_orientation::Task> fixOrientationTask;
  std::shared_ptr<page_split::Task> pageSplitTask;
  std::shared_ptr<deskew::Task> deskewTask;
  std::shared_ptr<select_content::Task> selectContentTask;
  std::shared_ptr<page_layout::Task> pageLayoutTask;
  std::shared_ptr<output::Task> outputTask;

  if (batch) {
    debug = false;
  }

  if (lastFilterIdx >= m_stages->outputFilterIdx()) {
    outputTask = m_stages->outputFilter()->createTask(page.id(), m_thumbnailCache, m_outFileNameGen, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->pageLayoutFilterIdx()) {
    pageLayoutTask = m_stages->pageLayoutFilter()->createTask(page.id(), outputTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->selectContentFilterIdx()) {
    selectContentTask = m_stages->selectContentFilter()->createTask(page.id(), pageLayoutTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->deskewFilterIdx()) {
    deskewTask = m_stages->deskewFilter()->createTask(page.id(), selectContentTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->pageSplitFilterIdx()) {
    pageSplitTask = m_stages->pageSplitFilter()->createTask(page, deskewTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->fixOrientationFilterIdx()) {
    fixOrientationTask = m_stages->fixOrientationFilter()->createTask(page.id(), pageSplitTask, batch);
    debug = false;
  }
  assert(fixOrientationTask);
  return std::make_shared<LoadFileTask>(batch ? BackgroundTask::BATCH : BackgroundTask::INTERACTIVE, page,
                                        m_thumbnailCache, m_pages, fixOrientationTask);
}  // MainWindow::createCompositeTask

std::shared_ptr<CompositeCacheDrivenTask> MainWindow::createCompositeCacheDrivenTask(const int lastFilterIdx) {
  std::shared_ptr<fix_orientation::CacheDrivenTask> fixOrientationTask;
  std::shared_ptr<page_split::CacheDrivenTask> pageSplitTask;
  std::shared_ptr<deskew::CacheDrivenTask> deskewTask;
  std::shared_ptr<select_content::CacheDrivenTask> selectContentTask;
  std::shared_ptr<page_layout::CacheDrivenTask> pageLayoutTask;
  std::shared_ptr<output::CacheDrivenTask> outputTask;

  if (lastFilterIdx >= m_stages->outputFilterIdx()) {
    outputTask = m_stages->outputFilter()->createCacheDrivenTask(m_outFileNameGen);
  }
  if (lastFilterIdx >= m_stages->pageLayoutFilterIdx()) {
    pageLayoutTask = m_stages->pageLayoutFilter()->createCacheDrivenTask(outputTask);
  }
  if (lastFilterIdx >= m_stages->selectContentFilterIdx()) {
    selectContentTask = m_stages->selectContentFilter()->createCacheDrivenTask(pageLayoutTask);
  }
  if (lastFilterIdx >= m_stages->deskewFilterIdx()) {
    deskewTask = m_stages->deskewFilter()->createCacheDrivenTask(selectContentTask);
  }
  if (lastFilterIdx >= m_stages->pageSplitFilterIdx()) {
    pageSplitTask = m_stages->pageSplitFilter()->createCacheDrivenTask(deskewTask);
  }
  if (lastFilterIdx >= m_stages->fixOrientationFilterIdx()) {
    fixOrientationTask = m_stages->fixOrientationFilter()->createCacheDrivenTask(pageSplitTask);
  }

  assert(fixOrientationTask);
  return fixOrientationTask;
}  // MainWindow::createCompositeCacheDrivenTask
