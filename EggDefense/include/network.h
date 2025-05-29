#ifndef NETWORK_H
#define NETWORK_H

#include "defs.h" // Includes MAX limits etc.
#include <stdbool.h> // For bool type

// --- Client -> Server Commands ---
typedef enum {
    CLIENT_CMD_NONE = 0,
    CLIENT_CMD_READY,         // Client is ready to join/start
    CLIENT_CMD_PLACE_TOWER,   // Client requests to place a tower
    CLIENT_CMD_HEARTBEAT      // Client is still connected
} ClientCommandType;

// Client -> Server Packet Structure
typedef struct {
    ClientCommandType command;
    int playerIndex;        // Client's assigned index (-1 if not assigned yet)
    int towerTypeIndex;     // Index of tower type to place (for PLACE_TOWER)
    int targetX;            // X coordinate for placement (for PLACE_TOWER)
    int targetY;            // Y coordinate for placement (for PLACE_TOWER)
} ClientPacketData;


// --- Server -> Client Commands ---
typedef enum {
    SERVER_CMD_NONE = 0,
    SERVER_CMD_WAITING,             // Server tells client how many are connected
    SERVER_CMD_ASSIGN_INDEX,        // Server assigns a player index to the client
    SERVER_CMD_GAME_START,          // Server signals the game is starting
    SERVER_CMD_STATE_UPDATE,        // Server sends a snapshot of the game state
    SERVER_CMD_GAME_OVER,           // Server signals the game has ended
    SERVER_CMD_REJECT_FULL,         // Server rejects connection because it's full
    SERVER_CMD_PLACE_TOWER_CONFIRM, // Server confirms successful tower placement
    SERVER_CMD_PLACE_TOWER_REJECT   // Server rejects tower placement (e.g., no money, bad spot)
} ServerCommandType;


// --- Structs for Snapshot Data (Minimal data needed for rendering/client state) ---
typedef struct {
    float x, y;
    float angle;
    int type;           // e.g., 0=red, 1=blue, 2=yellow
    int hp;             // Current HP (optional for client? maybe needed for effects)
    bool active;
    int side;           // 0 for left path, 1 for right path
} EnemySnapshotData;

typedef struct {
    float x, y;
    int typeIndex;        // e.g., 0=super, 1=bat, 2=brown
    float attackAnimTimer;// > 0 if currently in attack animation frame
    bool active;
    int ownerPlayerIndex; // Which player placed this tower (-1 for singleplayer/neutral)
} BirdSnapshotData;

typedef struct {
    float x, y;
    float angle;
    int projectileTextureIndex; // Index corresponding to projectile texture (0=dart, 1=bullet)
    bool active;
} ProjectileSnapshotData;


// Game State Snapshot Structure (Server -> Client)
typedef struct {
    EnemySnapshotData enemies[MAX_ENEMIES];             int numEnemiesActive;
    BirdSnapshotData placedBirds[MAX_PLACED_BIRDS];     int numPlacedBirds;
    ProjectileSnapshotData projectiles[MAX_PROJECTILES]; int numProjectiles;

    int money;              // Current shared money (in MP) or player money (in SP)
    int leftPlayerHP;       // Remaining HP for the left side goal
    int rightPlayerHP;      // Remaining HP for the right side goal
    int currentWave;        // Aktuell våg 
    bool gameOver;          // True if the game has ended
    int winner;             // Player index of the winner (0 or 1 in 2-player, relevant for MP), or -1 if draw/SP loss
} GameStateSnapshot;


// Server -> Client Packet Structure
typedef struct {
    ServerCommandType command;
    int assignedPlayerIndex; // Sent with ASSIGN_INDEX
    int clientsConnected;    // Sent with WAITING
    GameStateSnapshot snapshot; // Sent with STATE_UPDATE and GAME_OVER
} ServerPacketData;

#endif // NETWORK_H