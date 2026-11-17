// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "MainWindow.h"


#include <QDir>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <algorithm>
#include <cmath>
#include <memory>

#include "ErrorWidget.h"
#include "FixDpiDialog.h"
#include "ImageInfo.h"
#include "ImageMetadataLoader.h"
#include "ImageSizeLimit.h"
#include "ImageViewBase.h"
#include "LoadFilesStatusDialog.h"
#include "PageSequence.h"
#include "ProcessingTaskQueue.h"
#include "ProjectPages.h"
#include "SmartFilenameOrdering.h"
#include "StageSequence.h"
#include "Utils.h"
#include "config.h"
#include "ui_RemovePagesDialog.h"
#include "version.h"

using namespace core;

/**
 * Note: showInsertFileDialog(BEFORE, ImageId()) is legal and means inserting at the end.
 */
void MainWindow::showInsertFileDialog(BeforeOrAfter beforeOrAfter, const ImageId& existing) {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }
  // We need to filter out files already in project (issue #88: use normalized paths so
  // dialog and project paths compare correctly when project already has images).
  class ProxyModel : public QSortFilterProxyModel {
   public:
    explicit ProxyModel(const ProjectPages& pages) {
      setDynamicSortFilter(true);

      const PageSequence sequence(pages.toPageSequence(IMAGE_VIEW));
      for (const PageInfo& page : sequence) {
        const QString path(QFileInfo(page.imageId().filePath()).absoluteFilePath());
        if (!path.isEmpty()) {
          m_inProjectPaths.insert(QDir::cleanPath(path));
        }
      }
    }

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
      const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
      const QVariant data(idx.data(QFileSystemModel::FilePathRole));
      if (data.isNull()) {
        return true;
      }
      const QString path(QDir::cleanPath(QFileInfo(data.toString()).absoluteFilePath()));
      return !m_inProjectPaths.contains(path);
    }

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override { return left.row() < right.row(); }

   private:
    QSet<QString> m_inProjectPaths;
  };

  QString dialogDir;
  if (existing.isNull()) {
    dialogDir = m_projectFile.isEmpty() ? QDir::homePath() : QFileInfo(m_projectFile).absolutePath();
  } else {
    dialogDir = QFileInfo(existing.filePath()).absolutePath();
    if (dialogDir.isEmpty()) {
      dialogDir = m_projectFile.isEmpty() ? QDir::homePath() : QFileInfo(m_projectFile).absolutePath();
    }
  }
  auto dialog = std::make_unique<QFileDialog>(this, tr("Files to insert"), dialogDir);
  dialog->setFileMode(QFileDialog::ExistingFiles);
  dialog->setProxyModel(new ProxyModel(*m_pages));
  dialog->setNameFilter(tr("Images not in project (%1)").arg("*.png *.tiff *.tif *.jpeg *.jpg"));
  // XXX: Adding individual pages from a multi-page TIFF where
  // some of the pages are already in project is not supported right now.
  if (dialog->exec() != QDialog::Accepted) {
    return;
  }

  QStringList files(dialog->selectedFiles());
  if (files.empty()) {
    return;
  }

  // The order of items returned by QFileDialog is platform-dependent,
  // so we enforce our own ordering.
  std::sort(files.begin(), files.end(), SmartFilenameOrdering());

  // I suspect on some platforms it may be possible to select the same file twice,
  // so to be safe, remove duplicates.
  files.erase(std::unique(files.begin(), files.end()), files.end());


  std::vector<ImageFileInfo> newFiles;
  std::vector<QString> loadedFiles;
  std::vector<QString> failedFiles;  // Those we failed to read metadata from.
  // dialog->selectedFiles() returns file list in reverse order.
  for (int i = files.size() - 1; i >= 0; --i) {
    const QFileInfo fileInfo(files[i]);
    ImageFileInfo imageFileInfo(fileInfo, std::vector<ImageMetadata>());

    const ImageMetadataLoader::Status status = ImageMetadataLoader::load(
        files.at(i), [&](const ImageMetadata& metadata) { imageFileInfo.imageInfo().push_back(metadata); });

    // The same limit as when a page is loaded (D-04): a file with an oversized page is listed as failed, with its size.
    const QString tooLargeText = (status == ImageMetadataLoader::LOADED)
                                     ? ImageSizeLimit::oversizedPagesText(imageFileInfo.imageInfo())
                                     : QString();

    if (!tooLargeText.isEmpty()) {
      failedFiles.push_back(fileInfo.absoluteFilePath() + " (" + tooLargeText + ")");
    } else if (status == ImageMetadataLoader::LOADED) {
      newFiles.push_back(imageFileInfo);
      loadedFiles.push_back(fileInfo.absoluteFilePath());
    } else {
      failedFiles.push_back(fileInfo.absoluteFilePath());
    }
  }

  if (!failedFiles.empty()) {
    auto errDialog = std::make_unique<LoadFilesStatusDialog>(this);
    errDialog->setLoadedFiles(loadedFiles);
    errDialog->setFailedFiles(failedFiles);
    errDialog->setOkButtonName(QString(" %1 ").arg(tr("Skip failed files")));
    if ((errDialog->exec() != QDialog::Accepted) || loadedFiles.empty()) {
      return;
    }
  }

  // Check if there is at least one DPI that's not OK.
  if (std::find_if(newFiles.begin(), newFiles.end(), [&](const ImageFileInfo& p) -> bool { return !p.isDpiOK(); })
      != newFiles.end()) {
    auto dpiDialog = std::make_unique<FixDpiDialog>(newFiles, this);
    dpiDialog->setWindowModality(Qt::WindowModal);
    if (dpiDialog->exec() != QDialog::Accepted) {
      return;
    }

    newFiles = dpiDialog->files();
  }

  // Actually insert the new pages.
  for (const ImageFileInfo& file : newFiles) {
    int imageNum = -1;  // Zero-based image number in a multi-page TIFF.
    for (const ImageMetadata& metadata : file.imageInfo()) {
      ++imageNum;

      const int numSubPages = ProjectPages::adviseNumberOfLogicalPages(metadata, OrthogonalRotation());
      const ImageInfo imageInfo(ImageId(file.fileInfo(), imageNum), metadata, numSubPages, false, false);
      insertImage(imageInfo, beforeOrAfter, existing);
    }
  }
}  // MainWindow::showInsertFileDialog

