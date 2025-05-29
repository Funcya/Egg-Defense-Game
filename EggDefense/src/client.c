#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include "money_adt.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include "defs.h"
#include "engine.h"
#include "paths.h"

// --- Static Function Prototypes ---
static bool initialize_client(ClientInstance *client, const char *server_ip_str);
static void run_client_loop(ClientInstance *client);
static void shutdown_client(ClientInstance *client);
static void handle_server_packet(ClientInstance *client, UDPpacket *packet);
static void update_status_text(ClientInstance *client, const char *message);
static void apply_snapshot(ClientInstance *client, GameStateSnapshot *snapshot);
static void handle_client_click(ClientInstance *client, int clickX, int clickY);

// --- Public Entry Point ---
int run_client(const char *server_ip_str)
{
    ClientInstance client = {0};
    if (!initialize_client(&client, server_ip_str))
    {
        printf("Client initialization failed. Exiting. Error: %s\n", client.statusText);
        shutdown_client(&client);
        return 1;
    }
    run_client_loop(&client);
    shutdown_client(&client);
    printf("Client shut down.\n");
    return 0;
}


// --- Initialization ---
static bool initialize_client(ClientInstance *client, const char *server_ip_str)
{
    printf("Initializing Client...\n");
    client->is_running = true;
    client->state = CLIENT_STATE_INIT;
    client->playerIndex = -1;
    snprintf(client->statusText, sizeof(client->statusText), "Initializing...");
    if (!initialize_sdl(&client->window, &client->renderer, "Tower Defense - Client"))
    {
        snprintf(client->statusText, sizeof(client->statusText), "SDL Init Failed: %s", SDL_GetError());
        return false;
    }
    if (!initialize_subsystems())
    {
        snprintf(client->statusText, sizeof(client->statusText), "SDL Subsystem Init Failed");
        cleanup_sdl(client->window, client->renderer);
        return false;
    }
    // Pass client's audio struct to load V1 sounds
    if (!load_resources(client->renderer, &client->resources, &client->audio))
    {
        snprintf(client->statusText, sizeof(client->statusText), "Failed to load resources.");
        cleanup_subsystems();
        cleanup_sdl(client->window, client->renderer);
        return false;
    }
    initialize_game_state(&client->localGameState, &client->resources);
    client->placingBird = false;
    client->selectedOption = -1;
    memset(client->birdRotations, 0, sizeof(client->birdRotations));
    client->state = CLIENT_STATE_RESOLVING;
    update_status_text(client, "Resolving server address...");
    if (SDLNet_ResolveHost(&client->serverAddress, server_ip_str, SERVER_PORT) == -1)
    {
        snprintf(client->statusText, sizeof(client->statusText), "Error: ResolveHost: %s", SDLNet_GetError());
        client->state = CLIENT_STATE_ERROR;
        cleanup_resources(&client->resources, &client->audio);
        cleanup_subsystems();
        cleanup_sdl(client->window, client->renderer);
        return false;
    }
    client->socket = SDLNet_UDP_Open(0);
    if (!client->socket)
    {
        snprintf(client->statusText, sizeof(client->statusText), "Error: UDP_Open: %s", SDLNet_GetError());
        client->state = CLIENT_STATE_ERROR;
        cleanup_resources(&client->resources, &client->audio);
        cleanup_subsystems();
        cleanup_sdl(client->window, client->renderer);
        return false;
    }
    client->packet_in = SDLNet_AllocPacket(PACKET_BUFFER_SIZE);
    client->packet_out = SDLNet_AllocPacket(PACKET_BUFFER_SIZE);
    if (!client->packet_in || !client->packet_out)
    {
        snprintf(client->statusText, sizeof(client->statusText), "Error: AllocPacket: %s", SDLNet_GetError());
        if (client->packet_in)
            SDLNet_FreePacket(client->packet_in);
        if (client->packet_out)
            SDLNet_FreePacket(client->packet_out);
        SDLNet_UDP_Close(client->socket);
        cleanup_resources(&client->resources, &client->audio);
        cleanup_subsystems();
        cleanup_sdl(client->window, client->renderer);
        return false;
    }
    client->packet_out->address = client->serverAddress;
    client->state = CLIENT_STATE_MAIN_MENU;
    client->lastReadySendTime = SDL_GetTicks();
    client->lastHeartbeatSendTime = SDL_GetTicks();
    update_status_text(client, "Main Menu");
    printf("Client initialization complete. Showing Main Menu.\n");
    return true;
}

