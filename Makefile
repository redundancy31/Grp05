# Native C build. Run from an MSYS2 UCRT64 or Linux/Pi shell with GCC and GNU Make.
# Python is reserved for the optional comparison workflow.

CC = gcc
BUILD_DIR ?= build/native
ifeq ($(OS),Windows_NT)
GNUPLOT ?= C:/Program Files/gnuplot/bin/gnuplot.exe
else
GNUPLOT ?= gnuplot
endif
ifeq ($(OS),Windows_NT)
EXEEXT := .exe
else
EXEEXT :=
endif

WARN := -std=c11 -Wall -Wextra -Wpedantic
INCLUDES := -Iinclude -Iprovided
POSIX_FLAGS :=
ifeq ($(shell uname -s 2>/dev/null),Linux)
POSIX_FLAGS := -D_POSIX_C_SOURCE=200809L
endif
COMMON := $(WARN) $(INCLUDES) $(POSIX_FLAGS)
BASE_COMMON := $(WARN) -Iinclude $(POSIX_FLAGS)
VARIANT_COMMON := $(WARN) -Werror -DNDEBUG $(INCLUDES) $(POSIX_FLAGS)

LADDER_DIR := $(BUILD_DIR)/ladder
REPLAY_DIR := $(BUILD_DIR)/replay
STAGE0_DIR := $(BUILD_DIR)/stage0
TEST_DIR := $(BUILD_DIR)/tests
UTIL_DIR := $(BUILD_DIR)/tools
ifeq ($(shell uname -m 2>/dev/null),aarch64)
ASM_KERNEL_SRC := asm/aarch64_dotprod.S
ASM_KERNEL_FLAGS := -DTT_USE_AARCH64_ASM
else
ASM_KERNEL_SRC := asm/aarch64_dotprod_ref.c
ASM_KERNEL_FLAGS :=
endif

VERSION_IDS := \
 v0_baseline_strtok_running_double_heap_O0 \
 v1_baseline_strtok_running_double_heap_O2 \
 v2_helper_sscanf_naive_double_stream_O2 \
 v3_own_pointer_reader_naive_double_stream_O2 \
 v4_manual_digit_parser_naive_double_stream_O2 \
 v5_manual_digit_parser_running_double_stream_O2 \
 v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2 \
 v7_manual_digit_parser_running_double_stream_O3 \
 v8_manual_running_double_static_trade_array_O2 \
 v9_manual_running_double_exact_malloc_trade_array_O2 \
 v10_manual_running_double_soa_O2
LADDER_BINS := $(addprefix $(LADDER_DIR)/,$(addsuffix $(EXEEXT),$(VERSION_IDS)))
REPLAY_BINS := $(addprefix $(REPLAY_DIR)/,$(addsuffix $(EXEEXT),$(VERSION_IDS)))
STAGE0_BINS := $(addprefix $(STAGE0_DIR)/stage0_O,$(addsuffix $(EXEEXT),0 1 2 3))

.PHONY: all ladder replay stage0 helper test verify replay-test test-integration \
        asm-test asm-benchmark signals readiness test-python compare-python clean

all: ladder replay stage0 helper $(TEST_DIR)/native_tests$(EXEEXT) \
     $(TEST_DIR)/replay_support_tests$(EXEEXT) $(UTIL_DIR)/asm_kernel$(EXEEXT) \
     $(UTIL_DIR)/readiness$(EXEEXT)

ladder: $(LADDER_BINS)
replay: $(REPLAY_BINS)
stage0: $(STAGE0_BINS)
helper: $(UTIL_DIR)/test_helpers$(EXEEXT)

$(LADDER_DIR) $(REPLAY_DIR) $(STAGE0_DIR) $(TEST_DIR) $(UTIL_DIR):
	mkdir -p "$@"

$(LADDER_DIR)/v0_baseline_strtok_running_double_heap_O0$(EXEEXT): src/market_baseline.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(BASE_COMMON) -g3 -O0 $< -o "$@"
$(LADDER_DIR)/v1_baseline_strtok_running_double_heap_O2$(EXEEXT): src/market_baseline.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(BASE_COMMON) -O2 -DNDEBUG $< -o "$@"

