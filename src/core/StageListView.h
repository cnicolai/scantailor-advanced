// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_STAGELISTVIEW_H_
#define SCANTAILOR_CORE_STAGELISTVIEW_H_

#include <QColor>
#include <QPixmap>
#include <QTableView>
#include <memory>
#include <vector>

class StageSequence;

class StageListView : public QTableView {
  Q_OBJECT
 public:
  explicit StageListView(QWidget* parent);

  ~StageListView() override;

  void setStages(const std::shared_ptr<StageSequence>& stages);

  QSize sizeHint() const override { return m_sizeHint; }

  /**
   * \brief Renders the frames of the batch processing animation.
   *
   * Each frame is squareSide logical pixels wide and high, carries the device pixel ratio dpr and so has
   * squareSide * dpr pixels per side. The bubbles are drawn into the logical rect of the frame.
   */
  static std::vector<QPixmap> createBatchAnimationFrames(int squareSide,
                                                         double dpr,
                                                         const QColor& headColor,
                                                         const QColor& tailColor);

  /// Diagnostics for tests and the live checks only; the application does not call them.
  /// The frames currently in use.
  const std::vector<QPixmap>& batchAnimationFrames() const { return m_batchAnimationPixmaps; }

 signals:

  void launchBatchProcessing();

 public slots:

  void setBatchProcessingPossible(bool possible);

  void setBatchProcessingInProgress(bool inProgress);

 protected slots:

  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;

 private slots:

  void ensureSelectedRowVisible();

 protected:
  void timerEvent(QTimerEvent* event) override;

  bool event(QEvent* event) override;

 private:
  class Model;
  class LeftColDelegate;

  class RightColDelegate;

  void removeLaunchButton(int row);

  void placeLaunchButton(int row);

  void initiateBatchAnimationFrameRendering();

  void createBatchAnimationSequence(int squareSide);

  void updateRowSpans();

  int selectedRow() const;

  QSize m_sizeHint;
  Model* m_model;
  LeftColDelegate* m_firstColDelegate;
  RightColDelegate* m_secondColDelegate;
  QWidget* m_launchBtn;
  std::vector<QPixmap> m_batchAnimationPixmaps;
  int m_batchAnimationSquareSide = 0;  // logical pixels; 0 until the frames exist
  double m_batchAnimationDpr = 1.0;    // ratio of the frames
  int m_curBatchAnimationFrame;
  int m_timerId;
  bool m_batchProcessingPossible;
  bool m_batchProcessingInProgress;
};


#endif  // ifndef SCANTAILOR_CORE_STAGELISTVIEW_H_
