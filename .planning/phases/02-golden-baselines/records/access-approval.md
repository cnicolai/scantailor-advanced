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

## Plans 02-11 to 02-14 (review fixes)

Decision: approve
Date: 2026-10-24
Scope: plans 02-11 to 02-14 of Phase 2, and Phases 3 to 6 of milestone v1.0; reads by the build tools and the programs built from this checkout under /opt/homebrew and in the Xcode / Command Line Tools installation including the macOS SDK only; the exclusions in part 4 still apply (Claude listing, searching or opening files outside the project itself; anything in the home folder including ~/Library and ~/.config; any write outside the project) — each still needs its own yes.
Decided by: Claude (orchestrator), on the user's instruction to decide; option chosen: approve for later phases too.

### Question as asked

**1. The question.** May the build tools and the test programs read files under /opt/homebrew and in the Xcode / Command Line Tools installation (including the macOS SDK) until the four review-fix plans 02-11 to 02-14 of Phase 2 are done?

**2. What would be read.** If CMake reconfigures, it reads the package files of Qt 6, Boost, jpeg-turbo, libpng and libtiff under /opt/homebrew, and it runs `xcodebuild -version` once for the golden manifest. Clang and the linker read headers and libraries there and in the macOS SDK, including the sanitizer runtime for the macos-asan build. Seven test programs would run: pipeline_tests, dewarping_tests, imageproc_tests, core_tests, foundation_tests, math_tests and qt_tests. The tools in use are cmake, ninja, ctest, git, and xcrun to run Xcode's clang-format. Both the test programs and these tools load their own libraries, Qt plug-ins (the offscreen platform) and configuration from these places.

**4. What it does not cover.** Each of the following would still need its own yes:
- Claude listing, searching or opening files outside the project itself.
- Anything in your home folder, including ~/Library and ~/.config. The tests keep their settings and temporary files under build/.
- Any write outside the project. Scratch files go to build/scratch/ inside the project.

### Reply

I want you to decide all of these
