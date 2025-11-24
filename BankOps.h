#ifndef BANKOPS_H
#define BANKOPS_H

#include "BankTypes.h"
#include "BankUtils.h"
#include "BankStorage.h"

static int read_pin(char *pinbuf, size_t sz) {
    char p[32];
    read_line("Enter 4-digit PIN: ", p, sizeof(p));
    if (strlen(p) == PIN_LEN && is_all_digits(p)) {
        strncpy(pinbuf, p, sz-1);
        pinbuf[sz-1] = '\0';
        return 1;
    }
    printf("Invalid PIN. Please enter exactly 4 digits.\n");
    return 0;
}

static int auth_load(const char *acnum, const char *purpose, Account *out) {
    if (!valid_acnum(acnum)) { printf("Invalid account number format.\n"); return 0; }
    Account a;
    if (!load_account(acnum, &a)) { printf("Account not found.\n"); return 0; }
    for (int tries = 1; tries <= 3; ++tries) {
        char pin[PIN_LEN+1];
        if (!read_pin(pin, sizeof(pin))) { continue; }
        if (strcmp(pin, a.pin) == 0) {
            *out = a;
            char ok[256]; snprintf(ok, sizeof(ok), "AUTH %s ok for %s", a.number, purpose);
            append_log(ok);
            return 1;
        }
        printf("Incorrect PIN (attempt %d/3).\n", tries);
    }
    char fail[256]; snprintf(fail, sizeof(fail), "AUTH %s fail for %s", acnum, purpose);
    append_log(fail);
    return 0;
}

static void op_create(void) {
    Account a; memset(&a, 0, sizeof(a));
    generate_account_number(a.number, sizeof(a.number));
    printf("\n-- Create Account --\n");
    printf("Assigned Account No: %s\n", a.number);

    char name[128];
    do { read_line("Enter full name: ", name, sizeof(name)); trim_spaces(name); }
    while (strlen(name) == 0 || strlen(name) > MAX_NAME_LEN);
    strncpy(a.name, name, MAX_NAME_LEN);

    char id[64];
    do { read_line("Enter Identification Number (ID): ", id, sizeof(id)); trim_spaces(id); }
    while (strlen(id) < 4 || strlen(id) > ID_MAX_LEN);
    strncpy(a.id, id, ID_MAX_LEN);

    char pin[16];
    while (!read_pin(pin, sizeof(pin)));
    strncpy(a.pin, pin, PIN_LEN);

    printf("Account Type:\n  1) Savings\n  2) Current\n");
    int t = read_int_range("Choose 1-2: ", 1, 2);
    a.type = (t == 1) ? ACC_SAVINGS : ACC_CURRENT;

    a.balance = 0.00;

    if (!save_account(&a)) { printf("Error: could not save account.\n"); return; }
    index_add(a.number);

    char logbuf[256];
    snprintf(logbuf, sizeof(logbuf), "CREATE %s name=%s type=%d balance=%.2f",
             a.number, a.name, (int)a.type, a.balance);
    append_log(logbuf);

    printf("Account created successfully.\n");
}

static void op_view(void) {
    char ac[32];
    read_line("\nEnter account number to view: ", ac, sizeof(ac));
    trim_spaces(ac);
    if (!valid_acnum(ac)) { printf("Invalid account number format.\n"); return; }
    Account a;
    if (!load_account(ac, &a)) { printf("Account not found.\n"); return; }
    printf("\nAccount: %s\nName   : %s\nID     : %s\nType   : %s\nBalance: RM %.2f\n",
           a.number, a.name, a.id, a.type==ACC_SAVINGS?"Savings":"Current", a.balance);
    append_log("VIEW account");
}

static void op_deposit(void) {
    char ac[32];
    read_line("\nEnter account number: ", ac, sizeof(ac));
    trim_spaces(ac);

    Account a;
    if (!auth_load(ac, "deposit", &a)) return;

    double amt = read_money_two_dp("Deposit amount (0 < amt <= 50000): RM ", 0.01, 50000.0);
    a.balance = round2(a.balance + amt);
    if (!save_account(&a)) { printf("Error: save failed.\n"); return; }

    char logbuf[256];
    snprintf(logbuf, sizeof(logbuf), "DEPOSIT %s amount=%.2f newbal=%.2f", a.number, amt, a.balance);
    append_log(logbuf);

    printf("Deposit successful. New balance: RM %.2f\n", a.balance);
}

static void op_withdraw(void) {
    char ac[32];
    read_line("\nEnter account number: ", ac, sizeof(ac));
    trim_spaces(ac);
    Account a;
    if (!auth_load(ac, "withdraw", &a)) return;

    printf("Available balance: RM %.2f\n", a.balance);

    double amt;
    for (;;) {
        amt = read_money_two_dp("Withdraw amount (> 0): RM ", 0.01, 1000000000000.0);
        if (amt > a.balance) {
            printf("Oh no. Insufficient funds. Please try again.\n");
            continue;
        }
        break;
    }

    a.balance = round2(a.balance - amt);
    if (!save_account(&a)) { printf("Error: save failed.\n"); return; }

    char logbuf[256];
    snprintf(logbuf, sizeof(logbuf), "WITHDRAW %s amount=%.2f newbal=%.2f", a.number, amt, a.balance);
    append_log(logbuf);

    printf("Withdrawal successful. New balance: RM %.2f\n", a.balance);
}

