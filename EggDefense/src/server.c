#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include "money_adt.h"

#include "engine.h"
#include "paths.h"
#include "defs.h"  // för WINDOW_WIDTH

// --- Static Function Prototypes ---
static bool initialize_server(ServerInstance* server);
static void run_server_loop(ServerInstance* server);
static void shutdown_server(ServerInstance* server);
static void handle_client_packet(ServerInstance* server, UDPpacket* packet);
static int find_client_index(ServerInstance* server, IPaddress address);
static int add_client(ServerInstance* server, IPaddress address);
static void broadcast_packet(ServerInstance* server, ServerPacketData* data);
static void send_packet_to_client(ServerInstance* server, int clientIndex, ServerPacketData* data);
static void prepare_snapshot(GameState* current_state, GameStateSnapshot* snapshot);
static void update_server_game_state(ServerInstance* server, float dt);
static void render_debug_view(ServerInstance* server);

// --- Public Entry Point ---
int run_server(void) {
    ServerInstance server = {0};
    srand((unsigned int)time(NULL));
    server.is_running = true;
    // SDL init
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    if (!initialize_subsystems()) {
        printf("Failed to initialize SDL subsystems.\n");
        SDL_Quit();
        return 1;
    }
    
    // Endast i debug-läge: skapa fönster, renderer och ladda grafik/sounds
    if (!initialize_sdl(&server.debugWindow, &server.debugRenderer, "TD Server Debug")) {
        fprintf(stderr, "Server SDL init failed.\n");
        cleanup_subsystems();
        SDL_Quit();
        return 1;
    }
    if (!load_resources(server.debugRenderer, &server.resources, &server.audio)) {
        fprintf(stderr, "Server critical resource loading failed.\n");
        cleanup_sdl(server.debugWindow, server.debugRenderer);
        cleanup_subsystems();
        SDL_Quit();
        return 1;
    }


    GameStatus currentStatus = GAME_STATE_MAIN_MENU;
    printf("Server started. Displaying Main Menu.\n");
    while (server.is_running && currentStatus == GAME_STATE_MAIN_MENU) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                server.is_running = false;
                break;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    currentStatus = GAME_STATE_PLAYING;
                    printf("Space pressed, initializing network...\n");
                }
                else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    server.is_running = false;
                    break;
                }
            }
        }
        SDL_SetRenderDrawColor(server.debugRenderer, 0, 0, 0, 255);
        SDL_RenderClear(server.debugRenderer);
        render_main_menu(server.debugRenderer, &server.resources, MODE_SERVER);
        SDL_Delay(10);
    }

    if (server.is_running) {
        if (!initialize_server(&server)) {
            printf("Server network/state initialization failed.\n");
            shutdown_server(&server);
            return 1;
        }
        run_server_loop(&server);
    }
    shutdown_server(&server);
    printf("Server shut down normally.\n");
    return 0;
}

// --- Initialization ---
static bool initialize_server(ServerInstance* server) {
    server->num_clients = 0;
    server->lastTickTime = SDL_GetTicks();
    initialize_game_state(&server->gameState, &server->resources);

    printf("Opening UDP socket on port %d...\n", SERVER_PORT);
    server->socket = SDLNet_UDP_Open(SERVER_PORT);
    if (!server->socket) {
        printf("SDLNet_UDP_Open Error: %s\n", SDLNet_GetError());
        return false;
    }
    server->packet_in  = SDLNet_AllocPacket(PACKET_BUFFER_SIZE);
    server->packet_out = SDLNet_AllocPacket(PACKET_BUFFER_SIZE);
    if (!server->packet_in || !server->packet_out) {
        printf("SDLNet_AllocPacket Error: %s\n", SDLNet_GetError());
        if (server->packet_in)  SDLNet_FreePacket(server->packet_in);
        if (server->packet_out) SDLNet_FreePacket(server->packet_out);
        if (server->socket)     SDLNet_UDP_Close(server->socket);
        server->socket = NULL;
        return false;
    }

    printf("Server network initialized. Waiting for players...\n");
    return true;
}

