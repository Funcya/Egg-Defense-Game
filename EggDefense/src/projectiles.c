#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include "engine.h"

// Updates projectiles: movement, boundary checks, collision detection (marks inactive on hit)
void update_projectiles(GameState *gameState, float dt) {
    if (!gameState || dt <= 0) return;

    for (int i = 0; i < gameState->numProjectiles; i++) {
        Projectile *proj = &gameState->projectiles[i];
        
        // Movement
        proj->x += proj->vx * PROJECTILE_SPEED * dt;
        proj->y += proj->vy * PROJECTILE_SPEED * dt;

        // Boundary Check
        if (proj->x < 0 || proj->x > WINDOW_WIDTH ||
            proj->y < 0 || proj->y > WINDOW_HEIGHT ) {
            gameState->projectiles[i] = gameState->projectiles[--gameState->numProjectiles];
        }

        // Collision Check
        bool hit = false;
        for (int j = 0; j < gameState->numEnemiesActive; j++) {
            Enemy *enemy = &gameState->enemies[j];
            if (!enemy->active) continue;

            float dx = proj->x - enemy->x;
            float dy = proj->y - enemy->y;
            if (dx*dx + dy*dy < 100) {
                hit = true;
                break;
            }
        }
        if (hit) {
            // remove projectile on collision
            gameState->projectiles[i] = gameState->projectiles[--gameState->numProjectiles];
        }
    }
}
