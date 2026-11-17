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

class MainWindow::PageSelectionProviderImpl : public PageSelectionProvider {
 public:
  explicit PageSelectionProviderImpl(MainWindow* wnd) : m_wnd(wnd) {}

  PageSequence allPages() const override { return m_wnd ? m_wnd->allPages() : PageSequence(); }

  std::set<PageId> selectedPages() const override { return m_wnd ? m_wnd->selectedPages() : std::set<PageId>(); }

  std::vector<PageRange> selectedRanges() const override {
    return m_wnd ? m_wnd->selectedRanges() : std::vector<PageRange>();
  }

 private:
  QPointer<MainWindow> m_wnd;
};


MainWindow::MainWindow()
    : m_pages(std::make_shared<ProjectPages>()),
      m_stages(std::make_shared<StageSequence>(m_pages, newPageSelectionAccessor())),
      m_workerThreadPool(std::make_unique<WorkerThreadPool>()),
      m_interactiveQueue(std::make_unique<ProcessingTaskQueue>()),
      m_outOfMemoryDialog(std::make_unique<OutOfMemoryDialog>()),
      m_curFilter(0),
      m_savedMainAreaViewState(),
      m_ignoreSelectionChanges(0),
      m_ignorePageOrderingChanges(0),
      m_debug(false),
      m_closing(false) {
  ApplicationSettings& settings = ApplicationSettings::getInstance();

  m_maxLogicalThumbSize = settings.getMaxLogicalThumbnailSize();
  const ThumbnailSequence::ViewMode viewMode = settings.isSingleColumnThumbnailDisplayEnabled()
                                                   ? ThumbnailSequence::SINGLE_COLUMN
                                                   : ThumbnailSequence::MULTI_COLUMN;
  m_thumbSequence = std::make_unique<ThumbnailSequence>(m_maxLogicalThumbSize, viewMode);

  m_autoSaveTimer.setSingleShot(true);
  connect(&m_autoSaveTimer, SIGNAL(timeout()), SLOT(autoSaveProject()));

  setupUi(this);
  setupMacKeys();
  setupIcons();

  sortOptions->setVisible(false);

  createBatchProcessingWidget();
  m_processingIndicationWidget = std::make_unique<ProcessingIndicationWidget>();

  filterList->setStages(m_stages);
  filterList->selectRow(0);

  setupThumbView();  // Expects m_thumbSequence to be initialized.
  m_tabbedDebugImages = std::make_unique<TabbedDebugImages>();

  m_debug = actionDebug->isChecked();

  m_imageFrameLayout = new QStackedLayout(imageViewFrame);
  m_imageFrameLayout->setStackingMode(QStackedLayout::StackAll);
  // Both signals fire when a widget enters or leaves the main area, including when a test or removeWidgetsFromLayout()
  // does it directly.
  connect(m_imageFrameLayout, &QStackedLayout::currentChanged, this, &MainWindow::updateZoomActions);
  connect(m_imageFrameLayout, &QStackedLayout::widgetRemoved, this, &MainWindow::updateZoomActions);

  m_optionsFrameLayout = new QStackedLayout(filterOptions);

  m_statusBarPanel = new StatusBarPanel;
  QMainWindow::statusBar()->addPermanentWidget(m_statusBarPanel);
  connect(m_thumbSequence.get(), &ThumbnailSequence::newSelectionLeader, [this](const PageInfo& pageInfo) {
    PageSequence pageSequence = m_thumbSequence->toPageSequence();
    if (pageSequence.numPages() > 0) {
      m_statusBarPanel->updatePage(pageSequence.pageNo(pageInfo.id()) + 1, pageSequence.numPages(), pageInfo.id());
    } else {
      m_statusBarPanel->clear();
    }
  });

  m_unitsMenuActionGroup = new QActionGroup(this);
  for (QAction* action : menuUnits->actions()) {
    m_unitsMenuActionGroup->addAction(action);
  }
  switch (unitsFromString(settings.getUnits())) {
    case PIXELS:
      actionPixels->setChecked(true);
      break;
    case MILLIMETRES:
      actionMilimeters->setChecked(true);
      break;
    case CENTIMETRES:
      actionCentimetres->setChecked(true);
      break;
    case INCHES:
      actionInches->setChecked(true);
      break;
  }
  connect(actionPixels, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(PIXELS);
      settings.setUnits(unitsToString(PIXELS));
    }
  });
  connect(actionMilimeters, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(MILLIMETRES);
      settings.setUnits(unitsToString(MILLIMETRES));
    }
  });
  connect(actionCentimetres, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(CENTIMETRES);
      settings.setUnits(unitsToString(CENTIMETRES));
    }
  });
  connect(actionInches, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(INCHES);
      settings.setUnits(unitsToString(INCHES));
    }
  });

  thumbColumnViewBtn->setChecked(settings.isSingleColumnThumbnailDisplayEnabled());
  deviationHighlightingBtn->setChecked(settings.isHighlightDeviationEnabled());

  setupActions();
  connect(thumbColumnViewBtn, &QToolButton::clicked, this, [this, &settings](bool checked) {
    settings.setSingleColumnThumbnailDisplayEnabled(checked);
    updateThumbnailViewMode();
    m_thumbSequence->updateSceneItemsPos();
  });
  connect(sortingOrderBtn, &QToolButton::clicked, this,
          [this](bool) { pageOrderingChanged(m_stages->filterAt(m_curFilter)->selectedPageOrder()); });
  connect(pauseSortingBtn, &QToolButton::toggled, this, [this](bool) {
    if (!isProjectLoaded()) {
      return;
    }
    resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);
  });
  connect(deviationHighlightingBtn, &QToolButton::clicked, this, [this, &settings](bool checked) {
    settings.setHighlightDeviationEnabled(checked);
    m_thumbSequence->invalidateAllThumbnails();
  });

  connect(actionFixDpi, SIGNAL(triggered(bool)), SLOT(fixDpiDialogRequested()));
  connect(actionReverseTwoPageOrder, SIGNAL(triggered(bool)), SLOT(toggleTwoPageSpreadReadingOrder()));
  connect(actionRelinking, SIGNAL(triggered(bool)), SLOT(showRelinkingDialog()));
