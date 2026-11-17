// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_SYSTEMLOADWIDGET_H_
#define SCANTAILOR_APP_SYSTEMLOADWIDGET_H_

#include <QString>
#include <QWidget>

#include "MemoryBudget.h"
#include "ui_SystemLoadWidget.h"

class SystemLoadWidget : public QWidget {
  Q_OBJECT
 public:
  explicit SystemLoadWidget(QWidget* parent = nullptr);

  /**
   * \brief Tells the slider how many tasks memory allows at once.
   *
   * Only the tooltip changes: the slider range and the stored setting stay as they are.
   */
  void setMemoryCap(int cap);

  /// "<threads>/<max>", followed by ", limited to <cap> by memory" when the memory cap is below threads.
  QString toolTipText(int threads) const;

 private slots:

  void sliderPressed();

  void sliderMoved(int threads);

  void valueChanged(int threads);

  void decreaseLoad();

  void increaseLoad();

 private:
  void showHideToolTip(int threads);

  void setupIcons();

  Ui::SystemLoadWidget ui;
  int m_maxThreads;
  int m_memoryCap = MemoryBudget::kNoCap;
};


#endif  // ifndef SCANTAILOR_APP_SYSTEMLOADWIDGET_H_
