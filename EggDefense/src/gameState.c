#include <string.h>
#include <stdio.h>
#include "engine.h"
#include "money_adt.h" // *** VIKTIGT: Inkludera den nya headerfilen ***

// Initialiserar GameState till standardvärden
void initialize_game_state(GameState *gameState, GameResources *resources) {
    if (!gameState || !resources) return;
    memset(gameState, 0, sizeof(GameState)); // Nollställer allt

    for (int t = 0; t < NUM_TEAMS; ++t) {
        gameState->team_money[t] = money_manager_create();
        if (!gameState->team_money[t]) {
            fprintf(stderr, "ERROR: Failed to create MoneyManager for team %d\n", t);
        }
    }

    // *** BEHÅLL ALLT DETTA ***
    gameState->leftPlayerHP = PLAYER_START_HP;
    gameState->rightPlayerHP = PLAYER_START_HP;
    gameState->spawnTimer = 0.0f;
    gameState->enemySpawnCounter = 0;
    gameState->numEnemiesActive = 0;
    gameState->numPlacedBirds = 0;
    gameState->numProjectiles = 0;
    gameState->placingBird = false;
    gameState->selectedOption = -1;
    gameState->gameOver = false;
    gameState->winner = -1;
    gameState->currentWave = 0;
    gameState->inWaveDelay = true;
    gameState->spawnCooldown = 4.0f;
    gameState->paths = createPaths();
    // *** SLUT PÅ BEHÅLL ***

    printf("Game state initialized (with MoneyManager).\n"); // Uppdaterat meddelande
}

// *** Lägg till en funktion för att städa upp, om du inte redan har en ***
// Denna kan anropas från t.ex. cleanup_sdl eller när spelet avslutas.
void cleanup_game_state(GameState *gameState) {
    for (int t = 0; t < NUM_TEAMS; ++t) {
        money_manager_destroy(gameState->team_money[t]);
    }
     // Lägg till annan städning för GameState här om det behövs
}
