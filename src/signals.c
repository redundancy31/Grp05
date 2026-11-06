/* Fresh, bounded-memory development signal data and GNUplot script generator. */
#define main frozen_market_program_main
#include "market_baseline.c"
#undef main

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <direct.h>
#define make_directory(path) _mkdir(path)
#else
#include <sys/stat.h>
#define make_directory(path) mkdir(path, 0777)
#endif

#define SIGNAL_ROWS 3700U
#define SHORT_WINDOW 30U
#define LONG_WINDOW 60U
#define SIGNALS_IMPLEMENTATION_ID "native-signals-v1"

typedef struct {
    double price;
    double short_sma;
    double long_sma;
    int signal;
    int ready;
} SignalRow;

static int ensure_output_directory(const char *path) {
    if (make_directory(path) == 0 || errno == EEXIST) return 1;
    fprintf(stderr, "signals: cannot create output directory '%s' (errno=%d)\n", path, errno);
    return 0;
}

static int generate(const char *input, const char *output_directory) {
    FILE *fp = fopen(input, "r");
    if (fp == NULL) {
        fprintf(stderr, "signals: cannot open explicit input '%s'\n", input);
        return 1;
    }
    SignalRow rows[SIGNAL_ROWS] = {{0}};
    char line[512];
    size_t count = 0;
    double short_sum = 0.0;
    double long_sum = 0.0;
    double previous_diff = 0.0;
    int diff_ready = 0;
    size_t first_signal = 0;
    size_t last_signal = 0;
    int buys = 0;
    int sells = 0;

    while (count < SIGNAL_ROWS && fgets(line, sizeof line, fp) != NULL) {
        Trade trade = {0};
        const int parsed = parse_trade_line(line, &trade, (int)count + 1);
        if (parsed == 1) continue;
        if (parsed != 0) {
            fprintf(stderr, "signals: malformed trade at source row %zu (code=%d)\n", count + 1, parsed);
            fclose(fp);
            return 1;
        }
        SignalRow *row = &rows[count];
        row->price = trade.price;
        short_sum += trade.price;
        long_sum += trade.price;
        if (count >= SHORT_WINDOW) short_sum -= rows[count - SHORT_WINDOW].price;
        if (count >= LONG_WINDOW) long_sum -= rows[count - LONG_WINDOW].price;
        if (count + 1U >= LONG_WINDOW) {
            row->short_sma = short_sum / SHORT_WINDOW;
            row->long_sma = long_sum / LONG_WINDOW;
            row->ready = 1;
            const double diff = row->short_sma - row->long_sma;
            if (diff_ready && previous_diff <= 0.0 && diff > 0.0) {
                row->signal = 1;
                ++buys;
            } else if (diff_ready && previous_diff >= 0.0 && diff < 0.0) {
                row->signal = -1;
                ++sells;
            }
            if (row->signal != 0 && count + 1U >= 1500U && count + 1U <= 3500U) {
                if (first_signal == 0) first_signal = count + 1U;
                last_signal = count + 1U;
            }
            previous_diff = diff;
            diff_ready = 1;
        }
        ++count;
    }
    if (ferror(fp)) {
        fprintf(stderr, "signals: read error for '%s'\n", input);
        fclose(fp);
        return 1;
    }
    fclose(fp);

    int region_buys = 0;
    int region_sells = 0;
    if (first_signal != 0) {
        for (size_t ordinal = first_signal; ordinal <= last_signal; ++ordinal) {
            if (rows[ordinal - 1U].signal > 0) ++region_buys;
            if (rows[ordinal - 1U].signal < 0) ++region_sells;
        }
    }
    if (first_signal == 0 || region_buys == 0 || region_sells == 0) {
        fprintf(stderr, "signals: no region with both BUY and SELL signals in rows 1500..3500 (%d buys, %d sells total)\n",
                buys, sells);
        return 1;
    }

    const size_t start = first_signal > 200U ? first_signal - 200U : 1U;
    size_t end = last_signal + 200U;
    if (end > count) end = count;
    if (!ensure_output_directory(output_directory)) return 1;
    char data_path[1024];
    char script_path[1024];
    char png_path[1024];
    if (snprintf(data_path, sizeof data_path, "%s/signals.dat", output_directory) >= (int)sizeof data_path ||
        snprintf(script_path, sizeof script_path, "%s/signals.gnuplot", output_directory) >= (int)sizeof script_path ||
        snprintf(png_path, sizeof png_path, "%s/signals.png", output_directory) >= (int)sizeof png_path) {
        fputs("signals: output path is too long\n", stderr);
        return 1;
    }
    FILE *data = fopen(data_path, "w");
    FILE *script = fopen(script_path, "w");
    if (data == NULL || script == NULL) {
        fprintf(stderr, "signals: cannot create output files under '%s'\n", output_directory);
        if (data != NULL) fclose(data);
        if (script != NULL) fclose(script);
        return 1;
    }
    fprintf(data, "# implementation=%s source=src/signals.c input=%s rows_read=%zu short=%u long=%u first_qualifying_signal=%zu last_qualifying_signal=%zu\n",
            SIGNALS_IMPLEMENTATION_ID, input, count, SHORT_WINDOW, LONG_WINDOW, first_signal, last_signal);
    fprintf(data, "# ordinal price short_sma long_sma signal\n");
    for (size_t ordinal = start; ordinal <= end; ++ordinal) {
        const SignalRow *row = &rows[ordinal - 1U];
        if (!row->ready) continue;
        fprintf(data, "%zu %.12f %.12f %.12f %d\n", ordinal, row->price,
                row->short_sma, row->long_sma, row->signal);
    }
    int data_error = ferror(data);
    if (fclose(data) != 0) data_error = 1;
    fprintf(script, "# implementation=%s; source=src/signals.c; input=%s; short=30; long=60\n",
            SIGNALS_IMPLEMENTATION_ID, input);
    fprintf(script, "set terminal pngcairo size 1500,850 font 'Verdana,10'\n");
    fprintf(script, "set output '%s'\n", png_path);
    fprintf(script, "set title 'Tick-to-Trade signals (Group 05 development data)'\n");
    fprintf(script, "set xlabel 'Original trade ordinal'\nset ylabel 'Price'\nset key left top\n");
    fprintf(script, "plot '%s' using 1:2 with lines title 'price', '%s' using 1:3 with lines title 'SMA 30', '%s' using 1:4 with lines title 'SMA 60', '%s' using 1:($5==1 ? $2 : 1/0) with points pt 7 ps 1.2 lc rgb 'green' title 'BUY', '%s' using 1:($5==-1 ? $2 : 1/0) with points pt 7 ps 1.2 lc rgb 'red' title 'SELL'\n", data_path, data_path, data_path, data_path, data_path);
    int script_error = ferror(script);
    if (fclose(script) != 0) script_error = 1;
    if (data_error || script_error) {
        fputs("signals: failed while writing signal outputs\n", stderr);
        return 1;
    }
    printf("signals: rows=%zu range=%zu..%zu buys=%d sells=%d\ndata=%s\nscript=%s\npng=%s\n",
           count, start, end, region_buys, region_sells, data_path, script_path, png_path);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: signals <explicit-development-csv> [output-directory]");
        puts("Uses fixed Group 05 windows 30/60 and writes fresh data/script under build/native/signals by default.");
        return 0;
    }
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <explicit-development-csv> [output-directory]\n", argv[0]);
        return 2;
    }
    return generate(argv[1], argc == 3 ? argv[2] : "build/native/signals");
}
