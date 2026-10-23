Decision: approve
Date: 2026-10-23
Scope: Phase 2 (Golden Baselines), plans 02-01 to 02-10 only. The approval does not extend to later phases.

## Question as asked

May the build tools and Phase 2 test programs read files under /opt/homebrew and the Xcode/CLT installation (including the macOS SDK) until plans 02-01 to 02-10 are done?

What would be read: CMake reads the Qt 6, Boost, jpeg-turbo, libpng and libtiff package files and runs `xcodebuild -version` once per configure (for the golden manifest). clang and the linker read headers and libraries there and in the SDK, including the sanitizer runtime (02-10). Test programs and tools (cmake, ninja, ctest, git, jq, python3) load their libraries and Qt plug-ins (offscreen platform) from there. Version queries (`clang --version`, `cmake --version`, `brew list --versions qtbase boost jpeg-turbo libpng libtiff`) read what is installed.

Not covered, each would still need its own yes: Claude itself listing or opening files outside the project; anything in your home folder (~/Library, ~/.config) — the tests keep settings and temporary files under build/; any write outside the project.

Options offered: "Approve" (Phase 2 only, plans 02-01 to 02-10, with the exclusions above; the tracer then starts) / "Approve, later phases too" / "Decline".

## Reply

Approve
