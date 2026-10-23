// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TEST_SUPPORT_RNG_H_
#define SCANTAILOR_TEST_SUPPORT_RNG_H_

#include <cstdint>

namespace test_support {

/**
 * splitmix64 pseudo random generator.  Its output is fixed by the constants below, unlike the
 * standard library distributions, so generated test pages are identical on every toolchain.
 */
class Rng {
 public:
  explicit Rng(uint64_t seed) : m_state(seed) {}

  uint64_t next() {
    uint64_t z = (m_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }

  /// Value in [0, n).  The slight modulo bias is irrelevant for test pages.
  uint32_t below(uint32_t n) { return static_cast<uint32_t>(next() % n); }

 private:
  uint64_t m_state;
};

}  // namespace test_support

#endif  // SCANTAILOR_TEST_SUPPORT_RNG_H_