$(LADDER_DIR)/v2_helper_sscanf_naive_double_stream_O2$(EXEEXT): src/market_variant.c provided/trade_io.c include/market.h provided/trade_io.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(VARIANT_COMMON) -O2 -DREADER_KIND=0 -DSMA_KIND=0 -DFIXED_PRICE=0 -DSTORAGE_KIND=0 src/market_variant.c provided/trade_io.c -o "$@"
$(LADDER_DIR)/v3_own_pointer_reader_naive_double_stream_O2$(EXEEXT): src/market_variant.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(VARIANT_COMMON) -O2 -DREADER_KIND=1 -DSMA_KIND=0 -DFIXED_PRICE=0 -DSTORAGE_KIND=0 $< -o "$@"
$(LADDER_DIR)/v4_manual_digit_parser_naive_double_stream_O2$(EXEEXT): src/market_variant.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(VARIANT_COMMON) -O2 -DREADER_KIND=2 -DSMA_KIND=0 -DFIXED_PRICE=0 -DSTORAGE_KIND=0 $< -o "$@"
$(LADDER_DIR)/v5_manual_digit_parser_running_double_stream_O2$(EXEEXT): src/market_variant.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(VARIANT_COMMON) -O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=0 -DFIXED_TRADE_CAPACITY=12000000U $< -o "$@"
$(LADDER_DIR)/v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2$(EXEEXT): src/market_variant.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(VARIANT_COMMON) -O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=1 -DSTORAGE_KIND=0 $< -o "$@"
$(LADDER_DIR)/v7_manual_digit_parser_running_double_stream_O3$(EXEEXT): src/market_variant.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(VARIANT_COMMON) -O3 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=0 -DFIXED_TRADE_CAPACITY=12000000U $< -o "$@"
$(LADDER_DIR)/v8_manual_running_double_static_trade_array_O2$(EXEEXT): src/market_variant.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(VARIANT_COMMON) -O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=1 -DFIXED_TRADE_CAPACITY=12000000U $< -o "$@"
$(LADDER_DIR)/v9_manual_running_double_exact_malloc_trade_array_O2$(EXEEXT): src/market_variant.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(VARIANT_COMMON) -O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=2 -DFIXED_TRADE_CAPACITY=12000000U $< -o "$@"
$(LADDER_DIR)/v10_manual_running_double_soa_O2$(EXEEXT): src/market_soa.c include/market.h provided/timing.h | $(LADDER_DIR)
	$(CC) $(VARIANT_COMMON) -O2 -DREADER_KIND=2 -DSMA_KIND=1 -DFIXED_PRICE=0 -DSTORAGE_KIND=3 -DFIXED_TRADE_CAPACITY=12000000U $< -o "$@"

# Replay settings mirror scripts/project.py replay_specs as audited: v0 O0,
# v1 O2, v7 O3; other strategy variants use their named reader/SMA/storage.
REPLAY_COMMON := $(WARN) -Werror $(INCLUDES) $(POSIX_FLAGS)
define replay_rule
$(REPLAY_DIR)/$(1)$(EXEEXT): src/replay_engine.c src/replay_support.c include/replay_support.h include/market.h provided/timing.h provided/led.h provided/trade_io.h $(2) | $(REPLAY_DIR)
	$$(CC) $$(REPLAY_COMMON) $(3) -DREADER_KIND=$(4) -DSMA_KIND=$(5) -DFIXED_PRICE=$(6) -DSTORAGE_KIND=$(7) $(2) src/replay_engine.c src/replay_support.c -o "$$@"
endef
$(eval $(call replay_rule,v0_baseline_strtok_running_double_heap_O0,,-O0 -g3,3,1,0,3))
$(eval $(call replay_rule,v1_baseline_strtok_running_double_heap_O2,,-O2 -DNDEBUG,3,1,0,3))
$(eval $(call replay_rule,v2_helper_sscanf_naive_double_stream_O2,provided/trade_io.c,-O2 -DNDEBUG,0,0,0,0))
$(eval $(call replay_rule,v3_own_pointer_reader_naive_double_stream_O2,,-O2 -DNDEBUG,1,0,0,0))
$(eval $(call replay_rule,v4_manual_digit_parser_naive_double_stream_O2,,-O2 -DNDEBUG,2,0,0,0))
$(eval $(call replay_rule,v5_manual_digit_parser_running_double_stream_O2,,-O2 -DNDEBUG,2,1,0,0))
$(eval $(call replay_rule,v6_manual_digit_parser_running_fixed_i64_1e8_stream_O2,,-O2 -DNDEBUG,2,1,1,0))
$(eval $(call replay_rule,v7_manual_digit_parser_running_double_stream_O3,,-O3 -DNDEBUG,2,1,0,0))
$(eval $(call replay_rule,v8_manual_running_double_static_trade_array_O2,,-O2 -DNDEBUG -DFIXED_TRADE_CAPACITY=12000000U,2,1,0,1))
$(eval $(call replay_rule,v9_manual_running_double_exact_malloc_trade_array_O2,,-O2 -DNDEBUG -DFIXED_TRADE_CAPACITY=12000000U,2,1,0,2))
$(eval $(call replay_rule,v10_manual_running_double_soa_O2,,-O2 -DNDEBUG -DFIXED_TRADE_CAPACITY=12000000U,2,1,0,0))

