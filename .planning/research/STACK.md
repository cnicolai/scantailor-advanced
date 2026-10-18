# Technology Stack

**Project:** ScanTailor Advanced (personal macOS fork), port to Apple Silicon macOS
**Dimension:** Stack (toolchain and dependencies)
**Researched:** 2026-10-18
**Overall confidence:** HIGH for versions and CMake/Qt/Boost mechanics, MEDIUM for bare-executable behaviour (needs a 5-minute empirical check in phase 1)

Versions below were read from the Homebrew formulae JSON API (formulae.brew.sh/api) and the Qt, CMake, Boost and LLVM docs on 2026-10-18. No local `brew` was run, as instructed. Re-confirm with `brew info <formula>` before the first build, because Homebrew moves quickly.

The target machine reports Darwin 25.6, which is macOS 26 (Tahoe). Homebrew publishes arm64 bottles for it, so nothing needs compiling from source.

## Recommended Stack

### Toolchain

| Technology | Version (Homebrew, 2026-10-18) | Purpose | Why | Confidence |
|------------|-------------------------------|---------|-----|------------|
| Apple Clang + libc++ (Xcode Command Line Tools, full Xcode not required) | whatever ships with CLT 26.x; run `clang --version` and record it in phase 1 | C++17 compiler | The only compiler Homebrew's Qt and Boost are built against on macOS (Boost is built with `-std=c++17 -stdlib=libc++`). Qt 6.12 asks for "Xcode 16 (macOS 15 SDK) or higher"; CLT 26.x satisfies that. Do not use Homebrew GCC, because the ABI would differ from the libc++ that Qt and Boost use. | HIGH (policy), LOW (exact clang number, not found in sources) |
| CMake | 4.4.4 (`cmake`) | Build system | Qt 6.12 itself requires CMake 3.25+ ("Using Qt now requires CMake 3.25 or later", What's New in Qt 6.12). Homebrew's 4.4.4 is far above that. | HIGH |
| Ninja | 1.13.2 (`ninja`) | Generator | Faster incremental builds than Makefiles, and `compile_commands.json` for clangd. Homebrew's own Qt build uses it. | HIGH |
| macOS SDK | the SDK in CLT 26.x | System headers, zlib | Provides `zlib.h` and `libz`. Leave `CMAKE_OSX_DEPLOYMENT_TARGET` unset so it defaults to the host. Homebrew libraries are built for the host version, and a lower target only produces linker warnings. | HIGH |

C++ standard: stay on C++17 (`CMAKE_CXX_STANDARD 17`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`). Qt 6 needs C++17 or newer, and Homebrew builds Boost with `-std=c++17`. A grep of `src/` found none of the library features that libc++ gates behind `_LIBCPP_ENABLE_CXX17_REMOVED_*` or that C++17 removed: `auto_ptr`, `random_shuffle`, `unary_function`, `binary_function`, `bind1st`, `bind2nd`, `ptr_fun`, `mem_fun_ref`, `std::iterator<>` and `register`. I expect no C++17 compile errors from the language or library. Moving to C++20 is not part of this port, because it would add risk from Boost.Lambda and Qt-era code with no benefit. (Confidence HIGH on the grep, MEDIUM on "no errors", since only a real build proves it.)

### Core Framework (Qt)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Qt 6 via the split formulae `qtbase`, `qtsvg`, `qttools` | 6.12.0 | GUI, XML, network, SVG, OpenGL widgets, QtTest, Linguist tools | Qt 6.12 supports macOS 14.4 and later (including 26) on arm64. All modules the project uses are covered: Core, Gui, Widgets, Xml, Network, OpenGL, OpenGLWidgets and Test live in `qtbase`; `Svg` (module plus the SVG image and icon plugins) is in `qtsvg`; `LinguistTools` (lrelease/lupdate) is in `qttools`. | HIGH |

Do not run `brew install qt`. The `qt` formula is a meta-package of 38 sub-formulae, including `qtwebengine`, `qtmultimedia`, `qtquick3d` and `qtdeclarative`. The project uses none of them. Install `qtbase qtsvg qttools` instead. Add `qttranslations` only if the app should show Qt's own dialog strings in the translated languages (check `Application::installLanguage` in phase 1).

Facts that affect the build:
- Homebrew installs `qtbase` into the Homebrew prefix (`/opt/homebrew`) as macOS frameworks, so the Qt CMake packages are at `/opt/homebrew/lib/cmake/Qt6*`. The plugin directory is `/opt/homebrew/share/qt/plugins`, found relative to the Qt libraries. Nothing like the old Windows `setLibraryPaths` workaround is needed.
- Qt 6 turns high-DPI scaling on unconditionally. The `Qt::AA_EnableHighDpiScaling` and `AA_UseHighDpiPixmaps` block in `src/app/main.cpp` sits inside `#if QT_VERSION_MAJOR == 5`, so it disappears with Qt 5 support. Drawing code that builds its own `QImage`/`QPixmap` buffers still needs `devicePixelRatio` handling (a code task, not a stack choice).
- Declare a floor of `find_package(Qt6 6.8 ...)` and call `qt_standard_project_setup(REQUIRES 6.8)`. The installed version is 6.12.0. 6.8 keeps the build honest about the Qt policy set (QTP00xx) without tying it to the newest minor.
- Qt 6.12 changes nothing in QtWidgets that this project uses. The 6.12 notes mention only macOS-specific items (ScreenCaptureKit for `QScreen::grabWindow`, modernised unified toolbar) that the app does not touch. (HIGH)

### Boost

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Boost (`boost`) | 1.92.0 (also `boost@1.85` as a pinned fallback) | multi_index, intrusive, function, bind, lambda, scoped_array, iterator_facade, and Boost.Test | Homebrew builds Boost with `threading=multi link=shared,static` and `--layout=system`, so both `libboost_unit_test_framework.dylib` and `.a` exist. | HIGH |

Status of the Boost libraries this project uses, checked against the 1.92 release notes (which mention none of them):

| Library | Use in repo | Status in 1.92 | Action |
|---------|-------------|----------------|--------|
| Boost.Bind | 18 files, already `<boost/bind/bind.hpp>` plus `boost::placeholders::_1` | Not removed. The global-placeholder form of `<boost/bind.hpp>` is the deprecated one, and the repo already avoids it. | None. Do not define `BOOST_BIND_GLOBAL_PLACEHOLDERS`. |
| Boost.Lambda | 18 files (`_1`, `if_then_else`, `bind` in `rasterOpGeneric` callers, GaussBlur, tracers) | Not removed in 1.92. No removal announced. | Leave as is for the port. Replacing it with C++ lambdas is optional debt cleanup, file by file, with tests. |
| Boost.Foreach | 26 `#include <boost/foreach.hpp>` lines, but no `BOOST_FOREACH` use anywhere | Not removed. | The includes are dead. Delete them in a cleanup pass. Never a blocker. |
| Boost.Function | 20 includes | Fine | Candidate for `std::function` later. |
| Boost.MultiIndex / Intrusive | several containers | Updated in 1.92 (MultiIndex gained fancy-pointer support, no breaking change) | None. `BOOST_MULTI_INDEX_DISABLE_SERIALIZATION` stays. |
| Boost.Test | 45 test files | Maintained | See below. |

(Confidence: HIGH that none were removed in 1.92, from the official release notes; MEDIUM that Boost.Lambda keeps compiling in every future release, so pin and watch `brew upgrade boost`.)

Boost.Test, static vs shared: both work with the current tests.
- Every test `main.cpp` does `#define BOOST_AUTO_TEST_MAIN` before `<boost/test/unit_test.hpp>`. Boost.Test's `config.hpp` maps this to `BOOST_TEST_MAIN`, so the legacy macro still works. With `BOOST_TEST_DYN_LINK` defined, the header emits `main()` itself, which is what the current layout expects (one TU per executable).
- Choose shared (the default when `Boost_USE_STATIC_LIBS` is not set). It gives smaller test binaries and matches what the config package selects by default. The existing `add_definitions(-DBOOST_TEST_DYN_LINK)` (guarded by `NOT Boost_USE_STATIC_LIBS`) is correct for it. Keep it.
- Static also works (`-DBoost_USE_STATIC_LIBS=ON`) because Homebrew ships the `.a`; there is no reason to prefer it.
- After `brew upgrade boost`, the dylib name and `Boost_DIR` (`/opt/homebrew/lib/cmake/Boost-1.92.0`) change. A stale CMake cache then fails to configure. Fix: `cmake -U Boost_DIR -U 'Boost_*' ...` or delete the build directory. (MEDIUM, from how the config package is versioned)

CMake and Boost: raise the policy, or you will use a removed module. CMake 3.30 introduced policy CMP0167 and stopped shipping the `FindBoost` module in favour of Boost's own `BoostConfig.cmake` (available since Boost 1.70). With `cmake_minimum_required(VERSION 3.9)`, the policy stays unset, so CMake warns and falls back to the old behaviour. Setting the policy to NEW (below) makes `find_package(Boost)` use `BoostConfig.cmake`, which provides `Boost::headers` and `Boost::unit_test_framework`. (HIGH)

### Imaging libraries

| Library | Homebrew formula | Version | Notes | Confidence |
|---------|------------------|---------|-------|------------|
| JPEG | `jpeg-turbo` | 3.2.0 | Not keg-only, found automatically in `/opt/homebrew`. It is also what `qtbase` depends on, so Qt and the app use the same libjpeg and there is no duplicate-symbol hazard. `JpegMetadataLoader.cpp` uses the classic libjpeg API (`jpeg_create_decompress`, custom `jpeg_source_mgr`, `boolean`), which libjpeg-turbo keeps. | HIGH (versions); MEDIUM (compiles unchanged) |
| PNG | `libpng` | 1.6.59 | Not keg-only, no macOS dependencies. Also a `qtbase` dependency. `PngMetadataLoader.cpp` uses the standard setjmp-based read API. | HIGH |
| TIFF | `libtiff` | 4.7.2 | Not keg-only. Depends on jpeg-turbo, webp, xz, zstd. The code uses the legacy typedefs `tsize_t`, `tdata_t`, `toff_t`, `thandle_t`. In libtiff's current `tiffio.h`, `tsize_t` and `tdata_t` are still defined but marked "deprecated and should be replaced". They compile today. Treat them as cleanup debt (`tmsize_t`, `void*`), not as a port blocker. `-DTIFF_DISABLE_DEPRECATED` does not exist, so nothing forces a change. | HIGH (verified in tiffio.h), MEDIUM (a future libtiff may drop them) |
| zlib | none | system (SDK) | The Homebrew `zlib` (1.3.2) is keg-only ("provided by macOS"). CMake's `FindZLIB` finds the SDK's `zlib.h`/`libz.dylib`, which is correct and sufficient. Do not `brew link zlib` or add it to the prefix path. | HIGH |

Do not install the Homebrew `jpeg` formula (libjpeg v10). It is keg-only and "conflicts with `jpeg-turbo`". If it ever gets forced into the prefix, `FindJPEG` could pick the wrong library and the app would carry two libjpegs next to Qt's.

### Development Tools

| Tool | Version | Purpose | Notes | Confidence |
|------|---------|---------|-------|------------|
| CTest (bundled with CMake) | 4.4.4 | Test runner | Existing `add_test` calls work. See "Testing" below. | HIGH |
| Boost.Test | 1.92.0 | Unit tests (math, imageproc, core, foundation) | Keep. Qt Test (`qt_tests`) stays for GUI-adjacent tests. | HIGH |
| clang-format | 23.1.3 (`brew install clang-format`) | Formatting | The repo's `.clang-format` is `BasedOnStyle: Chromium` with many explicit keys, written for clang-format ~10-14. A 2026 binary may reformat large parts of the tree differently. Run it only on files being touched, and check `clang-format --dump-config` loads without errors. Drop the GitHub lint workflow (CI is out of scope). | HIGH (version), MEDIUM (no drift) |
| AddressSanitizer + UBSan | included in Apple Clang | Memory and UB checks | `-fsanitize=address,undefined -fno-omit-frame-pointer` as a separate build type or preset. They work with uninstrumented Homebrew Qt/Boost. See below. | HIGH |
| ThreadSanitizer | included in Apple Clang | Race checks for `WorkerThreadPool`/`BackgroundTask` | Separate build, cannot be combined with ASan. Expect noise from uninstrumented Qt. Use targeted runs of the core/imageproc tests only. | MEDIUM |
| libc++ hardening | `_LIBCPP_HARDENING_MODE` macro | Bounds and precondition checks (`vector::operator[]`, `span`, `optional`) | Add `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE` to Debug builds. libc++ documents it as header-based, and it "mostly takes effect" with the system library. Cheap and a good fit for a codebase with raw image buffers and manual ownership. Set it globally via `add_compile_definitions`, never per target (ODR). | MEDIUM (the macro is documented; its exact effect with Apple's libc++ is not) |
| Homebrew `llvm` | optional | clang-tidy, LeakSanitizer | Only if a leak check is wanted. Apple Clang has no usable LeakSanitizer (`detect_leaks` is unsupported). On macOS use `leaks --atExit -- ./core_tests` or Instruments. Homebrew LLVM LSan on Apple Silicon has reports of spurious results, so treat its output as a lead, not a verdict. | MEDIUM |
| ccache | optional | Rebuild speed | Skip unless full rebuilds hurt. | LOW priority |

## Build-system recommendations (modern CMake)

**Minimum version: `cmake_minimum_required(VERSION 3.25)`.** Reasons: (1) Qt 6.12 itself requires 3.25 to build and to use, so the project cannot meaningfully go lower; (2) CMake 4.0 removed compatibility with minimums below 3.5, and since 3.31 anything below 3.10 prints a deprecation warning, so the current `3.9` is already on the way out; (3) 3.25 gives `CMakePresets.json` schema v6 and `--test-dir`/`--output-on-failure` workflows without extra work. Homebrew provides 4.4.4, so nothing is lost. (HIGH)

Set explicitly:
```cmake
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
if (POLICY CMP0167)               # use BoostConfig.cmake, not the removed FindBoost
  cmake_policy(SET CMP0167 NEW)
endif()
project(ScanTailorAdvanced VERSION 1.2.1 LANGUAGES C CXX)   # no space in the project name
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

Find dependencies by target, not by variable:
```cmake
find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core Gui Widgets Xml Network Svg OpenGL OpenGLWidgets Test LinguistTools)
qt_standard_project_setup(REQUIRES 6.8)   # sets CMAKE_AUTOMOC/AUTOUIC ON, includes GNUInstallDirs
set(CMAKE_AUTORCC ON)                     # not set by qt_standard_project_setup; app/CMakeLists already sets it
find_package(Boost 1.85 CONFIG REQUIRED COMPONENTS unit_test_framework)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
```

- `qt_standard_project_setup()` (Qt 6.3+; the `REQUIRES` argument needs 6.5+) must come right after `find_package(Qt6)` and before targets are defined. It turns on `AUTOMOC`/`AUTOUIC` and includes `GNUInstallDirs`. The project already sets AUTOMOC and AUTOUIC by hand; replace those lines. (HIGH)
- Replace the `Qt_*_lib` variable indirection and `Qt::`/`Qt5::` branches in `CMakeLists.txt`, `src/core/CMakeLists.txt` and the other module files with plain `Qt6::Core Qt6::Gui Qt6::Widgets ...` in `target_link_libraries`. With Qt 5 gone, the `if(Qt6_FOUND)` branches and `QT_BINDIR` can be deleted.
- Replace `include_directories(${Boost_INCLUDE_DIRS})` with `Boost::headers` (a target from `BoostConfig.cmake`). The simplest bridge for a codebase of this size is a top-level `link_libraries(Boost::headers)` or an INTERFACE target linked by `foundation`, `math` and `imageproc`. `BoostConfig.cmake` does not reliably define `Boost_INCLUDE_DIRS`. (MEDIUM, confirm during the first configure)
- Replace the manual `target_include_directories(... $<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>)` in `src/app/CMakeLists.txt` with `target_link_libraries(... TIFF::TIFF)` on the library that includes `tiffio.h` (`core`).
- Translations: `Qt_add_translation(qm_files ...)` is documented as deprecated in Qt 6.12 (still works, no removal date). Keep it in the port; move to `qt_add_translations(TARGETS ... TS_FILES ...)` (stable since 6.7) or `qt_add_lrelease` only as optional cleanup. The `.qm` files are generated into the build root, which is also where the executable lands (`CMAKE_RUNTIME_OUTPUT_DIRECTORY`), so the existing `TRANSLATION_DIRS=".:translations:..."` lookup relative to `applicationDirPath()` works without a bundle.
- Finding Homebrew: CMake has searched `/opt/homebrew` on Apple Silicon by default since 3.19.2 (it is added to `CMAKE_SYSTEM_PREFIX_PATH`), so `find_package(Qt6)` works with no hints. Still pin it in a `CMakePresets.json` with `CMAKE_PREFIX_PATH=/opt/homebrew` so a stray Anaconda, MacPorts or `/usr/local` copy of libpng/libtiff on `PATH` cannot shadow Homebrew. Do not hard-code `/opt/homebrew` in `CMakeLists.txt`. (HIGH for the 3.19.2 behaviour; MEDIUM for the shadowing risk)
- `Boost_USE_STATIC_LIBS`/`Boost_USE_MULTITHREADED`, the MinGW `Boost_ARCHITECTURE`/`Boost_COMPILER` blocks and all `WIN32`/`DEPENDENCIES_ROOT` blocks go away with the Windows removal. They are inert on macOS but misleading.
- Warnings: the existing `-Wall -Wextra -Wpedantic -Werror=return-type -Wno-unused` set is guarded by `CMAKE_COMPILER_IS_GNUCC`, which is false for Apple Clang, so the macOS build currently gets no warnings. Use `target_compile_options(... $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wall -Wextra -Werror=return-type -Wno-unused>)` (via `add_compile_options` at the top level). Expect a one-off wave of warnings. Do not add `-Werror` globally. The GCC-only `-march=native -mtune=native` is also not applied on Clang, which is correct: on arm64 the compiler's default target already uses the Apple Silicon baseline, so no tuning flag is needed.
- **Floating-point reproducibility.** Clang's default is `-ffp-contract=on` (fuse multiply-add within a statement), and on arm64 FMA instructions are always available, so numeric code (spline fitting, dewarping, deskew, Gaussian blur, thresholds) can round differently here than in the x86 builds the user's existing output came from. The project constraint says output must not change unintentionally. Recommendation: build with `-ffp-contract=off` at first (small speed cost), establish the test baseline, then decide in a later phase whether to allow fusion. This is the one non-obvious toolchain setting for this port. (HIGH that the default is `on` per Clang docs; MEDIUM on how many results actually change)
- Packaging: delete `CPack`, `install(...)` of desktop/mime/icon files, `uninstall`, the `flatpak/`, `build-deb.sh`, `build-windows.sh` machinery together with the Linux/Windows removal. `PORTABLE_VERSION` (default ON, stores config in `<appdir>/config`) is a product decision, not a toolchain one. Decide explicitly.
- Sanitizer/Debug preset: `-fsanitize=address,undefined -fno-omit-frame-pointer -g` plus `ASAN_OPTIONS=detect_leaks=0` (leak checking is not supported with Apple Clang). `DEVELOPER_VERSION` already defaults to ON for Debug builds.

## Do we need `MACOSX_BUNDLE`?

**Recommendation: no bundle for now. Run `build/scantailor-advanced` directly, and verify four behaviours in phase 1.**

What Qt does for a plain executable (verified in Qt's source): `QCocoaIntegration` states in its constructor comment that "applications launched from plain executables (without an app bundle) are 'background' applications that do not take keyboard focus or have a dock icon or task switcher entry. Qt Gui apps generally want to be foreground applications so change the process type", and it does so unless `QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM` is set. Qt's app delegate also activates the app ignoring other apps ("we must ignore the terminal"). So window, keyboard focus, Dock icon and Cmd-Tab entry work without a bundle. (HIGH, from `qcocoaintegration.mm` and `qcocoaapplicationdelegate.mm`)

Menus: Qt converts the `QMenuBar` into the native macOS menu bar, creates the application menu, and moves items by `QAction::menuRole`. About, Preferences and Quit work from `QAction::AboutRole`, `PreferencesRole`, `QuitRole` (default `TextHeuristicRole` guesses from the text). Nothing in this is bundle-dependent. Cmd-based shortcuts come from `QKeySequence::StandardKey` (portable) or `Qt::CTRL`, which Qt maps to Cmd on macOS. (HIGH, Qt docs)

What a bare executable loses:
- The app menu title and the Dock label are the executable name (`scantailor-advanced`), not "ScanTailor Advanced", because there is no `CFBundleName`. Cosmetic.
- No generic Dock icon from `Info.plist`; `QApplication::setWindowIcon()` sets the Dock icon at runtime, so that is fixable in code.
- No file-type association or drop-on-Dock-icon (`CFBundleDocumentTypes`, `QFileOpenEvent`). Opening projects from the command line (`args.at(1)`) and from the in-app dialog still work.
- Retina: with no `Info.plist` at all, the process is not subject to the "no NSHighResolutionCapable key means low-resolution mode" behaviour that hits a bundle with an incomplete plist. A secondary source reports unbundled executables render at full resolution. I could not find an official statement. (MEDIUM) **Verify empirically in phase 1**: print `window()->devicePixelRatio()` / `screen()->devicePixelRatio()` (expect 2.0 on a Retina display) and check that a thumbnail and the page view look sharp. If blurry, a bundle fixes it.

When to add a bundle: if the Retina check fails, or the user wants the proper app name, an icon and file association. It costs little in CMake: `qt_add_executable(... MACOSX_BUNDLE)` or `set_target_properties(... MACOSX_BUNDLE ON MACOSX_BUNDLE_INFO_PLIST ...)` with a minimal plist (`NSHighResolutionCapable` = true, `CFBundleName`, `LSMinimumSystemVersion`). The catch is paths: `config.h` resolves translations (`".:translations:..."`) and the portable `config` directory relative to `applicationDirPath()`, which becomes `scantailor-advanced.app/Contents/MacOS`. So a bundle means moving the `.qm` output and the portable-config logic, not just flipping a flag. That is why the recommendation is "not yet", and why it must stay a deliberate later change.

Remove the `WIN32` keyword from `add_executable(${target_name} WIN32 ...)`; it is ignored on macOS but is dead Windows baggage. Use `qt_add_executable`.

## Testing

- **CTest:** keep `enable_testing()` at the top level and the per-module `add_test`. Run with `ctest --test-dir build --output-on-failure -j<N>`. Each Boost.Test binary is one CTest test (`--log_level=message`), so a failure shows only the executable name. Use `--run_test=Suite/Case` by hand to narrow. Do not try per-case CTest discovery (Boost.Test has no `gtest_discover_tests` equivalent).
- **Qt Test (`qt_tests`) and headless runs:** add `set_tests_properties(qt_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")`. Homebrew's `qtbase` ships the `offscreen` platform plugin, so GUI-adjacent tests then run with no window server (over SSH, in scripts). (MEDIUM: the plugin shipped in `qtbase` is standard Qt, but I did not list the Homebrew plugin directory.)
- **Boost.Test link mode:** shared, as above. Set `BOOST_TEST_DYN_LINK` consistently in every test TU, which `add_definitions` already does.
- **Test data:** synthetic images generated in test code or as small in-tree fixtures, per the project constraint. No stack implication.
- **Sanitizers:** run `core_tests`, `imageproc_tests`, `math_tests` under ASan+UBSan in a separate build tree. Uninstrumented Homebrew Qt/Boost libraries are fine with ASan and UBSan; they only add noise under TSan.

## Installation

```bash
# Xcode Command Line Tools (if not present)
xcode-select --install

# Build tools and dependencies (arm64 bottles exist for macOS 15, 26 and 27)
brew install cmake ninja boost jpeg-turbo libpng libtiff qtbase qtsvg qttools

# Optional
brew install clang-format          # formatting touched files only
brew install qttranslations        # only if Qt's own dialog strings should be translated
brew install llvm ccache           # only for clang-tidy / LSan / faster rebuilds

# Do NOT install: qt (meta-formula with 38 packages), jpeg (keg-only, conflicts with jpeg-turbo)

# Configure and build (after the CMake modernisation)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/homebrew \
      -DCMAKE_CXX_FLAGS="-ffp-contract=off"
cmake --build build
ctest --test-dir build --output-on-failure
./build/scantailor-advanced
```

A `CMakePresets.json` (schema v6, CMake 3.25+) with `macos-release`, `macos-debug` and `macos-asan` configure presets, plus a matching test preset, makes these three commands permanent. It needs `generator: Ninja`, `CMAKE_PREFIX_PATH: /opt/homebrew`, and `CMAKE_EXPORT_COMPILE_COMMANDS: ON`.

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Qt install | Homebrew `qtbase qtsvg qttools` | Homebrew `qt` meta-formula | Pulls 38 modules including WebEngine, for nothing the app uses. |
| Qt install | Homebrew Qt 6.12 | Qt online installer / aqtinstall | Not needed; Homebrew is the stated constraint. Reconsider only if Homebrew drops a bottle for the user's macOS. |
| Qt version | Qt 6.12.0 only | Keep Qt 5 (`qt@5` 5.15.17) | `qt@5` has a deprecation date of 2026-05-19, so it is deprecated; a second code path is untested. |
| JPEG | `jpeg-turbo` | `jpeg` (libjpeg 10) | Keg-only, conflicts with `jpeg-turbo`, and Qt links turbo anyway. |
| zlib | SDK zlib | Homebrew `zlib` | Keg-only ("provided by macOS"); the SDK copy is what `FindZLIB` should use. |
| Boost discovery | `BoostConfig.cmake` (policy CMP0167 NEW) | Old `FindBoost` module | Deprecated by CMake itself since 3.30; the module is not shipped in newer versions. |
| Boost.Test | Shared library | Header-only `boost/test/included/` | Would recompile the framework into every test TU; slower builds. Static `.a` works too but gives no benefit. |
| Generator | Ninja | Xcode generator | Xcode project adds nothing for a CLI build; only choose it for IDE debugging. |
| Compiler | Apple Clang | Homebrew LLVM clang | Different libc++ and runtime than Qt/Boost were built with. Use it only for tools (clang-tidy, LSan). |
| Build mode | Bare executable | `MACOSX_BUNDLE` in the build tree | Moves `applicationDirPath()`, which breaks the translation and portable-config lookup. Revisit if the Retina check fails. |
| Test runner | CTest + Boost.Test | Switching to Catch2/GoogleTest | A rewrite of 45 test files for no gain. |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| `brew install qt` | 38 sub-formulae including `qtwebengine` | `qtbase qtsvg qttools` |
| Homebrew `jpeg` (v10) | Keg-only, conflicts with `jpeg-turbo` | `jpeg-turbo` |
| `qt@5` / Qt 5 fallback | Deprecated formula; project decision to drop it | Qt 6 only |
| `cmake_minimum_required(VERSION 3.9)` | Deprecated since 3.31, leaves CMP0167 unset | `3.25` |
| `find_package(Boost ...)` in module mode with an unset CMP0167 | Warning, and the module is being phased out | CMP0167 NEW, `CONFIG` |
| `include_directories(${Boost_INCLUDE_DIRS})` | Not reliably defined by `BoostConfig.cmake` | `Boost::headers` |
| `-march=native` on Apple Silicon | Not needed, Apple Clang's arm64 default is already the baseline | no flag |
| `-fsanitize=leak` / `ASAN_OPTIONS=detect_leaks=1` with Apple Clang | Unsupported | `leaks --atExit`, Instruments |
| Hard-coded Homebrew paths in `CMakeLists.txt` | Breaks on a machine with a different prefix | `CMAKE_PREFIX_PATH` via preset |
| C++20 in this milestone | Extra risk from Boost.Lambda/`result_of`-era code, no payoff | Stay on C++17 |
| `BOOST_BIND_GLOBAL_PLACEHOLDERS` | Re-enables the deprecated global `_1`/`_2` | Keep `boost::placeholders::_1` |

## Stack Patterns by Variant

**If `ImageViewBase` OpenGL rendering stays optional (recommended):**
- Keep `Qt6::OpenGL` and `Qt6::OpenGLWidgets` linked (both in `qtbase`), and default the "hardware acceleration" setting to off on macOS.
- Because OpenGL is deprecated by Apple. A forum post reports it still ships in the macOS 27 betas but may be removed later; this is an unconfirmed rumour (LOW). Qt's `QOpenGLWidget` is supported by Qt but has no Metal path in widget apps. A raster `QPainter` viewport is the safe default on a Mac, and a Metal-era removal would then not break the app. The project should decide in a later phase whether to delete the OpenGL code path (`OpenGLSupport.*`, `ImageViewBase` viewport, settings checkbox).

**If the Retina probe shows blurry rendering from the bare executable:**
- Add a build-tree bundle with a minimal `Info.plist` (`NSHighResolutionCapable` true) and fix the path lookups (see above).

**If `brew upgrade` changes Qt or Boost under an existing build tree:**
- Re-run `cmake -S . -B build` after clearing `Boost_DIR`. Qt minor-version upgrades keep binary compatibility within Qt 6, but Boost's versioned dylib name and CMake directory do not.

## Version Compatibility

| Package | Compatible with | Notes |
|---------|-----------------|-------|
| Qt 6.12.0 | macOS 14.4+ (incl. 26), arm64; Xcode 16 / macOS 15 SDK or newer; CMake 3.25+ | Homebrew bottles: macOS 15, 26, 27 (arm64). No Sonoma bottle. |
| Boost 1.92.0 | Apple Clang, libc++, C++17 | Built `-std=c++17 -stdlib=libc++`. `boost@1.85` exists as a fallback. 1.92 still supports C++03/11; Heap and Lockfree already announce C++17 as their next minimum, and neither is used here. |
| CMake 4.4.4 | `cmake_minimum_required` 3.5+ | Anything below 3.5 is an error since 4.0; below 3.10 warns. `CMAKE_OSX_SYSROOT` defaults to empty since 4.0, which is fine with CLT's Clang. |
| libtiff 4.7.2 | jpeg-turbo 3.2.0, system zlib | `tsize_t`/`tdata_t` deprecated but present. |
| libpng 1.6.59 / jpeg-turbo 3.2.0 | Qt 6.12 (`qtbase` links both) | Same copies for Qt and the app. |
| Xcode 27 / macOS 27 (September 2026) | Qt 6.12, Apple Silicon | If the user moves to it: ld64 and `-ld_classic` are gone (the build does not use them); libc++'s `multimap::find` may no longer return the first matching element, which matters only if code relies on it with `multi_index`/`std::multimap` (not found in a quick scan). Xcode 27 needs macOS 26.6+. The details come from one developer blog quoting Apple (MEDIUM). |

## Open items for phase-specific verification

1. First configure: does `BoostConfig.cmake` expose `Boost::headers` and `Boost::unit_test_framework` and define `BOOST_UNIT_TEST_FRAMEWORK_DYN_LINK` on the target? (The manual `BOOST_TEST_DYN_LINK` covers the gap.)
2. Does `JpegMetadataLoader.cpp` compile unchanged against jpeg-turbo 3.2.0 (the `boolean` return type of source-manager callbacks)? Expected yes.
3. Retina probe on the bare executable (see bundle section).
4. First clean build with `-Wall -Wextra`: size of the warning wave. Not a blocker.
5. `clang-format` 23 against the repo `.clang-format`: diff noise on an untouched file.
6. Capture `clang --version` and `xcodebuild -version` (or CLT version via `pkgutil`) in the phase-1 notes; this research could not find the Apple Clang version number for Xcode 26.x or 27.

## Sources

- Homebrew formulae JSON API, read 2026-10-18 (HIGH, official): `formulae.brew.sh/api/formula/{qt,qtbase,qtsvg,qttools,boost,jpeg-turbo,jpeg,libpng,libtiff,zlib,cmake,ninja,clang-format}.json`
- Homebrew `boost.rb` and `qtbase.rb` formula sources on GitHub (HIGH): build flags (`link=shared,static`, `-std=c++17`), Qt prefix and framework layout
- Qt 6.12 docs (HIGH): macOS platform page (macOS 14.4+, Xcode 16), What's New in Qt 6.12 (CMake 3.25, macOS changes), `qt_standard_project_setup`, CMake get-started, `qt_add_translation` (deprecated), `qt_add_translations`, macOS deployment and issues pages, HighDPI page
- Qt Project development list, May 2026, proposal to raise the minimum CMake for Qt 6.12 to 3.25 (MEDIUM, mailing list; confirmed by the What's New page)
- Qt source `qtbase/src/plugins/platforms/cocoa/qcocoaintegration.mm` and `qcocoaapplicationdelegate.mm` (HIGH): foreground transform for plain executables, activation
- CMake docs (HIGH): CMP0167, `cmake_minimum_required`, 4.0 release notes, policy list; CMake issue tracker and 3.19.2 release notes for `/opt/homebrew` prefix search (MEDIUM, via search result)
- Boost 1.92.0 release page (HIGH); Boost.Test source `config.hpp`, `unit_test.hpp` and usage-variants docs (HIGH)
- libtiff `tiffio.h` on GitLab (HIGH); libc++ Hardening docs (HIGH); Clang users manual for `-ffp-contract` (HIGH)
- LLVM discourse, MacPorts and Squeak threads on LeakSanitizer on Apple platforms (MEDIUM)
- Single developer blog quoting Apple's Xcode 27 notes (LOW to MEDIUM); a forum post on OpenGL in macOS 27 (LOW)
- Project files read: `.planning/PROJECT.md`, `.planning/codebase/STACK.md`, `CMakeLists.txt`, `src/app/CMakeLists.txt`, `src/*/tests/CMakeLists.txt`, `src/app/main.cpp`, and greps over `src/` for Boost, Qt-version, TIFF/JPEG/PNG and C++17-removed-feature usage

---
*Stack research for: porting a C++17/Qt image-processing desktop app to macOS arm64*
*Researched: 2026-10-18*
