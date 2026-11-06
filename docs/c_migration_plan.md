# C-first migration plan and pre-edit audit

Audit recorded before implementation on 2026-11-06 (Asia/Singapore). This is a baseline and work plan, not a claim that the migration or coursework is complete.

## Repository state before edits

- The checkout was clean: `git status --short --branch` reported `main...grp05/main` with no changed or untracked files.
- Starting commit: `d095640dc2ee77e80ff63a54f73e981e8b4d4080`.
- Created migration branch `c-first-migration-2026-11-06`; no source or evidence files had been edited at branch creation.
- The source repository is `Grp05/` under the supplied workspace. The root contains `src/`, `include/`, `provided/`, `python/`, `scripts/`, `tests/`, `asm/`, `config/`, `docs/`, `plots/`, `build/`, and `results/`.
- `build/` is ignored generated output. The read-only readiness report found 22/22 registered existing binaries matched their metadata. Existing result and evidence directories were not changed.

## Environment and baseline checks

- Host reported by the project doctor: Windows 11, AMD64. GCC is MSYS2 UCRT64 GCC 16.2.0; Python is MSYS2 Python 3.14.7.
- GNU Make is missing from PATH and `C:\msys64\ucrt64\bin\make.exe` is absent. GNUplot is installed at `C:\Program Files\gnuplot\bin\gnuplot.exe`. NumPy is unavailable. No Pi or AArch64 host is present.
- The normal Python launcher at `C:\Users\siamu\AppData\Local\Microsoft\WindowsApps\python.exe` could not be executed; explicit MSYS2 Python works.
- The authoritative final PDF exists at `provided/CSC1103-1104_Mini_Project_2026-27_Tick-to-Trade_final.pdf` (507,981 bytes). No PDF text extraction utility or Python PDF package was available during this baseline. Existing requirements docs cite page references, but those references still need checking against the final PDF before claiming an authoritative page-by-page audit.
- `python scripts/project.py doctor` (using explicit MSYS2 Python): succeeded; detected the three data files, correct Group 05 windows, missing Make and NumPy.
- `python scripts/project.py readiness`: succeeded with source/layout PASS and existing build metadata PASS. It reports Pi evidence absent, plus manual submission items outstanding. It also reports the existing zero-byte root `signals.png`; it was left untouched.
- `python scripts/project.py --help`, `test --help`, and `verify --help`: succeeded and expose current commands and flags.
- A direct Stage 0 link attempt with both `src/stage0.c` and `provided/test_helpers.c` failed with duplicate `main`; inspection confirmed the helper is a separate executable. Corrected baseline: Stage 0 alone compiled with `-std=c11 -Wall -Wextra -Wpedantic -g3 -O0 -Iinclude` and `--n 10000 --repeat 1` returned exit status 0, iterative and recursive sum 50,005,000, and a measured 23,300 ns for that one local sample. This is only a smoke run, not retained performance evidence.
- The project quick tests and official verifier were not run: the existing Python driver hardcodes `build/` and the verifier writes to `results/laptop/correctness_matrix.csv`; running those against this checkout could overwrite pre-existing build/evidence. A fresh isolated build/test workflow is required before those checks can be safely repeated.
- Git operations require a command-local `safe.directory` override because the workspace files are owned by the desktop account while this process runs as the Codex sandbox identity. No global Git configuration was changed.

## Current build, command, and dependency graph

`Makefile` has `PYTHON ?= python3`; every listed target invokes `scripts/project.py`. `all` runs `build-all`; `debug`, `release`, `variants`, `soa`, assembly kernel, replay, test, verify, and clean targets all delegate to the Python driver. Therefore default build and test are currently Python-dependent.

`scripts/project.py` uses Python standard-library modules for argument parsing, build specification and flags, subprocess execution, CSV/JSON processing, benchmarking, medians, verification, plot output, replay orchestration, rival/race orchestration, perf normalization, assembly generation, and readiness. It imports `scripts/rival.py`, which implements rival simulation and reaction sweep logic. `tests/` contains seven Python test modules, including Python-engine tests; `tests/test_replay_support.c` is a native C test but is built and launched by the Python driver. `provided/test_helpers.c` is an independent C helper demonstration, not a library linked with Stage 0.

