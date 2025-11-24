#ifndef BANKTYPES_H
#define BANKTYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
  #include <direct.h>
  #define MKDIR(path) _mkdir(path)
#else
  #include <sys/stat.h>
  #define MKDIR(path) mkdir(path, 0777)
#endif

#define DB_DIR "database"
#define INDEX_FILE "database/index.txt"
#define LOG_FILE "database/transaction.log"

#define MAX_NAME_LEN 63
#define ID_MAX_LEN 20
#define PIN_LEN 4
#define ACNUM_MAX 16

#define DEPOSIT_MIN 0.01
#define DEPOSIT_MAX 50000.0
#define WITHDRAW_MIN 0.01

typedef enum {
    ACC_SAVINGS = 1,
    ACC_CURRENT = 2
} AccountType;

typedef struct {
    char number[ACNUM_MAX];
    char name[MAX_NAME_LEN+1];
    char id[ID_MAX_LEN+1];
    char pin[PIN_LEN+1];
    AccountType type;
    double balance;
} Account;

#endif
