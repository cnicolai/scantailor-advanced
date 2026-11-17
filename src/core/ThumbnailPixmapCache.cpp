// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ThumbnailPixmapCache.h"

#include <GrayImage.h>
#include <Scale.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <algorithm>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <cmath>

#include "AtomicFileOverwriter.h"
#include "HiDpi.h"
#include "ImageId.h"
#include "ImageLoader.h"
#include "OutOfMemoryHandler.h"
#include "RelinkablePath.h"

using namespace ::boost;
using namespace ::boost::multi_index;
using namespace imageproc;

class ThumbnailPixmapCache::Item {
 public:
  enum Status {
    /**
     * The background threaed hasn't touched it yet.
     */
    QUEUED,

    /**
     * The image is currently being loaded by a background
     * thread, or it has been loaded, but the main thread
     * hasn't yet received the loaded image, or it's currently
     * converting it to a pixmap.
     */
    IN_PROGRESS,

    /**
     * The image was loaded and then converted to a pixmap
     * by the main thread.
     */
    LOADED,

    /**
     * The image could not be loaded.
     */
    LOAD_FAILED
  };

  ImageId imageId;

  mutable QPixmap pixmap;

  /**< Guaranteed to be set if status is LOADED */

  mutable std::vector<std::weak_ptr<CompletionHandler>> completionHandlers;

  /**
   * The total image loading attempts (of any images) by
   * ThumbnailPixmapCache at the time of the creation of this item.
   * This information is used for request expiration.
   * \see ThumbnailLoadResult::REQUEST_EXPIRED
   */
  int precedingLoadAttempts;

  mutable Status status;

  Item(const ImageId& imageId, int precedingLoadAttempts, Status st);

  Item(const Item& other);

 private:
  Item& operator=(const Item& other) = delete;  // Assignment is forbidden.
};


class ThumbnailPixmapCache::Impl : public QThread {
 public:
  Impl(const QString& thumbDir, const QSize& maxThumbSize, int maxCachedPixmaps, int expirationThreshold);

  ~Impl() override;

  void setThumbDir(const QString& thumbDir);

  const QSize& getMaxThumbSize() const;

  void setMaxThumbSize(const QSize& maxSize);

  void setDevicePixelRatio(double dpr);

  double getDevicePixelRatio() const;

  Status request(const ImageId& imageId,
                 QPixmap& pixmap,
                 bool loadNow = false,
                 const std::weak_ptr<CompletionHandler>* completionHandler = nullptr);

  void ensureThumbnailExists(const ImageId& imageId, const QImage& image);

  void recreateThumbnail(const ImageId& imageId, const QImage& image);

 protected:
  void run() override;

  void customEvent(QEvent* e) override;

 private:
  class LoadResultEvent;
  class ItemsByKeyTag;
  class LoadQueueTag;
  class RemoveQueueTag;

  using Container = multi_index_container<
      Item,
      indexed_by<hashed_unique<tag<ItemsByKeyTag>, member<Item, ImageId, &Item::imageId>, std::hash<ImageId>>,
                 sequenced<tag<LoadQueueTag>>,
                 sequenced<tag<RemoveQueueTag>>>>;

  using ItemsByKey = Container::index<ItemsByKeyTag>::type;
  using LoadQueue = Container::index<LoadQueueTag>::type;
  using RemoveQueue = Container::index<RemoveQueueTag>::type;

  class BackgroundLoader : public QObject {
   public:
    explicit BackgroundLoader(Impl& owner);

   protected:
    void customEvent(QEvent* e) override;

   private:
    Impl& m_owner;
  };


  void backgroundProcessing();

  static QImage loadSaveThumbnail(const ImageId& imageId, const QString& thumbDir, const QSize& effectiveSize);

  static QString getThumbFilePath(const ImageId& imageId, const QString& thumbDir, const QSize& effectiveSize);

  static QImage makeThumbnail(const QImage& image, const QSize& effectiveSize);

  void queuedToInProgress(const LoadQueue::iterator& lqIt);

  void postLoadResult(const LoadQueue::iterator& lqIt,
                      const QImage& image,
                      ThumbnailLoadResult::Status status,
                      int generation);

  void processLoadResult(LoadResultEvent* result);

  void removeExcessLocked();

  void removeItemLocked(const RemoveQueue::iterator& it);

  void removeLoadedItemsLocked();

  void cachePixmapUnlocked(const ImageId& imageId, const QPixmap& pixmap);

