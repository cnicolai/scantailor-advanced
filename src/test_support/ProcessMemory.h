// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_PROCESSMEMORY_H_
#define SCANTAILOR_TEST_SUPPORT_PROCESSMEMORY_H_

#include <mach/mach.h>

#include <QtGlobal>

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

}  // namespace test_support

#endif  // ifndef SCANTAILOR_TEST_SUPPORT_PROCESSMEMORY_H_
