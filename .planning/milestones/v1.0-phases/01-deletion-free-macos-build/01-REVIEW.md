---
phase: 01-deletion-free-macos-build
reviewed: 2026-10-23T00:00:00Z
depth: standard
files_reviewed: 2
files_reviewed_list:
  - CMakeLists.txt
  - CMakePresets.json
findings:
  critical: 0
  warning: 4
  info: 4
  total: 8
status: issues_found
---

# Phase 1: Code Review Report

**Reviewed:** 2026-10-23
**Depth:** standard
**Files Reviewed:** 2
**Status:** issues_found

## Summary

I reviewed the `CMakeLists.txt` diff against 5eaac18 and the new `CMakePresets.json`. I checked the generated `compile_commands.json` of the existing `build/macos-release` and `build/macos-asan` trees. These are read-only reads inside the project. No builds or tests were run.

The core mechanics are correct:

- The Clang flag block is outside the `DEFAULT_COMPILER_FLAGS_IS_SET` cache guard, so it applies on every configure.
- The quoted `$<COMPILE_LANGUAGE:C,CXX>` generator expression expands correctly.
- The release command line contains `-O3 -DNDEBUG ... -Wall -Wextra -Werror=return-type -Wno-unused -ffp-contract=off`.
- The asan command line contains `-g ... -ffp-contract=off -fsanitize=address,undefined -fno-omit-frame-pointer -fsanitize-recover=address`.
- `-ffp-contract=off` comes after `CMAKE_CXX_FLAGS` and the config flags, so it wins over any user-supplied contraction flag.
- Sanitizer flags do not leak into the release preset (`SANITIZE` is empty there).
- Preset inheritance and `configurePreset` references resolve.

No blockers. The main concerns are:

- The asan test preset cannot fail a test run on a sanitizer finding.
- `-ffp-contract=off` changes numeric behaviour relative to the Apple Clang default.
- The `SANITIZE` plumbing is unvalidated.
- The Boost handling relies on FindBoost-only variables that are inert under CMP0167 NEW.

## Warnings

### WR-01: `macos-asan` test preset cannot gate on sanitizer findings

**File:** `CMakePresets.json:84-87` (with `CMakeLists.txt:77-79`)
**Issue:** The preset combines these settings:

- `-fsanitize-recover=address` at compile time.
- `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0`.
- UBSan in its default recoverable mode (no `-fno-sanitize-recover=undefined`, no `UBSAN_OPTIONS=halt_on_error=1`).

With this combination, ASan and UBSan only print reports. The process still exits 0 unless the test itself fails. `ctest --preset macos-asan` will therefore report green even when a sanitizer finds a heap overflow or UB. `outputOnFailure` only prints output for tests that already failed, so the reports can disappear into passing-test output. The comment in `CMakeLists.txt` ("so one run collects every report") shows this is deliberate for collection. As a phase gate or later regression guard, though, it is a silent pass. `detect_leaks=0` also removes the only exit-code-affecting ASan check on macOS.
**Fix:** Keep the collect-everything behaviour only in a separate opt-in preset, for example `macos-asan-collect`. For the default `macos-asan` test preset, either make findings fatal or fail on report text:
```json
"environment": {
  "ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
  "UBSAN_OPTIONS": "print_stacktrace=1:halt_on_error=1"
}
```
Then drop `-fsanitize-recover=address`, or add `-fno-sanitize-recover=undefined` for UBSan. If recovery is kept, add a CTest `FAIL_REGULAR_EXPRESSION` such as `ERROR: AddressSanitizer|runtime error:` on the tests.

### WR-02: `-ffp-contract=off` is not the Apple Clang default, so output may differ from the unmodified build

**File:** `CMakeLists.txt:68-69`
**Issue:** Apple Clang on arm64 defaults to `-ffp-contract=on`. That setting fuses multiply-add within a statement into FMA, and it is active in the pre-phase `build/unmodified` tree. Forcing `off` in every build type is a reasonable determinism choice. It does change floating-point results in the last bit (FMA versus separate multiply and add) in the deskew, dewarp, scale and transform code. This conflicts with the project constraint "Output for existing settings should not change unintentionally". The change is invisible in review because nothing in the diff or the tests compares output. The comment says the freeze is "fixed before Phase 2 baselines", so the baselines will be self-consistent. Nothing in this phase checks whether the user-visible output of the fork still matches the reference build for real pages. The flag is also Clang-only: a GCC build keeps its default `-ffp-contract=fast` under `gnu++17`, so the comment "results do not depend on build type" holds only for Clang.
**Fix:** Do a one-off comparison of output TIFFs from `build/unmodified` and `build/macos-release` on a synthetic project, and record the result (identical, or the bounded difference) in the phase notes. Limit the comment to say it applies to Clang only.

### WR-03: `SANITIZE` is injected without validation or quoting

**File:** `CMakeLists.txt:72-80`
**Issue:**
- `-fsanitize=${SANITIZE}` is unquoted. A value such as `address;undefined` (list syntax) becomes two arguments, the second one a bare `undefined` that the compiler treats as a file or flag. A value containing a space breaks too.
- The block is not conditioned on the compiler. With MSVC or an unsupported toolchain, `-fsanitize=...` produces an obscure error.
- `SANITIZE MATCHES "address"` is a substring match, so `kernel-address` and `hwaddress` also trigger `-fsanitize-recover=address`.
- The flags are applied to every target, including the Qt autogen helper targets. They are not applied to `try_compile` and `find_package` probes. That is usually fine but untested here.
**Fix:**
```cmake
if (SANITIZE)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "SANITIZE is only supported with Clang or GCC.")
  endif()
  string(REPLACE ";" "," _sanitize "${SANITIZE}")
  add_compile_options("-fsanitize=${_sanitize}" -fno-omit-frame-pointer)
  add_link_options("-fsanitize=${_sanitize}")
  if (_sanitize MATCHES "(^|,)address(,|$)")
    add_compile_options(-fsanitize-recover=address)
  endif()
endif()
```

