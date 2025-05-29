#ifndef MONEY_ADT_H
#define MONEY_ADT_H

#include <stdbool.h>
#include "defs.h" // För START_MONEY, MONEY_INTERVAL, MONEY_GAIN

// ----- Datastruktur (Opaque Pointer för bättre inkapsling) -----
// Vi deklarerar bara att typen finns här. Definitionen finns i .c-filen.
typedef struct MoneyManagerData* MoneyManager;

// ----- Funktionsdeklarationer (Gränssnitt) -----

/**
 * @brief Skapar och initialiserar en ny pengahanterare.
 * @return En pekare till den nya MoneyManager-instansen, eller NULL vid fel.
 */
MoneyManager money_manager_create();

void money_manager_set_balance(MoneyManager mm, int amount);

/**
 * @brief Frigör minnet som används av pengahanteraren.
 * @param mm Pekare till pengahanteraren som ska förstöras.
 */
void money_manager_destroy(MoneyManager mm);

/**
 * @brief Uppdaterar pengahanteraren baserat på förfluten tid (dt).
 * Hanterar den periodiska pengaökningen.
 * @param mm Pekare till pengahanteraren.
 * @param dt Förfluten tid i sekunder sedan senaste uppdatering.
 */
void money_manager_update(MoneyManager mm, float dt);

/**
 * @brief Hämtar den nuvarande pengasumman.
 * @param mm Pekare till pengahanteraren.
 * @return Den nuvarande pengasumman.
 */
int money_manager_get_balance(const MoneyManager mm);

/**
 * @brief Försöker spendera en viss summa pengar.
 * @param mm Pekare till pengahanteraren.
 * @param amount Summan som ska spenderas.
 * @return true om pengarna kunde spenderas (tillräckligt saldo), annars false.
 */
bool money_manager_spend(MoneyManager mm, int amount);

/**
 * @brief Lägger till en viss summa pengar.
 * @param mm Pekare till pengahanteraren.
 * @param amount Summan som ska läggas till.
 */
void money_manager_add(MoneyManager mm, int amount);

#endif // MONEY_ADT_H