$(STAGE0_DIR)/stage0_O0$(EXEEXT): src/stage0.c provided/timing.h provided/led.h | $(STAGE0_DIR)
	$(CC) $(BASE_COMMON) -g3 -O0 $< -o "$@"
$(STAGE0_DIR)/stage0_O1$(EXEEXT): src/stage0.c provided/timing.h provided/led.h | $(STAGE0_DIR)
	$(CC) $(BASE_COMMON) -O1 $< -o "$@"
$(STAGE0_DIR)/stage0_O2$(EXEEXT): src/stage0.c provided/timing.h provided/led.h | $(STAGE0_DIR)
	$(CC) $(BASE_COMMON) -O2 $< -o "$@"
$(STAGE0_DIR)/stage0_O3$(EXEEXT): src/stage0.c provided/timing.h provided/led.h | $(STAGE0_DIR)
	$(CC) $(BASE_COMMON) -O3 $< -o "$@"

$(UTIL_DIR)/test_helpers$(EXEEXT): provided/test_helpers.c provided/timing.h provided/led.h | $(UTIL_DIR)
	$(CC) $(COMMON) $< -o "$@"
$(TEST_DIR)/native_tests$(EXEEXT): tests/native_tests.c src/market_baseline.c include/market.h provided/timing.h | $(TEST_DIR)
	$(CC) $(COMMON) tests/native_tests.c -o "$@"
$(TEST_DIR)/replay_support_tests$(EXEEXT): tests/test_replay_support.c src/replay_support.c include/replay_support.h provided/timing.h | $(TEST_DIR)
	$(CC) $(COMMON) tests/test_replay_support.c src/replay_support.c -o "$@"
$(UTIL_DIR)/asm_kernel$(EXEEXT): asm/aarch64_dotprod_bench.c $(ASM_KERNEL_SRC) asm/aarch64_dotprod_ref.c include/aarch64_dotprod.h | $(UTIL_DIR)
	$(CC) $(COMMON) $(ASM_KERNEL_FLAGS) asm/aarch64_dotprod_bench.c asm/aarch64_dotprod_ref.c $(ASM_KERNEL_SRC) -o "$@"
$(UTIL_DIR)/readiness$(EXEEXT): src/readiness.c | $(UTIL_DIR)
	$(CC) $(COMMON) $< -o "$@"
$(UTIL_DIR)/signals$(EXEEXT): src/signals.c src/market_baseline.c include/market.h provided/timing.h | $(UTIL_DIR)
	$(CC) $(COMMON) src/signals.c -o "$@"

# Build recipe and platform flag edits invalidate all native outputs. Source
# and public-header dependencies above cover code changes.
$(LADDER_BINS) $(REPLAY_BINS) $(STAGE0_BINS) \
 $(UTIL_DIR)/test_helpers$(EXEEXT) $(TEST_DIR)/native_tests$(EXEEXT) \
 $(TEST_DIR)/replay_support_tests$(EXEEXT) $(UTIL_DIR)/asm_kernel$(EXEEXT) \
 $(UTIL_DIR)/readiness$(EXEEXT) $(UTIL_DIR)/signals$(EXEEXT): Makefile config/platform_flags.mk

test: $(TEST_DIR)/native_tests$(EXEEXT) $(TEST_DIR)/replay_support_tests$(EXEEXT) $(UTIL_DIR)/asm_kernel$(EXEEXT)
	"$(TEST_DIR)/native_tests$(EXEEXT)"
	"$(TEST_DIR)/replay_support_tests$(EXEEXT)"
	"$(UTIL_DIR)/asm_kernel$(EXEEXT)" --test