// --- Main Server Loop ---
static void run_server_loop(ServerInstance* server) {
    bool game_started = false;
    Uint32 time_per_tick = 1000 / GAME_TICK_RATE;

    while (server->is_running) {
        Uint32 currentTime = SDL_GetTicks();
        Uint32 elapsedTime = currentTime - server->lastTickTime;
        SDL_Event event;

        // Quit handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) server->is_running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                server->is_running = false;
        }
        if (!server->is_running) break;

        // Receive incoming client packets
        while (SDLNet_UDP_Recv(server->socket, server->packet_in) > 0) {
            handle_client_packet(server, server->packet_in);
        }

        if (elapsedTime >= time_per_tick) {
            float dt = (float)elapsedTime / 1000.0f;
            if (dt > 0.1f) dt = 0.1f;
            server->lastTickTime = currentTime;

            if (!game_started) {
                bool allReady = (server->num_clients == MAX_PLAYERS);
                if (allReady) {
                    for (int i = 0; i < server->num_clients; ++i) {
                        if (!server->clients[i].ready) {
                            allReady = false;
                            break;
                        }
                    }
                }
                if (allReady) {
                    printf("All %d players ready! Starting game.\n", MAX_PLAYERS);
                    game_started = true;
                    server->gameState.spawnTimer = 0.0f;
                    //server->gameState.moneyTimer = 0.0f;
                    ServerPacketData sp = {.command = SERVER_CMD_GAME_START};
                    broadcast_packet(server, &sp);
                } else {
                    static Uint32 lastBroadcast = 0;
                    if (currentTime - lastBroadcast > 1000) {
                        ServerPacketData wp = {
                            .command = SERVER_CMD_WAITING,
                            .clientsConnected = server->num_clients
                        };
                        broadcast_packet(server, &wp);
                        lastBroadcast = currentTime;
                    }
                }
            }

            if (game_started) {
                // 1) Uppdatera game state
                update_server_game_state(server, dt);

                // 2) Kolla om spelet tog slut den här tick: 
                if (server->gameState.gameOver) {
                    printf("Server detected Game Over. Winner: %d\n", server->gameState.winner);
                    // Skicka GAME_OVER till alla klienter
                    for (int ci = 0; ci < server->num_clients; ++ci) {
                        ServerPacketData op = {.command = SERVER_CMD_GAME_OVER};
                        prepare_snapshot(&server->gameState, &op.snapshot);
                        Team team = (ci == 0 || ci == 2) ? TEAM_LEFT : TEAM_RIGHT;
                        op.snapshot.money = money_manager_get_balance(server->gameState.team_money[team]);
                        send_packet_to_client(server, ci, &op);
                    }
                    // Avmarkera så att vi inte skickar fler updates
                    game_started = false;
                }
                else {
                    // 3) Om spelet fortfarande pågår, skicka STATE_UPDATE
                    for (int ci = 0; ci < server->num_clients; ++ci) {
                        ServerPacketData stp = {.command = SERVER_CMD_STATE_UPDATE};
                        prepare_snapshot(&server->gameState, &stp.snapshot);
                        Team team = (ci == 0 || ci == 2) ? TEAM_LEFT : TEAM_RIGHT;
                        stp.snapshot.money = money_manager_get_balance(server->gameState.team_money[team]);
                        send_packet_to_client(server, ci, &stp);
                    }
                }
            }
            render_debug_view(server);
        }


        Uint32 frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < time_per_tick) SDL_Delay(time_per_tick - frameTime);
        else SDL_Delay(1);
    }
}

// --- Game State Update ---
static void update_server_game_state(ServerInstance* server, float dt) {
    GameState* gs        = &server->gameState;
    GameResources* res   = &server->resources;
    Audio* audio         = &server->audio;

    for (int t = 0; t < NUM_TEAMS; ++t) {
    money_manager_update(server->gameState.team_money[t], dt);
    }

    if (gs->inWaveDelay) {
        gs->spawnCooldown -= dt;
        if (gs->spawnCooldown <= 0.0f) {
            gs->inWaveDelay = false;
            if (gs->currentWave > 0) {
                play_sound(audio, audio->levelUpSound);
            }
            gs->currentWave++;
            gs->spawnTimer = ENEMY_SPAWN_INTERVAL;
        }
    } else {
        gs->spawnTimer += dt;
        if (gs->spawnTimer >= ENEMY_SPAWN_INTERVAL) {
            gs->spawnTimer -= ENEMY_SPAWN_INTERVAL;
            spawn_enemy_pair(gs, res);
        }
    }

    update_enemies(gs, dt);
    update_towers(gs, audio, dt, res);
    update_projectiles(gs, dt);
}

// --- Networking Helpers ---
static int find_client_index(ServerInstance* server, IPaddress address) {
    for (int i = 0; i < server->num_clients; ++i) {
        if (server->clients[i].address.host == address.host &&
            server->clients[i].address.port == address.port)
        {
            return i;
        }
    }
    return -1;
}