  void cachePixmapLocked(const ImageId& imageId, const QPixmap& pixmap);

  mutable QMutex m_mutex;
  BackgroundLoader m_backgroundLoader;
  Container m_items;
  ItemsByKey& m_itemsByKey; /**< ImageId => Item mapping */

  /**
   * An "std::list"-like view of QUEUED items in the order they are
   * going to be loaded.  Actually the list contains all kinds of items,
   * but all QUEUED ones precede any others.  New QUEUED items are added
   * to the front of this list for purposes of request expiration.
   * \see ThumbnailLoadResult::REQUEST_EXPIRED
   */
  LoadQueue& m_loadQueue;

  /**
   * An "std::list"-like view of LOADED items in the order they are
   * going to be removed. Actually the list contains all kinds of items,
   * but all LOADED ones precede any others.  Note that we don't bother
   * removing items without a pixmap, which would be all except LOADED
   * items.  New LOADED items are added after the last LOADED item
   * already present in the list.
   */
  RemoveQueue& m_removeQueue;

  /**
   * An iterator of m_removeQueue that marks the end of LOADED items.
   */
  RemoveQueue::iterator m_endOfLoadedItems;

  QString m_thumbDir;

  /** The quality setting. The thumbnail files are built at effectiveThumbSize(m_maxThumbSize, m_dpr). */
  QSize m_maxThumbSize;

  /** Device pixel ratio of the screen that shows the thumbnails. Guarded by m_mutex. */
  double m_dpr = 1.0;

  /**
   * Incremented whenever the effective thumbnail size changes (a new ratio or a new quality). Guarded by m_mutex.
   * A load or a recreation that started under an older generation must not leave a pixmap of the old size marked
   * LOADED, so its result is discarded.
   */
  int m_sizeGeneration = 0;

  int m_maxCachedPixmaps;

  /**
   * \see ThumbnailPixmapCache::ThumbnailPixmapCache()
   */
  int m_expirationThreshold;

  int m_numQueuedItems;
  int m_numLoadedItems;

  /**
   * Total image loading attempts so far.  Used for request expiration.
   * \see ThumbnailLoadResult::REQUEST_EXPIRED
   */
  int m_totalLoadAttempts;

  bool m_threadStarted;
  bool m_shuttingDown;
};


class ThumbnailPixmapCache::Impl::LoadResultEvent : public QEvent {
 public:
  LoadResultEvent(const Impl::LoadQueue::iterator& lqIt,
                  const QImage& image,
                  ThumbnailLoadResult::Status status,
                  int generation);

  ~LoadResultEvent() override;

  Impl::LoadQueue::iterator lqIter() const { return m_lqIter; }

  const QImage& image() const { return m_image; }

  void releaseImage() { m_image = QImage(); }

  ThumbnailLoadResult::Status status() const { return m_status; }

  int generation() const { return m_generation; }

 private:
  Impl::LoadQueue::iterator m_lqIter;
  QImage m_image;
  ThumbnailLoadResult::Status m_status;
  int m_generation;
};


/*========================== ThumbnailPixmapCache ===========================*/

ThumbnailPixmapCache::ThumbnailPixmapCache(const QString& thumbDir,
                                           const QSize& maxThumbSize,
                                           const int maxCachedPixmaps,
                                           const int expirationThreshold)
    : m_impl(std::make_unique<Impl>(RelinkablePath::normalize(thumbDir),
                                    maxThumbSize,
                                    maxCachedPixmaps,
                                    expirationThreshold)) {}

ThumbnailPixmapCache::~ThumbnailPixmapCache() = default;

void ThumbnailPixmapCache::setThumbDir(const QString& thumbDir) {
  m_impl->setThumbDir(RelinkablePath::normalize(thumbDir));
}

ThumbnailPixmapCache::Status ThumbnailPixmapCache::loadFromCache(const ImageId& imageId, QPixmap& pixmap) {
  return m_impl->request(imageId, pixmap);
}

ThumbnailPixmapCache::Status ThumbnailPixmapCache::loadNow(const ImageId& imageId, QPixmap& pixmap) {
  return m_impl->request(imageId, pixmap, true);
}

ThumbnailPixmapCache::Status ThumbnailPixmapCache::loadRequest(
    const ImageId& imageId,
    QPixmap& pixmap,
    const std::weak_ptr<CompletionHandler>& completionHandler) {
  return m_impl->request(imageId, pixmap, false, &completionHandler);
}