#ifdef ENABLE_DEBUG_FEATURES
  // The separator exists only while Debug Mode is visible, so Tools never ends on a separator.
  menuDebug->insertSeparator(actionDebug);
  connect(actionDebug, SIGNAL(toggled(bool)), SLOT(debugToggled(bool)));
#else
  actionDebug->setVisible(false);
#endif

  connect(actionSettings, SIGNAL(triggered(bool)), this, SLOT(openSettingsDialog()));
  connect(actionDefaults, SIGNAL(triggered(bool)), this, SLOT(openDefaultParamsDialog()));

  connect(actionNewProject, SIGNAL(triggered(bool)), this, SLOT(newProject()));
  connect(actionOpenProject, SIGNAL(triggered(bool)), this, SLOT(openProject()));
  connect(actionSaveProject, SIGNAL(triggered(bool)), this, SLOT(saveProjectTriggered()));
  connect(actionSaveProjectAs, SIGNAL(triggered(bool)), this, SLOT(saveProjectAsTriggered()));
  connect(actionCloseProject, SIGNAL(triggered(bool)), this, SLOT(closeProject()));
  connect(actionQuit, SIGNAL(triggered(bool)), this, SLOT(close()));

  updateProjectActions();
  updateWindowTitle();
  updateMainArea();
  updateZoomActions();

  QSettings qSettings;
  if (qSettings.value("mainWindow/maximized") == false) {
    const QVariant geom(qSettings.value("mainWindow/nonMaximizedGeometry"));
    if (!restoreGeometry(geom.toByteArray())) {
      resize(1014, 689);  // A sensible value.
    }
  }

  m_maxLogicalThumbSizeUpdater.setSingleShot(true);
  connect(&m_maxLogicalThumbSizeUpdater, &QTimer::timeout, this, &MainWindow::updateMaxLogicalThumbSize);

  m_sceneItemsPosUpdater.setSingleShot(true);
  connect(&m_sceneItemsPosUpdater, &QTimer::timeout, m_thumbSequence.get(), &ThumbnailSequence::updateSceneItemsPos);
}

MainWindow::~MainWindow() {
  m_interactiveQueue->cancelAndClear();
  if (m_batchQueue) {
    m_batchQueue->cancelAndClear();
  }
  m_workerThreadPool->shutdown();

  removeWidgetsFromLayout(m_imageFrameLayout);
  removeWidgetsFromLayout(m_optionsFrameLayout);
  m_tabbedDebugImages->clear();
}

PageSequence MainWindow::allPages() const {
  return m_thumbSequence->toPageSequence();
}

std::set<PageId> MainWindow::selectedPages() const {
  return m_thumbSequence->selectedItems();
}

std::vector<PageRange> MainWindow::selectedRanges() const {
  return m_thumbSequence->selectedRanges();
}

void MainWindow::updateThumbViewMinWidth() {
  const int sb = thumbView->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  int innerWidth = thumbView->maximumViewportSize().width() - sb;
  if (thumbView->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, nullptr, thumbView)) {
    innerWidth -= thumbView->frameWidth() * 2;
  }
  const int deltaX = thumbView->size().width() - innerWidth;
  thumbView->setMinimumWidth((int) std::ceil(m_maxLogicalThumbSize.width() + deltaX));
}

void MainWindow::setupThumbView() {
  updateThumbViewMinWidth();
  m_thumbSequence->attachView(thumbView);
  thumbView->installEventFilter(this);
  m_thumbDprTracker = new ThumbnailDprTracker(thumbView);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
  if ((obj == thumbView) && (ev->type() == QEvent::Resize)) {
    if (!m_sceneItemsPosUpdater.isActive()) {
      m_sceneItemsPosUpdater.start(150);
    }
  }

  if ((obj == thumbView || obj == thumbView->verticalScrollBar()) && (ev->type() == QEvent::Wheel)) {
    auto* wheelEvent = static_cast<QWheelEvent*>(ev);
    if (wheelEvent->modifiers() == Qt::AltModifier) {
      const QPoint& angleDelta = wheelEvent->angleDelta();
      const int wheelDist = angleDelta.x() + angleDelta.y();
      if (std::abs(wheelDist) >= 30) {
        scaleThumbnails(std::copysign(1, wheelDist));
      }
      wheelEvent->accept();
      return true;
    }
  }
  return false;
}

void MainWindow::closeEvent(QCloseEvent* const event) {
  if (m_closing) {
    event->accept();
  } else {
    event->ignore();
    startTimer(0);
  }
}

void MainWindow::timerEvent(QTimerEvent* const event) {
  // We only use the timer event for delayed closing of the window.
  killTimer(event->timerId());

  if (closeProjectInteractive()) {
    m_closing = true;
    QSettings settings;
    settings.setValue("mainWindow/maximized", isMaximized());
    if (!isMaximized()) {
      settings.setValue("mainWindow/nonMaximizedGeometry", saveGeometry());
    }
    close();
  }
}