### WR-04: Boost handling still relies on FindBoost-only variables under CMP0167 NEW

**File:** `CMakeLists.txt:17-19`, `219-238`
**Issue:** CMP0167 NEW removes CMake's `FindBoost` module, so `find_package(Boost ...)` now uses Boost's own `BoostConfig.cmake`. `Boost_USE_MULTITHREADED` (line 221) and `Boost_USE_STATIC_LIBS` (line 219, Windows only) are FindBoost inputs and are inert in config mode. The `-DBOOST_TEST_DYN_LINK` define on line 236 is keyed to `Boost_USE_STATIC_LIBS` being unset. On macOS the define is therefore always added, regardless of whether `Boost::unit_test_framework` resolves to the shared or the static variant. It currently matches in the existing build (408 compile commands carry the define, and Homebrew Boost provides shared libraries). The match is accidental. If only a static Boost were found, the define would cause a mismatched test-main or link error. The commented-out `Boost_NO_WARN_NEW_VERSIONS` (line 194) is dead under the new policy.
**Fix:** Drive the define from the imported target and remove the dead variables:
```cmake
if (BUILD_TESTS)
  get_target_property(_ut_type Boost::unit_test_framework TYPE)
  if (_ut_type STREQUAL "SHARED_LIBRARY")
    add_definitions(-DBOOST_TEST_DYN_LINK)
  endif()
endif()
```
Alternatively, set `Boost_USE_STATIC_LIBS`/`BUILD_SHARED_LIBS` explicitly for macOS and document why.

## Info

### IN-01: `cmake_minimum_required(3.25)` leaves stale docs and non-macOS CI

**File:** `CMakeLists.txt:1`
**Issue:** Raising the minimum to 3.25 matches `CMakePresets.json` version 6 and is correct for the Mac target. Project documentation still says "CMake 3.9+". The Ubuntu 22.04 CI and Flatpak workflows probably cannot satisfy 3.25 from apt (22.04 ships 3.22). I did not check the workflow files, as they are out of this review's scope. Windows and Linux builds are a non-target for this fork, but those workflows will now fail at configure time.
**Fix:** Update the "CMake 3.9+" statement in the docs. Disable or pin the CI jobs that cannot reach 3.25.

### IN-02: Presets omit architecture and host-condition guards, and `PORTABLE_VERSION` is untyped

**File:** `CMakePresets.json:10-20`
**Issue:**
- `CMAKE_PREFIX_PATH=/opt/homebrew` is hardcoded and the presets have no `condition` on `${hostSystemName}`. They show up as valid on any host and fail in `find_package` there.
- There is no `CMAKE_OSX_ARCHITECTURES`. A shell running under Rosetta builds x86_64 against arm64-only Homebrew libraries and fails at link time. The current trees show `-arch arm64`, so this is not a present bug.
- `PORTABLE_VERSION`, `BUILD_TESTS` and `CMAKE_EXPORT_COMPILE_COMMANDS` are untyped strings. They work because `option()` leaves the cached value alone, but `{"type": "BOOL", "value": "OFF"}` is clearer and avoids a STRING-typed cache entry for a BOOL option.
**Fix:** Add `"condition": {"type": "equals", "lhs": "${hostSystemName}", "rhs": "Darwin"}` to `macos-base`. Add `"CMAKE_OSX_ARCHITECTURES": "arm64"`. Use typed cache variables for the booleans.

### IN-03: `macos-debug` and `macos-asan` differ from `macos-release` beyond optimisation

**File:** `CMakeLists.txt:321-325` (with `CMakePresets.json:29-43`)
**Issue:** `DEVELOPER_VERSION` defaults to ON when `CMAKE_BUILD_TYPE` is Debug, so the debug and asan presets define `ENABLE_DEBUG_FEATURES`. The current `build/*/config.h` files confirm this; the only use is in `src/app/MainWindow.cpp`. Debug builds also lack `-DQT_NO_DEBUG` and `-O3`. Output baselines or behaviour comparisons taken from the debug or asan presets are not representative of the release build.
**Fix:** Take Phase 2 output baselines from `macos-release`, and note this in the phase notes. If sanitizer runs should match the release pipeline, set `"DEVELOPER_VERSION": "OFF"` in `macos-asan`.

### IN-04: Comments cite decision IDs that are not resolvable in the source tree

**File:** `CMakeLists.txt:65`, `68`
**Issue:** The comments reference `D-08` and `D-03` and "Phase 2 baselines". These point to planning artifacts, which are not in the source tree and can drift. The Clang block also checks only `CMAKE_CXX_COMPILER_ID` while adding options for C as well, so a mixed C/C++ toolchain would get the wrong C flags.
**Fix:** State the rationale inline ("-Wpedantic omitted: Qt/Boost headers are not pedantic-clean under Clang"). Optionally gate on `CMAKE_C_COMPILER_ID` as well.

---

_Reviewed: 2026-10-23_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