void ThumbnailPixmapCache::ensureThumbnailExists(const ImageId& imageId, const QImage& image) {
  m_impl->ensureThumbnailExists(imageId, image);
}

void ThumbnailPixmapCache::recreateThumbnail(const ImageId& imageId, const QImage& image) {
  m_impl->recreateThumbnail(imageId, image);
}

void ThumbnailPixmapCache::setMaxThumbSize(const QSize& maxSize) {
  m_impl->setMaxThumbSize(maxSize);
}

const QSize& ThumbnailPixmapCache::getMaxThumbSize() const {
  return m_impl->getMaxThumbSize();
}

QSize ThumbnailPixmapCache::effectiveThumbSize(const QSize& quality, const double dpr) {
  const double ratio = hidpi::sanitizedRatio(dpr);
  const auto scaleSide = [ratio](const int side) {
    // The min() caps the value before qRound(), so a huge side cannot overflow.
    const double scaled = std::min(side * ratio, static_cast<double>(MAX_EFFECTIVE_THUMB_SIDE));
    return std::clamp(qRound(scaled), 1, MAX_EFFECTIVE_THUMB_SIDE);
  };
  return QSize(scaleSide(quality.width()), scaleSide(quality.height()));
}

void ThumbnailPixmapCache::setDevicePixelRatio(const double dpr) {
  m_impl->setDevicePixelRatio(dpr);
}

double ThumbnailPixmapCache::getDevicePixelRatio() const {
  return m_impl->getDevicePixelRatio();
}

/*======================= ThumbnailPixmapCache::Impl ========================*/

ThumbnailPixmapCache::Impl::Impl(const QString& thumbDir,
                                 const QSize& maxThumbSize,
                                 const int maxCachedPixmaps,
                                 const int expirationThreshold)
    : m_backgroundLoader(*this),
      m_items(),
      m_itemsByKey(m_items.get<ItemsByKeyTag>()),
      m_loadQueue(m_items.get<LoadQueueTag>()),
      m_removeQueue(m_items.get<RemoveQueueTag>()),
      m_endOfLoadedItems(m_removeQueue.end()),
      m_thumbDir(thumbDir),
      m_maxThumbSize(maxThumbSize),
      m_maxCachedPixmaps(maxCachedPixmaps),
      m_expirationThreshold(expirationThreshold),
      m_numQueuedItems(0),
      m_numLoadedItems(0),
      m_totalLoadAttempts(0),
      m_threadStarted(false),
      m_shuttingDown(false) {
  // Note that QDir::mkdir() will fail if the parent directory,
  // that is $OUT/cache doesn't exist. We want that behaviour,
  // as otherwise when loading a project from a different machine,
  // a whole bunch of bogus directories would be created.
  QDir().mkdir(m_thumbDir);

  m_backgroundLoader.moveToThread(this);
}

ThumbnailPixmapCache::Impl::~Impl() {
  {
    const QMutexLocker locker(&m_mutex);

    if (!m_threadStarted) {
      return;
    }

    m_shuttingDown = true;
  }

  quit();
  wait();
}

void ThumbnailPixmapCache::Impl::setThumbDir(const QString& thumbDir) {
  QMutexLocker locker(&m_mutex);

  if (thumbDir == m_thumbDir) {
    return;
  }

  m_thumbDir = thumbDir;

  for (const Item& item : m_loadQueue) {
    // This trick will make all queued tasks to expire.
    m_totalLoadAttempts = std::max(m_totalLoadAttempts, item.precedingLoadAttempts + m_expirationThreshold + 1);
  }
}