static int add_client(ServerInstance* server, IPaddress address) {
    if (server->num_clients >= MAX_PLAYERS) return -1;
    int newIndex = server->num_clients++;
    server->clients[newIndex].address        = address;
    server->clients[newIndex].ready          = false;
    server->clients[newIndex].lastPacketTime = SDL_GetTicks();
    return newIndex;
}

static void handle_client_packet(ServerInstance* server, UDPpacket* packet) {
    if ((size_t)packet->len < sizeof(ClientCommandType)) return;
    ClientPacketData cd;
    size_t copySize = ((size_t)packet->len < sizeof(ClientPacketData))
                      ? (size_t)packet->len
                      : sizeof(ClientPacketData);
    memcpy(&cd, packet->data, copySize);

    int ci = find_client_index(server, packet->address);
    if (ci == -1) {
        if (cd.command == CLIENT_CMD_READY) {
            int ni = add_client(server, packet->address);
            if (ni != -1) {
                printf("Player %d connected: %x:%d\n", ni,
                       packet->address.host, packet->address.port);
                ServerPacketData ap = {
                    .command = SERVER_CMD_ASSIGN_INDEX,
                    .assignedPlayerIndex = ni
                };
                send_packet_to_client(server, ni, &ap);
                server->clients[ni].ready = true;
                printf("Player %d marked as ready.\n", ni);
                ServerPacketData wp = {
                    .command = SERVER_CMD_WAITING,
                    .clientsConnected = server->num_clients
                };
                broadcast_packet(server, &wp);
            } else {
                printf("Connection rejected (Server Full) for %x:%d\n",
                       packet->address.host, packet->address.port);
                ServerPacketData rp = {.command = SERVER_CMD_REJECT_FULL};
                server->packet_out->address = packet->address;
                server->packet_out->len     = sizeof(ServerCommandType);
                memcpy(server->packet_out->data, &rp, server->packet_out->len);
                SDLNet_UDP_Send(server->socket, -1, server->packet_out);
            }
        }
        return;
    }

    server->clients[ci].lastPacketTime = SDL_GetTicks();
    if (cd.playerIndex != ci && cd.command != CLIENT_CMD_READY) {
        // Mismatch—ignorera
    }

    switch (cd.command) {
        case CLIENT_CMD_READY:
            if (!server->clients[ci].ready) {
                server->clients[ci].ready = true;
                printf("Player %d signaled ready.\n", ci);
                ServerPacketData wp = {
                    .command = SERVER_CMD_WAITING,
                    .clientsConnected = server->num_clients
                };
                broadcast_packet(server, &wp);
            }
            break;

        case CLIENT_CMD_PLACE_TOWER:
            if (server->gameState.gameOver) break;

            // ——— Här infogas lag‐kontrollen ———
            {
                int midX    = WINDOW_WIDTH / 2;
                bool leftTeam = (ci == 0 || ci == 2);
                if ((leftTeam  && cd.targetX > midX) ||
                    (!leftTeam && cd.targetX < midX)) {
                    ServerPacketData reply = {0};
                    reply.command = SERVER_CMD_PLACE_TOWER_REJECT;
                    send_packet_to_client(server, ci, &reply);
                    break;
                }
            }

            // Själva placeringen
            {
                bool placed = place_tower(&server->gameState,
                                          &server->resources,
                                          cd.towerTypeIndex,
                                          cd.targetX,
                                          cd.targetY,
                                          ci);
                ServerPacketData reply = {0};
                reply.command = placed
                    ? SERVER_CMD_PLACE_TOWER_CONFIRM
                    : SERVER_CMD_PLACE_TOWER_REJECT;
                send_packet_to_client(server, ci, &reply);
            }
            break;

        case CLIENT_CMD_HEARTBEAT:
            break;

        default:
            printf("Warn: Unknown cmd %d from client %d.\n", cd.command, ci);
            break;
    }
}

static void broadcast_packet(ServerInstance* server, ServerPacketData* data) {
    if (!server->packet_out) return;
    server->packet_out->len = sizeof(ServerPacketData);
    memcpy(server->packet_out->data, data, server->packet_out->len);
    for (int i = 0; i < server->num_clients; ++i) {
        server->packet_out->address = server->clients[i].address;
        if (SDLNet_UDP_Send(server->socket, -1, server->packet_out) == 0) {
            printf("Warn: broadcast send failed to P%d\n", i);
        }
    }
}