bool MainWindow::compareFiles(const QString& fpath1, const QString& fpath2) {
  QFile file1(fpath1);
  QFile file2(fpath2);

  if (!file1.open(QIODevice::ReadOnly)) {
    return false;
  }
  if (!file2.open(QIODevice::ReadOnly)) {
    return false;
  }

  if (!file1.isSequential() && !file2.isSequential()) {
    if (file1.size() != file2.size()) {
      return false;
    }
  }

  const int chunkSize = 4096;
  while (true) {
    const QByteArray chunk1(file1.read(chunkSize));
    const QByteArray chunk2(file2.read(chunkSize));
    if (chunk1.size() != chunk2.size()) {
      return false;
    } else if (chunk1.size() == 0) {
      return true;
    }
  }
}

std::shared_ptr<const PageOrderProvider> MainWindow::currentPageOrderProvider() const {
  if (pauseSortingBtn->isChecked()) {
    return nullptr;
  }

  const int idx = sortOptions->currentIndex();
  if (idx < 0) {
    return nullptr;
  }

  const std::shared_ptr<AbstractFilter> filter(m_stages->filterAt(m_curFilter));
  std::shared_ptr<const PageOrderProvider> currentOrderProvider = filter->pageOrderOptions()[idx].provider();
  return (currentOrderProvider && sortingOrderBtn->isChecked()) ? currentOrderProvider->reversed()
                                                                : currentOrderProvider;
}

void MainWindow::updateSortOptions() {
  const ScopedIncDec<int> guard(m_ignorePageOrderingChanges);

  const std::shared_ptr<AbstractFilter> filter(m_stages->filterAt(m_curFilter));

  sortOptions->clear();

  for (const PageOrderOption& opt : filter->pageOrderOptions()) {
    sortOptions->addItem(opt.name());
  }

  sortOptions->setVisible(sortOptions->count() > 0);

  pauseSortingBtn->setEnabled(sortOptions->count() > 0);
  if (sortOptions->count() == 0) {
    pauseSortingBtn->setChecked(false);
  }

  if (sortOptions->count() > 0) {
    sortOptions->setCurrentIndex(filter->selectedPageOrder());
  }
}

void MainWindow::resetThumbSequence(const std::shared_ptr<const PageOrderProvider>& pageOrderProvider,
                                    const ThumbnailSequence::SelectionAction selectionAction) {
  if (m_thumbnailCache) {
    const std::shared_ptr<CompositeCacheDrivenTask> task(createCompositeCacheDrivenTask(m_curFilter));

    m_thumbSequence->setThumbnailFactory(
        std::make_shared<ThumbnailFactory>(m_thumbnailCache, m_maxLogicalThumbSize, task));
  }

  m_thumbSequence->reset(currentPageSequence(), selectionAction, pageOrderProvider);

  if (!m_thumbnailCache) {
    // Empty project.
    assert(m_pages->numImages() == 0);
    m_thumbSequence->setThumbnailFactory(nullptr);
  }

  if (selectionAction != ThumbnailSequence::KEEP_SELECTION) {
    const PageId page(m_selectedPage.get(getCurrentView()));
    if (m_thumbSequence->setSelection(page)) {
      // OK
    } else if (m_thumbSequence->setSelection(PageId(page.imageId(), PageId::LEFT_PAGE))) {
      // OK
    } else if (m_thumbSequence->setSelection(PageId(page.imageId(), PageId::RIGHT_PAGE))) {
      // OK
    } else if (m_thumbSequence->setSelection(PageId(page.imageId(), PageId::SINGLE_PAGE))) {
      // OK
    } else {
      // Last resort.
      m_thumbSequence->setSelection(m_thumbSequence->firstPage().id());
    }
  }
}

void MainWindow::setOptionsWidget(FilterOptionsWidget* widget, const Ownership ownership) {
  if (isBatchProcessingInProgress()) {
    if (ownership == TRANSFER_OWNERSHIP) {
      delete widget;
    }
    return;
  }

  if (m_optionsWidget != widget) {
    removeWidgetsFromLayout(m_optionsFrameLayout);
  }
  // Delete the old widget we were owning, if any.
  m_optionsWidgetCleanup.clear();

  if (ownership == TRANSFER_OWNERSHIP) {
    m_optionsWidgetCleanup.add(widget);
  }

  if (m_optionsWidget == widget) {
    return;
  }

  if (m_optionsWidget) {
    disconnect(m_optionsWidget, SIGNAL(reloadRequested()), this, SLOT(reloadRequested()));
    disconnect(m_optionsWidget, SIGNAL(invalidateThumbnail(const PageId&)), this,
               SLOT(invalidateThumbnail(const PageId&)));
    disconnect(m_optionsWidget, SIGNAL(invalidateThumbnail(const PageInfo&)), this,
               SLOT(invalidateThumbnail(const PageInfo&)));
    disconnect(m_optionsWidget, SIGNAL(invalidateAllThumbnails()), this, SLOT(invalidateAllThumbnails()));
    disconnect(m_optionsWidget, SIGNAL(goToPage(const PageId&)), this, SLOT(goToPage(const PageId&)));
    disconnect(m_optionsWidget, SIGNAL(fixDpiRequested()), this, SLOT(fixDpiDialogRequested()));
  }

  m_optionsFrameLayout->addWidget(widget);
  m_optionsWidget = widget;

  // We use an asynchronous connection here, because the slot
  // will probably delete the options panel, which could be
  // responsible for the emission of this signal.  Qt doesn't
  // like when we delete an object while it's emitting a signal.
  connect(widget, SIGNAL(reloadRequested()), this, SLOT(reloadRequested()), Qt::QueuedConnection);
  connect(widget, SIGNAL(invalidateThumbnail(const PageId&)), this, SLOT(invalidateThumbnail(const PageId&)));
  connect(widget, SIGNAL(invalidateThumbnail(const PageInfo&)), this, SLOT(invalidateThumbnail(const PageInfo&)));
  connect(widget, SIGNAL(invalidateAllThumbnails()), this, SLOT(invalidateAllThumbnails()));
  connect(widget, SIGNAL(goToPage(const PageId&)), this, SLOT(goToPage(const PageId&)));
  connect(widget, SIGNAL(fixDpiRequested()), this, SLOT(fixDpiDialogRequested()));
}  // MainWindow::setOptionsWidget

