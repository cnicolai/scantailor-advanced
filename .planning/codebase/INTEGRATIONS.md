---
last_mapped_commit: 5eaac1884cdcabb6514bd632114f688631bd8dbc
last_mapped_at: 2026-10-18
---
# External Integrations

**Analysis Date:** 2026-10-18 (patched after the Phase 3 strip, D-10; not re-mapped)

ScanTailor Advanced is a desktop application that runs entirely locally. It has no runtime network API calls, no hosted backend, and no authentication provider. The integrations below are the file formats and OS services it depends on. CI and distribution services were removed in Phase 3.

## APIs & External Services

**Runtime network services:**
- Not detected. No `QNetworkAccessManager`, `QTcpSocket`, or HTTP client usage was found in `src/`. The only URLs in source are a DTD reference in `src/core/Utils.cpp` (HTML output header) and comment links in `src/core/filters/output/OutputGenerator.cpp`.

**Image and document formats (file I/O, not network):**
- libjpeg - JPEG input and output. Linked via `JPEG::JPEG` in `CMakeLists.txt`.
- libpng - PNG input and output. Linked via `PNG::PNG`.
- libtiff - TIFF input and output, including multi-page TIFF. Used directly through `tiffio.h` in `src/imageproc/` and `src/core/filters/output/`.
- zlib - Linked as `ZLIB::ZLIB` for PNG and TIFF compression.
- Qt XML (`QtXml`, `QDomDocument`) - Reads and writes project XML files. Used in `src/core/` (for example `src/core/Utils.cpp` and the project persistence code).

**Desktop and OS services (via Qt 6):**
- Qt Svg - Renders SVG icons from `src/resources/light_scheme/icons/` and `src/resources/dark_scheme/icons/`.
- Qt OpenGL / OpenGLWidgets - Optional accelerated viewer. Probed in `src/core/OpenGLSupport.cpp`. Stays until Phase 6 (STRIP-07).
- Qt platform plugin - The macOS `cocoa` plugin. The X11/Wayland fallback in `main.cpp` and the `SCANTAILOR_NO_XCB_FALLBACK` variable were removed in Phase 3. The tests select the `offscreen` plugin themselves.

## Data Storage

**Databases:**
- Not applicable. No database client, ORM, or SQL library is used.

**File storage:**
- Local filesystem only. Input scans are read from user-selected paths; output images and project files are written to user-selected directories.
- Project files are XML written with Qt XML (`src/core/`).
- Default parameter profiles are stored under `QStandardPaths::AppDataLocation`, in a `profiles` subfolder: `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles` (`src/core/DefaultParamsProfileManager.cpp`). Portable mode was removed in Phase 3.

**Caching:**
- None detected. Processing results are held in memory and in project files.

## Authentication & Identity

**Auth Provider:**
- Not applicable. The application has no user accounts, login, or tokens.

## Settings Persistence

**QSettings (INI format, set in `src/app/main.cpp`):**
- File: `~/.config/scantailor-advanced/scantailor-advanced.ini`.
- `src/core/ApplicationSettings.cpp` and `src/core/ApplicationSettings.h` - application preferences. The Settings dialog has no Language row (translations removed in Phase 3).
- `src/core/RecentProjects.cpp` - recent project list.
- `src/core/WorkerThreadPool.h` - thread pool settings.
- `src/core/CollapsibleGroupBox.cpp` - UI group state.
- `src/core/filters/page_layout/OptionsWidget.cpp` - filter option persistence.
- The tests keep their own settings in `build/<preset>/test-tmp` and never touch this file.

## Monitoring & Observability

**Error Tracking:**
- None. No crash reporter or telemetry SDK is present.

**Logs:**
- Local output to the console. Developer diagnostics are gated by the `DEVELOPER_VERSION` CMake option (`ENABLE_DEBUG_FEATURES` in `config.h`), which is set in `CMakeLists.txt`.

## CI/CD & Deployment

**Hosting and distribution:**
- None. GitHub Releases, Flatpak, the Debian package, the Windows installer and the portable build were all removed in Phase 3. The fork is built from source and run from `build/<preset>/scantailor-advanced`.

**CI Pipeline:**
- None. The `.github/` directory (five workflows and the Dependabot configuration) was removed in Phase 3. The gate before a commit is a local Release and Debug ctest run, 7 of 7 suites (`TESTING.md`).

## Environment Configuration

**Required environment variables (build time):**
- None. Homebrew is found through `CMAKE_PREFIX_PATH=/opt/homebrew` in `CMakePresets.json`.

**Optional environment variables (runtime):**
- None read by the app. `UPDATE_GOLDENS=1` re-records the golden files of `pipeline_tests` (`TESTING.md`); the tests set `QT_QPA_PLATFORM`, `LC_ALL` and `TMPDIR` themselves.

**Secrets location:**
- Not applicable for the application. No API keys or credentials are used. No CI token exists since Phase 3 removed the GitHub workflows. Environment files were not read in this analysis.

## Webhooks & Callbacks

**Incoming:**
- None.

**Outgoing:**
- None.

---

*Integration audit: 2026-10-18; patched after the Phase 3 strip*
