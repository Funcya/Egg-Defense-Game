#include <SDL2/SDL.h>
#include <stdio.h>

#include "engine.h"
#include "defs.h"  // för WINDOW_WIDTH
#include "money_adt.h"

// Handles input based on the current game mode/context
// Returns true if quit was requested, false otherwise
// Sets the quit_flag_ptr directly if quit is requested
void handle_input(InputContext context,
                  GameState *gameState,
                  GameResources *resources,
                  ClientInstance *client,
                  bool *quit_flag_ptr)
{
    if (!quit_flag_ptr) return;

    SDL_Event event;
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    int leftBoundary  = (int)(WINDOW_WIDTH * 0.48f);
    int rightBoundary = (int)(WINDOW_WIDTH * 0.52f);

    while (SDL_PollEvent(&event)) {
        // Global quit
        if (event.type == SDL_QUIT) {
            printf("SDL_QUIT event detected!\n");
            *quit_flag_ptr = true;
        }
        // ESC: cancel placement or quit
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            bool wasPlacing = false;
            if (context == INPUT_CONTEXT_SINGLEPLAYER && gameState && gameState->placingBird) {
                gameState->placingBird   = false;
                gameState->selectedOption = -1;
                wasPlacing = true;
                printf("Placement cancelled via ESC.\n");
            }
            else if (context == INPUT_CONTEXT_CLIENT && client && client->placingBird) {
                client->placingBird      = false;
                client->selectedOption   = -1;
                wasPlacing = true;
                printf("Placement cancelled via ESC.\n");
            }
            if (!wasPlacing) {
                printf("ESC pressed - setting quit flag.\n");
                *quit_flag_ptr = true;
            }
        }

        // Context-specific input
        if (context == INPUT_CONTEXT_MAIN_MENU) {
            // Handled in main loop
        }
        else if (context == INPUT_CONTEXT_SINGLEPLAYER && gameState && resources) {
            int clickX = event.button.x;
            int clickY = event.button.y;
            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT) {
                if (!gameState->placingBird) {
                    // Välj torn-ikon
                    for (int i = 0; i < 3; ++i) {
                        SDL_Rect ir = resources->towerOptions[i].iconRect;
                        if (clickX >= ir.x && clickX <= ir.x + ir.w
                         && clickY >= ir.y && clickY <= ir.y + ir.h) {
                            // Använd TEAM_LEFT som standard-lag i singleplayer
                            int current_balance = money_manager_get_balance(
                                gameState->team_money[TEAM_LEFT]
                            );
                            int tower_cost = resources->towerOptions[i].prototype.cost;

                            if (current_balance >= tower_cost) {
                                printf("Selected tower type %d for placement.\n", i);
                                gameState->placingBird   = true;
                                gameState->selectedOption = i;
                            } else {
                                printf("Cannot afford tower (cost %d, money %d).\n",
                                       tower_cost, current_balance);
                            }
                            break;
                        }
                    }
                } else {
                    // Placera eller avbryt
                    if (clickX < leftBoundary || clickX > rightBoundary) {
                        if (gameState->selectedOption != -1) {
                            place_tower(gameState, resources,
                                        gameState->selectedOption,
                                        clickX, clickY, -1);
                        }
                    } else {
                        printf("Cancelled placement (clicked in middle zone).\n");
                    }
                    gameState->placingBird   = false;
                    gameState->selectedOption = -1;
                }
            }
        }
        else if (context == INPUT_CONTEXT_CLIENT && client && resources) {
            int clickX = event.button.x;
            int clickY = event.button.y;
            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT)
            {
                if (!client->placingBird) {
                    // Välj torn-ikon
                    for (int i = 0; i < 3; ++i) {
                        SDL_Rect ir = resources->towerOptions[i].iconRect;
                        if (clickX >= ir.x && clickX <= ir.x + ir.w
                         && clickY >= ir.y && clickY <= ir.y + ir.h)
                        {
                            // Kontrollera om laget har råd innan vi går vidare
                            Team team = (client->playerIndex == 0 || client->playerIndex == 2) ? TEAM_LEFT : TEAM_RIGHT;
                            int balance = money_manager_get_balance(gameState->team_money[team]);
                            int cost    = resources->towerOptions[i].prototype.cost;
                            if (balance < cost) {
                                printf("Kan inte köpa torn: kostnad %d, saldo %d\n",
                                       cost, balance);
                                // Avbryt placering direkt
                                client->placingBird    = false;
                                client->selectedOption = -1;
                                break;
                            }

                            // Om vi kommer hit har laget råd
                            printf("Selected tower type %d for placement request.\n", i);
                            client->placingBird    = true;
                            client->selectedOption = i;
                            break;
                        }
                    }
                } else {
                    // Kolla UI-gap + lagets sida
                    int  midX     = WINDOW_WIDTH / 2;
                    bool leftTeam = (client->playerIndex == 0 || client->playerIndex == 2);
                    bool inUIGap  = (clickX >= leftBoundary && clickX <= rightBoundary);
                    bool validSide= leftTeam
                                    ? (clickX < midX)
                                    : (clickX >= midX);

                    if (!inUIGap
                        && validSide
                        && client->selectedOption != -1
                        && client->playerIndex != -1)
                    {
                        printf("Requesting placement: type %d at (%d, %d) by player %d\n",
                               client->selectedOption,
                               clickX, clickY,
                               client->playerIndex);
#ifdef CLIENT
                        ClientPacketData pd = {0};
                        pd.command        = CLIENT_CMD_PLACE_TOWER;
                        pd.playerIndex    = client->playerIndex;
                        pd.towerTypeIndex = client->selectedOption;
                        pd.targetX        = clickX;
                        pd.targetY        = clickY;
                        send_client_packet(client, &pd);
#endif
                    } else {
                        printf("Cancelled placement (clicked in invalid zone).\n");
                    }

                    client->placingBird    = false;
                    client->selectedOption = -1;
                }
            }
        }
    }
}

