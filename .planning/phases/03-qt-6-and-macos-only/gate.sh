#!/usr/bin/env bash
# Phase 3 gate (D-11). Always run as: bash .planning/phases/03-qt-6-and-macos-only/gate.sh <mode> [label]
#   snapshot        write the build-command reference of the current (committed) tree
#   check <label>   configure, build and ctest macos-release and macos-debug on the committed tree,
#                   compare goldens with a796c2ae, write the build-command diff against HEAD~1
# Scratch output stays in build/scratch/gate (gitignored). UPDATE_GOLDENS is never passed on.
set -u

INPUTS=(src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in)
TARGETS="scantailor-advanced pipeline_tests core_tests qt_tests dewarping_tests imageproc_tests math_tests foundation_tests"
PRESETS=(macos-release macos-debug)
GOLDEN_BASE=a796c2ae
GATE_ROOT=build/scratch/gate
REF_DIR="$GATE_ROOT/ref"
EXPECTED_SUMMARY='100% tests passed out of 7'

if [ "$(git rev-parse --show-toplevel 2>/dev/null)" != "$(pwd -P)" ]; then
  echo "gate: run from the repository root"
  exit 2
fi
if [ "${UPDATE_GOLDENS+set}" = "set" ]; then
  echo "gate: UPDATE_GOLDENS is set; refusing (D-11)"
  exit 2
fi
if [ -n "$(git status --porcelain -- "${INPUTS[@]}")" ]; then
  echo "gate: uncommitted changes in the build inputs (${INPUTS[*]}); the gate measures a committed tree"
  git status --porcelain -- "${INPUTS[@]}"
  exit 2
fi

mode="${1:-}"

input_key() {
  git ls-tree -r "$1" -- "${INPUTS[@]}" | git hash-object --stdin
}

# Command list of one preset, normalised and sorted.
command_list() {
  local p="$1" t="$TARGETS"
  if ninja -C "build/$p" -t targets all 2>/dev/null | grep -q '^ui_snapshot: '; then
    t="$t ui_snapshot"
  fi
  # shellcheck disable=SC2086
  ninja -C "build/$p" -t commands $t 2>/dev/null \
    | sed -E 's#CMakeFiles/d/[0-9a-f]{64}\.d#CMakeFiles/d/HASH.d#g' \
    | LC_ALL=C sort
}

diff_lines() {
  grep -c -e '^<' -e '^>' "$1" || true
}

case "$mode" in
  snapshot)
    mkdir -p "$REF_DIR"
    key="$(input_key HEAD)"
    rc=0
    for p in "${PRESETS[@]}"; do
      env -u UPDATE_GOLDENS cmake --preset "$p" > "$REF_DIR/configure-$p.txt" 2>&1
      c=$?
      if [ "$c" -ne 0 ]; then
        echo "gate: configure $p failed ($c), see $REF_DIR/configure-$p.txt"
        rc=1
        continue
      fi
      command_list "$p" > "$REF_DIR/cmds-$p-$key.txt"
      echo "$REF_DIR/cmds-$p-$key.txt"
    done
    exit "$rc"
    ;;

  check)
    label="${2:-}"
    if [ -z "$label" ]; then
      echo "usage: gate.sh check <label>"
      exit 2
    fi
    D="$GATE_ROOT/$label"
    rm -rf "$D"
    mkdir -p "$D" "$REF_DIR"
    git rev-parse HEAD > "$D/tested-commit.txt"
    key_head="$(input_key HEAD)"
    key_prev="$(input_key 'HEAD~1')"
    pass=1
    lines=()

    for p in "${PRESETS[@]}"; do
      env -u UPDATE_GOLDENS cmake --preset "$p" > "$D/$p-configure.txt" 2>&1
      c_conf=$?
      env -u UPDATE_GOLDENS cmake --build --preset "$p" > "$D/$p-build.txt" 2>&1
      c_build=$?
      env -u UPDATE_GOLDENS ctest --preset "$p" > "$D/$p-ctest.txt" 2>&1
      c_test=$?
      summ="$(grep 'tests passed' "$D/$p-ctest.txt" | tail -n 1)"
      lines+=("$p: configure=$c_conf build=$c_build ctest=$c_test summary=$summ")
      if [ "$c_conf" -ne 0 ] || [ "$c_build" -ne 0 ] || [ "$c_test" -ne 0 ] || [ "$summ" != "$EXPECTED_SUMMARY" ]; then
        pass=0
      fi
    done

    goldens="unchanged"
    if ! git diff --quiet "$GOLDEN_BASE" -- src/pipeline_tests/golden \
       || [ -n "$(git status --porcelain -- src/pipeline_tests/golden)" ]; then
      goldens="CHANGED"
      pass=0
    fi

    dl=()
    for p in "${PRESETS[@]}"; do
      command_list "$p" > "$D/cmds-$p.txt"
      ref="$REF_DIR/cmds-$p-$key_prev.txt"
      if [ -f "$ref" ]; then
        cp "$ref" "$D/cmds-$p-ref.txt"
        diff "$D/cmds-$p-ref.txt" "$D/cmds-$p.txt" > "$D/cmds-$p.diff"
      else
        echo "no reference for HEAD~1; run snapshot on the parent first" | tee "$D/cmds-$p.diff"
        pass=0
      fi
      cp "$D/cmds-$p.txt" "$REF_DIR/cmds-$p-$key_head.txt"
      dl+=("cmds-$p: diff_lines=$(diff_lines "$D/cmds-$p.diff")")
    done

    {
      echo "label: $label"
      echo "tested-commit: $(cat "$D/tested-commit.txt")"
      printf '%s\n' "${lines[@]}"
      echo "goldens: $goldens"
      printf '%s\n' "${dl[@]}"
      if [ "$pass" -eq 1 ]; then echo "result: PASS"; else echo "result: FAIL"; fi
    } > "$D/summary.txt"
    cat "$D/summary.txt"
    [ "$pass" -eq 1 ]
    exit $?
    ;;

  *)
    echo "usage: gate.sh snapshot | check <label>"
    exit 2
    ;;
esac
