// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "WorkerThreadPool.h"

#include <QCoreApplication>
#include <QThread>
#include <QThreadPool>
#include <algorithm>
#include <utility>

#include "OutOfMemoryHandler.h"

class WorkerThreadPool::TaskResultEvent : public QEvent {
 public:
  TaskResultEvent(BackgroundTaskPtr task, FilterResultPtr result)
      : QEvent(User), m_task(std::move(task)), m_result(std::move(result)) {}

  const BackgroundTaskPtr& task() const { return m_task; }

  const FilterResultPtr& result() const { return m_result; }

 private:
  BackgroundTaskPtr m_task;
  FilterResultPtr m_result;
};


WorkerThreadPool::WorkerThreadPool(QObject* parent) : QObject(parent), m_pool(new QThreadPool(this)) {
  updateNumberOfThreads();
}

WorkerThreadPool::~WorkerThreadPool() = default;

void WorkerThreadPool::shutdown() {
  m_pool->waitForDone();
}

bool WorkerThreadPool::hasSpareCapacity() const {
  return m_pool->activeThreadCount() < m_pool->maxThreadCount();
}

void WorkerThreadPool::submitTask(const BackgroundTaskPtr& task) {
  class Runnable : public QRunnable {
   public:
    Runnable(WorkerThreadPool& owner, BackgroundTaskPtr task) : m_owner(owner), m_task(std::move(task)) {
      setAutoDelete(true);
    }

    void run() override {
      if (m_task->isCancelled()) {
        return;
      }

      try {
        const FilterResultPtr result((*m_task)());
        if (result) {
          QCoreApplication::postEvent(&m_owner, new TaskResultEvent(m_task, result));
        }
      } catch (const std::bad_alloc&) {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();
      }
    }

   private:
    WorkerThreadPool& m_owner;
    BackgroundTaskPtr m_task;
  };


  updateNumberOfThreads();
  m_pool->start(new Runnable(*this, task));
}  // WorkerThreadPool::submitTask

void WorkerThreadPool::customEvent(QEvent* event) {
  if (auto* evt = dynamic_cast<TaskResultEvent*>(event)) {
    emit taskResult(evt->task(), evt->result());
  }
}

void WorkerThreadPool::setMemoryCap(const int cap) {
  m_memoryCap = cap;
  updateNumberOfThreads();
}

int WorkerThreadPool::maxThreadCount() const {
  return m_pool->maxThreadCount();
}

void WorkerThreadPool::updateNumberOfThreads() {
  const int ideal = QThread::idealThreadCount();
  const int slider = storedThreadCount(m_settings, ideal);
  m_pool->setMaxThreadCount(MemoryBudget::effectiveThreads(ideal, slider, m_memoryCap));
}

QString WorkerThreadPool::threadCountKey() {
  return QStringLiteral("settings/batch_processing_threads");
}

int WorkerThreadPool::storedThreadCount(const QSettings& settings, const int maxThreads) {
  return std::clamp(settings.value(threadCountKey(), maxThreads).toInt(), 1, std::max(1, maxThreads));
}
