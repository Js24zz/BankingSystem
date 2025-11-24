#ifndef BANKMENU_H
#define BANKMENU_H

#include "BankTypes.h"
#include "BankUtils.h"
#include "BankOps.h"
#include "BankStorage.h"

static void show_banner(void) {
    char ts[32]; now_string(ts, sizeof(ts));
    int n = count_accounts();
    printf("\n=============================================\n");
    printf("            BANKING SYSTEM - C\n");
    printf("  %s | Accounts: %d\n", ts, n);
    printf("=============================================\n");
}

static void show_menu(void) {
    printf("\nMenu (type number or keyword):\n");
    printf(" 1) create     - Create Account\n");
    printf(" 2) deposit    - Deposit\n");
    printf(" 3) withdraw   - Withdrawal\n");
    printf(" 4) remit      - Remittance\n");
    printf(" 5) view       - View Account\n");
    printf(" 6) list       - List Accounts\n");
    printf(" 7) delete     - Delete Account\n");
    printf(" 0) exit       - Exit\n");
}

static int match_choice(const char *in, const char *kw, int num) {
    if (strcmp(in, kw) == 0) return num;
    char *end = NULL;
    long v = strtol(in, &end, 10);
    if (end != in && *end == '\0' && (int)v == num) return num;
    return -1;
}

static void run_app(void) {
    for (;;) {
        show_banner();
        show_menu();
        char choice[32];
        read_line("\nSelect: ", choice, sizeof(choice));
        trim_spaces(choice);
        for (char *p = choice; *p; ++p) *p = (char)tolower((unsigned char)*p);

        if (match_choice(choice, "create", 1) != -1) { op_create(); }
        else if (match_choice(choice, "deposit", 2) != -1) { op_deposit(); }
        else if (match_choice(choice, "withdraw", 3) != -1) { op_withdraw(); }
        else if (match_choice(choice, "remit", 4) != -1) { op_remit(); }
        else if (match_choice(choice, "view", 5) != -1) { op_view(); }
        else if (match_choice(choice, "list", 6) != -1) { op_list(); }
        else if (match_choice(choice, "delete", 7) != -1) { op_delete(); }
        else if (match_choice(choice, "exit", 0) != -1) {
            append_log("--- SESSION END ---");
            printf("Goodbye. Wishing you prosperity every day!\n");
            break;
        } else {
            printf("Invalid option. Please select a valid menu option.\n");
        }
        press_enter_to_continue();
    }
}

#endif
