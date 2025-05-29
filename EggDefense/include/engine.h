#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>
#include <stddef.h> // För offsetof i client/server
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_hints.h>
#include "money_adt.h"

// --- Project Headers ---
#include "defs.h"
#include "paths.h"
#include "network.h"

// Global Game State Enum
typedef enum {
    GAME_STATE_MAIN_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_GAME_OVER
} GameStatus;

//Enum for Build Mode (Used for UI rendering)
typedef enum {
    MODE_UNKNOWN,
    MODE_SINGLEPLAYER,
    MODE_SERVER,
    MODE_CLIENT
} BuildMode;

// projectile 
typedef struct {
    float x, y;           // Current position
    float vx, vy;           // Velocity vector (normalized direction)
    SDL_Texture *texture;   // Texture to render
    int textureIndex;     // 0=dart, 1=bullet
    bool active;          // Is the projectile currently in flight?
    float angle;          // Angle for rotation
} Projectile;

// enemy 
typedef struct {
    int hp;               // Current health points
    float speed;          // Movement speed 
    float x, y;           // Current position
    int currentSegment;   // Index of the path segment currently on
    float segmentProgress;// Progress along the current segment (0.0 to 1.0)
    SDL_Texture *texture;   // Texture to render (can change based on HP/type)
    int type;             // Type of enemy (0=red, 1=blue, 2=yellow)
    bool active;          // Is the enemy currently on the map?
    int side;             // Which path, 0 = left, 1 = right
    float angle;          // Angle for rotation (based on path direction)
} Enemy;

// placed tower/bird
typedef struct {
    int damage;             // Damage per hit 
    float range;            // Attack radius
    float attackSpeed;      // Attacks per second
    int cost;               // Cost to place
    SDL_Texture *projectileTexture; // Which projectile texture to use
    int projectileTextureIndex; // 0=dart, 1=bullet
    float x, y;             // Position on map
    bool active;            // Is this tower slot used?
    float attackTimer;      // Time since last attack
    float attackAnimTimer;  // Timer for showing attack animation frame
    float rotation;         // Current rotation angle
    SDL_Texture *texture;       // Current texture (base or attack)
    SDL_Texture *baseTexture;   // base appearance
    SDL_Texture *attackTexture; // attack Appearance
    int ownerPlayerIndex;   // Which player owns this tower (-1 if singleplayer)
    int towerTypeIndex;     // Index for networking/identification (0=super, 1=bat, 2=brown)
} Bird;

// selectable tower option in the UI
typedef struct {
    Bird prototype;         // Base stats and textures for this tower type
    SDL_Texture *iconTexture; // Texture for the UI button
    SDL_Rect iconRect;        // Position and size of the UI button
} TowerOption;

// Audio Resources
typedef struct {
    Mix_Chunk *popSound; // Sound effect for projectile hit/enemy pop
    Mix_Chunk *levelUpSound; //sound effect when wave levels up
    Mix_Music *bgm;      // Background music
} Audio;

// Graphics and Font Resources
typedef struct {
    SDL_Texture *mapTexture;
    SDL_Texture *mainMenuBg;
    SDL_Texture *shadow;
    TTF_Font *font;
    SDL_Texture *enemyTextures[3]; // 0=red, 1=blue, 2=yellow
    SDL_Texture *projectileTextures[2]; // 0=dart, 1=bullet
    SDL_Texture *towerBaseTextures[3]; // 0=super, 1=bat, 2=brown
    SDL_Texture *towerAttackTextures[3]; // 0=super, 1=bat, 2=brown
    SDL_Texture *towerIconTextures[3]; // 0=super, 1=bat, 2=brown
    TowerOption towerOptions[3];
} GameResources;

// Main Game State Container
typedef struct {
    Enemy enemies[MAX_ENEMIES];             int numEnemiesActive;
    Bird placedBirds[MAX_PLACED_BIRDS];     int numPlacedBirds;
    Projectile projectiles[MAX_PROJECTILES]; int numProjectiles;

    // Game Status & Player Info
    MoneyManager team_money[NUM_TEAMS];
    int leftPlayerHP;
    int rightPlayerHP;
    bool gameOver;
    int winner;
    
    // wave
    int currentWave;
    float spawnCooldown;         // tid kvar tills fiender får spawna igen
    bool inWaveDelay;            // flagga för om vi är i våg-paus

    // Timers & Counters
    float spawnTimer;
    int enemySpawnCounter;

    // Path Data
    Paths *paths;

    // UI State
    bool placingBird;
    int selectedOption;

} GameState;