// --- Main Client Loop ---
static void run_client_loop(ClientInstance *client)
{
    while (client->is_running)
    {
        Uint32 currentTime = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                client->is_running = false;
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    if (client->placingBird)
                    {
                        client->placingBird = false;
                        client->selectedOption = -1;
                        printf("Placement cancelled via ESC.\n");
                    }
                    else
                    {
                        client->is_running = false;
                    }
                }
                else if (event.key.keysym.sym == SDLK_SPACE && client->state == CLIENT_STATE_MAIN_MENU)
                {
                    client->state = CLIENT_STATE_CONNECTING;
                    update_status_text(client, "Connecting...");
                    ClientPacketData rp = {.command = CLIENT_CMD_READY, .playerIndex = -1};
                    send_client_packet(client, &rp);
                    client->lastReadySendTime = currentTime;
                }
            }
            if (client->state >= CLIENT_STATE_RUNNING && client->state < CLIENT_STATE_GAME_OVER)
            {
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
                {
                    handle_client_click(client, event.button.x, event.button.y);
                }
            }
        } // End PollEvent

        if (!client->is_running)
            break;

        // --- State Machine & Network ---
        switch (client->state)
        {
        case CLIENT_STATE_MAIN_MENU:
            render_main_menu(client->renderer, &client->resources, MODE_CLIENT);
            break;
        case CLIENT_STATE_CONNECTING:
            if (currentTime - client->lastReadySendTime > CLIENT_READY_INTERVAL)
            {
                ClientPacketData rp = {.command = CLIENT_CMD_READY, .playerIndex = -1};
                send_client_packet(client, &rp);
                client->lastReadySendTime = currentTime;
            }
            while (SDLNet_UDP_Recv(client->socket, client->packet_in) > 0)
            {
                if (client->packet_in->address.host == client->serverAddress.host && client->packet_in->address.port == client->serverAddress.port)
                    handle_server_packet(client, client->packet_in);
            }
            SDL_SetRenderDrawColor(client->renderer, 0, 0, 0, 255);
            SDL_RenderClear(client->renderer);
            render_text(client->renderer, client->resources.font, client->statusText, 10, WINDOW_HEIGHT - 30, (SDL_Color){255, 255, 255, 255}, false);
            SDL_RenderPresent(client->renderer);
            break;
        case CLIENT_STATE_WAITING_FOR_START: // Fallthrough
        case CLIENT_STATE_RUNNING:
            if (currentTime - client->lastHeartbeatSendTime > CLIENT_HEARTBEAT_INTERVAL)
            {
                ClientPacketData hbp = {.command = CLIENT_CMD_HEARTBEAT, .playerIndex = client->playerIndex};
                send_client_packet(client, &hbp);
                client->lastHeartbeatSendTime = currentTime;
            }
            while (SDLNet_UDP_Recv(client->socket, client->packet_in) > 0)
            {
                if (client->packet_in->address.host == client->serverAddress.host && client->packet_in->address.port == client->serverAddress.port)
                    handle_server_packet(client, client->packet_in);
            }
            calculate_tower_rotations(&client->localGameState, client->birdRotations);
            SDL_SetRenderDrawColor(client->renderer, 0, 0, 0, 255);
            SDL_RenderClear(client->renderer);
            render_game(client->renderer, &client->localGameState, &client->resources, client->birdRotations, client->placingBird, client->selectedOption, client->playerIndex);
            render_text(client->renderer, client->resources.font, client->statusText, 10, WINDOW_HEIGHT - 30, (SDL_Color){255, 255, 255, 255}, false);
            SDL_RenderPresent(client->renderer);
            break;
        case CLIENT_STATE_GAME_OVER:
            while (SDLNet_UDP_Recv(client->socket, client->packet_in) > 0)
            {
                if (client->packet_in->address.host == client->serverAddress.host && client->packet_in->address.port == client->serverAddress.port)
                    handle_server_packet(client, client->packet_in);
            }
            calculate_tower_rotations(&client->localGameState, client->birdRotations);
            SDL_SetRenderDrawColor(client->renderer, 0, 0, 0, 255);
            SDL_RenderClear(client->renderer);
            render_game(client->renderer, &client->localGameState, &client->resources, client->birdRotations, false, -1, client->playerIndex);
            render_game_over(client->renderer, &client->resources, client->gameOverMessage);
            SDL_RenderPresent(client->renderer);
            break;
        case CLIENT_STATE_DISCONNECTED: // Fallthrough
        case CLIENT_STATE_ERROR:
            SDL_SetRenderDrawColor(client->renderer, 0, 0, 0, 255);
            SDL_RenderClear(client->renderer);
            render_text(client->renderer, client->resources.font, client->statusText, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, (SDL_Color){255, 0, 0, 255}, true);
            render_text(client->renderer, client->resources.font, "Press ESC to quit", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 30, (SDL_Color){255, 255, 255, 255}, true);
            SDL_RenderPresent(client->renderer);
            break;
        case CLIENT_STATE_INIT: // Fallthrough
        case CLIENT_STATE_RESOLVING:
            snprintf(client->statusText, sizeof(client->statusText), "Error: Invalid client state %d in main loop.", client->state);
            client->state = CLIENT_STATE_ERROR;
            break;
        }
        SDL_Delay(1);
    }
    printf("Client loop finished.\n");
}