ImageViewBase* MainWindow::findPrimaryImageView(QWidget* root) {
  if (!root) {
    return nullptr;
  }
  if (auto* tabs = qobject_cast<output::TabbedImageView*>(root)) {
    QWidget* const page = tabs->currentWidget();
    if (page) {
      return Utils::castOrFindChild<ImageViewBase*>(page);
    }
    return nullptr;
  }
  return Utils::castOrFindChild<ImageViewBase*>(root);
}

void MainWindow::applySavedMainAreaViewState(ImageViewBase* view, const SavedMainAreaViewState& state) {
  if (!view) {
    return;
  }
  view->setZoomLevel(state.zoom);
  if (!state.hasScrollNorm) {
    return;
  }
  QScrollBar* const h = view->horizontalScrollBar();
  QScrollBar* const v = view->verticalScrollBar();
  const int hr = h->maximum() - h->minimum();
  const int vr = v->maximum() - v->minimum();
  if (hr <= 0 || vr <= 0) {
    return;
  }
  const double nx = qBound(0.0, state.scrollNormX, 1.0);
  const double ny = qBound(0.0, state.scrollNormY, 1.0);
  h->setValue(h->minimum() + qRound(nx * hr));
  v->setValue(v->minimum() + qRound(ny * vr));
}

void MainWindow::scheduleSavedMainAreaViewStateRestore(const QPointer<ImageViewBase>& view) {
  if (view.isNull()) {
    return;
  }
  const SavedMainAreaViewState state = m_savedMainAreaViewState;
  QTimer::singleShot(0, this, [view, state]() {
    if (view.isNull()) {
      return;
    }
    MainWindow::applySavedMainAreaViewState(view.data(), state);
  });
}

void MainWindow::setImageWidget(QWidget* widget, const Ownership ownership, DebugImages* debugImages, bool overlay) {
  if (isBatchProcessingInProgress() && (widget != m_batchProcessingWidget.get())) {
    if (ownership == TRANSFER_OWNERSHIP) {
      delete widget;
    }
    return;
  }

  if (!overlay && m_imageFrameLayout->count() > 0) {
    QWidget* oldW = m_imageFrameLayout->widget(0);
    if (ImageViewBase* oldView = findPrimaryImageView(oldW)) {
      m_savedMainAreaViewState.zoom = oldView->zoomLevel();
      QScrollBar* const h = oldView->horizontalScrollBar();
      QScrollBar* const v = oldView->verticalScrollBar();
      const int hr = h->maximum() - h->minimum();
      const int vr = v->maximum() - v->minimum();
      if (hr > 0 && vr > 0) {
        m_savedMainAreaViewState.hasScrollNorm = true;
        m_savedMainAreaViewState.scrollNormX = double(h->value() - h->minimum()) / hr;
        m_savedMainAreaViewState.scrollNormY = double(v->value() - v->minimum()) / vr;
      } else {
        m_savedMainAreaViewState.hasScrollNorm = false;
      }
    }
  }

  if (!overlay) {
    removeImageWidget();
  }

  if (ownership == TRANSFER_OWNERSHIP) {
    m_imageWidgetCleanup.add(widget);
  }

  if (!debugImages || debugImages->empty()) {
    if (widget != m_imageFrameLayout->currentWidget()) {
      m_imageFrameLayout->addWidget(widget);
      if (overlay) {
        m_imageFrameLayout->setCurrentWidget(widget);
      }
      if (ImageViewBase* newView = findPrimaryImageView(widget)) {
        scheduleSavedMainAreaViewStateRestore(QPointer<ImageViewBase>(newView));
      }
    }
  } else {
    m_tabbedDebugImages->addTab(widget, "Main");
    AutoRemovingFile file;
    QString label;
    while (!(file = debugImages->retrieveNext(&label)).get().isNull()) {
      QWidget* view = new DebugImageView(file);
      m_imageWidgetCleanup.add(view);
      m_tabbedDebugImages->addTab(view, label);
    }
    m_imageFrameLayout->addWidget(m_tabbedDebugImages.get());
  }
}

void MainWindow::removeImageWidget() {
  removeWidgetsFromLayout(m_imageFrameLayout);

  m_tabbedDebugImages->clear();
  // Delete the old widget we were owning, if any.
  m_imageWidgetCleanup.clear();
}

void MainWindow::invalidateThumbnail(const PageId& pageId) {
  m_thumbSequence->invalidateThumbnail(pageId);
}

void MainWindow::invalidateThumbnail(const PageInfo& pageInfo) {
  m_thumbSequence->invalidateThumbnail(pageInfo);
}

