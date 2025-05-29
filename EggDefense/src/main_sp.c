#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "engine.h"
#include "paths.h"

void run_singleplayer() {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    GameResources resources = {0};
    Audio audio = {0};
    GameState gameState = {0};
    float birdRotations[MAX_PLACED_BIRDS] = {0};

    if (!initialize_sdl(&window, &renderer, "Tower Defense - Singleplayer")) return;
    if (!initialize_subsystems()) { cleanup_sdl(window, renderer); return; }
    if (!load_resources(renderer, &resources, &audio)) {
        cleanup_resources(&resources, &audio); cleanup_subsystems(); cleanup_sdl(window, renderer); return;
    }
    initialize_game_state(&gameState, &resources);

    bool quit = false;
    GameStatus currentStatus = GAME_STATE_MAIN_MENU;
    Uint32 last_time = SDL_GetTicks();

    while (!quit) {
        float dt = get_delta_time(&last_time);
        InputContext inputCtx = (currentStatus == GAME_STATE_MAIN_MENU) ? INPUT_CONTEXT_MAIN_MENU : INPUT_CONTEXT_SINGLEPLAYER;
        handle_input(inputCtx, &gameState, &resources, NULL, &quit);
        if (quit) break;

        switch (currentStatus) {
            case GAME_STATE_MAIN_MENU:
                const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
                if (keyboardState[SDL_SCANCODE_SPACE]) {
                    currentStatus = GAME_STATE_PLAYING;
                     play_music(audio.bgm);
                    last_time = SDL_GetTicks();
                }
                render_main_menu(renderer, &resources, MODE_SINGLEPLAYER);
                break;

            case GAME_STATE_PLAYING:
                //updating game
                for (int t = 0; t < NUM_TEAMS; ++t) {
                    money_manager_update(gameState.team_money[t], dt);
                }
                update_enemies(&gameState, dt);
                update_towers(&gameState, &audio, dt, &resources);
                update_projectiles(&gameState, dt);
                if (gameState.inWaveDelay) {
                    gameState.spawnCooldown -= dt;
                    if (gameState.spawnCooldown <= 0.0f) { // start next wave when wave cool down timer is finished
                        gameState.inWaveDelay = false;
                        if (gameState.currentWave>0)
                        {
                            play_sound(&audio, audio.levelUpSound); //play level up sound effect at new wave, except first
                            gameState.spawnTimer = ENEMY_SPAWN_INTERVAL; //spawn new enemies instantly when new wave starts, exept first
                        }
                        
                        gameState.currentWave++;
                    }
                }
                else {
                    gameState.spawnTimer += dt;
                    if (gameState.spawnTimer >= ENEMY_SPAWN_INTERVAL) {
                        gameState.spawnTimer = 0;
                        spawn_enemy_pair(&gameState, &resources);
                    }
                }
                
                //rendering new frame
                calculate_tower_rotations(&gameState, birdRotations);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                render_game(renderer, &gameState, &resources, birdRotations, gameState.placingBird, gameState.selectedOption, -1);
                if (gameState.gameOver) {
                    currentStatus = GAME_STATE_GAME_OVER;
                    stop_music();
                }
                break;

            case GAME_STATE_GAME_OVER:
                {
                 char gameOverMsg[128];
                 if (gameState.leftPlayerHP <= 0 && gameState.rightPlayerHP <= 0) { snprintf(gameOverMsg, sizeof(gameOverMsg), "GAME OVER - DRAW!"); }
                 else if (gameState.leftPlayerHP <= 0) { snprintf(gameOverMsg, sizeof(gameOverMsg), "GAME OVER - Player 1 Loses!"); }
                 else if (gameState.rightPlayerHP <= 0) { snprintf(gameOverMsg, sizeof(gameOverMsg), "GAME OVER - Player 2 Loses!"); }
                 else { snprintf(gameOverMsg, sizeof(gameOverMsg), "GAME OVER"); }
                 render_game_over(renderer, &resources, gameOverMsg);
                }
                break;
        }

        if (currentStatus != GAME_STATE_MAIN_MENU) {
            SDL_RenderPresent(renderer);
        }
    }

    printf("Shutting down singleplayer...\n");
    cleanup_resources(&resources, &audio);
    cleanup_subsystems();
    cleanup_sdl(window, renderer);
    printf("Singleplayer shutdown complete.\n");
}