verify: $(TEST_DIR)/native_tests$(EXEEXT) $(LADDER_BINS)
	@test -f "test20 - Copy (6).csv" -a -f "test20_expected - Copy (6).txt" || { echo "verify requires the supplied Group 05 test20 CSV and expected-answer file" >&2; exit 1; }
	"$(TEST_DIR)/native_tests$(EXEEXT)" --official "test20 - Copy (6).csv" "test20_expected - Copy (6).txt"
	@set -e; for exe in $(LADDER_BINS); do \
	  "$$exe" "test20 - Copy (6).csv" 3 5 --trace > "$(BUILD_DIR)/verify_trace.tmp"; \
	  grep -F 'TRACE_ROW|7|75436.94000000|0.00040000|1|75436.93989253|75436.93666667|75436.93600000|BUY' "$(BUILD_DIR)/verify_trace.tmp" >/dev/null; \
	  grep -F 'TRACE_ROW|10|75436.93000000|0.00007000|0|75436.93779696|75436.93333333|75436.93600000|SELL' "$(BUILD_DIR)/verify_trace.tmp" >/dev/null; \
	  grep -F 'TRACE_BAR|1787293073|75436.94000000|75436.94000000|75436.94000000|75436.94000000|2' "$(BUILD_DIR)/verify_trace.tmp" >/dev/null; \
	  grep -F 'TRACE_BAR|1787293074|75436.94000000|75436.94000000|75436.93000000|75436.94000000|5' "$(BUILD_DIR)/verify_trace.tmp" >/dev/null; \
	  grep -F 'TRACE_BAR|1787293075|75436.94000000|75436.94000000|75436.93000000|75436.93000000|13' "$(BUILD_DIR)/verify_trace.tmp" >/dev/null; \
	  echo "official test20 trace PASS: $$exe"; \
	done

replay-test: $(TEST_DIR)/replay_support_tests$(EXEEXT)
	"$(TEST_DIR)/replay_support_tests$(EXEEXT)"
asm-test: $(UTIL_DIR)/asm_kernel$(EXEEXT)
	"$(UTIL_DIR)/asm_kernel$(EXEEXT)" --test
asm-benchmark: $(UTIL_DIR)/asm_kernel$(EXEEXT)
	"$(UTIL_DIR)/asm_kernel$(EXEEXT)" --benchmark 5
signals: $(UTIL_DIR)/signals$(EXEEXT)
	rm -f "$(BUILD_DIR)/signals/signals.dat" "$(BUILD_DIR)/signals/signals.gnuplot" "$(BUILD_DIR)/signals/signals.png"
	"$(UTIL_DIR)/signals$(EXEEXT)" "dev_1M - Copy (6).csv" "$(BUILD_DIR)/signals"
	@"$(GNUPLOT)" "$(BUILD_DIR)/signals/signals.gnuplot" || { echo "GNUplot unavailable or failed; fresh data/script remain under $(BUILD_DIR)/signals, no PNG is accepted" >&2; exit 1; }
	test -s "$(BUILD_DIR)/signals/signals.dat" -a -s "$(BUILD_DIR)/signals/signals.gnuplot" -a -s "$(BUILD_DIR)/signals/signals.png"
readiness: $(UTIL_DIR)/readiness$(EXEEXT)
	"$(UTIL_DIR)/readiness$(EXEEXT)"
test-integration: verify replay-test asm-test $(REPLAY_DIR)/v1_baseline_strtok_running_double_heap_O2$(EXEEXT)
	"$(REPLAY_DIR)/v1_baseline_strtok_running_double_heap_O2$(EXEEXT)" "test20 - Copy (6).csv" --version-id v1_baseline_strtok_running_double_heap_O2 --speed 1000 --limit 20 --short 3 --long 5 --output off > "$(BUILD_DIR)/replay_smoke.log"
	grep -F 'stop_reason=end_of_file' "$(BUILD_DIR)/replay_smoke.log" >/dev/null
test-python:
	python3 -m unittest discover -s tests -p "test_*.py" -v
compare-python:
	python3 scripts/project.py compare-python --dataset test20 --repeats 1
clean:
	rm -rf "$(BUILD_DIR)"
