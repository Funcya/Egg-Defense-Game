#ifndef DEFS_H
#define DEFS_H

typedef enum { //Team
    TEAM_LEFT = 0,
    TEAM_RIGHT = 1
} Team;

// Game Window & Logic Constants
#define WINDOW_WIDTH 1500
#define WINDOW_HEIGHT 900
#define MAX_ENEMIES 100
#define MAX_PLACED_BIRDS 20
#define MAX_PROJECTILES 100
#define PROJECTILE_SPEED 3500.0f
#define PLAYER_START_HP 100
#define START_MONEY 200
#define MONEY_INTERVAL 14.0f
#define MONEY_GAIN 150
#define NUM_TEAMS 2

//enemy spawn
#define ENEMY_SPAWN_INTERVAL 2.5f // Slowed down spawn rate
#define NEW_WAVE_BY_ENEMY_SPAWN 12 // numbers of enemies to spawn to increase wave
#define WAVE_PAUSE_LENGTH 5.0f

// Network Constants
#define SERVER_PORT 9999
#define MAX_PLAYERS 4
#define PACKET_BUFFER_SIZE 8192
#define GAME_TICK_RATE 60
#define CLIENT_HEARTBEAT_INTERVAL 2000
#define CLIENT_READY_INTERVAL 500
#define SERVER_CLIENT_TIMEOUT 10000

// Rendering Constants
#define BIRD_RENDER_SCALE 0.30f // Doubled tower render size
#define ENEMY_RENDER_SCALE_WIDTH 0.06f
#define ENEMY_RENDER_SCALE_HEIGHT 0.08f
// #define PROJECTILE_RENDER_SCALE 0.1f // Original value
#define PROJECTILE_RENDER_SCALE 1.0f // Set to 100% size for testing
#define ICON_SCALE_DIVISOR 3

// Path Definition
#define NUM_POINTS 15

// Ensure M_PI is defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif // DEFS_H