// --- Client Click Handling Logic ---
static void handle_client_click(ClientInstance *client, int clickX, int clickY)
{
    if (!client) return;

    int leftBoundary  = (int)(WINDOW_WIDTH * 0.48f);
    int rightBoundary = (int)(WINDOW_WIDTH * 0.52f);

    if (!client->placingBird)
    {
        // Välj torn-ikon
        for (int i = 0; i < 3; ++i)
        {
            SDL_Rect ir = client->resources.towerOptions[i].iconRect;
            if (clickX >= ir.x && clickX <= ir.x + ir.w
             && clickY >= ir.y && clickY <= ir.y + ir.h)
            {
                printf("Selected tower type %d for placement request.\n", i);
                client->placingBird    = true;
                client->selectedOption = i;
                break;
            }
        }
    }
    else
    {
        // Kontrollera UI-gap + lagets sida
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

            ClientPacketData pd = {0};
            pd.command        = CLIENT_CMD_PLACE_TOWER;
            pd.playerIndex    = client->playerIndex;
            pd.towerTypeIndex = client->selectedOption;
            pd.targetX        = clickX;
            pd.targetY        = clickY;
            send_client_packet(client, &pd);
        }
        else
        {
            printf("Cancelled placement (clicked in invalid zone).\n");
        }

        client->placingBird    = false;
        client->selectedOption = -1;
    }
}