ThumbnailPixmapCache::Status ThumbnailPixmapCache::Impl::request(
    const ImageId& imageId,
    QPixmap& pixmap,
    const bool loadNow,
    const std::weak_ptr<CompletionHandler>* completionHandler) {
  assert(QCoreApplication::instance()->thread() == QThread::currentThread());

  QMutexLocker locker(&m_mutex);

  if (m_shuttingDown) {
    return LOAD_FAILED;
  }

  const ItemsByKey::iterator kIt(m_itemsByKey.find(imageId));
  if (kIt != m_itemsByKey.end()) {
    if (kIt->status == Item::LOADED) {
      pixmap = kIt->pixmap;

      // Move it after all other candidates for removal.
      const RemoveQueue::iterator rqIt(m_items.project<RemoveQueueTag>(kIt));
      m_removeQueue.relocate(m_endOfLoadedItems, rqIt);
      return LOADED;
    } else if (kIt->status == Item::LOAD_FAILED) {
      pixmap = kIt->pixmap;
      return LOAD_FAILED;
    }
  }

  if (loadNow) {
    const QString thumbDir(m_thumbDir);
    const QSize effectiveSize(effectiveThumbSize(m_maxThumbSize, m_dpr));

    locker.unlock();

    pixmap = QPixmap::fromImage(loadSaveThumbnail(imageId, thumbDir, effectiveSize));
    if (pixmap.isNull()) {
      return LOAD_FAILED;
    }

    cachePixmapUnlocked(imageId, pixmap);
    return LOADED;
  }

  if (!completionHandler) {
    return LOAD_FAILED;
  }

  if (kIt != m_itemsByKey.end()) {
    assert(kIt->status == Item::QUEUED || kIt->status == Item::IN_PROGRESS);
    kIt->completionHandlers.push_back(*completionHandler);

    if (kIt->status == Item::QUEUED) {
      // Because we've got a new request for this item,
      // we move it to the beginning of the load queue.
      // Note that we don't do it for IN_PROGRESS items,
      // because all QUEUED items must precede any other
      // items in the load queue.
      const LoadQueue::iterator lqIt(m_items.project<LoadQueueTag>(kIt));
      m_loadQueue.relocate(m_loadQueue.begin(), lqIt);
    }
    return QUEUED;
  }

  // Create a new item.
  const LoadQueue::iterator lqIt(m_loadQueue.push_front(Item(imageId, m_totalLoadAttempts, Item::QUEUED)).first);
  // Now our new item is at the beginning of the load queue and at the
  // end of the remove queue.

  assert(lqIt->status == Item::QUEUED);
  assert(lqIt->completionHandlers.empty());

  if (m_endOfLoadedItems == m_removeQueue.end()) {
    m_endOfLoadedItems = m_items.project<RemoveQueueTag>(lqIt);
  }
  lqIt->completionHandlers.push_back(*completionHandler);

  if (m_numQueuedItems++ == 0) {
    if (m_threadStarted) {
      // Wake the background thread up.
      QCoreApplication::postEvent(&m_backgroundLoader, new QEvent(QEvent::User));
    } else {
      // Start the background thread.
      start();
      m_threadStarted = true;
    }
  }
  return QUEUED;
}  // ThumbnailPixmapCache::Impl::request

void ThumbnailPixmapCache::Impl::ensureThumbnailExists(const ImageId& imageId, const QImage& image) {
  if (m_shuttingDown) {
    return;
  }

  if (image.isNull()) {
    return;
  }

  QMutexLocker locker(&m_mutex);
  const QString thumbDir(m_thumbDir);
  const QSize effectiveSize(effectiveThumbSize(m_maxThumbSize, m_dpr));
  locker.unlock();

  const QString thumbFilePath(getThumbFilePath(imageId, thumbDir, effectiveSize));
  if (QFile::exists(thumbFilePath)) {
    return;
  }

  const QImage thumbnail(makeThumbnail(image, effectiveSize));

  AtomicFileOverwriter overwriter;
  QIODevice* iodev = overwriter.startWriting(thumbFilePath);
  if (iodev && thumbnail.save(iodev, "PNG")) {
    overwriter.commit();
  }
}

namespace {

/**
 * Deletes the thumbnail files of the same image that have another size than \p keepFilePath ("<prefix>_q<width>.png").
 * Those files were made from the image as it was before and would show stale content once the effective size
 * returns to their width.
 */
void removeOtherSizeThumbnails(const QString& keepFilePath) {
  const QFileInfo keepInfo(keepFilePath);
  const QString keepName = keepInfo.fileName();
  const int qPos = keepName.lastIndexOf(QStringLiteral("_q"));
  if (qPos < 0) {
    return;
  }
  // The prefix is everything up to and including "_q": file name, page and hash of the original path, then "_q".
  const QString prefix = keepName.left(qPos + 2);
  const QString suffix = QStringLiteral(".png");

  const QDir dir(keepInfo.absolutePath());
  const QStringList names = dir.entryList(QDir::Files);
  for (const QString& name : names) {
    if (name == keepName || !name.startsWith(prefix) || !name.endsWith(suffix)) {
      continue;
    }
    const QString width = name.mid(prefix.size(), name.size() - prefix.size() - suffix.size());
    if (width.isEmpty() || !std::all_of(width.begin(), width.end(), [](const QChar c) { return c.isDigit(); })) {
      continue;
    }
    QFile::remove(dir.filePath(name));
  }
}

}  // namespace

