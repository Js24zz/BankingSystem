#include "BankTypes.h"
#include "BankUtils.h"
#include "BankStorage.h"
#include "BankOps.h"
#include "BankMenu.h"

int main(void) {
    storage_init();
    run_app();
    return 0;
}
