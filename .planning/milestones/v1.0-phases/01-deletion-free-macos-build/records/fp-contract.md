# Floating-point contraction setting

**Setting:** -ffp-contract=off for C and C++
**Set in:** CMakeLists.txt, line 69 (`add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-ffp-contract=off>")`, inside the `if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")` block, commit 606bea0)
**Applies to:** every configure, with or without a preset, and every build type; the block sits outside the `DEFAULT_COMPILER_FLAGS_IS_SET` cache guard and before `add_subdirectory(src)`
**Decision:** D-03. Apple Clang on arm64 fuses `a * b + c` into one `fmadd` by default, even at `-O0` (see Evidence). Whether a given expression is fused then depends on code layout and optimisation, so results could differ between build types and between compiler versions. Turning contraction off makes every `a * b + c` round twice, as written, and the setting is fixed before any Phase 2 baseline image exists (BUILD-07).
**Reversibility:** costly. Phase 2 records golden images built with `-ffp-contract=off`; changing the setting afterwards means re-recording and re-validating every golden image (D-03).

## Evidence

Compiler: Apple clang version 21.0.0 (clang-2100.3.34.2), target arm64-apple-darwin25.6.0. Test function (`{scratch}/fp/fp.cpp`):

```cpp
double f(double a, double b, double c) {
  return a * b + c;
}
```

Arithmetic instruction lines of `clang++ -std=c++17 -S <flags> fp.cpp`, as the compiler printed them:

```
### clang++ -std=c++17 -S -O0
	fmadd	d0, d0, d1, d2
### clang++ -std=c++17 -S -O2
	fmadd	d0, d0, d1, d2
### clang++ -std=c++17 -S -O2 -ffp-contract=off
	fmul	d0, d0, d1
	fadd	d0, d0, d2
### clang++ -std=c++17 -S -O2 -ffp-contract=fast
	fmadd	d0, d0, d1, d2
```

Reading: without the flag the compiler fuses at both `-O0` and `-O2`; with `-ffp-contract=off` it emits a separate `fmul` and `fadd`.

## Verification

Compile commands checked with `jq` on `build/<preset>/compile_commands.json` after `cmake --preset <name> --fresh` (every entry must contain `-Wall`, `-Wextra`, `-Werror=return-type`, `-Wno-unused` and `-ffp-contract=off`):

```
macos-release entries=408 missing=0
macos-debug entries=408 missing=0
macos-asan entries=408 missing=0
```