void ThumbnailPixmapCache::Impl::recreateThumbnail(const ImageId& imageId, const QImage& image) {
  if (m_shuttingDown) {
    return;
  }

  if (image.isNull()) {
    return;
  }

  QMutexLocker locker(&m_mutex);

  // Write the thumbnail for the size in force. If the size changes while we write (another display, another
  // quality), write again for the new size, so that neither the file nor the item in memory keep the old content.
  while (true) {
    const QString thumbDir(m_thumbDir);
    const QSize effectiveSize(effectiveThumbSize(m_maxThumbSize, m_dpr));
    const int generation = m_sizeGeneration;
    locker.unlock();

    const QString thumbFilePath(getThumbFilePath(imageId, thumbDir, effectiveSize));
    const QImage thumbnail(makeThumbnail(image, effectiveSize));
    bool thumbWritten = false;

    // Note that we may be called from multiple threads at the same time.
    AtomicFileOverwriter overwriter;
    QIODevice* iodev = overwriter.startWriting(thumbFilePath);
    if (iodev && thumbnail.save(iodev, "PNG")) {
      thumbWritten = overwriter.commit();
    } else {
      overwriter.abort();
    }

    if (!thumbWritten) {
      return;
    }

    // Files of the other sizes (the other display's ratio, an earlier quality) still hold the old image.
    removeOtherSizeThumbnails(thumbFilePath);

    locker.relock();
    if (generation == m_sizeGeneration) {
      break;
    }
  }

  const ItemsByKey::iterator kIt(m_itemsByKey.find(imageId));
  if (kIt == m_itemsByKey.end()) {
    return;
  }

  switch (kIt->status) {
    case Item::LOADED:
    case Item::LOAD_FAILED:
      removeItemLocked(m_items.project<RemoveQueueTag>(kIt));
      break;
    case Item::QUEUED:
      break;
    case Item::IN_PROGRESS:
      // We have a small race condition in this case.
      // We don't know if the other thread has already loaded
      // the thumbnail or not.  In case it did, again we
      // don't know if it loaded the old or new version.
      // Well, let's just pretend the thumnail was loaded
      // (or failed to load) before we wrote the new version.
      break;
  }
}  // ThumbnailPixmapCache::Impl::recreateThumbnail

void ThumbnailPixmapCache::Impl::run() {
  backgroundProcessing();
  exec();  // Wait for further processing requests (via custom events).
}

void ThumbnailPixmapCache::Impl::customEvent(QEvent* e) {
  processLoadResult(dynamic_cast<LoadResultEvent*>(e));
}

void ThumbnailPixmapCache::Impl::backgroundProcessing() {
  // This method is called from a background thread.
  assert(QCoreApplication::instance()->thread() != QThread::currentThread());

  while (true) {
    try {
      // We are going to initialize these while holding the mutex.
      LoadQueue::iterator lqIt;
      ImageId imageId;
      QString thumbDir;
      QSize effectiveSize;
      int generation = 0;

      {
        const QMutexLocker locker(&m_mutex);

        if (m_shuttingDown || m_items.empty()) {
          break;
        }

        lqIt = m_loadQueue.begin();
        imageId = lqIt->imageId;

        if (lqIt->status != Item::QUEUED) {
          // All QUEUED items precede any other items
          // in the load queue, so it means there are no
          // QUEUED items at all.
          assert(m_numQueuedItems == 0);
          break;
        }

        // By marking the item as IN_PROGRESS, we prevent it
        // from being processed again before the GUI thread
        // receives our LoadResultEvent.
        queuedToInProgress(lqIt);

        if (m_totalLoadAttempts - lqIt->precedingLoadAttempts > m_expirationThreshold) {
          // Expire this request.  The reasoning behind
          // request expiration is described in
          // ThumbnailLoadResult::REQUEST_EXPIRED
          // documentation.

          postLoadResult(lqIt, QImage(), ThumbnailLoadResult::REQUEST_EXPIRED, m_sizeGeneration);
          continue;
        }

        // Expired requests don't count as load attempts.
        ++m_totalLoadAttempts;

        // Copy those while holding the mutex.
        thumbDir = m_thumbDir;
        effectiveSize = effectiveThumbSize(m_maxThumbSize, m_dpr);
        generation = m_sizeGeneration;
      }  // mutex scope
      const QImage image(loadSaveThumbnail(imageId, thumbDir, effectiveSize));

      const ThumbnailLoadResult::Status status
          = image.isNull() ? ThumbnailLoadResult::LOAD_FAILED : ThumbnailLoadResult::LOADED;
      postLoadResult(lqIt, image, status, generation);
    } catch (const std::bad_alloc&) {
      OutOfMemoryHandler::instance().handleOutOfMemorySituation();
    }
  }
}  // ThumbnailPixmapCache::Impl::backgroundProcessing

