#include "money_adt.h"
#include <stdlib.h> // För malloc, free
#include <stdio.h>  // För printf (felsökning)

// ----- Intern Datastruktur -----
// Definitionen av strukturen är dold här i .c-filen.
struct MoneyManagerData {
    int current_money;
    float money_timer;
    // Lägg till fler fält här om det behövs i framtiden
};

// ----- Funktionsimplementationer -----

void money_manager_set_balance(MoneyManager mm, int amount) {
    if (!mm) return; // Kolla om pekaren är giltig
    if (amount < 0) amount = 0; // Förhindra negativt saldo (valfritt)
    mm->current_money = amount;
    // Eventuellt återställa timern? Beror på hur du vill att det ska fungera.
    // mm->money_timer = 0.0f;
}

MoneyManager money_manager_create() {
    MoneyManager mm = (MoneyManager)malloc(sizeof(struct MoneyManagerData));
    if (!mm) {
        perror("Failed to allocate memory for MoneyManager");
        return NULL;
    }
    mm->current_money = START_MONEY; // Från defs.h
    mm->money_timer = 0.0f;
    printf("MoneyManager created. Initial balance: %d\n", mm->current_money);
    return mm;
}

void money_manager_destroy(MoneyManager mm) {
    if (mm) {
        free(mm);
        printf("MoneyManager destroyed.\n");
    }
}

void money_manager_update(MoneyManager mm, float dt) {
    if (!mm || dt <= 0) return;

    mm->money_timer += dt;
    if (mm->money_timer >= MONEY_INTERVAL) { 
        mm->money_timer -= MONEY_INTERVAL;
        mm->current_money += MONEY_GAIN; 
    }
}

int money_manager_get_balance(const MoneyManager mm) {
    if (!mm) return 0; 
    return mm->current_money;
}

bool money_manager_spend(MoneyManager mm, int amount) {
    if (!mm || amount < 0) return false;

    if (mm->current_money >= amount) {
        mm->current_money -= amount;
        printf("Spent %d. Remaining balance: %d\n", amount, mm->current_money);
        return true;
    } else {
        printf("Failed to spend %d. Insufficient funds (have %d).\n", amount, mm->current_money);
        return false;
    }
}

void money_manager_add(MoneyManager mm, int amount) {
     if (!mm || amount < 0) return; // Kan inte lägga till negativt
     mm->current_money += amount;
     printf("Added %d. New balance: %d\n", amount, mm->current_money);
}