void MainWindow::invalidateAllThumbnails() {
  m_thumbSequence->invalidateAllThumbnails();
}

std::shared_ptr<AbstractCommand<void>> MainWindow::relinkingDialogRequester() {
  class Requester : public AbstractCommand<void> {
   public:
    explicit Requester(MainWindow* wnd) : m_wnd(wnd) {}

    virtual void operator()() {
      if (!m_wnd.isNull()) {
        m_wnd->showRelinkingDialog();
      }
    }

   private:
    QPointer<MainWindow> m_wnd;
  };
  return std::make_shared<Requester>(this);
}

void MainWindow::showRelinkingDialog() {
  if (!isProjectLoaded()) {
    return;
  }

  auto* dialog = new RelinkingDialog(m_projectFile, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);

  m_pages->listRelinkablePaths(dialog->pathCollector());
  dialog->pathCollector()(RelinkablePath(m_outFileNameGen.outDir(), RelinkablePath::Dir));

  connect(dialog, &QDialog::accepted, [this, dialog]() { this->performRelinking(dialog->relinker()); });

  dialog->show();
}

void MainWindow::performRelinking(const std::shared_ptr<AbstractRelinker>& relinker) {
  assert(relinker);

  if (!isProjectLoaded()) {
    return;
  }

  m_pages->performRelinking(*relinker);
  m_stages->performRelinking(*relinker);
  m_outFileNameGen.performRelinking(*relinker);

  Utils::maybeCreateCacheDir(m_outFileNameGen.outDir());

  m_thumbnailCache->setThumbDir(Utils::outputDirToThumbDir(m_outFileNameGen.outDir()));
  resetThumbSequence(currentPageOrderProvider());
  m_selectedPage.set(m_thumbSequence->selectionLeader().id(), getCurrentView());

  reloadRequested();
}

void MainWindow::goToPage(const PageId& pageId, const ThumbnailSequence::SelectionAction selectionAction) {
  // Do not toggle Follow page here (issue #51): keyboard navigation used to force it on while mouse
  // selection did not. Batch processing and similar flows set focus explicitly where needed.

  m_thumbSequence->setSelection(pageId, selectionAction);

  // If the page was already selected, it will be reloaded.
  // That's by design.
  updateMainArea();
}

void MainWindow::currentPageChanged(const PageInfo& pageInfo,
                                    const QRectF& thumbRect,
                                    const ThumbnailSequence::SelectionFlags flags) {
  m_selectedPage.set(pageInfo.id(), getCurrentView());

  if ((flags & ThumbnailSequence::SELECTED_BY_USER) || focusButton->isChecked()) {
    if (!(flags & ThumbnailSequence::AVOID_SCROLLING_TO)) {
      const int scrollMargin = 20;  // Keep selected thumbnail away from edge (issue #51).
      thumbView->ensureVisible(thumbRect, scrollMargin, scrollMargin);
    }
  }

  if (flags & ThumbnailSequence::SELECTED_BY_USER) {
    if (isBatchProcessingInProgress()) {
      stopBatchProcessing();
    } else if (!(flags & ThumbnailSequence::REDUNDANT_SELECTION)) {
      // Start loading / processing the newly selected page.
      updateMainArea();
    }
  }

  if ((flags & ThumbnailSequence::SELECTION_CLEARED) && selectionModeBtn->isChecked()) {
    selectionModeBtn->setChecked(false);
  }

  updateAutoSaveTimer();
}

void MainWindow::pageContextMenuRequested(const PageInfo& pageInfo_, const QPoint& screenPos, bool selected) {
  if (isBatchProcessingInProgress()) {
    return;
  }
  // Make a copy to prevent it from being invalidated.
  const PageInfo pageInfo = pageInfo_;

  if (!selected) {
    goToPage(pageInfo.id());
  }

  // Parent widget helps correct multi-monitor placement (issue #75).
  QMenu menu(thumbView);

  auto& iconProvider = IconProvider::getInstance();
  QAction* insBefore = menu.addAction(iconProvider.getIcon("insert-before"), tr("Insert before ..."));
  QAction* insAfter = menu.addAction(iconProvider.getIcon("insert-after"), tr("Insert after ..."));

  menu.addSeparator();

  QAction* remove = menu.addAction(iconProvider.getIcon("user-trash"), tr("Remove from project ..."));

  QAction* action = menu.exec(screenPos);
  if (action == insBefore) {
    showInsertFileDialog(BEFORE, pageInfo.imageId());
  } else if (action == insAfter) {
    showInsertFileDialog(AFTER, pageInfo.imageId());
  } else if (action == remove) {
    showRemovePagesDialog(m_thumbSequence->selectedItems());
  }
}  // MainWindow::pageContextMenuRequested

void MainWindow::toggleTwoPageSpreadReadingOrder() {
  if (!isProjectLoaded() || !m_pages) {
    return;
  }
  const Qt::LayoutDirection nextDir
      = (m_pages->layoutDirection() == Qt::LeftToRight) ? Qt::RightToLeft : Qt::LeftToRight;
  m_pages->setLayoutDirection(nextDir);
  m_outFileNameGen.setLayoutDirection(nextDir);
  resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);
  invalidateAllThumbnails();
}

void MainWindow::pastLastPageContextMenuRequested(const QPoint& screenPos) {
  if (!isProjectLoaded()) {
    return;
  }

  QMenu menu(thumbView);
  menu.addAction(IconProvider::getInstance().getIcon("insert-here"), tr("Insert here ..."));

  if (menu.exec(screenPos)) {
    showInsertFileDialog(BEFORE, ImageId());
  }
}

