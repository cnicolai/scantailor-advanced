// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "MainWindow.h"


#include <QCursor>
#include <QStackedLayout>
#include <QtWidgets/QInputDialog>
#include <algorithm>
#include <cmath>
#include <memory>

#include "ErrorWidget.h"
#include "ImageViewBase.h"
#include "OutOfMemoryDialog.h"
#include "OutOfMemoryHandler.h"
#include "PageSequence.h"
#include "Utils.h"
#include "WorkerThreadPool.h"
#include "config.h"
#include "filters/output/TabbedImageView.h"
#include "version.h"

using namespace core;

void MainWindow::setupActions() {
  addAction(actionFirstPage);
  addAction(actionLastPage);
  addAction(actionNextPage);
  addAction(actionPrevPage);
  addAction(actionPrevPageQ);
  addAction(actionNextPageW);
  addAction(actionNextSelectedPage);
  addAction(actionPrevSelectedPage);
  addAction(actionNextSelectedPageW);
  addAction(actionPrevSelectedPageQ);
  addAction(actionGotoPage);
  addAction(actionMagnifyThumbnails);
  addAction(actionDiminishThumbnails);
  addAction(actionZoomIn);
  addAction(actionZoomOut);
  addAction(actionReloadPage);

  addAction(actionSwitchFilter1);
  addAction(actionSwitchFilter2);
  addAction(actionSwitchFilter3);
  addAction(actionSwitchFilter4);
  addAction(actionSwitchFilter5);
  addAction(actionSwitchFilter6);
  // Should be enough to save a project.
  OutOfMemoryHandler::instance().allocateEmergencyMemory(3 * 1024 * 1024);

  connect(actionFirstPage, SIGNAL(triggered(bool)), SLOT(goFirstPage()));
  connect(actionLastPage, SIGNAL(triggered(bool)), SLOT(goLastPage()));
  connect(actionPrevPage, SIGNAL(triggered(bool)), SLOT(goPrevPage()));
  connect(actionNextPage, SIGNAL(triggered(bool)), SLOT(goNextPage()));
  connect(actionPrevPageQ, SIGNAL(triggered(bool)), this, SLOT(goPrevPage()));
  connect(actionNextPageW, SIGNAL(triggered(bool)), this, SLOT(goNextPage()));
  connect(actionPrevSelectedPage, SIGNAL(triggered(bool)), SLOT(goPrevSelectedPage()));
  connect(actionNextSelectedPage, SIGNAL(triggered(bool)), SLOT(goNextSelectedPage()));
  connect(actionPrevSelectedPageQ, SIGNAL(triggered(bool)), this, SLOT(goPrevSelectedPage()));
  connect(actionNextSelectedPageW, SIGNAL(triggered(bool)), this, SLOT(goNextSelectedPage()));
  connect(actionGotoPage, SIGNAL(triggered(bool)), this, SLOT(execGotoPageDialog()));
  connect(actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAboutDialog()));
  connect(&OutOfMemoryHandler::instance(), SIGNAL(outOfMemory()), SLOT(handleOutOfMemorySituation()));
  connect(prevPageBtn, &QToolButton::clicked, this, [this]() {
    if (filterSelectedBtn->isChecked()) {
      goPrevSelectedPage();
    } else {
      goPrevPage();
    }
  });
  connect(nextPageBtn, &QToolButton::clicked, this, [this]() {
    if (filterSelectedBtn->isChecked()) {
      goNextSelectedPage();
    } else {
      goNextPage();
    }
  });
  connect(gotoPageBtn, SIGNAL(clicked()), this, SLOT(execGotoPageDialog()));

  auto magnifyThumbnails = [this]() { scaleThumbnails(1); };
  auto diminishThumbnails = [this]() { scaleThumbnails(-1); };
  connect(magnifyThumbnailsBtn, &QPushButton::clicked, magnifyThumbnails);
  connect(diminishThumbnailsBtn, &QPushButton::clicked, diminishThumbnails);
  connect(actionMagnifyThumbnails, &QAction::triggered, magnifyThumbnails);
  connect(actionDiminishThumbnails, &QAction::triggered, diminishThumbnails);
  connect(actionZoomIn, &QAction::triggered, this, &MainWindow::zoomCurrentViewIn);
  connect(actionZoomOut, &QAction::triggered, this, &MainWindow::zoomCurrentViewOut);
  connect(actionReloadPage, SIGNAL(triggered(bool)), SLOT(reloadCurrentPage()));

  connect(actionSwitchFilter1, SIGNAL(triggered(bool)), SLOT(switchFilter1()));
  connect(actionSwitchFilter2, SIGNAL(triggered(bool)), SLOT(switchFilter2()));
  connect(actionSwitchFilter3, SIGNAL(triggered(bool)), SLOT(switchFilter3()));
  connect(actionSwitchFilter4, SIGNAL(triggered(bool)), SLOT(switchFilter4()));
  connect(actionSwitchFilter5, SIGNAL(triggered(bool)), SLOT(switchFilter5()));
  connect(actionSwitchFilter6, SIGNAL(triggered(bool)), SLOT(switchFilter6()));

  connect(filterList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
          SLOT(filterSelectionChanged(const QItemSelection&)));
  connect(filterList, SIGNAL(launchBatchProcessing()), this, SLOT(startBatchProcessing()));

  connect(m_workerThreadPool.get(), SIGNAL(taskResult(const BackgroundTaskPtr&, const FilterResultPtr&)), this,
          SLOT(filterResult(const BackgroundTaskPtr&, const FilterResultPtr&)));

  connect(m_thumbSequence.get(),
          SIGNAL(newSelectionLeader(const PageInfo&, const QRectF&, ThumbnailSequence::SelectionFlags)), this,
          SLOT(currentPageChanged(const PageInfo&, const QRectF&, ThumbnailSequence::SelectionFlags)));
  connect(m_thumbSequence.get(), SIGNAL(pageContextMenuRequested(const PageInfo&, const QPoint&, bool)), this,
          SLOT(pageContextMenuRequested(const PageInfo&, const QPoint&, bool)));
  connect(m_thumbSequence.get(), SIGNAL(pastLastPageContextMenuRequested(const QPoint&)),
          SLOT(pastLastPageContextMenuRequested(const QPoint&)));
  connect(selectionModeBtn, SIGNAL(clicked(bool)), m_thumbSequence.get(), SLOT(setSelectionModeEnabled(bool)));

  connect(thumbView->verticalScrollBar(), SIGNAL(sliderMoved(int)), this, SLOT(thumbViewScrolled()));
  connect(thumbView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(thumbViewScrolled()));
  connect(focusButton, SIGNAL(clicked(bool)), this, SLOT(thumbViewFocusToggled(bool)));
  connect(sortOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(pageOrderingChanged(int)));
}

