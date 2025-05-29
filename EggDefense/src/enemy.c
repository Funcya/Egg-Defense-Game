#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "engine.h"    

static float newCords(float a, float b, float t) {
    return a + (b - a) * t;
}

void update_enemies(GameState *gameState, float dt) {
    if (!gameState || dt <= 0.0f) return;

    Paths *paths = gameState->paths;          
    int numPoints = getNumPointsPaths(paths);

    for (int i = 0; i < gameState->numEnemiesActive; ) {
        Enemy *enemy = &gameState->enemies[i];

        if (!enemy->active) {
            if (i < gameState->numEnemiesActive - 1) {
                gameState->enemies[i] = gameState->enemies[gameState->numEnemiesActive - 1];
            }
            gameState->numEnemiesActive--;
            continue;
        }

        if (enemy->currentSegment < numPoints - 1) {
            SDL_Point p1 = (enemy->side == 0)
                ? leftPointPaths(paths, enemy->currentSegment)
                : rightPointPaths(paths, enemy->currentSegment);
            SDL_Point p2 = (enemy->side == 0)
                ? leftPointPaths(paths, enemy->currentSegment + 1)
                : rightPointPaths(paths, enemy->currentSegment + 1);

            float segDist = distance_between_points((float)p1.x, (float)p1.y,
                                                    (float)p2.x, (float)p2.y);
            float moveAmt = (enemy->speed * dt) / segDist;
            enemy->segmentProgress += moveAmt;

            if (enemy->segmentProgress >= 1.0f) {
                enemy->currentSegment++;
                if (enemy->currentSegment >= numPoints - 1) {
                    enemy->active = false;
                    int *hpPtr = (enemy->side == 0)
                        ? &gameState->leftPlayerHP
                        : &gameState->rightPlayerHP;
                    *hpPtr -= enemy->hp;
                    if (*hpPtr < 0) *hpPtr = 0;
                    continue;
                } else {
                    enemy->segmentProgress = 0.0f;
                }
            }

            p1 = (enemy->side == 0)
                ? leftPointPaths(paths, enemy->currentSegment)
                : rightPointPaths(paths, enemy->currentSegment);
            p2 = (enemy->side == 0)
                ? leftPointPaths(paths, enemy->currentSegment + 1)
                : rightPointPaths(paths, enemy->currentSegment + 1);

            enemy->x = newCords((float)p1.x, (float)p2.x, enemy->segmentProgress);
            enemy->y = newCords((float)p1.y, (float)p2.y, enemy->segmentProgress);

            float dx = (float)p2.x - (float)p1.x;
            float dy = (float)p2.y - (float)p1.y;
            enemy->angle = atan2f(dy, dx) * 180.0f / M_PI + 90.0f;
        }

        i++;
    }

    if (!gameState->gameOver &&
       (gameState->leftPlayerHP <= 0 || gameState->rightPlayerHP <= 0)) {
        gameState->gameOver = true;
        gameState->winner   = (gameState->leftPlayerHP <= 0) ? 1 : 0;
        printf("GAME OVER Condition Met (Detected in update_enemies).\n");
    }
}

void spawn_enemy_pair(GameState *gameState, GameResources *resources) {
    if (!gameState || !resources ||
        gameState->numEnemiesActive > MAX_ENEMIES - 2) {
        return;
    }

    gameState->enemySpawnCounter++;
    int type = gameState->enemySpawnCounter % 3;

    if (gameState->enemySpawnCounter % NEW_WAVE_BY_ENEMY_SPAWN == 0 &&
        gameState->enemySpawnCounter != 0) {
        gameState->inWaveDelay  = true;
        gameState->spawnCooldown = WAVE_PAUSE_LENGTH;
        return;
    }

    int baseHp = (type == 0 ? 1 : type == 1 ? 3 : 5);
    float scalingFactor = 1.0f;
    for (int i = 1; i <= gameState->currentWave; ++i) {
        scalingFactor *= (1.0f + 0.2f + 0.04f * (i - 1));
    }
    baseHp = (int)ceilf(baseHp * scalingFactor);

    int hp = baseHp;

    SDL_Texture *tex;
    if (hp <= 4)       tex = resources->enemyTextures[0];
    else if (hp <= 8)  tex = resources->enemyTextures[1];
    else               tex = resources->enemyTextures[2];

    float speed = 150.0f;
    Paths *paths = gameState->paths;

    Enemy *eL = &gameState->enemies[gameState->numEnemiesActive++];
    eL->active          = true;
    eL->side            = 0;
    eL->type            = type;
    eL->hp              = hp;
    eL->speed           = speed;
    eL->currentSegment  = 0;
    eL->segmentProgress = 0.0f;
    {
        SDL_Point start = leftPointPaths(paths, 0);
        eL->x      = (float)start.x;
        eL->y      = (float)start.y;
    }
    eL->angle   = 0.0f;
    eL->texture = tex;

    Enemy *eR = &gameState->enemies[gameState->numEnemiesActive++];
    eR->active          = true;
    eR->side            = 1;
    eR->type            = type;
    eR->hp              = hp;
    eR->speed           = speed;
    eR->currentSegment  = 0;
    eR->segmentProgress = 0.0f;
    {
        SDL_Point start = rightPointPaths(paths, 0);
        eR->x      = (float)start.x;
        eR->y      = (float)start.y;
    }
    eR->angle   = 0.0f;
    eR->texture = tex;
}
