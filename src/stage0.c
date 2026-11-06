/* Stage 0: checked integer summation and a repeatable O0/O1/O2/O3 exercise. */
#include "../provided/timing.h"
#include "../provided/led.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_EXPERIMENT_N UINT64_C(100000000)
#define CALIBRATION_N UINT64_C(10000)
#define CALIBRATION_BATCH 10U
#define DEFAULT_REPEATS 20U
#define MAX_REPEATS 10000U
#define MAX_RECURSION_N UINT64_C(20000)

static int parse_unsigned(const char *text, uint64_t limit, uint64_t *value_out) {
    if (text == NULL || *text == '\0') return 0;
    uint64_t value = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return 0;
        const uint64_t digit = (uint64_t)(*p - '0');
        if (value > (limit - digit) / 10U) return 0;
        value = value * 10U + digit;
    }
    *value_out = value;
    return 1;
}

static uint64_t sum_iterative(uint64_t n) {
    uint64_t total = 0;
    for (uint64_t i = 1; i <= n; ++i) total += i;
    return total;
}

static uint64_t sum_recursive(uint64_t n) {
    return n == 0 ? 0 : n + sum_recursive(n - 1U);
}

static int closed_form_sum(uint64_t n, uint64_t *sum_out) {
    uint64_t left = n;
    uint64_t right;
    if ((n & 1U) == 0) {
        left /= 2U;
        right = n + 1U;
    } else {
        right = n / 2U + 1U;
    }
    if (right != 0 && left > UINT64_MAX / right) return 0;
    *sum_out = left * right;
    return 1;
}

static int compare_u64(const void *a, const void *b) {
    const uint64_t x = *(const uint64_t *)a;
    const uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static uint64_t median_ns(uint64_t *samples, unsigned count) {
    qsort(samples, count, sizeof *samples, compare_u64);
    if ((count & 1U) != 0) return samples[count / 2U];
    const uint64_t a = samples[count / 2U - 1U];
    const uint64_t b = samples[count / 2U];
    return a + (b - a) / 2U;
}

static int benchmark_calibration(unsigned repeats) {
    uint64_t *iter_samples = (uint64_t *)malloc((size_t)repeats * sizeof *iter_samples);
    uint64_t *recur_samples = (uint64_t *)malloc((size_t)repeats * sizeof *recur_samples);
    if (iter_samples == NULL || recur_samples == NULL) {
        free(iter_samples);
        free(recur_samples);
        fputs("Stage 0: cannot allocate sample arrays.\n", stderr);
        return 0;
    }

    uint64_t iterative_checksum = 0;
    uint64_t recursive_checksum = 0;
    int correct = 1;
    for (unsigned i = 0; i < repeats; ++i) {
        uint64_t start = now_ns();
        for (unsigned j = 0; j < CALIBRATION_BATCH; ++j) {
            const uint64_t n = CALIBRATION_N + (uint64_t)(j & 1U);
            const uint64_t value = sum_iterative(n);
            iterative_checksum += value;
            if (value != n * (n + 1U) / 2U) correct = 0;
        }
        const uint64_t iter_elapsed = now_ns() - start;
        iter_samples[i] = iter_elapsed / CALIBRATION_BATCH;

        start = now_ns();
        for (unsigned j = 0; j < CALIBRATION_BATCH; ++j) {
            const uint64_t n = CALIBRATION_N + (uint64_t)(j & 1U);
            const uint64_t value = sum_recursive(n);
            recursive_checksum += value;
            if (value != n * (n + 1U) / 2U) correct = 0;
        }
        const uint64_t recur_elapsed = now_ns() - start;
        recur_samples[i] = recur_elapsed / CALIBRATION_BATCH;
    }
    if (!correct || iterative_checksum != recursive_checksum) {
        fputs("Stage 0: calibration correctness check failed.\n", stderr);
        free(iter_samples); free(recur_samples); return 0;
    }

    const uint64_t iterative_median = median_ns(iter_samples, repeats);
    const uint64_t recursive_median = median_ns(recur_samples, repeats);
    printf("calibration_n=%" PRIu64 " batch=%u repeats=%u iterative_median_ns_per_sum=%" PRIu64
           " recursive_median_ns_per_sum=%" PRIu64 " iterative_checksum=%" PRIu64
           " recursive_checksum=%" PRIu64 "\n",
           CALIBRATION_N, CALIBRATION_BATCH, repeats, iterative_median, recursive_median,
           iterative_checksum, recursive_checksum);
    free(iter_samples);
    free(recur_samples);
    return 1;
}

static void usage(const char *program) {
    printf("Usage: %s [--n N] [--repeat R]\n", program);
    puts("  --n N       sum 1..N (default 100000000; recursion is skipped above 20000)");
    puts("  --repeat R  calibration samples (default 20; range 1..10000)");
}

int main(int argc, char **argv) {
    uint64_t n = DEFAULT_EXPERIMENT_N;
    uint64_t repeat_value = DEFAULT_REPEATS;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) { usage(argv[0]); return 0; }
        if ((strcmp(argv[i], "--n") == 0 || strcmp(argv[i], "--repeat") == 0) && i + 1 < argc) {
            const int is_repeat = strcmp(argv[i], "--repeat") == 0;
            uint64_t parsed = 0;
            const uint64_t limit = is_repeat ? MAX_REPEATS : UINT64_MAX;
            if (!parse_unsigned(argv[++i], limit, &parsed) || (is_repeat && parsed == 0)) {
                fprintf(stderr, "Invalid %s value: %s\n", is_repeat ? "repeat" : "N", argv[i]);
                usage(argv[0]);
                return 2;
            }
            if (is_repeat) repeat_value = parsed;
            else n = parsed;
        } else {
            fprintf(stderr, "Unknown or incomplete argument: %s\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }

    uint64_t expected = 0;
    if (!closed_form_sum(n, &expected)) {
        fprintf(stderr, "N=%" PRIu64 " overflows the uint64_t sum.\n", n);
        return 3;
    }

    led_init();
    const uint64_t iterative = sum_iterative(n);
    printf("N=%" PRIu64 " iterative_sum=%" PRIu64 " expected_sum=%" PRIu64 "\n",
           n, iterative, expected);
    if (iterative != expected) {
        led_on(LED_RED); led_off(LED_GREEN); led_cleanup();
        fputs("Stage 0: iterative sum does not match checked closed form.\n", stderr);
        return 4;
    }

    if (n <= MAX_RECURSION_N) {
        const uint64_t recursive = sum_recursive(n);
        printf("recursive_sum=%" PRIu64 " recursion_status=OK\n", recursive);
        if (recursive != iterative) {
            led_on(LED_RED); led_off(LED_GREEN); led_cleanup();
            fputs("Stage 0: iterative and recursive sums differ.\n", stderr);
            return 5;
        }
    } else {
        puts("recursive_sum=SKIPPED recursion_status=bounded-depth");
    }

    led_off(LED_RED); led_on(LED_GREEN);
    const int benchmark_ok = benchmark_calibration((unsigned)repeat_value);
    led_off(LED_GREEN);
    led_cleanup();
    return benchmark_ok ? 0 : 6;
}
