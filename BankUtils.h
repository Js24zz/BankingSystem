#ifndef BANKUTILS_H
#define BANKUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

static void trim_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) { s[--n] = '\0'; }
}

static void trim_spaces(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    size_t i = 0;
    while (i < len && isspace((unsigned char)s[i])) i++;
    size_t j = len;
    while (j > i && isspace((unsigned char)s[j-1])) j--;
    size_t k = 0;
    while (i < j) s[k++] = s[i++];
    s[k] = '\0';
}

static int is_all_digits(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; ++p) if (!isdigit((unsigned char)*p)) return 0;
    return 1;
}

static int valid_acnum(const char *s) {
    if (!s) return 0;
    size_t len = strlen(s);
    if (len < 7 || len > 9) return 0;
    return is_all_digits(s);
}

static void read_line(const char *prompt, char *buf, size_t sz) {
    if (prompt) printf("%s", prompt);
    if (fgets(buf, (int)sz, stdin) == NULL) { buf[0] = '\0'; return; }
    trim_newline(buf);
}

static int read_int_range(const char *prompt, int minv, int maxv) {
    char line[128];
    for (;;) {
        read_line(prompt, line, sizeof(line));
        char *end = NULL;
        long x = strtol(line, &end, 10);
        if (end != line && *end == '\0' && x >= minv && x <= maxv) return (int)x;
        printf("Invalid input. Please enter a number between %d and %d.\n", minv, maxv);
    }
}

static double round2(double x) {
    long long t = (long long)(x * 100.0 + 0.5);
    return t / 100.0;
}

static double read_money_two_dp(const char *prompt, double minv, double maxv) {
    char line[128];
    for (;;) {
        read_line(prompt, line, sizeof(line));
        trim_spaces(line);
        if (!line[0]) { printf("Invalid amount.\n"); continue; }
        int dots = 0, ok = 1;
        for (const char *p = line; *p; ++p) {
            if (*p == '.') { if (++dots > 1) { ok = 0; break; } }
            else if (!isdigit((unsigned char)*p)) { ok = 0; break; }
        }
        if (!ok) { printf("Use digits only, optional '.' and max 2 decimals.\n"); continue; }
        char *dot = strchr(line, '.');
        if (dot && strlen(dot + 1) > 2) { printf("Use at most 2 decimal places.\n"); continue; }
        char *end = NULL;
        double v = strtod(line, &end);
        if (end == line || *end != '\0') { printf("Invalid amount.\n"); continue; }
        if (v < minv || v > maxv) { printf("Enter a value between %.2f and %.2f.\n", minv, maxv); continue; }
        return round2(v);
    }
}

static void now_string(char *buf, size_t sz) {
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    strftime(buf, sz, "%Y-%m-%d %H:%M:%S", tmv);
}

static void press_enter_to_continue(void) {
    printf("\nPress ENTER to continue...");
    fflush(stdout);
    char c[8];
    fgets(c, sizeof(c), stdin);
}

#endif

