#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include "engine.h"
#include "money_adt.h"

// Starts attack animation and plays sound
static void begin_attack_animation(Bird *bird, const Audio *audio) {
    // Reset animation timer
    bird->attackAnimTimer = 0.15f;
    // Switch to attack texture
    bird->texture = bird->attackTexture;
    // Play sound effect
    play_sound(audio, audio->popSound);
}

// Applies damage to the target enemy
static void apply_tower_damage(Enemy *target, const Bird *bird) {
    target->hp -= bird->damage;
    printf("Enemy type %d took %d damage, HP remaining: %d\n", target->type, bird->damage, target->hp);
    if (target->hp <= 0) {
        target->active = false;
    }
}

// Spawns a projectile from bird towards target
static void spawn_projectile(GameState *gameState, const Bird *bird, const Enemy *target)
{
    if (gameState->numProjectiles >= MAX_PROJECTILES) return;
    Projectile *newProj = &gameState->projectiles[gameState->numProjectiles++];
    newProj->active = true;
    newProj->x = bird->x;
    newProj->y = bird->y;
    newProj->texture = bird->projectileTexture;
    newProj->textureIndex = bird->projectileTextureIndex;

    float dx = target->x - bird->x;
    float dy = target->y - bird->y;
    float mag = sqrtf(dx * dx + dy * dy);
    if (mag > 0.01f) {
        newProj->vx = dx / mag;
        newProj->vy = dy / mag;
        newProj->angle = atan2f(dy, dx) * 180.0f / M_PI + 90.0f;
    } else {
        newProj->vx = 0;
        newProj->vy = -1;
        newProj->angle = 0;
    }
}

// Updates towers: target acquisition and firing
void update_towers(GameState *gameState,
                   const Audio *audio,
                   float dt,
                   GameResources *resources)
{
    if (!gameState || dt <= 0 || !resources) return;

    for (int i = 0; i < gameState->numPlacedBirds; i++) {
        Bird *bird = &gameState->placedBirds[i];
        if (!bird->active) continue;

        // Update attack animation timer
        if (bird->attackAnimTimer > 0) {
            bird->attackAnimTimer -= dt;
            if (bird->attackAnimTimer <= 0) {
                bird->attackAnimTimer = 0;
                if (bird->baseTexture) {
                    bird->texture = bird->baseTexture;
                }
            }
        }

        // Increment attack cooldown
        bird->attackTimer += dt;

        // Target acquisition
        Enemy *target = NULL;
        float bestProgress = -1.0f;
        bool birdIsLeft   = (bird->x < WINDOW_WIDTH * 0.48f);
        bool birdIsRight  = (bird->x > WINDOW_WIDTH * 0.52f);
        bool birdIsCenter = !birdIsLeft && !birdIsRight;

        for (int j = 0; j < gameState->numEnemiesActive; j++) {
            Enemy *enemy = &gameState->enemies[j];
            if (!enemy->active) continue;
            if (!birdIsCenter) {
                if (birdIsLeft && enemy->side != 0) continue;
                if (birdIsRight && enemy->side != 1) continue;
            }
            float dist = distance_between_points(
                bird->x, bird->y, enemy->x, enemy->y);
            if (dist <= bird->range) {
                float progress = (float)enemy->currentSegment +
                                 enemy->segmentProgress;
                if (progress > bestProgress) {
                    bestProgress = progress;
                    target = enemy;
                }
            }
        }

        if (target && bird->attackTimer >= (1.0f / bird->attackSpeed)) {
            // Reset cooldown
            bird->attackTimer = 0.0f;
            begin_attack_animation(bird, audio);
            apply_tower_damage(target, bird);
            // make enemy texture "step down" when taking damage
            if (target->hp > 0) {
                if (target->hp <= 1) target->texture = resources->enemyTextures[0];
                else if (target->hp <= 3) target->texture = resources->enemyTextures[1];
                else target->texture = resources->enemyTextures[2];
            }
            spawn_projectile(gameState, bird, target);
        }
    }
}

// function has repetitive code which is going to be changed!
// Calculates the visual rotation for each tower based on its current target
void calculate_tower_rotations(GameState *gameState, float birdRotations[]) {
    if (!gameState || !birdRotations) return;

    for (int i = 0; i < MAX_PLACED_BIRDS; ++i) {
        birdRotations[i] = 0.0f;
    }

    for (int i = 0; i < gameState->numPlacedBirds; i++) {
        Bird *bird = &gameState->placedBirds[i];
        if (!bird->active) continue;


        Enemy *target = NULL;
        float bestProgress = -1.0f;
        bool birdIsLeft   = (bird->x < WINDOW_WIDTH * 0.48f);
        bool birdIsRight  = (bird->x > WINDOW_WIDTH * 0.52f);
        bool birdIsCenter = !birdIsLeft && !birdIsRight;

        for (int j = 0; j < gameState->numEnemiesActive; j++) {
            Enemy *enemy = &gameState->enemies[j];
            if (!enemy->active) continue;
            if (!birdIsCenter) {
                if (birdIsLeft && enemy->side != 0) continue;
                if (birdIsRight && enemy->side != 1) continue;
            }
            float dist = distance_between_points(
                bird->x, bird->y, enemy->x, enemy->y);
            if (dist <= bird->range) {
                float progress = (float)enemy->currentSegment +
                                 enemy->segmentProgress;
                if (progress > bestProgress) {
                    bestProgress = progress;
                    target = enemy;
                }
            }
        }

        if (target) {
            float dx = target->x - bird->x;
            float dy = target->y - bird->y;
            birdRotations[i] = atan2f(dy, dx) * 180.0f / M_PI + 90.0f;
            
        }
    }
}

bool place_tower(GameState *gameState,
                 GameResources *resources,
                 int towerTypeIndex,
                 int x,
                 int y,
                 int ownerPlayerIndex)
{
    if (!gameState || gameState->numPlacedBirds >= MAX_PLACED_BIRDS) {
         return false;
    }

    const TowerOption *option = &resources->towerOptions[towerTypeIndex];
    int cost = option->prototype.cost;

    Team team = (ownerPlayerIndex == 0 || ownerPlayerIndex == 2) ? TEAM_LEFT : TEAM_RIGHT;
    if (money_manager_get_balance(gameState->team_money[team]) < cost) {
         printf("Cannot afford tower (cost %d, balance %d).\n", cost, money_manager_get_balance(gameState->team_money[team]));
         return false;
    }

    int leftBoundary  = (int)(WINDOW_WIDTH * 0.45);
    int rightBoundary = (int)(WINDOW_WIDTH * 0.55);
    if (x >= leftBoundary && x <= rightBoundary) return false;

    if (!money_manager_spend(gameState->team_money[team], cost)) {
        printf("Spending %d failed unexpectedly.\n", cost);
        return false;
    }
    Bird *newBird = &gameState->placedBirds[gameState->numPlacedBirds];
    *newBird = option->prototype;
    newBird->active           = true;
    newBird->x                = (float)x;
    newBird->y                = (float)y;
    newBird->ownerPlayerIndex = ownerPlayerIndex;
    newBird->attackTimer      = 0.0f;
    newBird->attackAnimTimer  = 0.0f;
    newBird->rotation         = 0.0f;
    newBird->texture          = newBird->baseTexture;
    gameState->numPlacedBirds++;
    int newBalance = money_manager_get_balance(gameState->team_money[team]);
    printf("Placed tower type %d at (%d,%d) by player %d. Money left: %d\n", towerTypeIndex, x, y, ownerPlayerIndex, newBalance);

    return true;
}
