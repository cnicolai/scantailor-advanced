// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_PROCESSMEMORY_H_
#define SCANTAILOR_TEST_SUPPORT_PROCESSMEMORY_H_

#include <mach/mach.h>

#include <QtGlobal>
#include <atomic>
#include <chrono>
#include <thread>

namespace test_support {

// Footprint of the process in bytes as macOS accounts it (compressed and swapped pages included), or -1 if the
// kernel call fails.
inline qint64 physFootprintBytes() {
  task_vm_info_data_t info;
  mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) {
    return -1;
  }
  return qint64(info.phys_footprint);
}

// The peak is a high-water mark, so a transient allocation shows even after it is freed.
inline qint64 peakPhysFootprintBytes() {
  task_vm_info_data_t info;
  mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) {
    return -1;
  }
  return qint64(info.ledger_phys_footprint_peak);
}

// Watches the current footprint (not the high-water mark) of the whole process from a sampler thread while the
// owner runs the code under test. maxGrowthBytes() is the largest sample above the footprint at construction, so
// a refusal test cannot pass vacuously because an earlier test case left a high peak behind. Samples come every
// millisecond and once more when the sampler stops, with the results of the call still alive. Other threads of
// the process (a thumbnail thread, for example) are covered; allocations that are made and freed between two
// samples are not, which is why the callers also bound the time of the call.
class FootprintSampler {
 public:
  FootprintSampler() : m_baseline(physFootprintBytes()), m_max(m_baseline), m_thread([this] { run(); }) {}

  FootprintSampler(const FootprintSampler&) = delete;
  FootprintSampler& operator=(const FootprintSampler&) = delete;

  ~FootprintSampler() { stop(); }

  // False if the kernel call failed, so the growth means nothing.
  bool valid() const { return m_baseline >= 0; }

  // Stops the sampler (idempotent) and returns the largest footprint seen above the start.
  qint64 stop() {
    if (m_thread.joinable()) {
      m_stop = true;
      m_thread.join();
      raiseMax(physFootprintBytes());
    }
    return m_max - m_baseline;
  }

 private:
  void run() {
    while (!m_stop) {
      raiseMax(physFootprintBytes());
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void raiseMax(const qint64 sample) {
    qint64 seen = m_max;
    while (sample > seen && !m_max.compare_exchange_weak(seen, sample)) {
    }
  }

  const qint64 m_baseline;
  std::atomic<qint64> m_max;
  std::atomic<bool> m_stop{false};
  std::thread m_thread;
};

}  // namespace test_support

#endif  // ifndef SCANTAILOR_TEST_SUPPORT_PROCESSMEMORY_H_
