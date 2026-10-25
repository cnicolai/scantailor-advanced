// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_THUMBNAILPIXMAPCACHE_H_
#define SCANTAILOR_CORE_THUMBNAILPIXMAPCACHE_H_

#include <boost/weak_ptr.hpp>
#include <memory>

#include "AbstractCommand.h"
#include "NonCopyable.h"
#include "ThumbnailLoadResult.h"

class ImageId;
class QImage;
class QPixmap;
class QString;
class QSize;

class ThumbnailPixmapCache {
  DECLARE_NON_COPYABLE(ThumbnailPixmapCache)

 public:
  enum Status { LOADED, LOAD_FAILED, QUEUED };

  using CompletionHandler = AbstractCommand<void, const ThumbnailLoadResult&>;

  /**
   * \brief Constructor.  To be called from the GUI thread only.
   *
   * \param thumbDir The directory to store thumbnails in.  If the
   *        provided directory doesn't exist, it will be created.
   * \param maxSize The maximum width and height for thumbnails.
   *        The actual thumbnail size is going to depend on its aspect
   *        ratio, but it won't exceed the provided maximum.
   * \param maxCachedPixmaps The maximum number of pixmaps to store
   *        in memory.
   * \param expirationThreshold Requests are served from newest to
   *        oldest ones. If a request is still not served after a certain
   *        number of newer requests have been served, that request is
   *        expired.  \p expirationThreshold specifies the exact number
   *        of requests that cause older requests to expire.
   *
   * \see ThumbnailLoadResult::REQUEST_EXPIRED
   */
  ThumbnailPixmapCache(const QString& thumbDir, const QSize& maxSize, int maxCachedPixmaps, int expirationThreshold);

  /**
   * \brief Destructor.  To be called from the GUI thread only.
   */
  virtual ~ThumbnailPixmapCache();

  void setThumbDir(const QString& thumbDir);

  /**
   * The largest side, in pixels, of a thumbnail file. It is the dialog maximum for the thumbnail quality (1500)
   * times the device pixel ratio 2, so every value the dialog allows is kept and multiplied on a Retina display,
   * while a hand-edited absurd value cannot lead to a huge allocation or an overflow.
   */
  static constexpr int MAX_EFFECTIVE_THUMB_SIDE = 3000;

  /**
   * \brief The size of the thumbnail files for a quality value and a device pixel ratio.
   *
   * Each side is the quality side times the (sanitized) ratio, rounded and clamped to 1..MAX_EFFECTIVE_THUMB_SIDE.
   */
  static QSize effectiveThumbSize(const QSize& quality, double dpr);

  /**
   * \brief The quality setting, that is the maximum thumbnail size at device pixel ratio 1.
   *
   * It is not multiplied by the device pixel ratio.
   */
  const QSize& getMaxThumbSize() const;

  void setMaxThumbSize(const QSize& maxSize);

  /**
   * \brief Sets the device pixel ratio of the screen that shows the thumbnails.
   *
   * Thumbnail files are built at the quality size times this ratio and named by that width. When the ratio
   * changes, the pixmaps held in memory are dropped and background loads that started earlier are discarded.
   * Files on disk are never touched (files of other sizes stay where they are). An equal ratio does nothing.
   *
   * \note This function is to be called from the GUI thread only.
   */
  void setDevicePixelRatio(double dpr);

  double getDevicePixelRatio() const;

  /**
   * \brief Take the pixmap from cache, if it's there.
   *
   * If it's not, LOAD_FAILED will be returned.
   *
   * \note This function is to be called from the GUI thread only.
   */
  Status loadFromCache(const ImageId& imageId, QPixmap& pixmap);

  /**
   * \brief Take the pixmap from cache or from disk, blocking if necessary.
   *
   * \note This function is to be called from the GUI thread only.
   */
  Status loadNow(const ImageId& imageId, QPixmap& pixmap);

  /**
   * \brief Take the pixmap from cache or schedule a load request.
   *
   * If the pixmap is in cache, return it immediately.  Otherwise,
   * schedule its loading in background.  Once the load request
   * has been processed, the provided \p call_handler will be called.
   *
   * \note This function is to be called from the GUI thread only.
   *
   * \param imageId The identifier of the full size image and its thumbnail.
   * \param[out] pixmap If the pixmap is cached, store it here.
   * \param completionHandler A functor that will be called on request
   * completion.  The best way to construct such a functor would be:
   * \code
   * class X : public boost::signals::trackable
   * {
   * public:
   *  void handleCompletion(const ThumbnailLoadResult& result);
   * };
   *
   * X x;
   * cache->loadRequest(imageId, pixmap, boost::bind(&X::handleCompletion, x, _1));
   * \endcode
   * Note that deriving X from boost::signals::trackable (with public inheritance)
   * allows to safely delete the x object without worrying about callbacks
   * it may receive in the future.  Keep in mind however, that deleting
   * x is only safe when done from the GUI thread.  Another thing to
   * keep in mind is that only boost::bind() can handle trackable binds.
   * Other methods, for example boost::lambda::bind() can't do that.
   */
  Status loadRequest(const ImageId& imageId,
                     QPixmap& pixmap,
                     const std::weak_ptr<CompletionHandler>& completionHandler);

  /**
   * \brief If no thumbnail exists for this image, create it.
   *
   * Using this function is optional.  It just presents an optimization
   * opportunity.  Suppose you have the full size image already loaded,
   * and want to avoid a second load when its thumbnail is requested.
   *
   * \param imageId The identifier of the full size image and its thumbnail.
   * \param image The full-size image.
   *
   * \note This function may be called from any thread, even concurrently.
   */
  void ensureThumbnailExists(const ImageId& imageId, const QImage& image);

  /**
   * \brief Re-create and replace the existing thumnail.
   *
   * The thumbnail is written for the current size (quality times device pixel ratio). The thumbnail files of the
   * same image with other sizes are deleted, as they show the old image; they are built again when needed.
   *
   * \param imageId The identifier of the full size image and its thumbnail.
   * \param image The full-size image or a thumbnail.
   *
   * \note This function may be called from any thread, even concurrently.
   */
  void recreateThumbnail(const ImageId& imageId, const QImage& image);

 private:
  class Item;
  class Impl;

  std::unique_ptr<Impl> m_impl;
};


#endif  // ifndef SCANTAILOR_CORE_THUMBNAILPIXMAPCACHE_H_