void MainWindow::thumbViewFocusToggled(const bool checked) {
  const QRectF rect(m_thumbSequence->selectionLeaderSceneRect());
  if (rect.isNull()) {
    // No selected items.
    return;
  }

  if (checked) {
    const int scrollMargin = 20;  // Follow Page: keep thumbnail visible with padding (issue #51).
    thumbView->ensureVisible(rect, scrollMargin, scrollMargin);
  }
}

void MainWindow::thumbViewScrolled() {
  const QRectF rect(m_thumbSequence->selectionLeaderSceneRect());
  if (rect.isNull()) {
    // No items selected.
    return;
  }

  const QRectF viewportRect(thumbView->viewport()->rect());
  const QRectF viewportItemRect(thumbView->viewportTransform().mapRect(rect));

  const double intersectionThreshold = 0.5;
  if ((viewportItemRect.top() >= viewportRect.top())
      && (viewportItemRect.top() + viewportItemRect.height() * intersectionThreshold <= viewportRect.bottom())) {
    // Item is visible.
  } else if ((viewportItemRect.bottom() <= viewportRect.bottom())
             && (viewportItemRect.bottom() - viewportItemRect.height() * intersectionThreshold >= viewportRect.top())) {
    // Item is visible.
  } else {
    focusButton->setChecked(false);
  }
}

void MainWindow::filterSelectionChanged(const QItemSelection& selected) {
  if (m_ignoreSelectionChanges) {
    return;
  }

  if (selected.empty()) {
    return;
  }

  m_interactiveQueue->cancelAndClear();
  if (m_batchQueue) {
    // Should not happen, but just in case.
    m_batchQueue->cancelAndClear();
  }

  const bool wasBelowFixOrientation = isBelowFixOrientation(m_curFilter);
  const bool wasBelowSelectContent = isBelowSelectContent(m_curFilter);
  m_curFilter = selected.front().top();
  const bool nowBelowFixOrientation = isBelowFixOrientation(m_curFilter);
  const bool nowBelowSelectContent = isBelowSelectContent(m_curFilter);

  m_stages->filterAt(m_curFilter)->selected();

  updateSortOptions();

  // Propagate context boxes down the stage list, if necessary.
  if (!wasBelowSelectContent && nowBelowSelectContent) {
    // IMPORTANT: this needs to go before resetting thumbnails,
    // because it may affect them.
    if (m_contentBoxPropagator) {
      m_contentBoxPropagator->propagate(*m_pages);
    }  // Otherwise probably no project is loaded.
  }
  // Propagate page orientations (that might have changed) to the "Split Pages" stage.
  if (!wasBelowFixOrientation && nowBelowFixOrientation) {
    // IMPORTANT: this needs to go before resetting thumbnails,
    // because it may affect them.
    if (m_pageOrientationPropagator) {
      m_pageOrientationPropagator->propagate(*m_pages);
    }  // Otherwise probably no project is loaded.
  }

  const int horScrollBarPos = thumbView->horizontalScrollBar()->value();
  const int verScrollBarPos = thumbView->verticalScrollBar()->value();

  resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);

  if (!focusButton->isChecked()) {
    thumbView->horizontalScrollBar()->setValue(horScrollBarPos);
    thumbView->verticalScrollBar()->setValue(verScrollBarPos);
  }

  updateMainArea();
}  // MainWindow::filterSelectionChanged

void MainWindow::pageOrderingChanged(int idx) {
  if (m_ignorePageOrderingChanges) {
    return;
  }

  const int horScrollBarPos = thumbView->horizontalScrollBar()->value();
  const int verScrollBarPos = thumbView->verticalScrollBar()->value();

  m_stages->filterAt(m_curFilter)->selectPageOrder(idx);

  m_thumbSequence->reset(currentPageSequence(), ThumbnailSequence::KEEP_SELECTION, currentPageOrderProvider());

  if (!focusButton->isChecked()) {
    thumbView->horizontalScrollBar()->setValue(horScrollBarPos);
    thumbView->verticalScrollBar()->setValue(verScrollBarPos);
  }
}

void MainWindow::reloadRequested() {
  // Start loading / processing the current page.
  updateMainArea();
}

void MainWindow::debugToggled(const bool enabled) {
  m_debug = enabled;
}

void MainWindow::fixDpiDialogRequested() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  assert(!m_fixDpiDialog);
  m_fixDpiDialog = new FixDpiDialog(m_pages->toImageFileInfo(), this);
  m_fixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_fixDpiDialog->setWindowModality(Qt::WindowModal);

  connect(m_fixDpiDialog, SIGNAL(accepted()), SLOT(fixedDpiSubmitted()));

  m_fixDpiDialog->show();
}

void MainWindow::fixedDpiSubmitted() {
  assert(m_fixDpiDialog);
  assert(m_pages);
  assert(m_thumbSequence);

  const PageInfo selectedPageBefore(m_thumbSequence->selectionLeader());

  m_pages->updateMetadataFrom(m_fixDpiDialog->files());

  // The thumbnail list also stores page metadata, including the DPI.
  m_thumbSequence->reset(currentPageSequence(), ThumbnailSequence::KEEP_SELECTION,
                         m_thumbSequence->pageOrderProvider());

  const PageInfo selectedPageAfter(m_thumbSequence->selectionLeader());

  // Reload if the current page was affected.
  // Note that imageId() isn't supposed to change - we check just in case.
  if ((selectedPageBefore.imageId() != selectedPageAfter.imageId())
      || (selectedPageBefore.metadata() != selectedPageAfter.metadata())) {
    reloadRequested();
  }
}