// Client-Specific State
typedef enum {
    CLIENT_STATE_INIT,
    CLIENT_STATE_MAIN_MENU,
    CLIENT_STATE_RESOLVING,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_WAITING_FOR_START,
    CLIENT_STATE_RUNNING,
    CLIENT_STATE_GAME_OVER,
    CLIENT_STATE_DISCONNECTED,
    CLIENT_STATE_ERROR
} ClientState;

typedef struct ClientInstance {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool is_running;
    GameResources resources;
    Audio audio;
    GameState localGameState;
    ClientState state;
    char statusText[256];
    char gameOverMessage[128];
    bool placingBird;
    int selectedOption;
    float birdRotations[MAX_PLACED_BIRDS];
    int playerIndex;
    UDPsocket socket;
    IPaddress serverAddress;
    UDPpacket* packet_in;
    UDPpacket* packet_out;
    Uint32 lastReadySendTime;
    Uint32 lastHeartbeatSendTime;
} ClientInstance;


// Server-Specific State 
typedef struct {
    IPaddress address;
    bool ready;
    Uint32 lastPacketTime;
} ClientInfo;

typedef struct ServerInstance {
    SDL_Window* debugWindow;
    SDL_Renderer* debugRenderer;
    bool is_running;
    GameResources resources;
    Audio audio; 
    GameState gameState;
    float birdRotations[MAX_PLACED_BIRDS];
    UDPsocket socket;
    UDPpacket* packet_in;
    UDPpacket* packet_out;
    int num_clients;
    ClientInfo clients[MAX_PLAYERS];
    Uint32 lastTickTime;
} ServerInstance;


// Function Declarations
// engine.c: Core SDL, Resource, Audio, Time Management
bool initialize_sdl(SDL_Window **window, SDL_Renderer **renderer, const char* title);
bool initialize_subsystems();
bool initialize_audio(Audio *audio);
bool load_resources(SDL_Renderer *renderer, GameResources *resources, Audio *audio);
void cleanup_resources(GameResources *resources, Audio *audio);
void cleanup_sdl(SDL_Window *window, SDL_Renderer *renderer);
void cleanup_subsystems();
SDL_Texture* load_texture(SDL_Renderer *renderer, const char *path);
void play_sound(const Audio *audio, Mix_Chunk* sound);
void play_music(Mix_Music* music);
void stop_music();
float get_delta_time(Uint32 *last_time);
float distance_between_points(float x1, float y1, float x2, float y2);

// gameState.c: Initialization and placement logic
void initialize_game_state(GameState *gameState, GameResources *resources);
bool place_tower(GameState *gameState, GameResources *resources, int towerTypeIndex, int x, int y, int ownerPlayerIndex);

// enemy.c: Enemy logic
void update_enemies(GameState *gameState, float dt);
void spawn_enemy_pair(GameState *gameState, GameResources *resources);

// birds.c: Tower logic
void update_towers(GameState *gameState, const Audio *audio, float dt, GameResources *resources); 
void calculate_tower_rotations(GameState *gameState, float birdRotations[]);

// projectiles.c: Projectile logic
void update_projectiles(GameState *gameState, float dt);

// money.c: Money logic
void handle_money_gain(GameState *gameState, float dt);

// render.c: Drawing functions
void render_main_menu(SDL_Renderer *renderer, GameResources *resources, BuildMode mode); // Takes BuildMode
void render_game(SDL_Renderer *renderer, GameState *gameState, GameResources *resources, float birdRotations[], bool placingBird, int selectedOption, int localPlayerIndex);
void render_placement_preview(SDL_Renderer *renderer, GameResources *resources, int selectedOption, int mouseX, int mouseY);
void render_game_over(SDL_Renderer* renderer, GameResources* resources, const char* message);
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color, bool center);

// input.c: Input handling
typedef enum { INPUT_CONTEXT_MAIN_MENU, INPUT_CONTEXT_SINGLEPLAYER, INPUT_CONTEXT_CLIENT } InputContext;
void handle_input(InputContext context, GameState *gameState, GameResources *resources, ClientInstance *client, bool *quit_flag_ptr);

// client.c: Client network handling and main loop
int run_client(const char* server_ip_str);
void send_client_packet(ClientInstance* client, ClientPacketData* data); // Used by input.c

// server.c: Server network handling and main loop
int run_server(void);
// Internal server/client helpers like apply_snapshot, prepare_snapshot, etc. are static and not declared here

void run_singleplayer(void); 

#endif // ENGINE_H