static double remittance_fee(AccountType from, AccountType to, double amt) {
    if (from == ACC_SAVINGS && to == ACC_CURRENT) return round2(amt * 0.02);
    if (from == ACC_CURRENT && to == ACC_SAVINGS) return round2(amt * 0.03);
    return 0.0;
}

static void op_remit(void) {
    char from[32], to[32];
    printf("\n-- Remittance --\n");
    read_line("From account number: ", from, sizeof(from));
    read_line("To account number  : ", to, sizeof(to));
    trim_spaces(from); trim_spaces(to);
    if (!valid_acnum(from) || !valid_acnum(to)) { printf("Invalid account number format.\n"); return; }
    if (strcmp(from, to) == 0) { printf("Cannot remit to the same account.\n"); return; }

    Account A, B;
    if (!auth_load(from, "remittance", &A)) return;
    if (!load_account(to, &B)) { printf("Destination account not found.\n"); return; }

    double amt, fee, total_deduct;
    for (;;) {
        amt = read_money_two_dp("Amount to transfer (> 0): RM ", 0.01, 1000000000000.0);
        fee = remittance_fee(A.type, B.type, amt);
        total_deduct = round2(amt + fee);
        if (total_deduct > A.balance) {
            if (fee > 0.0)
                printf("Oh no. Insufficient funds (includes fee RM %.2f). Please try again.\n", fee);
            else
                printf("Oh no. Insufficient funds. Please try again.\n");
            continue;
        }
        break;
    }

    A.balance = round2(A.balance - total_deduct);
    B.balance = round2(B.balance + amt);

    if (!save_account(&A) || !save_account(&B)) { printf("Error: transfer failed while saving.\n"); return; }

    char logbuf[256];
    snprintf(logbuf, sizeof(logbuf), "REMIT %s->%s amount=%.2f fee=%.2f balFrom=%.2f balTo=%.2f",
             A.number, B.number, amt, fee, A.balance, B.balance);
    append_log(logbuf);

    printf("Remittance successful. Fee charged: RM %.2f\n", fee);
}

static void op_list(void) {
    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) { printf("No accounts.\n"); return; }
    printf("\n-- Accounts --\n");
    char ac[64];
    int i = 0;
    while (fgets(ac, sizeof(ac), f)) {
        trim_newline(ac); trim_spaces(ac);
        if (!ac[0]) continue;
        Account a;
        if (load_account(ac, &a)) {
            printf("%2d) %s  %-16s  RM %.2f  (%s)\n", ++i,
                   a.number, a.name, a.balance,
                   a.type==ACC_SAVINGS?"Savings":"Current");
        }
    }
    if (i == 0) printf("No accounts found.\n");
    fclose(f);
    append_log("LIST accounts");
}

static void op_delete(void) {
    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) { printf("No accounts.\n"); return; }

    char numbers[512][ACNUM_MAX];
    int n = 0;
    printf("\n-- Delete Account --\n");
    while (n < 512 && fgets(numbers[n], sizeof(numbers[n]), f)) {
        trim_newline(numbers[n]); trim_spaces(numbers[n]);
        if (numbers[n][0]) {
            Account a;
            if (load_account(numbers[n], &a)) {
                printf("%2d) %s  %-16s\n", n+1, a.number, a.name);
                n++;
            }
        }
    }
    fclose(f);
    if (n == 0) { printf("No accounts to delete.\n"); return; }

    int idx = read_int_range("Choose account number to delete (by index): ", 1, n);
    const char *ac = numbers[idx-1];

    Account a;
    if (!load_account(ac, &a)) { printf("Account not found.\n"); return; }

    char accheck[32]; read_line("Confirm account number: ", accheck, sizeof(accheck));
    trim_spaces(accheck);
    if (!valid_acnum(accheck) || strcmp(accheck, a.number) != 0) { printf("Confirmation failed.\n"); return; }

    char last4[8]; read_line("Enter last 4 of ID: ", last4, sizeof(last4));
    size_t idlen = strlen(a.id);
    if (strlen(last4) != 4 || (idlen < 4) || strncmp(last4, a.id + (idlen - 4), 4) != 0) {
        printf("ID verification failed.\n"); return;
    }

    for (int tries = 1; tries <= 3; ++tries) {
        char pin[PIN_LEN+1];
        if (!read_pin(pin, sizeof(pin))) continue;
        if (strcmp(pin, a.pin) == 0) goto PIN_OK;
        printf("Incorrect PIN (attempt %d/3).\n", tries);
    }
    printf("PIN verification failed.\n");
    return;

PIN_OK:
    if (!delete_account_file(ac)) { printf("Error: could not delete file.\n"); return; }
    index_remove(ac);

    char logbuf[128];
    snprintf(logbuf, sizeof(logbuf), "DELETE %s", ac);
    append_log(logbuf);

    printf("Account deleted.\n");
}

#endif
