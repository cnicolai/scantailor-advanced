---
last_mapped_commit: 5eaac1884cdcabb6514bd632114f688631bd8dbc
last_mapped_at: 2026-10-18
---
# External Integrations

**Analysis Date:** 2026-10-18

ScanTailor Advanced is a desktop application that runs entirely locally. It has no runtime network API calls, no hosted backend, and no authentication provider. The integrations below are the file formats, OS services, and CI/distribution services it depends on.

## APIs & External Services

**Runtime network services:**
- Not detected. No `QNetworkAccessManager`, `QTcpSocket`, or HTTP client usage was found in `src/`. The only URLs in source are a DTD reference in `src/core/Utils.cpp` (HTML output header) and comment links in `src/core/filters/output/OutputGenerator.cpp`.

**Image and document formats (file I/O, not network):**
- libjpeg - JPEG input and output. Linked via `JPEG::JPEG` in `CMakeLists.txt`.
- libpng - PNG input and output. Linked via `PNG::PNG`.
- libtiff - TIFF input and output, including multi-page TIFF. Used directly through `tiffio.h` in `src/imageproc/` and `src/core/filters/output/`.
- zlib - Linked as `ZLIB::ZLIB` for PNG and TIFF compression.
- Qt XML (`QtXml`, `QDomDocument`) - Reads and writes project XML files. Used in `src/core/` (for example `src/core/Utils.cpp` and the project persistence code).

**Desktop and OS services (via Qt):**
- Qt Svg - Renders SVG icons from `src/resources/light_scheme/icons/` and `src/resources/dark_scheme/icons/`.
- Qt OpenGL / OpenGLWidgets - Optional accelerated viewer. Probed in `src/core/OpenGLSupport.cpp`.
- Qt platform plugins - Chosen at runtime. On Linux with Wayland under Qt5, the app forces `xcb` unless `QT_QPA_PLATFORM` is set or `SCANTAILOR_NO_XCB_FALLBACK=1` is set (`README.md`).

## Data Storage

**Databases:**
- Not applicable. No database client, ORM, or SQL library is used.

**File storage:**
- Local filesystem only. Input scans are read from user-selected paths; output images and project files are written to user-selected directories.
- Project files are XML written with Qt XML (`src/core/`).
- Default parameter profiles are stored under `QStandardPaths::AppDataLocation`, in a `profiles` subfolder (`src/core/DefaultParamsProfileManager.cpp`).

**Caching:**
- None detected. Processing results are held in memory and in project files.

## Authentication & Identity

**Auth Provider:**
- Not applicable. The application has no user accounts, login, or tokens.

## Settings Persistence

**QSettings (platform-native store):**
- `src/core/ApplicationSettings.cpp` and `src/core/ApplicationSettings.h` - application preferences.
- `src/core/RecentProjects.cpp` - recent project list.
- `src/core/WorkerThreadPool.h` - thread pool settings.
- `src/core/CollapsibleGroupBox.cpp` - UI group state.
- `src/core/filters/page_layout/OptionsWidget.cpp` - filter option persistence.
- On Linux this maps to `~/.config`; on Windows, the registry or ini store per Qt defaults.

## Monitoring & Observability

**Error Tracking:**
- None. No crash reporter or telemetry SDK is present.

**Logs:**
- Local output to the console. Developer diagnostics are gated by the `DEVELOPER_VERSION` CMake option (`ENABLE_DEBUG_FEATURES` in `config.h`), which is set in `CMakeLists.txt`.

## CI/CD & Deployment

**Hosting and distribution:**
- GitHub Releases - Release artifacts are uploaded and published by `.github/workflows/release.yml` using `softprops/action-gh-release@v2`.
- Flatpak - Manifest `flatpak/org.scantailor.Advanced.json`, source from `https://github.com/ScanTailor-Advanced/scantailor-advanced.git`, runtime `org.kde.Platform` 5.15-24.08. Built by `.github/workflows/flatpak.yml`.
- Debian package - `build-deb.sh`, invoked from `.github/workflows/release.yml`.
- Windows installer and portable build - CPack NSIS plus the `PORTABLE_VERSION` option in `CMakeLists.txt`; scripted by `build-windows.sh`.

**CI Pipeline:**
- GitHub Actions (`.github/workflows/`):
  - `ci.yml` - Ubuntu build and CTest on push and pull request to `master` and `main`.
  - `cmake.yml` - Ubuntu CMake build.
  - `lint.yml` - clang-format check.
  - `flatpak.yml` - Flatpak build on ubuntu-22.04.
  - `release.yml` - release packaging.
- Dependabot for GitHub Actions (`.github/dependabot.yml`, daily interval).
- Actions in use: `actions/checkout` (v4/v5), `actions/upload-artifact` (v5/v6), `actions/download-artifact` (v5), `actions/cache` (v4), `softprops/action-gh-release` (v2).

## Environment Configuration

**Required environment variables (build time):**
- None strictly required on Linux. Windows builds read `DEPENDENCIES_ROOT` as a CMake cache variable, not an environment variable.

**Optional environment variables (runtime):**
- `SCANTAILOR_NO_XCB_FALLBACK` - keeps native Wayland when set to `1`.
- `QT_QPA_PLATFORM` - standard Qt platform plugin override.
- `XDG_SESSION_TYPE` - read to detect Wayland.

**Secrets location:**
- Not applicable for the application. No API keys or credentials are used. CI relies on the default `GITHUB_TOKEN` for release publishing. Environment files were not read in this analysis.

## Webhooks & Callbacks

**Incoming:**
- None.

**Outgoing:**
- None.

---

*Integration audit: 2026-10-18*