QImage ThumbnailPixmapCache::Impl::loadSaveThumbnail(const ImageId& imageId,
                                                     const QString& thumbDir,
                                                     const QSize& effectiveSize) {
  const QString thumbFilePath(getThumbFilePath(imageId, thumbDir, effectiveSize));

  QImage image(ImageLoader::load(thumbFilePath, 0));
  if (!image.isNull()) {
    return image;
  }

  image = ImageLoader::loadSource(imageId);
  if (image.isNull()) {
    return QImage();
  }

  const QImage thumbnail(makeThumbnail(image, effectiveSize));
  thumbnail.save(thumbFilePath, "PNG");
  return thumbnail;
}

QString ThumbnailPixmapCache::Impl::getThumbFilePath(const ImageId& imageId,
                                                     const QString& thumbDir,
                                                     const QSize& effectiveSize) {
  // Because a project may have several files with the same name (from
  // different directories), we add a hash of the original image path
  // to the thumbnail file name.
  const QByteArray origPathHash = QCryptographicHash::hash(imageId.filePath().toUtf8(), QCryptographicHash::Md5)
                                      .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
  const QString origPathHashStr = QString::fromLatin1(origPathHash.data(), origPathHash.size());
  const QString thumbnailQualityStr = QChar('q') + QString::number(effectiveSize.width());

  const QFileInfo origImgPath(imageId.filePath());
  QString thumbFilePath(thumbDir);
  thumbFilePath += QChar('/');
  thumbFilePath += origImgPath.completeBaseName();
  thumbFilePath += QChar('_');
  thumbFilePath += QString::number(imageId.zeroBasedPage());
  thumbFilePath += QChar('_');
  thumbFilePath += origPathHashStr;
  thumbFilePath += QChar('_');
  thumbFilePath += thumbnailQualityStr;
  thumbFilePath += QString::fromLatin1(".png");
  return thumbFilePath;
}