void MainWindow::openSettingsDialog() {
  auto* dialog = new SettingsDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  connect(dialog, SIGNAL(settingsChanged()), this, SLOT(onSettingsChanged()));
  dialog->show();
}

void MainWindow::openDefaultParamsDialog() {
  auto* dialog = new DefaultParamsDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  dialog->show();
}

void MainWindow::onSettingsChanged() {
  ApplicationSettings& settings = ApplicationSettings::getInstance();
  bool needInvalidate = true;

  if (m_thumbnailCache) {
    const QSize maxThumbSize = settings.getThumbnailQuality();
    if (m_thumbnailCache->getMaxThumbSize() != maxThumbSize) {
      m_thumbnailCache->setMaxThumbSize(maxThumbSize);
    }
  }

  updateThumbnailViewMode();

  const QSizeF maxLogicalThumbSize = settings.getMaxLogicalThumbnailSize();
  if (m_maxLogicalThumbSize != maxLogicalThumbSize) {
    m_maxLogicalThumbSize = maxLogicalThumbSize;
    updateMaxLogicalThumbSize();
    needInvalidate = false;
  }

  if (needInvalidate) {
    m_thumbSequence->invalidateAllThumbnails();
  }
}

void MainWindow::showAboutDialog() {
  Ui::AboutDialog ui;
  auto* dialog = new QDialog(this);
  ui.setupUi(dialog);
  ui.version->setText(QString(tr("version ")) + QString::fromUtf8(VERSION));

  QResource license(":/GPLv3.html");
  ui.licenseViewer->setHtml(QString::fromUtf8((const char*) license.data(), static_cast<int>(license.size())));

  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  dialog->show();
}

/**
 * This function is called asynchronously, always from the main thread.
 */
void MainWindow::handleOutOfMemorySituation() {
  deleteLater();

  m_outOfMemoryDialog->setParams(m_projectFile, m_stages, m_pages, m_selectedPage, m_outFileNameGen);

  closeProjectWithoutSaving();

  m_outOfMemoryDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_outOfMemoryDialog.release()->show();
}

/**
 * Note: the removed widgets are not deleted.
 */
void MainWindow::removeWidgetsFromLayout(QLayout* layout) {
  QLayoutItem* child;
  while ((child = layout->takeAt(0))) {
    delete child;
  }
}

void MainWindow::removeFilterOptionsWidget() {
  removeWidgetsFromLayout(m_optionsFrameLayout);
  // Delete the old widget we were owning, if any.
  m_optionsWidgetCleanup.clear();

  m_optionsWidget = nullptr;
}

ImageViewBase* MainWindow::currentPageView() const {
  return m_imageFrameLayout->count() > 0 ? findPrimaryImageView(m_imageFrameLayout->widget(0)) : nullptr;
}

bool MainWindow::isProjectLoaded() const {
  return !m_outFileNameGen.outDir().isEmpty();
}

bool MainWindow::isBelowSelectContent() const {
  return isBelowSelectContent(m_curFilter);
}

bool MainWindow::isBelowSelectContent(const int filterIdx) const {
  return filterIdx > m_stages->selectContentFilterIdx();
}

bool MainWindow::isBelowFixOrientation(int filterIdx) const {
  return filterIdx > m_stages->fixOrientationFilterIdx();
}

bool MainWindow::isOutputFilter() const {
  return isOutputFilter(m_curFilter);
}

bool MainWindow::isOutputFilter(const int filterIdx) const {
  return filterIdx == m_stages->outputFilterIdx();
}

PageView MainWindow::getCurrentView() const {
  return m_stages->filterAt(m_curFilter)->getView();
}

void MainWindow::updateMainArea() {
  if (m_pages->numImages() == 0) {
    filterList->setBatchProcessingPossible(false);
    setDockWidgetsVisible(false);
    showNewOpenProjectPanel();
    m_statusBarPanel->clear();
  } else if (isBatchProcessingInProgress()) {
    filterList->setBatchProcessingPossible(false);
    setImageWidget(m_batchProcessingWidget.get(), KEEP_OWNERSHIP);
  } else {
    setDockWidgetsVisible(true);
    const PageInfo page(m_thumbSequence->selectionLeader());
    if (page.isNull()) {
      filterList->setBatchProcessingPossible(false);
      removeImageWidget();
      removeFilterOptionsWidget();
    } else {
      // Note that loadPageInteractive may reset it to false.
      filterList->setBatchProcessingPossible(true);
      PageSequence pageSequence = m_thumbSequence->toPageSequence();
      if (pageSequence.numPages() > 0) {
        m_statusBarPanel->updatePage(pageSequence.pageNo(page.id()) + 1, pageSequence.numPages(), page.id());
      }
      loadPageInteractive(page);
    }
  }
}

bool MainWindow::checkReadyForOutput(const PageId* ignore) const {
  return m_stages->pageLayoutFilter()->checkReadyForOutput(*m_pages, ignore);
}

