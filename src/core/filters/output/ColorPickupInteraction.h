// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_COLORPICKUPINTERACTION_H_
#define SCANTAILOR_OUTPUT_COLORPICKUPINTERACTION_H_

#include <QColor>
#include <QCoreApplication>
#include <QImage>
#include <QRect>
#include <cstdint>
#include <memory>

#include "EditableZoneSet.h"
#include "FillColorProperty.h"
#include "InteractionHandler.h"
#include "InteractionState.h"

class ZoneInteractionContext;
class QShortcut;

namespace output {
class ColorPickupInteraction : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ColorPickupInteraction)
 public:
  ColorPickupInteraction(EditableZoneSet& zones, ZoneInteractionContext& context);

  ~ColorPickupInteraction() override;

  void startInteraction(const EditableZoneSet::Zone& zone, InteractionState& interaction);

  bool isActive(const InteractionState& interaction) const;

  /**
   * Median colour of the pixels inside the circle inscribed in the grabbed area, counted by logical pixels: a device
   * pixel takes part when its logical pixel lies in the circle of the ratio 1 algorithm, so at ratio 1 the result
   * equals the old one and at ratio 2 each logical pixel contributes its 2x2 block. Returns an invalid colour for a
   * null image or an empty sample.
   */
  static QColor medianColor(const QImage& grabbed);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  void takeColor();

  QRect targetBoundingRect() const;

  void switchToDefaultInteraction();

  static uint32_t bitMixColor(uint32_t color);

  static uint32_t bitUnmixColor(uint32_t mixed);

  EditableZoneSet& m_zones;
  ZoneInteractionContext& m_context;
  InteractionState::Captor m_interaction;
  std::shared_ptr<FillColorProperty> m_fillColorProp;
  int m_dontDrawCircle;
  std::unique_ptr<QShortcut> m_cancelShortcut;

  static const uint32_t m_sBitMixingLUT[3][256];
  static const uint32_t m_sBitUnmixingLUT[3][256];
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_COLORPICKUPINTERACTION_H_