// --- Networking Helpers ---
static void handle_server_packet(ClientInstance *client, UDPpacket *packet)
{
    if (!client || !packet || (size_t)packet->len < sizeof(ServerCommandType))
        return;
    ServerPacketData sd;
    size_t cs = ((size_t)packet->len < sizeof(ServerPacketData)) ? (size_t)packet->len : sizeof(ServerPacketData);
    memset(&sd, 0, sizeof(ServerPacketData));
    memcpy(&sd, packet->data, cs);
    switch (sd.command)
    {
    case SERVER_CMD_ASSIGN_INDEX:
        if (client->state == CLIENT_STATE_CONNECTING)
        {
            client->playerIndex = sd.assignedPlayerIndex;
            client->state = CLIENT_STATE_WAITING_FOR_START;
            char s[64];
            snprintf(s, sizeof(s), "Connected as P%d. Waiting...", client->playerIndex + 1);
            update_status_text(client, s);
        }
        break;
    case SERVER_CMD_WAITING:
        if (client->state == CLIENT_STATE_WAITING_FOR_START)
        {
            char s[64];
            snprintf(s, sizeof(s), "Waiting... (%d/%d Ready)", sd.clientsConnected, MAX_PLAYERS);
            update_status_text(client, s);
        }
        break;
    case SERVER_CMD_GAME_START:
        if (client->state == CLIENT_STATE_WAITING_FOR_START)
        {
            client->state = CLIENT_STATE_RUNNING;
            update_status_text(client, "Game Running!");
            play_music(client->audio.bgm);
        }
        break;
        case SERVER_CMD_STATE_UPDATE:
        if (client->state == CLIENT_STATE_RUNNING ||
            client->state == CLIENT_STATE_WAITING_FOR_START)
        {
            if ((size_t)packet->len >=
                offsetof(ServerPacketData, snapshot) + sizeof(GameStateSnapshot))
            {
                apply_snapshot(client, &sd.snapshot);
            }
            else
            {
                printf("Warn: STATE_UPDATE packet too small (%d bytes)\n", packet->len);
            }

            if (sd.snapshot.gameOver && client->state != CLIENT_STATE_GAME_OVER)
            {
                client->state = CLIENT_STATE_GAME_OVER;
                bool leftTeam    = (client->playerIndex == 0 || client->playerIndex == 2);
                bool teamVictory =
                    (sd.snapshot.winner == 0 && leftTeam) ||
                    (sd.snapshot.winner == 1 && !leftTeam);
                snprintf(client->gameOverMessage,
                         sizeof(client->gameOverMessage),
                         teamVictory ? "YOU WIN!" : "YOU LOSE!");
                update_status_text(client, "Game Over");
                stop_music();
            }
        }
        break;

        case SERVER_CMD_GAME_OVER:
        if (client->state != CLIENT_STATE_GAME_OVER)
        {
            client->state = CLIENT_STATE_GAME_OVER;
            if ((size_t)packet->len >=
                offsetof(ServerPacketData, snapshot) + sizeof(GameStateSnapshot))
            {
                apply_snapshot(client, &sd.snapshot);
            }
            bool leftTeam    = (client->playerIndex == 0 || client->playerIndex == 2);
            bool teamVictory =
                (sd.snapshot.winner == 0 && leftTeam) ||
                (sd.snapshot.winner == 1 && !leftTeam);
            snprintf(client->gameOverMessage,
                     sizeof(client->gameOverMessage),
                     teamVictory ? "YOU WIN!" : "YOU LOSE!");
            update_status_text(client, "Game Over");
            stop_music();
        }
        break;

    
    case SERVER_CMD_REJECT_FULL:
        if (client->state == CLIENT_STATE_CONNECTING)
        {
            update_status_text(client, "Server Full. Connection Rejected.");
            client->state = CLIENT_STATE_ERROR;
        }
        break;
    case SERVER_CMD_PLACE_TOWER_CONFIRM:
        printf("CLIENT: Server confirmed tower placement.\n");
        break;
    case SERVER_CMD_PLACE_TOWER_REJECT:
        printf("CLIENT: Server rejected tower placement.\n");
        break;
    default:
        printf("Warn: Unknown command %d received from server.\n", sd.command);
        break;
    }
}
void send_client_packet(ClientInstance *client, ClientPacketData *data)
{
    if (!client || !data || !client->socket || !client->packet_out || client->state == CLIENT_STATE_ERROR || client->state == CLIENT_STATE_DISCONNECTED)
        return;
    client->packet_out->len = sizeof(ClientPacketData);
    memcpy(client->packet_out->data, data, client->packet_out->len);
    if (SDLNet_UDP_Send(client->socket, -1, client->packet_out) == 0)
    {
        printf("Warn: SDLNet_UDP_Send failed (cmd %d): %s\n", data->command, SDLNet_GetError());
        update_status_text(client, "Error Sending Packet - Disconnected?");
        client->state = CLIENT_STATE_DISCONNECTED;
    }
}