static void send_packet_to_client(ServerInstance* server,
                                  int ci,
                                  ServerPacketData* data) {
    if (ci < 0 || ci >= server->num_clients || !server->packet_out) return;
    server->packet_out->len     = sizeof(ServerPacketData);
    memcpy(server->packet_out->data, data, server->packet_out->len);
    server->packet_out->address = server->clients[ci].address;
    if (SDLNet_UDP_Send(server->socket, -1, server->packet_out) == 0) {
        printf("Warn: send_packet failed to P%d (Cmd %d)\n",
               ci,
               data ? (int)data->command : -1);
    }
}

static void prepare_snapshot(GameState* cs, GameStateSnapshot* ss) {
    if (!cs || !ss) return;
    memset(ss, 0, sizeof(GameStateSnapshot));
    ss->leftPlayerHP     = cs->leftPlayerHP;
    ss->rightPlayerHP    = cs->rightPlayerHP;
    ss->currentWave      = cs->currentWave;
    ss->gameOver         = cs->gameOver;
    ss->winner           = cs->winner;
    ss->numEnemiesActive = cs->numEnemiesActive;
    for (int i = 0; i < cs->numEnemiesActive; ++i) {
        ss->enemies[i].x     = cs->enemies[i].x;
        ss->enemies[i].y     = cs->enemies[i].y;
        ss->enemies[i].angle = cs->enemies[i].angle;
        ss->enemies[i].type  = cs->enemies[i].type;
        ss->enemies[i].hp    = cs->enemies[i].hp;
        ss->enemies[i].active= cs->enemies[i].active;
        ss->enemies[i].side  = cs->enemies[i].side;
    }
    ss->numPlacedBirds = cs->numPlacedBirds;
    for (int i = 0; i < cs->numPlacedBirds; ++i) {
        ss->placedBirds[i].x               = cs->placedBirds[i].x;
        ss->placedBirds[i].y               = cs->placedBirds[i].y;
        ss->placedBirds[i].typeIndex       = cs->placedBirds[i].towerTypeIndex;
        ss->placedBirds[i].attackAnimTimer = cs->placedBirds[i].attackAnimTimer;
        ss->placedBirds[i].active          = cs->placedBirds[i].active;
        ss->placedBirds[i].ownerPlayerIndex= cs->placedBirds[i].ownerPlayerIndex;
    }
    ss->numProjectiles = cs->numProjectiles;
    for (int i = 0; i < cs->numProjectiles; ++i) {
        ss->projectiles[i].x                = cs->projectiles[i].x;
        ss->projectiles[i].y                = cs->projectiles[i].y;
        ss->projectiles[i].angle            = cs->projectiles[i].angle;
        ss->projectiles[i].projectileTextureIndex = cs->projectiles[i].textureIndex;
        ss->projectiles[i].active           = cs->projectiles[i].active;
    }
}

static void shutdown_server(ServerInstance* server) {
    printf("Shutting down server...\n");
    if (!server) return;
    if (server->packet_in)  SDLNet_FreePacket(server->packet_in);
    if (server->packet_out) SDLNet_FreePacket(server->packet_out);
    if (server->socket)     SDLNet_UDP_Close(server->socket);
    server->packet_in  = NULL;
    server->packet_out = NULL;
    server->socket     = NULL;
    if (server->debugRenderer) {
        cleanup_resources(&server->resources, &server->audio);
    }
    cleanup_sdl(server->debugWindow, server->debugRenderer);
    cleanup_subsystems();
    SDL_QuitSubSystem(SDL_INIT_TIMER);
    SDL_Quit();
    printf("Server shutdown complete.\n");
}

static void render_debug_view(ServerInstance* server) {
    if (!server->debugRenderer || !server->resources.font) return;
    calculate_tower_rotations(&server->gameState, server->birdRotations);
    SDL_SetRenderDrawColor(server->debugRenderer, 0, 50, 0, 255);
    SDL_RenderClear(server->debugRenderer);
    render_game(server->debugRenderer,
                &server->gameState,
                &server->resources,
                server->birdRotations,
                false,
                -1,
                -1);
    char st[128];
    snprintf(st, sizeof(st),
             "Server - Clients: %d/%d Birds: %d",
             server->num_clients,
             MAX_PLAYERS,
             server->gameState.numPlacedBirds
             );
    render_text(server->debugRenderer,
                server->resources.font,
                st,
                10,
                10,
                (SDL_Color){255,255,255,255},
                false);
    SDL_RenderPresent(server->debugRenderer);
}
