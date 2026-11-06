# Codebase map and migration status

## Native C paths

- `Makefile`: direct compiler rules for native v0–v10 offline and replay binaries, Stage 0 builds, C tests, benchmark/rival/perf/assembly utilities, kernel harness, signals, and readiness.
- `src/stage0.c`: checked iterative summation, depth-bounded recursive comparison, supplied timing/LED helper lifecycle, repeated samples and medians.
- `src/market_baseline.c`: preserved frozen baseline engine and trace output.
- `src/market_variant.c`, `src/market_soa.c`: preserved registered reader, numeric, storage, and SoA variants.
- `src/replay_engine.c`, `src/replay_support.c`: C timestamp replay, processing/release timing, console/hardware output, and replay support logic.
- `src/signals.c`: bounded first-3,700-trade development scan, Group 05 30/60 SMA crossover selection, and fresh `.dat`/GNUplot script generation.
- `src/readiness.c`: read-only native source and deterministic data-alias presence/ambiguity report. It does not inspect whether measurements are valid.
- `src/project_config.c`: shared v0-v10 registry and deterministic `test20`/`dev`/`stress` alias resolver. Benchmark uses this interface; existing verifier, signal, and replay CLIs still accept explicit paths separately.
- `src/native_benchmark.c`, `src/native_stats.c`: correctness-gated direct process runs, raw/summary evidence, statistics, throughput, speedups, and memory metrics.
- `src/rival_native.c`: streaming replay accounting, optional per-signal output, sweep CSV, graph data, and GNUplot script.
- `src/perf_summary.c`: native `perf stat -x,` parser; `src/asm_annotate.c` classifies actual assembly lines heuristically.
- `tests/native_tests.c`: native parser and synthetic engine checks; official-summary verification reads the supplied answer file when requested.
- `tests/test_replay_support.c`: native scheduling, timing, and output-state tests.
- `asm/`: portable C reference and AArch64 assembly for the isolated unsigned dot product; it is not a full VWAP implementation.

Generated native build and signal outputs go under `build/native/`. Existing `results/laptop/`, `results/pi/`, `plots/`, and historical archives are not native build destinations.

## Python workflows still present

`scripts/project.py` and `scripts/rival.py` remain for historical commands and existing Python tests. Native Make targets now own benchmark collection, perf normalization, assembly generation/annotation, rival capture/accounting/sweeps, and core replay. Python-first race management and optional Python extraction/comparison remain unfinished. Native dataset configuration is not yet shared by every verifier, plotter, and replay CLI. `python/market_python.py` and `python/market_numpy.py` have not been isolated under an optional-only directory.

## Native entry points currently implemented

From the repository root in MSYS2 UCRT64 or Linux with GCC and GNU Make:

```sh
make
make test
make verify
make replay-test
make asm-test
make signals
make readiness
make benchmark DATASET=test20 VERSION_ID=all REPEATS=5
make rival DATASET='test20 - Copy (6).csv' RIVAL_FAST=v5_manual_digit_parser_running_double_stream_O2 RIVAL_OUTPUT_DIR='build/native/rival-unique-name'
make perf-summary PERF_RAW=... PERF_SUMMARY=...
make assembly ASSEMBLY_DIR='build/native/assembly-unique-name'
```

GNU Make is not installed on the current Windows workstation, so these recipes were reviewed and the equivalent compile/run commands were exercised directly with GCC. `make benchmark`, native rival/sweep, native race, C perf normalization, and the old full-analysis command set do not exist yet; don't document them as native targets.

## Version identity

`Makefile` now contains the direct compiler rules for `build/native/ladder/` and `build/native/replay/`. The original `scripts/project.py` registry is retained as historical tooling. See `docs/version_manifest.md` for the actual flags and replay selection notes.
