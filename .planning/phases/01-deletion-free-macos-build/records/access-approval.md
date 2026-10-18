Decision: approve
Date: 2026-10-18
Scope: Phase 1 plans 01-01 to 01-07 (build-tool and built-program reads outside the project under /opt/homebrew and in the Xcode / Command Line Tools installation and macOS SDK, as listed under part (2))

## Question as asked

(1) The question: May the build tools and the programs built in Phase 1 read files under /opt/homebrew and in the Xcode / Command Line Tools installation (including the macOS SDK) until Phase 1, plans 01-01 to 01-07, is done?

(2) What would be read: CMake reads the package files of Qt 6, Boost, jpeg-turbo, libpng and libtiff under /opt/homebrew when it configures this checkout and one small probe program in the session scratchpad (plan 01-05); clang and the linker read headers and libraries there and in the macOS SDK, including the sanitizer runtime (plan 01-04); the five test programs, the probe and the tools in use (cmake, ninja, qtpaths, jq, magick, otool, python3, and clang-format if plan 01-02 installs it) load their own libraries, plug-ins and data files from these places and from macOS when they run, and plans 01-03 and 01-07 write the paths of the libraries a test program loads into a committed record; the version queries sw_vers, sysctl, xcodebuild -version, xcrun --show-sdk-version, xcode-select -p, pkgutil --pkg-info and clang --version read what is installed.

(4) What it does not cover, each still needing its own yes: Claude listing, searching or opening files outside the project itself (ls, find, cat, grep or its file tools); anything in the home folder, including ~/Library and ~/.config; the ScanTailor settings (their own question in plan 01-05, D-02); crash reports (plan 01-06); and any write outside the project other than brew install (D-09) and the session scratchpad.

## Reply

approve