The two engine files under `python/` implement the plain-Python Stage A comparison and NumPy-assisted batch comparison. NumPy is explicitly optional in the existing docs. The migration target is to leave these comparison engines as the only routine Python workflow; temporary independent checking and documented legacy tooling may remain outside required targets.

The C engine sources are `src/stage0.c`, `src/market_baseline.c`, `src/market_variant.c`, `src/market_soa.c`, `src/replay_engine.c`, and `src/replay_support.c`; public contracts are in `include/`. `asm/` has portable reference/benchmark code and AArch64 assembly. Version registrations and actual compiler commands are currently owned by `version_specs()` in `scripts/project.py`; do not infer flags from version labels alone. Observed flags include C11, warning flags, `-O0`, `-O2`, `-O3`, `-DNDEBUG`, `-DREADER_KIND`, `-DSMA_KIND`, `-DFIXED_PRICE`, `-DSTORAGE_KIND`, and platform POSIX feature flags. Full per-version source/flag identity remains in `docs/version_manifest.md` and driver definitions and must be preserved when moving build ownership.

## Inputs, numerical contracts, tests, and evidence

- Supplied Group 05 data and expected answer are in the project root with copy suffixes. The official small file is `test20 - Copy (6).csv` (1,527 bytes), answer `test20_expected - Copy (6).txt`; dev file `dev_1M - Copy (6).csv` (75,868,593 bytes); stress file `stress_full.csv` (677,776,624 bytes). `group_info - Copy (6).txt` sets test20 windows 3/5 and dev/stress windows 30/60 and declares dev trades 1,585,789–2,585,788 of stress. A byte-for-byte streaming comparison of those one million stress rows against the dev file passed.
- Existing `provided/trade_io.h` defaults to a teacher-specific absolute DATA_FOLDER and `GROUP_FOLDER "Group01"`. `open_trades(explicit_path)` exists and is the safer adapter for Group 05; the provided helper must not be silently rewritten.
- Existing numerical docs describe buyer-maker false as buy, SMA values from a full window, crossover signals after both averages are available on consecutive trades, VWAP and one-second OHLC output, plus fixed-point scaling/rounding and overflow policy. Read `docs/numeric_policy.md` and tests before any semantics change; the supplied expected-answer file remains authoritative and must not be edited.
- Existing tests: `test_market.py`, `test_python_engines.py`, `test_replay.py`, `test_replay_support.c`, `test_rival.py`, `test_performance_workflow.py`, `test_asm_kernel.py`, and `test_readiness.py` (seven Python test modules plus the C test). Coverage includes engine outputs, official test20 oracle checks through the project driver, replay, rival, analysis workflows, kernel, and readiness; current orchestration tests depend on Python and some modify build or result paths.
- Existing evidence includes laptop benchmark tables/plots, replay runs, rival/race runs, assembly workflows, and portable kernel results. `results/pi/` contains only its README; no Pi run can be verified here. Old results are legacy evidence for their recorded sources and must remain untouched. No new benchmark values are to be inferred from source or stale metadata.
- Current public CLI is the Python subcommand set listed by `scripts/project.py --help`: doctor, build variants, test, replay-test, verify, plot-signals, benchmark, replay, rival, race, profile, refresh-perf-summary, assembly, annotate-assembly, replay-check, compare-python, report, and readiness. Preserve actual behavior while migrating; audit path quoting, aliases, default data selection, replay units, and output destinations before porting.

## Migration sequence and gates

