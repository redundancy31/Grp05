# Codebase map and migration status

## Native C paths

- `Makefile`: direct compiler rules for native v0–v10 offline and replay binaries, four Stage 0 optimization builds, supplied helper, C tests, portable/native kernel harness, fresh signal plot generator, and read-only readiness.
- `src/stage0.c`: checked iterative summation, depth-bounded recursive comparison, supplied timing/LED helper lifecycle, repeated samples and medians.
- `src/market_baseline.c`: preserved frozen baseline engine and trace output.
- `src/market_variant.c`, `src/market_soa.c`: preserved registered reader, numeric, storage, and SoA variants.
- `src/replay_engine.c`, `src/replay_support.c`: C timestamp replay, processing/release timing, console/hardware output, and replay support logic.
- `src/signals.c`: bounded first-3,700-trade development scan, Group 05 30/60 SMA crossover selection, and fresh `.dat`/GNUplot script generation.
- `src/readiness.c`: read-only native source and deterministic data-alias presence/ambiguity report. It does not inspect whether measurements are valid.
- `tests/native_tests.c`: native parser and synthetic engine checks; official-summary verification reads the supplied answer file when requested.
- `tests/test_replay_support.c`: native scheduling, timing, and output-state tests.
- `asm/`: portable C reference and AArch64 assembly for the isolated unsigned dot product; it is not a full VWAP implementation.

Generated native build and signal outputs go under `build/native/`. Existing `results/laptop/`, `results/pi/`, `plots/`, and historical archives are not native build destinations.

## Python workflows still present

`scripts/project.py` still owns historical benchmarking/statistics, perf normalization, assembly listing orchestration/annotation, replay process orchestration, rival accounting/sweeps, race management, and some verification/readiness workflows. It imports `scripts/rival.py` for rival state and reaction analysis. These paths are not yet Python-independent. `python/market_python.py` and `python/market_numpy.py` remain optional comparison engines.

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
```

GNU Make is not installed on the current Windows workstation, so these recipes were reviewed and the equivalent compile/run commands were exercised directly with GCC. `make benchmark`, native rival/sweep, native race, C perf normalization, and the old full-analysis command set do not exist yet; don't document them as native targets.

## Version identity

`Makefile` now contains the direct compiler rules for `build/native/ladder/` and `build/native/replay/`. The original `scripts/project.py` registry is retained as historical tooling. See `docs/version_manifest.md` for the actual flags and replay selection notes.