void MainWindow::goFirstPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo firstPage(m_thumbSequence->firstPage());
  if (!firstPage.isNull()) {
    goToPage(firstPage.id());
  }
}

void MainWindow::goLastPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo lastPage(m_thumbSequence->lastPage());
  if (!lastPage.isNull()) {
    goToPage(lastPage.id());
  }
}

void MainWindow::goNextPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo nextPage(m_thumbSequence->nextPage(m_thumbSequence->selectionLeader().id()));
  if (!nextPage.isNull()) {
    goToPage(nextPage.id());
  }
}

void MainWindow::goPrevPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo prevPage(m_thumbSequence->prevPage(m_thumbSequence->selectionLeader().id()));
  if (!prevPage.isNull()) {
    goToPage(prevPage.id());
  }
}

void MainWindow::goNextSelectedPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo nextSelectedPage(m_thumbSequence->nextSelectedPage(m_thumbSequence->selectionLeader().id()));
  if (!nextSelectedPage.isNull()) {
    goToPage(nextSelectedPage.id(), ThumbnailSequence::KEEP_SELECTION);
  }
}

void MainWindow::goPrevSelectedPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo prevSelectedPage(m_thumbSequence->prevSelectedPage(m_thumbSequence->selectionLeader().id()));
  if (!prevSelectedPage.isNull()) {
    goToPage(prevSelectedPage.id(), ThumbnailSequence::KEEP_SELECTION);
  }
}

void MainWindow::switchFilter1() {
  filterList->selectRow(0);
}

void MainWindow::switchFilter2() {
  filterList->selectRow(1);
}

void MainWindow::switchFilter3() {
  filterList->selectRow(2);
}

void MainWindow::switchFilter4() {
  filterList->selectRow(3);
}

void MainWindow::switchFilter5() {
  filterList->selectRow(4);
}

void MainWindow::switchFilter6() {
  filterList->selectRow(5);
}