void MainWindow::showRemovePagesDialog(const std::set<PageId>& pages) {
  auto dialog = std::make_unique<QDialog>(this);
  Ui::RemovePagesDialog ui;
  ui.setupUi(dialog.get());
  ui.icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(48, 48));

  ui.text->setText(ui.text->text().arg(pages.size()));

  QPushButton* removeBtn = ui.buttonBox->button(QDialogButtonBox::Ok);
  removeBtn->setText(tr("Remove"));

  dialog->setWindowModality(Qt::WindowModal);
  if (dialog->exec() == QDialog::Accepted) {
    removeFromProject(pages);
    eraseOutputFiles(pages);
  }
}

/**
 * Note: insertImage(..., BEFORE, ImageId()) is legal and means inserting at the end.
 */
void MainWindow::insertImage(const ImageInfo& newImage, BeforeOrAfter beforeOrAfter, ImageId existing) {
  std::vector<PageInfo> pages(m_pages->insertImage(newImage, beforeOrAfter, existing, getCurrentView()));

  if (beforeOrAfter == BEFORE) {
    // The second one will be inserted first, then the first
    // one will be inserted BEFORE the second one.
    std::reverse(pages.begin(), pages.end());
  }

  for (const PageInfo& pageInfo : pages) {
    m_outFileNameGen.disambiguator()->registerFile(pageInfo.imageId().filePath());
    m_thumbSequence->insert(pageInfo, beforeOrAfter, existing);
    existing = pageInfo.imageId();
  }
}

void MainWindow::removeFromProject(const std::set<PageId>& pages) {
  m_interactiveQueue->cancelAndRemove(pages);
  if (m_batchQueue) {
    m_batchQueue->cancelAndRemove(pages);
  }

  m_pages->removePages(pages);


  const PageSequence itemsInOrder = m_thumbSequence->toPageSequence();
  std::set<PageId> newSelection;

  bool selectFirstNonDeleted = false;
  if (itemsInOrder.numPages() > 0) {
    // if first page was deleted select first not deleted page
    // otherwise select last not deleted page from beginning
    selectFirstNonDeleted = pages.find(itemsInOrder.pageAt(0).id()) != pages.end();

    PageId lastNonDeleted;
    for (const PageInfo& page : itemsInOrder) {
      const PageId& id = page.id();
      const bool wasDeleted = (pages.find(id) != pages.end());

      if (!wasDeleted) {
        if (selectFirstNonDeleted) {
          m_thumbSequence->setSelection(id);
          newSelection.insert(id);
          break;
        } else {
          lastNonDeleted = id;
        }
      } else if (!selectFirstNonDeleted && !lastNonDeleted.isNull()) {
        m_thumbSequence->setSelection(lastNonDeleted);
        newSelection.insert(lastNonDeleted);
        break;
      }
    }

    m_thumbSequence->removePages(pages);

    if (newSelection.empty()) {
      // fallback to old behaviour
      if (m_thumbSequence->selectionLeader().isNull()) {
        m_thumbSequence->setSelection(m_thumbSequence->firstPage().id());
      }
    }
  }

  updateMainArea();
}  // MainWindow::removeFromProject

void MainWindow::eraseOutputFiles(const std::set<PageId>& pages) {
  std::vector<PageId::SubPage> eraseVariations;
  eraseVariations.reserve(3);

  for (const PageId& pageId : pages) {
    eraseVariations.clear();
    switch (pageId.subPage()) {
      case PageId::SINGLE_PAGE:
        eraseVariations.push_back(PageId::SINGLE_PAGE);
        eraseVariations.push_back(PageId::LEFT_PAGE);
        eraseVariations.push_back(PageId::RIGHT_PAGE);
        break;
      case PageId::LEFT_PAGE:
        eraseVariations.push_back(PageId::SINGLE_PAGE);
        eraseVariations.push_back(PageId::LEFT_PAGE);
        break;
      case PageId::RIGHT_PAGE:
        eraseVariations.push_back(PageId::SINGLE_PAGE);
        eraseVariations.push_back(PageId::RIGHT_PAGE);
        break;
    }

    for (PageId::SubPage subpage : eraseVariations) {
      QFile::remove(m_outFileNameGen.filePathFor(PageId(pageId.imageId(), subpage)));
    }
  }
}
