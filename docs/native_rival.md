# Native streaming rival sweep

`src/rival_native.c` consumes one dataset and two replay captures (baseline and selected fast version). It requires the four CSVs to contain matching trade ordinals and requires the two `effective_decision` streams to match. Timing uses `processing_duration_ns`, not release-to-decision latency. At each actionable signal, duration strictly below the rival threshold fills at the triggering price; equality and slower processing fill at the next trade price. A final-trade action is unfilled. The position starts flat, buys one unit only when flat, sells only when holding, and ignores duplicate position actions. Sweep money difference is fast P&L minus baseline P&L and may be negative.

`make rival` now captures both selected replay variants into a new output directory, runs the C analyzer, and invokes GNUplot. It requires the exact dataset path and a fresh output directory. The default pair is baseline v0 and v1 O2; v1 is simply the explicit default comparison choice, not an assertion that it is fastest. Override the versions and windows when needed:

```sh
make rival DATASET='test20 - Copy (6).csv' RIVAL_BASELINE=v0_baseline_strtok_running_double_heap_O0 RIVAL_FAST=v5_manual_digit_parser_running_double_stream_O2 RIVAL_SPEED=1000 RIVAL_LIMIT=20 RIVAL_SHORT=3 RIVAL_LONG=5 RIVAL_OUTPUT_DIR='build/native/rival-20261106-153000'
```

The output directory's parent must exist and the output directory itself must not already exist. It contains baseline/fast decision and latency captures, process logs, `rival.csv`, graph data, GNUplot script, PNG, and a manifest with version/flag/dataset/window identity. Set `RIVAL_DETAILS=1` to also write detailed `rival.decisions.csv`; the default `RIVAL_DETAILS=0` keeps analysis output compact. Direct analyzer use is compact by default and accepts `--details` after the summary path to request the sibling decision CSV. Set `RIVAL_LIMIT=` to process replay through EOF; the replay decision/latency captures are per trade and can be large. Build with `make build/native/tools/rival_native.exe` on Windows (or the corresponding extensionless target on Linux). The analyzer accepts explicit paths, including paths with spaces:

```text
rival_native DATASET BASE_DECISIONS BASE_LATENCY FAST_DECISIONS FAST_LATENCY [SUMMARY.csv]
```

It emits ten thresholds from 0.1 to 100 microseconds, including 1.0, and retains only fixed-size account state, so analysis memory is O(1) in trade count. When a summary CSV path is supplied, it also writes sibling `.dat` and `.gnuplot` files; `make rival` invokes GNUplot and requires a fresh nonempty sibling PNG. Existing output paths are rejected to prevent accidental overwrite. The current native command does not yet produce per-signal decision logs or a complete source/build provenance manifest, and its floating-point accounting has less decimal precision than the historical Python `Decimal` implementation. The larger legacy rival evidence workflow is not fully replaced yet.