void MainWindow::loadPageInteractive(const PageInfo& page) {
  assert(!isBatchProcessingInProgress());

  m_interactiveQueue->cancelAndClear();

  if (isOutputFilter() && !checkReadyForOutput(&page.id())) {
    filterList->setBatchProcessingPossible(false);

    const QString errText(
        tr("Output is not yet possible, as the final size"
           " of pages is not yet known.\nTo determine it,"
           " run batch processing at \"Select Content\" or"
           " \"Margins\"."));

    removeFilterOptionsWidget();
    setImageWidget(new ErrorWidget(errText), TRANSFER_OWNERSHIP);
    return;
  }

  for (int i = 0; i < m_stages->count(); i++) {
    m_stages->filterAt(i)->loadDefaultSettings(page);
  }

  if (!isBatchProcessingInProgress()) {
    if (m_imageFrameLayout->indexOf(m_processingIndicationWidget.get()) != -1) {
      m_processingIndicationWidget->processingRestartedEffect();
    }
    bool currentWidgetIsImage = (Utils::castOrFindChild<ImageViewBase*>(m_imageFrameLayout->widget(0)) != nullptr);
    setImageWidget(m_processingIndicationWidget.get(), KEEP_OWNERSHIP, nullptr, currentWidgetIsImage);
    m_stages->filterAt(m_curFilter)->preUpdateUI(this, page);
  }

  assert(m_thumbnailCache);

  m_interactiveQueue->cancelAndClear();
  m_interactiveQueue->addProcessingTask(page, createCompositeTask(page, m_curFilter, false, m_debug));
  m_workerThreadPool->submitTask(m_interactiveQueue->takeForProcessing());
}  // MainWindow::loadPageInteractive

void MainWindow::updateWindowTitle() {
  QString projectName;

  if (m_projectFile.isEmpty()) {
    projectName = tr("Unnamed");
  } else {
    projectName = QFileInfo(m_projectFile).completeBaseName();
  }
  const QString version(QString::fromUtf8(VERSION));
  setWindowTitle(tr("%2 - ScanTailor Advanced [%1bit]").arg(sizeof(void*) * 8).arg(projectName));
}

void MainWindow::updateDisambiguationRecords(const PageSequence& pages) {
  for (const PageInfo& page : pages) {
    m_outFileNameGen.disambiguator()->registerFile(page.imageId().filePath());
  }
}

PageSelectionAccessor MainWindow::newPageSelectionAccessor() {
  auto provider = std::make_shared<PageSelectionProviderImpl>(this);
  return PageSelectionAccessor(provider);
}

void MainWindow::setDockWidgetsVisible(bool state) {
  filterDockWidget->setVisible(state);
  thumbnailsDockWidget->setVisible(state);
}

void MainWindow::scaleThumbnails(int scaleFactor) {
  const double dx = 25.0 * scaleFactor;
  const double dy = 16.0 * scaleFactor;
  const double width = qBound(100.0, m_maxLogicalThumbSize.width() + dx, 1000.0);
  const double height = qBound(64.0, m_maxLogicalThumbSize.height() + dy, 640.0);
  m_maxLogicalThumbSize = QSizeF(width, height);
  if (!m_maxLogicalThumbSizeUpdater.isActive()) {
    m_maxLogicalThumbSizeUpdater.start(350);
  }
  ApplicationSettings::getInstance().setMaxLogicalThumbnailSize(m_maxLogicalThumbSize);
}

void MainWindow::updateMaxLogicalThumbSize() {
  m_thumbSequence->setMaxLogicalThumbSize(m_maxLogicalThumbSize);
  updateThumbViewMinWidth();
  resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);
}

void MainWindow::setupIcons() {
  auto& iconProvider = IconProvider::getInstance();
  focusButton->setIcon(iconProvider.getIcon("eye"));
  prevPageBtn->setIcon(iconProvider.getIcon("triangle-up-arrow"));
  nextPageBtn->setIcon(iconProvider.getIcon("triangle-down-arrow"));
  filterSelectedBtn->setIcon(iconProvider.getIcon("check-mark"));
  gotoPageBtn->setIcon(iconProvider.getIcon("right-pointing"));
  selectionModeBtn->setIcon(iconProvider.getIcon("checkbox-styled"));
  thumbColumnViewBtn->setIcon(iconProvider.getIcon("column-view"));
  pauseSortingBtn->setIcon(iconProvider.getIcon("stop"));
  sortingOrderBtn->setIcon(iconProvider.getIcon("sorting-order"));
  deviationHighlightingBtn->setIcon(iconProvider.getIcon("six-spoked-asterisk"));
  diminishThumbnailsBtn->setIcon(iconProvider.getIcon("diminishing-glass"));
  magnifyThumbnailsBtn->setIcon(iconProvider.getIcon("magnifying-glass"));
}

void MainWindow::updateThumbnailViewMode() {
  const ThumbnailSequence::ViewMode viewMode
      = (ApplicationSettings::getInstance().isSingleColumnThumbnailDisplayEnabled()) ? ThumbnailSequence::SINGLE_COLUMN
                                                                                     : ThumbnailSequence::MULTI_COLUMN;
  if (m_thumbSequence->getViewMode() != viewMode) {
    m_thumbSequence->setViewMode(viewMode);
  }
}

void MainWindow::updateAutoSaveTimer() {
  if (m_autoSaveTimer.remainingTime() <= 0) {
    m_autoSaveTimer.start(60000);
  }
}

PageSequence MainWindow::currentPageSequence() {
  PageSequence pageSequence = m_pages->toPageSequence(getCurrentView());
  if (sortingOrderBtn->isChecked()) {
    std::reverse(pageSequence.begin(), pageSequence.end());
  }
  return pageSequence;
}

void MainWindow::reloadCurrentPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded())
    return;

  updateMainArea();
}
