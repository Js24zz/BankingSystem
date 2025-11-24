#ifndef BANKSTORAGE_H
#define BANKSTORAGE_H

#include "BankTypes.h"
#include "BankUtils.h"

static void storage_init(void) {
    MKDIR(DB_DIR);
    FILE *f = fopen(INDEX_FILE, "a");
    if (f) fclose(f);
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        char ts[32]; now_string(ts, sizeof(ts));
        fprintf(log, "[%s] --- SESSION START ---\n", ts);
        fclose(log);
    }
    srand((unsigned)time(NULL));
}

static void append_log(const char *event) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    char ts[32]; now_string(ts, sizeof(ts));
    fprintf(log, "[%s] %s\n", ts, event);
    fclose(log);
}

static void account_path(const char *number, char *out, size_t sz) {
    snprintf(out, sz, DB_DIR"/%s.txt", number);
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

static int index_contains(const char *number) {
    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return 0;
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        trim_newline(line); trim_spaces(line);
        if (strcmp(line, number) == 0) { fclose(f); return 1; }
    }
    fclose(f);
    return 0;
}

static void index_add(const char *number) {
    if (index_contains(number)) return;
    FILE *f = fopen(INDEX_FILE, "a");
    if (f) { fprintf(f, "%s\n", number); fclose(f); }
}

static void index_remove(const char *number) {
    FILE *in = fopen(INDEX_FILE, "r");
    if (!in) return;
    char tmpPath[128] = DB_DIR"/index.tmp";
    FILE *out = fopen(tmpPath, "w");
    if (!out) { fclose(in); return; }
    char line[64];
    while (fgets(line, sizeof(line), in)) {
        trim_newline(line); trim_spaces(line);
        if (strcmp(line, number) != 0 && line[0] != '\0')
            fprintf(out, "%s\n", line);
    }
    fclose(in); fclose(out);
    remove(INDEX_FILE);
    rename(tmpPath, INDEX_FILE);
}

static void generate_account_number(char *out, size_t sz) {
    for (;;) {
        int digits = 7 + rand() % 3;
        long base = 1;
        for (int i = 1; i < digits; ++i) base *= 10;
        long n = base + rand() % (9 * base);
        snprintf(out, sz, "%ld", n);
        char path[128]; account_path(out, path, sizeof(path));
        if (!file_exists(path) && !index_contains(out)) return;
    }
}

static int save_account(const Account *a) {
    char path[128]; account_path(a->number, path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "number=%s\n", a->number);
    fprintf(f, "name=%s\n", a->name);
    fprintf(f, "id=%s\n", a->id);
    fprintf(f, "pin=%s\n", a->pin);
    fprintf(f, "type=%d\n", (int)a->type);
    fprintf(f, "balance=%.2f\n", a->balance);
    fclose(f);
    return 1;
}

static int load_account(const char *number, Account *out) {
    char path[128]; account_path(number, path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char key[32], val[256];
    Account a; memset(&a, 0, sizeof(a));
    strncpy(a.number, number, ACNUM_MAX-1);
    while (fscanf(f, "%31[^=]=%255[^\n]\n", key, val) == 2) {
        if (strcmp(key, "name") == 0) {
            strncpy(a.name, val, MAX_NAME_LEN);
        } else if (strcmp(key, "id") == 0) {
            strncpy(a.id, val, ID_MAX_LEN);
        } else if (strcmp(key, "pin") == 0) {
            strncpy(a.pin, val, PIN_LEN);
        } else if (strcmp(key, "type") == 0) {
            a.type = (AccountType)atoi(val);
        } else if (strcmp(key, "balance") == 0) {
            a.balance = atof(val);
        }
    }
    fclose(f);
    *out = a;
    return 1;
}

static int delete_account_file(const char *number) {
    char path[128]; account_path(number, path, sizeof(path));
    return remove(path) == 0;
}

static int count_accounts(void) {
    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return 0;
    int c = 0; char line[64];
    while (fgets(line, sizeof(line), f)) {
        trim_newline(line); trim_spaces(line);
        if (line[0]) c++;
    }
    fclose(f);
    return c;
}

#endif