// Applies snapshot to local state AND plays sound if projectiles increased
static void apply_snapshot(ClientInstance *client, GameStateSnapshot *snapshot)
{
    if (!client || !snapshot)
        return;
    GameState *local = &client->localGameState;

    // --- Store old counts before applying snapshot ---
    int oldProjectileCount = local->numProjectiles;

    // --- Apply Snapshot Data ---
    Team team = (client->playerIndex == 0 || client->playerIndex == 2) ? TEAM_LEFT : TEAM_RIGHT;
    money_manager_set_balance(
    local->team_money[team],
    snapshot->money
);
    local->leftPlayerHP = snapshot->leftPlayerHP;
    local->rightPlayerHP = snapshot->rightPlayerHP;
    local->gameOver = snapshot->gameOver;
    local->winner = snapshot->winner;
    local->currentWave = snapshot->currentWave;
    // Enemies
    local->numEnemiesActive = snapshot->numEnemiesActive;
    for (int i = 0; i < local->numEnemiesActive; ++i)
    {
        local->enemies[i].x = snapshot->enemies[i].x;
        local->enemies[i].y = snapshot->enemies[i].y;
        local->enemies[i].angle = snapshot->enemies[i].angle;
        local->enemies[i].type = snapshot->enemies[i].type;
        local->enemies[i].hp = snapshot->enemies[i].hp;
        if (local->enemies[i].hp > 0) {
            if (local->enemies[i].hp <= 1) local->enemies[i].texture = client->resources.enemyTextures[0];
            else if (local->enemies[i].hp <= 3) local->enemies[i].texture = client->resources.enemyTextures[1];
            else local->enemies[i].texture = client->resources.enemyTextures[2];
        }
        local->enemies[i].active = snapshot->enemies[i].active;
        local->enemies[i].side = snapshot->enemies[i].side;
    }
    for (int i = local->numEnemiesActive; i < MAX_ENEMIES; ++i)
        local->enemies[i].active = false;
    // Towers
    local->numPlacedBirds = snapshot->numPlacedBirds;
    for (int i = 0; i < local->numPlacedBirds; ++i)
    {
        local->placedBirds[i].x = snapshot->placedBirds[i].x;
        local->placedBirds[i].y = snapshot->placedBirds[i].y;
        local->placedBirds[i].towerTypeIndex = snapshot->placedBirds[i].typeIndex;
        local->placedBirds[i].active = snapshot->placedBirds[i].active;
        local->placedBirds[i].ownerPlayerIndex = snapshot->placedBirds[i].ownerPlayerIndex;
        local->placedBirds[i].attackAnimTimer = snapshot->placedBirds[i].attackAnimTimer;
        if (local->placedBirds[i].towerTypeIndex >= 0 && local->placedBirds[i].towerTypeIndex < 3)
        {
            const Bird *pt = &client->resources.towerOptions[local->placedBirds[i].towerTypeIndex].prototype;
            local->placedBirds[i].baseTexture = pt->baseTexture;
            local->placedBirds[i].attackTexture = pt->attackTexture;
            local->placedBirds[i].projectileTexture = pt->projectileTexture;
            local->placedBirds[i].projectileTextureIndex = pt->projectileTextureIndex;
            local->placedBirds[i].range = pt->range;
            local->placedBirds[i].texture = (local->placedBirds[i].attackAnimTimer > 0) ? local->placedBirds[i].attackTexture : local->placedBirds[i].baseTexture;
        }
        else
        {
            local->placedBirds[i].texture = NULL;
            local->placedBirds[i].baseTexture = NULL;
            local->placedBirds[i].attackTexture = NULL;
        }
    }
    for (int i = local->numPlacedBirds; i < MAX_PLACED_BIRDS; ++i)
        local->placedBirds[i].active = false;
    // Projectiles
    local->numProjectiles = snapshot->numProjectiles;
    for (int i = 0; i < local->numProjectiles; ++i)
    {
        local->projectiles[i].x = snapshot->projectiles[i].x;
        local->projectiles[i].y = snapshot->projectiles[i].y;
        local->projectiles[i].angle = snapshot->projectiles[i].angle;
        local->projectiles[i].active = snapshot->projectiles[i].active;
        local->projectiles[i].textureIndex = snapshot->projectiles[i].projectileTextureIndex;
        if (local->projectiles[i].textureIndex >= 0 && local->projectiles[i].textureIndex < 2)
            local->projectiles[i].texture = client->resources.projectileTextures[local->projectiles[i].textureIndex];
        else
            local->projectiles[i].texture = NULL;
    }
    for (int i = local->numProjectiles; i < MAX_PROJECTILES; ++i)
        local->projectiles[i].active = false;

    // --- Play Sound Based on State Change ---
    // If the number of projectiles in the new snapshot is greater than what we had locally before applying it,
    // assume at least one projectile was fired and play the sound.
    if (local->numProjectiles > oldProjectileCount)
    {
        play_sound(&client->audio, client->audio.popSound);
    }
}

static void update_status_text(ClientInstance *client, const char *message)
{
    if (!client || !message)
        return;
    snprintf(client->statusText, sizeof(client->statusText), "%s", message);
    printf("CLIENT: %s\n", message);
}
static void shutdown_client(ClientInstance *client)
{
    printf("Shutting down client...\n");
    if (!client)
        return;
    if (client->packet_in)
        SDLNet_FreePacket(client->packet_in);
    if (client->packet_out)
        SDLNet_FreePacket(client->packet_out);
    if (client->socket)
        SDLNet_UDP_Close(client->socket);
    client->packet_in = NULL;
    client->packet_out = NULL;
    client->socket = NULL;
    cleanup_resources(&client->resources, &client->audio);
    cleanup_sdl(client->window, client->renderer);
    cleanup_subsystems();
    printf("Client shutdown complete.\n");
}