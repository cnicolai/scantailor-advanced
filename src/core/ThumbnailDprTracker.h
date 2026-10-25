// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_THUMBNAILDPRTRACKER_H_
#define SCANTAILOR_CORE_THUMBNAILDPRTRACKER_H_

#include <QObject>
#include <memory>

class QAbstractScrollArea;
class QEvent;
class ThumbnailPixmapCache;

/**
 * \brief Keeps a thumbnail cache at the device pixel ratio of the view that shows it.
 *
 * Worker threads write the thumbnail files at the size the cache has for its ratio, so the cache has to hear about
 * a display change (the window moved to a display with another ratio) before the view asks for pixmaps again.
 * The tracker watches the view for QEvent::DevicePixelRatioChange, hands the new ratio to the cache and repaints
 * the viewport, which makes the thumbnails request their new pixmaps (HIDPI-05).
 *
 * The tracker is owned by the view through the Qt parent. It holds the cache weakly, so a cache that the main window
 * replaces on a project switch is freed.
 */
class ThumbnailDprTracker : public QObject {
  Q_OBJECT

 public:
  explicit ThumbnailDprTracker(QAbstractScrollArea* view);

  /// Remembers the cache and sets the view's current ratio on it at once. A null cache is allowed.
  void setCache(const std::shared_ptr<ThumbnailPixmapCache>& cache);

 protected:
  /// Never consumes the event.
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  QAbstractScrollArea* m_view;
  std::weak_ptr<ThumbnailPixmapCache> m_cache;
};

#endif  // SCANTAILOR_CORE_THUMBNAILDPRTRACKER_H_
