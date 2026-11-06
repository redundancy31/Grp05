/* Native engine contract tests. Include the frozen engine in this test
 * translation unit so tests exercise its implementation without changing
 * the production source or its normal build identity. */
#define main market_baseline_program_main
#include "../src/market_baseline.c"
#undef main

#include <stdarg.h>

static int fail(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fputs("native tests: FAIL: ", stderr);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    return 1;
}

static int close_enough(double a, double b) {
    const double difference = a - b;
    return difference >= -0.00000001 && difference <= 0.00000001;
}

static int test_parser_contract(void) {
    Trade trade = {0};
    const char *valid = "42,12.50000000,0.25000000,3.12500000,1234567,False,True\r\n";
    if (parse_trade_line(valid, &trade, 1) != 0) return fail("valid CRLF row rejected");
    if (trade.trade_id != 42 || trade.timestamp_us != 1234567 || trade.is_buy != 1)
        return fail("integer or buyer-maker mapping mismatch");
    if (!close_enough(trade.price, 12.5) || !close_enough(trade.qty, 0.25))
        return fail("decimal fields parsed incorrectly");
    if (parse_trade_line("", &trade, 2) != 1) return fail("blank line was not skipped");
    if (parse_trade_line("trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n", &trade, 3) != 1)
        return fail("header row was not skipped");
    if (parse_trade_line("1,2,3\n", &trade, 4) >= 0)
        return fail("malformed short row was accepted");
    if (parse_trade_line("1,2,0,0,3,False,True\n", &trade, 5) >= 0)
        return fail("zero quantity was accepted");
    return 0;
}

static int test_synthetic_engine(void) {
    double vwap = 0.0;
    int64_t buys = 0, sells = 0, trades = 0;
    int first = -1, last = -1, bars = 0;
    int rc = compute_market_summary("tests/fixtures/market_synthetic.csv", 2, 3,
                                    &vwap, &buys, &sells, &trades, &first, &last, &bars);
    if (rc != 0) return fail("synthetic fixture computation returned %d", rc);
    if (trades != 8 || !close_enough(vwap, 10.125)) return fail("synthetic trade/VWAP contract mismatch");
    if (buys != 1 || sells != 1 || first != 5 || last != 8)
        return fail("synthetic crossover contract mismatch: buys=%lld sells=%lld first=%d last=%d",
                    (long long)buys, (long long)sells, first, last);
    if (bars != 2) return fail("synthetic EOF/bar boundary mismatch: bars=%d", bars);
    return 0;
}

static int read_oracle_summary(const char *path, double *vwap, int64_t *buys,
                              int64_t *sells, int *bar_count) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) return fail("cannot open supplied expected-answer file '%s'", path);
    char line[512];
    int have_vwap = 0, have_signals = 0, in_bars = 0;
    *bar_count = 0;
    while (fgets(line, sizeof line, fp) != NULL) {
        if (sscanf(line, "Final VWAP: %lf", vwap) == 1) have_vwap = 1;
        long long parsed_buys = 0, parsed_sells = 0;
        if (sscanf(line, "Signals: %lld BUY, %lld SELL", &parsed_buys,
                   &parsed_sells) == 2) {
            *buys = (int64_t)parsed_buys;
            *sells = (int64_t)parsed_sells;
            have_signals = 1;
        }
        if (strncmp(line, "One-second OHLC bars", strlen("One-second OHLC bars")) == 0) {
            in_bars = 1;
            continue;
        }
        if (in_bars) {
            long long second = 0;
            double open = 0, high = 0, low = 0, close = 0;
            int count = 0;
            if (sscanf(line, "%lld %lf %lf %lf %lf %d", &second, &open, &high,
                       &low, &close, &count) == 6) ++*bar_count;
        }
    }
    const int read_error = ferror(fp);
    fclose(fp);
    if (read_error || !have_vwap || !have_signals || *bar_count == 0)
        return fail("supplied expected-answer file '%s' is incomplete or unreadable", path);
    return 0;
}

static int test_official_test20(const char *path, const char *oracle_path) {
    FILE *dataset_check = fopen(path, "r");
    if (dataset_check == NULL) return fail("cannot open supplied official test20 dataset '%s'", path);
    fclose(dataset_check);
    double oracle_vwap = 0.0;
    int64_t oracle_buys = 0, oracle_sells = 0;
    int oracle_bars = 0;
    if (read_oracle_summary(oracle_path, &oracle_vwap, &oracle_buys,
                            &oracle_sells, &oracle_bars)) return 1;
    double vwap = 0.0;
    int64_t buys = 0, sells = 0, trades = 0;
    int first = -1, last = -1, bars = 0;
    int rc = compute_market_summary(path, 3, 5, &vwap, &buys, &sells, &trades,
                                    &first, &last, &bars);
    if (rc != 0) return fail("official test20 computation returned %d", rc);
    if (trades != 20 || !close_enough(vwap, oracle_vwap))
        return fail("official count/final VWAP mismatch: trades=%lld VWAP=%.12f",
                    (long long)trades, vwap);
    if (buys != oracle_buys || sells != oracle_sells || first != 7 || last != 10)
        return fail("official signals mismatch: buys=%lld sells=%lld first=%d last=%d",
                    (long long)buys, (long long)sells, first, last);
    if (bars != oracle_bars) return fail("official OHLC bar count mismatch: got %d expected %d", bars, oracle_bars);
    return 0;
}

int main(int argc, char **argv) {
    if (test_parser_contract() || test_synthetic_engine()) return 1;
    if (argc == 4 && strcmp(argv[1], "--official") == 0) {
        if (test_official_test20(argv[2], argv[3])) return 1;
        puts("native tests: synthetic and official test20 summary PASS");
        return 0;
    }
    if (argc != 1) return fail("usage: %s [--official test20.csv expected.txt]", argv[0]);
    puts("native tests: parser and synthetic indicators PASS");
    return 0;
}
