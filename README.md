# Tick-to-Trade on the Edge — Group 05

Educational CSC1103/CSC1104 coursework. The C engines process Binance-style CSV input and calculate cumulative VWAP, moving averages, crossover signals, OHLC bars, and timestamp-paced replay. The repository has a native C build and is partway through the wider C-first migration; several analysis workflows still depend on the historical Python driver. It is not connected to an exchange and does not perform real trading.

## Native build (MSYS2 UCRT64 or Linux)

Use an MSYS2 UCRT64 shell on Windows, or a normal Linux shell on the Pi. Install GCC and GNU Make through the platform's normal package manager. Do not paste MSYS shell recipes into PowerShell or cmd.exe. Put the supplied Group 05 files in the repository root with their supplied names.

```sh
make
make test
make verify
make replay-test
make asm-test
make readiness
make signals
```

The native build writes only under `build/native/`. `make` builds the offline v0–v10 ladder, replay executables, Stage 0 at `-O0/-O1/-O2/-O3`, the supplied helper, native tests, a readiness utility, and the portable kernel test program. It does not run stress benchmarks. `make verify` requires `test20 - Copy (6).csv` and checks the official Group 05 3/5-window outputs against the test contract, including trace rows and OHLC bars. `make signals` reads the explicit supplied development file, uses 30/60 windows, and writes fresh data, GNUplot script, and PNG under `build/native/signals/`; it requires GNUplot for the PNG stage.

The command `make benchmark`, native rival accounting/sweep, and native race aggregation are not implemented yet. These workflows still use historical Python orchestration and are not compulsory Python-independent workflows at this migration stage. `make test-python` and `make compare-python` are optional historical/reference workflows; NumPy is needed for the full comparison.

On the current Windows workstation, direct GCC compilation was validated because GNU Make is not installed. Do not interpret that as a successful `make` run. The supported validation command when Make is available is:

```sh
make test verify
```

## Direct C use

```sh
build/native/ladder/v1_baseline_strtok_running_double_heap_O2.exe "test20 - Copy (6).csv" 3 5 --trace
build/native/replay/v1_baseline_strtok_running_double_heap_O2.exe "test20 - Copy (6).csv" --speed 1000 --limit 20 --short 3 --long 5 --output console
build/native/stage0/stage0_O2.exe --n 100000000 --repeat 20
build/native/tools/signals.exe --help
```

On Linux, omit `.exe` from the executable paths. Replay supports `--speed 1|100|1000`, `--limit`, `--duration-seconds`, `--output off|console|hardware`, and latency/decision CSV options; run the executable with `--help` for the current interface. Hardware LED output is Pi-specific. A fallback console message is not physical LED evidence.

## Data and numerical contract

Group 05 uses short/long windows 3/5 for test20 and 30/60 for development and stress. Buyer-maker `False` maps to a buy. VWAP accumulates `price * quantity`; SMA output begins when its full window is available; a crossover requires two available SMA pairs. The official test20 file and expected-answer text are supplied inputs and must not be regenerated or edited. The declared dev slice (stress rows 1,585,789–2,585,788) was checked byte for byte against the supplied dev file.

## Python dependency status

`Makefile` no longer invokes Python for its implemented build, test, official test20 verification, signal generation, replay build, assembly-kernel test, or readiness targets. Python remains in `scripts/project.py`, `scripts/rival.py`, the analysis and profiling workflows, and Python/NumPy reference tests. Therefore the full set of compulsory workflows is **not yet Python-independent**. See [the migration status and limitations](docs/known_limitations.md) before treating this repository as complete.

## Repository and evidence

- `src/`, `include/`: authored C engines and interfaces.
- `provided/`: course-supplied helpers and final coursework PDF; retain attribution.
- `asm/`: portable kernel reference and AArch64 implementation.
- `tests/`: native C fixtures/tests and historical Python tests.
- `build/native/`: generated native binaries and fresh local plot outputs.
- `results/laptop/`, `results/pi/`: historical evidence; do not overwrite. Pi evidence is not present in this checkout.
- `python/`, `scripts/`: optional comparison engines and legacy orchestration, including responsibilities not yet ported.

See [docs/c_migration_plan.md](docs/c_migration_plan.md), [docs/requirements_matrix.md](docs/requirements_matrix.md), [docs/version_manifest.md](docs/version_manifest.md), [docs/numeric_policy.md](docs/numeric_policy.md), and [docs/known_limitations.md](docs/known_limitations.md) for implementation status and evidence boundaries.
