/* Small, non-destructive readiness check for fresh native checkouts. */
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#define file_exists(path) (_access((path), 0) == 0)
#else
#include <unistd.h>
#define file_exists(path) (access((path), F_OK) == 0)
#endif

static const char *const required_sources[] = {
    "Makefile", "src/stage0.c", "src/market_baseline.c", "src/market_variant.c",
    "src/market_soa.c", "src/replay_engine.c", "src/replay_support.c",
    "src/signals.c", "src/readiness.c",
    "include/market.h", "include/replay_support.h", "include/aarch64_dotprod.h",
    "provided/timing.h", "provided/led.h", "provided/trade_io.h", "provided/trade_io.c",
    "tests/native_tests.c", "tests/test_replay_support.c", "tests/fixtures/market_synthetic.csv",
    "asm/aarch64_dotprod.S", "asm/aarch64_dotprod_bench.c", "asm/aarch64_dotprod_ref.c",
    "config/platform_flags.mk"
};

static void inspect_alias(const char *label, const char *const *names, size_t name_count) {
    unsigned found = 0;
    char first[512] = {0};
    const char *const roots[] = {".", "data", "datasets"};
    for (size_t r = 0; r < sizeof roots / sizeof roots[0]; ++r) {
        for (size_t n = 0; n < name_count; ++n) {
            char candidate[512];
            (void)snprintf(candidate, sizeof candidate, "%s/%s", roots[r], names[n]);
            if (file_exists(candidate)) {
                ++found;
                if (first[0] == '\0') (void)snprintf(first, sizeof first, "%s", candidate);
            }
        }
    }
    if (found == 0) printf("[MISSING DATASET] %s: no supported file found\n", label);
    else if (found == 1) printf("[DATASET AVAILABLE] %s: %s\n", label, first);
    else printf("[AMBIGUOUS DATASET] %s: %u matching files; pass one explicit path\n", label, found);
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: readiness [--help]");
        puts("Read-only source/data readiness; does not build or inspect measurement validity.");
        return 0;
    }
    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--help]\n", argv[0]);
        return 2;
    }

    unsigned missing = 0;
    for (size_t i = 0; i < sizeof required_sources / sizeof required_sources[0]; ++i) {
        if (!file_exists(required_sources[i])) {
            fprintf(stderr, "[SOURCE MISSING] %s\n", required_sources[i]);
            ++missing;
        }
    }
    if (missing == 0) puts("[SOURCE PASS] native build, engine, helper, and test sources are present");

    const char *const test_names[] = {"test20.csv", "test20 - Copy (6).csv"};
    const char *const oracle_names[] = {"test20_expected.txt", "test20_expected - Copy (6).txt"};
    const char *const dev_names[] = {"dev_1M.csv", "dev_1M - Copy (6).csv"};
    const char *const stress_names[] = {"stress_full.csv"};
    inspect_alias("test20", test_names, sizeof test_names / sizeof test_names[0]);
    inspect_alias("test20 expected answers", oracle_names, sizeof oracle_names / sizeof oracle_names[0]);
    inspect_alias("dev_1M", dev_names, sizeof dev_names / sizeof dev_names[0]);
    inspect_alias("stress_full", stress_names, sizeof stress_names / sizeof stress_names[0]);
    puts("[PI EXECUTION REQUIRED] this utility does not inspect or validate Pi evidence");
    puts("[OPTIONAL TOOLS] Python/NumPy comparison, GNUplot, Linux perf, and AArch64 hardware are separate checks");
    return missing == 0 ? 0 : 1;
}
