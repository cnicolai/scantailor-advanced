// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_OUTPUT_PICTUREZONEEDITOR_H_
#define SCANTAILOR_OUTPUT_PICTUREZONEEDITOR_H_

#include <BinaryImage.h>

#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QTimer>
#include <QTransform>
#include <memory>

#include "DragHandler.h"
#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "ImagePixmapUnion.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "ZoneEditorBase.h"
#include "ZoneInteractionContext.h"
#include "ZoomHandler.h"

class ImageTransformation;
class InteractionState;
class QPainter;
class QMenu;

namespace output {
class Settings;


class PictureZoneEditor : public ZoneEditorBase {
  Q_OBJECT
 public:
  PictureZoneEditor(const QImage& image,
                    const ImagePixmapUnion& downscaledImage,
                    const imageproc::BinaryImage& pictureMask,
                    const QTransform& imageToVirt,
                    const QPolygonF& virtDisplayArea,
                    const PageId& pageId,
                    std::shared_ptr<Settings> settings);

  ~PictureZoneEditor() override;

  // Diagnostics for tests and the live checks only; the application does not call them.
  const QPixmap& screenPictureMask() const { return m_screenPictureMask; }

  bool screenPictureMaskIsCurrent() const { return validateScreenPictureMask(); }

 signals:

  void invalidateThumbnail(const PageId& pageId);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

 private slots:

  void advancePictureMaskAnimation();

  void initiateBuildingScreenPictureMask();

  void commitZones();

  void updateRequested();

 private:
  class MaskTransformTask;

  bool validateScreenPictureMask() const;

  void schedulePictureMaskRebuild();

  /// \p origin is in device pixels, the mask carries its device pixel ratio.
  void screenPictureMaskBuilt(const QPoint& origin, const QImage& mask);

  void paintOverPictureMask(QPainter& painter);

  void showPropertiesDialog(const EditableZoneSet::Zone& zone);

  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;

  imageproc::BinaryImage m_origPictureMask;
  QPixmap m_screenPictureMask;
  QPointF m_screenPictureMaskOrigin;  // logical widget coordinates
  QTransform m_screenPictureMaskXform;
  double m_screenPictureMaskDpr = 1.0;
  QTransform m_potentialPictureMaskXform;
  QTimer m_pictureMaskRebuildTimer;
  QTimer m_pictureMaskAnimateTimer;
  int m_pictureMaskAnimationPhase;  // degrees
  std::shared_ptr<MaskTransformTask> m_maskTransformTask;

  PageId m_pageId;
  std::shared_ptr<Settings> m_settings;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_PICTUREZONEEDITOR_H_