1. Complete this source/dependency/build/evidence baseline and verify final-PDF requirements and page references. Map current requirements to source, tests, outputs, local validation, Pi status, and limitations in the existing `docs/requirements_matrix.md`.
2. Add a native Makefile/compiler configuration that builds the same frozen C versions with recorded, truthful flags and an isolated output directory. Keep Python orchestration available during transition; do not delete legacy scripts.
3. Move correctness checks and core test execution to C, retain official-answer comparisons, and use independent synthetic cases. Run only in a fresh temporary build/output tree.
4. Migrate dataset aliases/configuration, correctness verification, signal generation, replay, terminal/LED behavior, rival accounting/sweeps, race numerical logic, and all required source/readiness checks to understandable C modules/commands.
5. Migrate benchmark collection and medians, throughput/speedups, memory labeling, perf normalization, and assembly generation. Preserve old evidence; write new measurements only to unique new directories with source/build/dataset identity.
6. Isolate optional plain-Python/NumPy comparison and document remaining historical scripts. Test a fresh checkout and paths with spaces without relying on Python for compulsory workflows.
7. Report Pi execution as required until actual Pi evidence is collected and inspected. This Windows workstation cannot satisfy the Pi gate or physical LED check.

## Risks and unresolved audit work

- The final PDF is present but still needs text/page inspection against existing docs. Do not claim complete authoritative requirement audit until that is done.
- Dataset discovery currently searches multiple directories and the driver uses Python-owned aliases. Confirm deterministic resolution and explicitly reject ambiguous files before replacing it.
- Build/test/verify and analysis outputs have side effects in `build/` and `results/`; use fresh isolated locations during migration and never overwrite existing result files.
- Frozen v0–v10 implementations, shared headers, flags, and timing boundaries affect measurement applicability. Do not refactor measured engines casually; any confirmed fix needs a regression case, revision note, and fresh measurements.
- Existing documentation includes historical paths and commands. Update only after each actual native CLI is implemented and its help/examples are exercised.
- User's brief prohibits deleting legacy tools, changing primary numerical semantics, installing tools, contacting remote services, or running long Pi work without explicit authorization. Those actions are not part of this baseline.

## Phase 0 audit record

The required pre-edit audit and migration plan are recorded above, with the source/build/dependency and environment baseline, safe migration branch, and independently checked dev-slice claim. The final-PDF page-reference cross-check remains pending because local extraction tooling was unavailable. This limitation does not replace or restart the audit; it remains an explicit audit gap. No overall compliance claim is made.

## Implementation progress after the baseline

The following native implementation work is now present on the migration branch:

- `Makefile` has direct GCC recipes for the 11 offline variants, 11 replay variants, four Stage 0 optimization builds, supplied helper, C tests, kernel harness, readiness, and signals. New artifacts are isolated under `build/native/`; cleanup is restricted to the selected native build directory. `make test`, `make verify`, `make replay-test`, `make asm-test`, `make signals`, and `make readiness` have implementations. There is no native benchmark target yet.
- `tests/native_tests.c` exercises strict parser cases and an independent 8-row SMA/crossover/EOF-bar fixture. Its explicit official mode reads the supplied expected-answer file and checks test20 summary/signal/bar counts. `Makefile verify` additionally checks the published BUY/SELL trace rows and all three exact bar outputs for all 11 offline executables. Full row-by-row comparison of every supplied SMA/VWAP value is still pending.
- `src/stage0.c` now uses supplied timing and LED helpers, validates unsigned CLI values, checks sum overflow without overflowing the closed form, bounds recursion, batches and checks repeated calibration samples, and reports medians without asserting that compiler output preserves recursion.
- `src/signals.c` scans at most the relevant 3,700 development trades, finds a qualifying 30/60 region with BUY and SELL, and writes new data and GNUplot script files under the native build directory. The Make target runs GNUplot and verifies the PNG is nonempty.
- `src/readiness.c` is a read-only native source/dataset presence check; it reports ambiguous data aliases and always labels Pi execution as pending.
- Updated README, numeric policy, codebase map, version manifest, and AI-assistance record now describe the implemented native subset and explicitly state that the migration is incomplete.

Direct local validation performed with GCC 16.2.0:

- Compiled all 11 offline variants and all 11 replay variants into `build/native/` with the audited flags.
- Compiled Stage 0 at `-O0`, `-O1`, `-O2`, and `-O3`; a quick O2 run at N=10,000/repeats=5 passed and malformed/negative/overflowing N inputs were rejected.
- Compiled/ran native parser/synthetic tests, the official test20 summary using the supplied expected-answer file, replay-support tests, and portable dot-product correctness tests; all passed.
- Ran the official test20 trace across all 11 offline variants and checked VWAP, signal ordinals and per-trade values, and exact OHLC output; all passed.
- Exercised a CSV path containing spaces for the market engine and replay; both completed successfully.
- Generated fresh signal `.dat`, GNUplot script, and nonempty PNG from the supplied dev file in a new `build/native/` subdirectory; observed 3 BUY and 2 SELL markers in the selected region.
- Ran the native readiness utility. It found the required sources and all supplied data and still reported Pi execution required.
- Generated local GCC `-O2` Stage 0 assembly. The output contained no `sum_recursive` symbol/call, so recursive source execution is not asserted.

GNU Make is unavailable after bounded checks of PATH and common MSYS2/Git tool locations. The Makefile itself has not been dynamically parsed or executed. NumPy and Pi hardware remain unavailable. No previous laptop/Pi results, plots, or archives were edited. The final coursework PDF's page references remain unverified because no local PDF extractor was found.

Still incomplete: native rival/accounting/sweep and race numerical work; native benchmark sample collection, medians, speedups, and evidence manifests; native perf parsing; native compiler assembly/annotation workflow; explicit alias/path configuration shared across all commands; a full C integration suite; and parity with all Python driver workflows. Consequently the compulsory project as a whole is still Python-dependent. This is an implementation checkpoint, not task completion.

## Native migration continuation

The preceding status is an earlier checkpoint. The following additions are now present in the current `updated-c` worktree; they do not make the overall migration complete:

- `src/rival_native.c` and the `make rival` path capture baseline/selected replay logs, validate ordered matching decisions, stream current/next-price fills and position P&L, write the 0.1–100 µs sweep, and generate graph inputs/script. Make invokes GNUplot and writes the PNG plus selected version/build flags and dataset/window manifest. Detailed signal logging is configurable. `tests/rival_native_tests.c` covers the threshold equality/current-vs-next fill, final unfilled signal, and mismatched sequence.
- `src/native_stats.c` supplies overflow-safe mean, conventional odd/even median, nearest-rank p50/p99, and sorting. `src/native_benchmark.c` always includes v0, validates summary outputs, performs five-or-more direct process launches, and writes fresh raw/summary/manifest output. `make benchmark` depends on official `verify`.
- `src/project_config.c` contains the 11-version registry and deterministic dataset aliases, used by native benchmark and config tests. Explicit benchmark windows are supported. Existing verification, signal, and replay interfaces have not all been migrated to this shared configuration yet.
- `src/perf_summary.c` normalizes perf CSV counters and scope, IPC, multiplexing percentages, unavailable rows, malformed rows, and diagnostics. `make profile` is Linux-only; actual counter collection was not available on Windows.
- `make assembly` uses GCC `-S -O2` on the replay main loop and records source/flags/compiler metadata. `src/asm_annotate.c` writes a separate line-numbered heuristic annotation and manual-review checklist without replacing the original listing.
- New local validations were run directly with GCC: native stats/config/perf/rival fixtures; five-repeat benchmark for all versions and a single selected version with v0 reference; test20 replay pair; rival graph PNG via installed GNUplot with spaces in paths; GCC host assembly generation and annotation. These do not constitute dynamic Makefile, Pi, Linux perf, or AArch64 validation.

Still incomplete: native live race and lag/P&L display; extraction of optional Python/NumPy engines and comparison from `scripts/project.py`; retirement of compulsory legacy Python responsibilities after replacement; use of the shared config in every verification/plot/replay path; broader C coverage for all parser/allocation/replay edge cases; and Pi validation. Do not describe the project as fully C-first until these paths are replaced and audited.
