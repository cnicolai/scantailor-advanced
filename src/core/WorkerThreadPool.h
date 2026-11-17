// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_WORKERTHREADPOOL_H_
#define SCANTAILOR_CORE_WORKERTHREADPOOL_H_

#include <QObject>
#include <QSettings>
#include <QString>
#include <memory>

#include "BackgroundTask.h"
#include "FilterResult.h"
#include "MemoryBudget.h"

class QThreadPool;

class WorkerThreadPool : public QObject {
  Q_OBJECT
 public:
  explicit WorkerThreadPool(QObject* parent = nullptr);

  ~WorkerThreadPool() override;

  /**
   * \brief Waits for pending jobs to finish and stop the thread.
   *
   * The destructor also performs these tasks, so this method is only
   * useful to prematuraly stop task processing.
   */
  void shutdown();

  bool hasSpareCapacity() const;

  /**
   * \brief Limits the number of tasks that run at once by memory, on top of the thread setting.
   *
   * The stored thread setting is never changed. Pass MemoryBudget::kNoCap to lift the cap.
   */
  void setMemoryCap(int cap);

  /** The number of tasks that may run at once: the smaller of the processor count, the setting and the cap. */
  int maxThreadCount() const;

  void submitTask(const BackgroundTaskPtr& task);

  /// The settings key of the batch thread slider; the one place that names it.
  static QString threadCountKey();

  /// The slider value stored in the settings, clamped to [1, maxThreads]; maxThreads if none is stored.
  static int storedThreadCount(const QSettings& settings, int maxThreads);

 signals:

  void taskResult(const BackgroundTaskPtr& task, const FilterResultPtr& result);

 private:
  class TaskResultEvent;

  void customEvent(QEvent* event) override;

  void updateNumberOfThreads();

  QThreadPool* m_pool;
  QSettings m_settings;
  int m_memoryCap = MemoryBudget::kNoCap;
};


#endif  // ifndef SCANTAILOR_CORE_WORKERTHREADPOOL_H_