void MainWindow::updateProjectActions() {
  const bool loaded = isProjectLoaded();
  actionSaveProject->setEnabled(loaded);
  actionSaveProjectAs->setEnabled(loaded);
  actionCloseProject->setEnabled(loaded);
  actionFixDpi->setEnabled(loaded);
  actionRelinking->setEnabled(loaded);
  actionReverseTwoPageOrder->setEnabled(loaded);
  // With no project there is no stage to switch to; the letter keys of the stage actions stay bound.
  for (QAction* const action : {actionSwitchFilter1, actionSwitchFilter2, actionSwitchFilter3, actionSwitchFilter4,
                                actionSwitchFilter5, actionSwitchFilter6}) {
    action->setEnabled(loaded);
  }
}

void MainWindow::setupMacKeys() {
  // The first sequence is the one a menu draws: the Command digit. The letter keeps the old key (D-01, D-03).
  actionSwitchFilter1->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_1), QKeySequence(Qt::Key_O)});
  actionSwitchFilter2->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_2), QKeySequence(Qt::Key_E)});
  actionSwitchFilter3->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_3), QKeySequence(Qt::Key_N)});
  actionSwitchFilter4->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_4), QKeySequence(Qt::Key_V)});
  actionSwitchFilter5->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_5), QKeySequence(Qt::Key_K)});
  actionSwitchFilter6->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_6), QKeySequence(Qt::Key_P)});

  // Command-plus is drawn in the menu (the key a German keyboard has); Command-equals serves a US layout (D-06).
  actionZoomIn->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Plus), QKeySequence(Qt::CTRL | Qt::Key_Equal)});
  actionZoomOut->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Minus)});

  // The thumbnail keys sit on the Option-Command group measured on the German layout (D-07).
  actionMagnifyThumbnails->setShortcuts(
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Plus), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Equal)});
  actionDiminishThumbnails->setShortcuts({QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Minus)});
}

void MainWindow::updateZoomActions() {
  // The view of the Output stage changes with its tabs without any layout signal, so follow the tab widget. The
  // connection dies with the tab widget; the unique flag keeps repeated calls from stacking it.
  if (m_imageFrameLayout->count() > 0) {
    if (auto* const tabs = Utils::castOrFindChild<output::TabbedImageView*>(m_imageFrameLayout->widget(0))) {
      connect(tabs, &output::TabbedImageView::primaryViewChanged, this, &MainWindow::updateZoomActions,
              Qt::UniqueConnection);
    }
  }
  const bool hasView = currentPageView() != nullptr;
  actionZoomIn->setEnabled(hasView);
  actionZoomOut->setEnabled(hasView);
}

void MainWindow::zoomCurrentViewIn() {
  zoomCurrentView(std::pow(2.0, 1.0 / 6));
}

void MainWindow::zoomCurrentViewOut() {
  zoomCurrentView(std::pow(2.0, -1.0 / 6));
}

void MainWindow::zoomCurrentView(const double factor) {
  ImageViewBase* const view = currentPageView();
  if (view == nullptr || view->interactionState().captured()) {
    return;
  }
  const double zoom = std::max(1.0, view->zoomLevel() * factor);

  // The zoom focus is the cursor if it is over the view, else the view centre. The factor per step matches the
  // plain plus and minus keys of ZoomHandler. Unlike ZoomHandler::zoom(), this does not move the image towards its
  // ideal position at zoom 1.0, and it only checks that no interaction holds the view.
  const QPoint local = view->mapFromGlobal(QCursor::pos());
  const QPointF focus = view->rect().contains(local) ? QPointF(local) : QRectF(view->rect()).center();
  view->setWidgetFocalPointWithoutMoving(focus);
  view->setZoomLevel(zoom);
}

void MainWindow::execGotoPageDialog() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  bool ok;
  const PageSequence pageSequence = m_thumbSequence->toPageSequence();
  const PageId& selectionLeader = m_thumbSequence->selectionLeader().id();
  int pageNumber
      = QInputDialog::getInt(this, tr("Go To Page"), tr("Enter the page number:"),
                             pageSequence.pageNo(selectionLeader) + 1, 1, (int) (pageSequence.numPages()), 1, &ok);
  if (ok) {
    const PageId& newSelectionLeader = pageSequence.pageAt(pageNumber - 1).id();
    if (selectionLeader != newSelectionLeader) {
      goToPage(newSelectionLeader);
    }
  }
}