QImage ThumbnailPixmapCache::Impl::makeThumbnail(const QImage& image, const QSize& effectiveSize) {
  if ((image.width() < effectiveSize.width()) && (image.height() < effectiveSize.height())) {
    return image;
  }

  QSize toSize(image.size());
  toSize.scale(effectiveSize, Qt::KeepAspectRatio);

  if ((image.format() == QImage::Format_Indexed8) && image.isGrayscale()) {
    // This will be faster than QImage::scale().
    return scaleToGray(GrayImage(image), toSize);
  }
  return image.scaled(toSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void ThumbnailPixmapCache::Impl::queuedToInProgress(const LoadQueue::iterator& lqIt) {
  assert(lqIt->status == Item::QUEUED);
  lqIt->status = Item::IN_PROGRESS;

  assert(m_numQueuedItems > 0);
  --m_numQueuedItems;

  // Move it item to the end of load queue.
  // The point is to keep QUEUED items before any others.
  m_loadQueue.relocate(m_loadQueue.end(), lqIt);

  // Going from QUEUED to IN_PROGRESS doesn't require
  // moving it in the remove queue, as we only remove
  // LOADED items.
}

void ThumbnailPixmapCache::Impl::postLoadResult(const LoadQueue::iterator& lqIt,
                                                const QImage& image,
                                                const ThumbnailLoadResult::Status status,
                                                const int generation) {
  auto* e = new LoadResultEvent(lqIt, image, status, generation);
  QCoreApplication::postEvent(this, e);
}

void ThumbnailPixmapCache::Impl::processLoadResult(LoadResultEvent* result) {
  assert(QCoreApplication::instance()->thread() == QThread::currentThread());

  QPixmap pixmap(QPixmap::fromImage(result->image()));
  result->releaseImage();

  std::vector<std::weak_ptr<CompletionHandler>> completionHandlers;
  ThumbnailLoadResult::Status status = result->status();

  {
    const QMutexLocker locker(&m_mutex);

    if (m_shuttingDown) {
      return;
    }

    const LoadQueue::iterator lqIt(result->lqIter());
    const RemoveQueue::iterator rqIt(m_items.project<RemoveQueueTag>(lqIt));

    const Item& item = *lqIt;

    if (status == ThumbnailLoadResult::LOADED && result->generation() != m_sizeGeneration) {
      // The ratio or the quality changed after this load started, so the pixmap has the old size. Do not store it
      // as LOADED; the expired result makes the view request the thumbnail again at the new size.
      status = ThumbnailLoadResult::REQUEST_EXPIRED;
      pixmap = QPixmap();
    }

    if ((status == ThumbnailLoadResult::LOADED) && pixmap.isNull()) {
      // That's a special case caused by cachePixmapLocked().
      assert(!item.pixmap.isNull());
    } else {
      item.pixmap = pixmap;
    }
    item.completionHandlers.swap(completionHandlers);

    if (status == ThumbnailLoadResult::LOADED) {
      // Maybe remove an older item.
      removeExcessLocked();

      item.status = Item::LOADED;
      ++m_numLoadedItems;

      // Move this item after all other LOADED items in
      // the remove queue.
      m_removeQueue.relocate(m_endOfLoadedItems, rqIt);

      // Move to the end of load queue.
      m_loadQueue.relocate(m_loadQueue.end(), lqIt);
    } else if (status == ThumbnailLoadResult::LOAD_FAILED) {
      // We keep items that failed to load, as they are cheap
      // to keep and helps us avoid trying to load them
      // again and again.

      item.status = Item::LOAD_FAILED;

      // Move to the end of load queue.
      m_loadQueue.relocate(m_loadQueue.end(), lqIt);
    } else {
      assert(status == ThumbnailLoadResult::REQUEST_EXPIRED);

      // Just remove it.
      removeItemLocked(rqIt);
    }
  }  // mutex scope
  // Notify listeners.
  const ThumbnailLoadResult loadResult(status, pixmap);
  using WeakHandler = std::weak_ptr<CompletionHandler>;
  for (const WeakHandler& wh : completionHandlers) {
    const std::shared_ptr<CompletionHandler> sh(wh.lock());
    if (sh) {
      (*sh)(loadResult);
    }
  }
}  // ThumbnailPixmapCache::Impl::processLoadResult

void ThumbnailPixmapCache::Impl::removeExcessLocked() {
  if (m_numLoadedItems >= m_maxCachedPixmaps) {
    assert(m_numLoadedItems > 0);
    assert(!m_removeQueue.empty());
    assert(m_removeQueue.front().status == Item::LOADED);
    removeItemLocked(m_removeQueue.begin());
  }
}

void ThumbnailPixmapCache::Impl::removeItemLocked(const RemoveQueue::iterator& it) {
  switch (it->status) {
    case Item::QUEUED:
      assert(m_numQueuedItems > 0);
      --m_numQueuedItems;
      break;
    case Item::LOADED:
      assert(m_numLoadedItems > 0);
      --m_numLoadedItems;
      break;
    default:;
  }

  if (m_endOfLoadedItems == it) {
    ++m_endOfLoadedItems;
  }

  m_removeQueue.erase(it);
}

void ThumbnailPixmapCache::Impl::removeLoadedItemsLocked() {
  if (m_numLoadedItems == 0) {
    return;
  }

  for (auto it = m_removeQueue.begin(); it != m_removeQueue.end();) {
    if (it->status == Item::LOADED) {
      it = m_removeQueue.erase(it);
    } else {
      ++it;
    }
  }

  m_numLoadedItems = 0;
  // QUEUED, IN_PROGRESS and LOAD_FAILED items stay in the queue; the first of them starts the non-LOADED range.
  // With an empty queue begin() == end(), which request() relies on.
  m_endOfLoadedItems = m_removeQueue.begin();
}

void ThumbnailPixmapCache::Impl::cachePixmapUnlocked(const ImageId& imageId, const QPixmap& pixmap) {
  const QMutexLocker locker(&m_mutex);
  cachePixmapLocked(imageId, pixmap);
}

void ThumbnailPixmapCache::Impl::cachePixmapLocked(const ImageId& imageId, const QPixmap& pixmap) {
  if (m_shuttingDown) {
    return;
  }

  const Item::Status newStatus = pixmap.isNull() ? Item::LOAD_FAILED : Item::LOADED;

  // Check if such item already exists.
  const ItemsByKey::iterator kIt(m_itemsByKey.find(imageId));
  if (kIt == m_itemsByKey.end()) {
    // Existing item not found.

    // Maybe remove an older item.
    removeExcessLocked();

    // Insert our new item.
    const RemoveQueue::iterator rqIt(
        m_removeQueue.insert(m_endOfLoadedItems, Item(imageId, m_totalLoadAttempts, newStatus)).first);
    // Our new item is now after all LOADED items in the
    // remove queue and at the end of the load queue.
    if (newStatus == Item::LOAD_FAILED) {
      --m_endOfLoadedItems;
    } else {
      // Count it, or removeLoadedItemsLocked() would skip the flush when only items from loadNow() are held.
      ++m_numLoadedItems;
    }

    rqIt->pixmap = pixmap;

    assert(rqIt->completionHandlers.empty());
    return;
  }

  switch (kIt->status) {
    case Item::LOADED:
      // There is no point in replacing LOADED items.
    case Item::IN_PROGRESS:
      // It's unsafe to touch IN_PROGRESS items.
      return;
    default:
      break;
  }

  if ((newStatus == Item::LOADED) && (kIt->status == Item::QUEUED)) {
    // Not so fast.  We can't go from QUEUED to LOADED directly.
    // Well, maybe we can, but we'd have to invoke the completion
    // handlers right now.  We'd rather do it asynchronously,
    // so let's transition it to IN_PROGRESS and send
    // a LoadResultEvent asynchronously.

    assert(!kIt->completionHandlers.empty());

    const LoadQueue::iterator lqIt(m_items.project<LoadQueueTag>(kIt));

    lqIt->pixmap = pixmap;
    queuedToInProgress(lqIt);
    postLoadResult(lqIt, QImage(), ThumbnailLoadResult::LOADED, m_sizeGeneration);
    return;
  }

  assert(kIt->status == Item::LOAD_FAILED);

  kIt->status = newStatus;
  kIt->pixmap = pixmap;

  if (newStatus == Item::LOADED) {
    const RemoveQueue::iterator rqIt(m_items.project<RemoveQueueTag>(kIt));
    m_removeQueue.relocate(m_endOfLoadedItems, rqIt);
    ++m_numLoadedItems;
  }
}  // ThumbnailPixmapCache::Impl::cachePixmapLocked

const QSize& ThumbnailPixmapCache::Impl::getMaxThumbSize() const {
  return m_maxThumbSize;
}

void ThumbnailPixmapCache::Impl::setMaxThumbSize(const QSize& maxSize) {
  const QMutexLocker locker(&m_mutex);

  removeLoadedItemsLocked();
  m_maxThumbSize = maxSize;
  ++m_sizeGeneration;
}

void ThumbnailPixmapCache::Impl::setDevicePixelRatio(const double dpr) {
  const double ratio = hidpi::sanitizedRatio(dpr);

  const QMutexLocker locker(&m_mutex);

  if (qFuzzyCompare(ratio, m_dpr)) {
    return;
  }

  removeLoadedItemsLocked();
  m_dpr = ratio;
  ++m_sizeGeneration;
}

double ThumbnailPixmapCache::Impl::getDevicePixelRatio() const {
  const QMutexLocker locker(&m_mutex);
  return m_dpr;
}

/*====================== ThumbnailPixmapCache::Item =========================*/

ThumbnailPixmapCache::Item::Item(const ImageId& imageId, const int precedingLoadAttempts, const Status st)
    : imageId(imageId), precedingLoadAttempts(precedingLoadAttempts), status(st) {}

ThumbnailPixmapCache::Item::Item(const Item& other) = default;

/*=============== ThumbnailPixmapCache::Impl::LoadResultEvent ===============*/

ThumbnailPixmapCache::Impl::LoadResultEvent::LoadResultEvent(const Impl::LoadQueue::iterator& lqIt,
                                                             const QImage& image,
                                                             const ThumbnailLoadResult::Status status,
                                                             const int generation)
    : QEvent(QEvent::User), m_lqIter(lqIt), m_image(image), m_status(status), m_generation(generation) {}

ThumbnailPixmapCache::Impl::LoadResultEvent::~LoadResultEvent() = default;

/*================== ThumbnailPixmapCache::BackgroundLoader =================*/

ThumbnailPixmapCache::Impl::BackgroundLoader::BackgroundLoader(Impl& owner) : m_owner(owner) {}

void ThumbnailPixmapCache::Impl::BackgroundLoader::customEvent(QEvent*) {
  m_owner.backgroundProcessing();
}
