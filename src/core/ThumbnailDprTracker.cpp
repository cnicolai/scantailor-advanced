// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ThumbnailDprTracker.h"

#include <QAbstractScrollArea>
#include <QEvent>
#include <QWidget>

#include "ThumbnailPixmapCache.h"

ThumbnailDprTracker::ThumbnailDprTracker(QAbstractScrollArea* view) : QObject(view), m_view(view) {
  m_view->installEventFilter(this);
}

void ThumbnailDprTracker::setCache(const std::shared_ptr<ThumbnailPixmapCache>& cache) {
  m_cache = cache;
  if (cache) {
    cache->setDevicePixelRatio(m_view->devicePixelRatioF());
  }
}

bool ThumbnailDprTracker::eventFilter(QObject* watched, QEvent* event) {
  if (watched == m_view && event->type() == QEvent::DevicePixelRatioChange) {
    // setDevicePixelRatio() does nothing for an equal ratio, so repeated events are harmless.
    if (const std::shared_ptr<ThumbnailPixmapCache> cache = m_cache.lock()) {
      cache->setDevicePixelRatio(m_view->devicePixelRatioF());
    }
    m_view->viewport()->update();
  }
  return false;
}
