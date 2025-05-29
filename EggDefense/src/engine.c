#include <stdio.h>
#include <string.h> 
#include <math.h>   
#include "engine.h" 

// Initializes SDL Core, Window, and Renderer
bool initialize_sdl(SDL_Window **window, SDL_Renderer **renderer, const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }

    *window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!*window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return false;
    }
    // Optional: Set logical size if you want a fixed render resolution regardless of window size
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SCALING, "SDL_MOUSE_RELATIVE_SCALING");
    SDL_RenderSetLogicalSize(*renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    

    printf("SDL Initialized Successfully.\n");
    return true;
}

// Initializes SDL Subsystems (IMG, TTF, Mixer, Net)
bool initialize_subsystems() {
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("IMG_Init Error: %s\n", IMG_GetError());
        return false;
    }

    if (TTF_Init() == -1) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        IMG_Quit();
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Mix_OpenAudio Error: %s\n", Mix_GetError());
        TTF_Quit();
        IMG_Quit();
        return false;
    }

    if (SDLNet_Init() == -1) {
        printf("SDLNet_Init Error: %s\n", SDLNet_GetError());
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        return false;
    }

    printf("SDL Subsystems Initialized Successfully.\n");
    return true;
}

// Initializes Audio
bool initialize_audio(Audio *audio) {
    if (!audio) return false;
    memset(audio, 0, sizeof(Audio)); 

    // Background Music
    audio->bgm = Mix_LoadMUS("resources/gamesound.mp3");
    if (!audio->bgm) {
        printf("Warning: Failed to load BGM 'resources/gamesound.mp3': %s\n", Mix_GetError());
    } else {
        Mix_VolumeMusic(64); // default volume
        printf("BGM Loaded.\n");
    }

    // Load Sound Effect
    audio->popSound = Mix_LoadWAV("resources/pop.wav");
    if (!audio->popSound) {
        printf("Warning: Failed to load SFX 'resources/pop.wav': %s\n", Mix_GetError());
    } else {
         printf("SFX Loaded.\n");
    }

    audio->levelUpSound = Mix_LoadWAV("resources/levelup.wav");
    if (!audio->levelUpSound) {
        printf("Warning: Failed to load SFX 'resources/levelup.wav': %s\n", Mix_GetError());
    } else {
         printf("SFX Loaded.\n");
    }

    return true;

}

// Helper to load a single texture
SDL_Texture* load_texture(SDL_Renderer *renderer, const char *path) {
    SDL_Texture *texture = IMG_LoadTexture(renderer, path);
    if (!texture) {
        printf("Error loading texture '%s': %s\n", path, IMG_GetError());
    }
    return texture;
}

// Loads all necessary game resources
bool load_resources(SDL_Renderer *renderer, GameResources *resources, Audio *audio) {
    if (!renderer || !resources || !audio) return false;

    printf("Loading resources...\n");
    memset(resources, 0, sizeof(GameResources)); // Clear struct first
    bool success = true;

    // Load Textures
    resources->mapTexture = load_texture(renderer, "resources/map.png");
    if (!resources->mapTexture) success = false;

    resources->mainMenuBg = load_texture(renderer, "resources/MainMenuPic3.png");
     if (!resources->mainMenuBg) {
        printf("CRITICAL ERROR: Main menu background 'resources/MainMenuPic3.png' not found!\n");
        success = false;
     }

    resources->shadow = IMG_LoadTexture(renderer, "resources/shadow.png");
    if (!resources->shadow) {
        printf("CRITICAL ERROR: shadow 'resources/shadow.png' not found!\n");
        success = false;
     }

    // Enemies
    resources->enemyTextures[0] = load_texture(renderer, "resources/redbloon.png");
    if (!resources->enemyTextures[0]) success = false;
    resources->enemyTextures[1] = load_texture(renderer, "resources/bluebloon.png");
    if (!resources->enemyTextures[1]) success = false;
    resources->enemyTextures[2] = load_texture(renderer, "resources/yellowbloon.png");
    if (!resources->enemyTextures[2]) success = false;

    // Projectiles
    resources->projectileTextures[0] = load_texture(renderer, "resources/dart.png"); // Index 0 = Dart
    if (!resources->projectileTextures[0]) success = false;
    resources->projectileTextures[1] = load_texture(renderer, "resources/bullet.png"); // Index 1 = Bullet
    if (!resources->projectileTextures[1]) success = false;

    // Tower Base Textures
    resources->towerBaseTextures[0] = load_texture(renderer, "resources/superbird1.png"); // Index 0 = Super
    if (!resources->towerBaseTextures[0]) success = false;
    resources->towerBaseTextures[1] = load_texture(renderer, "resources/batbird1.png");   // Index 1 = Bat
    if (!resources->towerBaseTextures[1]) success = false;
    resources->towerBaseTextures[2] = load_texture(renderer, "resources/brownbird1.png"); // Index 2 = Brown
    if (!resources->towerBaseTextures[2]) success = false;

    // Tower Attack Textures
    resources->towerAttackTextures[0] = load_texture(renderer, "resources/superbird1attack.png");
    if (!resources->towerAttackTextures[0]) success = false;
    resources->towerAttackTextures[1] = load_texture(renderer, "resources/batbird1attack.png");
    if (!resources->towerAttackTextures[1]) success = false;
    resources->towerAttackTextures[2] = load_texture(renderer, "resources/brownbird1attack.png");
    if (!resources->towerAttackTextures[2]) success = false;

    // Tower Icon Textures
    resources->towerIconTextures[0] = load_texture(renderer, "resources/superbird1icon.png");
    if (!resources->towerIconTextures[0]) success = false;
    resources->towerIconTextures[1] = load_texture(renderer, "resources/batbird1icon.png");
    if (!resources->towerIconTextures[1]) success = false;
    resources->towerIconTextures[2] = load_texture(renderer, "resources/brownbird1icon.png");
    if (!resources->towerIconTextures[2]) success = false;

    // Load Font
    resources->font = TTF_OpenFont("resources/font.ttf", 24);
    if (!resources->font) {
        printf("TTF_OpenFont Error for 'resources/font.ttf': %s\n", TTF_GetError());
        success = false;
    }

    // Initialize Audio
    if (!initialize_audio(audio)) {
         printf("Audio initialization finished with warnings.\n");
    }

    // Define Tower Options
    // Superbird (Type 0)
    Bird p0 = { .damage = 1, .range = WINDOW_WIDTH * 0.1f, .attackSpeed = 5.0f, .cost = 1000,
                .projectileTexture = resources->projectileTextures[1], .projectileTextureIndex = 1, // Bullet
                .baseTexture = resources->towerBaseTextures[0], .attackTexture = resources->towerAttackTextures[0],
                .texture = resources->towerBaseTextures[0], .towerTypeIndex = 0, .ownerPlayerIndex = -1 };
                resources->towerOptions[0] = (TowerOption){ .prototype = p0, .iconTexture = resources->towerIconTextures[0] };

    // Batbird (Type 1)
    Bird p1 = { .damage = 10, .range = WINDOW_WIDTH * 0.1f, .attackSpeed = 0.5f, .cost = 400,
                .projectileTexture = resources->projectileTextures[0], .projectileTextureIndex = 0, // Dart
                .baseTexture = resources->towerBaseTextures[1], .attackTexture = resources->towerAttackTextures[1],
                .texture = resources->towerBaseTextures[1], .towerTypeIndex = 1, .ownerPlayerIndex = -1 };
                resources->towerOptions[1] = (TowerOption){ .prototype = p1, .iconTexture = resources->towerIconTextures[1] };

    // Brownbird (Type 2)
    Bird p2 = { .damage = 3, .range = WINDOW_WIDTH * 0.16f, .attackSpeed = 1.2f, .cost = 200,
                .projectileTexture = resources->projectileTextures[0], .projectileTextureIndex = 0, // Dart
                .baseTexture = resources->towerBaseTextures[2], .attackTexture = resources->towerAttackTextures[2],
                .texture = resources->towerBaseTextures[2], .towerTypeIndex = 2, .ownerPlayerIndex = -1 };
                resources->towerOptions[2] = (TowerOption){ .prototype = p2, .iconTexture = resources->towerIconTextures[2] };

    // Layout UI Icons
    int spacing = 20;
    int total_icons_height = 0;
    int icon_w = 0, icon_h = 0;

    for (int i = 0; i < 3; i++) {
        if (resources->towerOptions[i].iconTexture) {
             SDL_QueryTexture(resources->towerOptions[i].iconTexture, NULL, NULL, &icon_w, &icon_h);
             resources->towerOptions[i].iconRect.w = icon_w / ICON_SCALE_DIVISOR;
             resources->towerOptions[i].iconRect.h = icon_h / ICON_SCALE_DIVISOR;
        } else {
             resources->towerOptions[i].iconRect.w = 40;
             resources->towerOptions[i].iconRect.h = 40;
        }
        total_icons_height += resources->towerOptions[i].iconRect.h;
    }
    total_icons_height += 2 * spacing; // Add spacing between icons

    int current_y = WINDOW_HEIGHT / 2 - total_icons_height / 2; // Start Y position for vertical centering

    for (int i = 0; i < 3; i++) {
        resources->towerOptions[i].iconRect.x = WINDOW_WIDTH / 2 - resources->towerOptions[i].iconRect.w / 2; // Center horizontally
        resources->towerOptions[i].iconRect.y = current_y;
        current_y += resources->towerOptions[i].iconRect.h + spacing; // Move down for next icon
    }

    if (success) {
        printf("Resources loaded successfully.\n");
    } else {
        printf("Error: Some resources failed to load!\n");
    }
    return success;
}


// Cleans up resources
void cleanup_resources(GameResources *resources, Audio *audio) {
    if (!resources || !audio) return;
    printf("Cleaning up resources...\n");

    // Textures
    if (resources->mapTexture) SDL_DestroyTexture(resources->mapTexture);
    if (resources->mainMenuBg) SDL_DestroyTexture(resources->mainMenuBg);
    for (int i = 0; i < 3; i++) if (resources->enemyTextures[i]) SDL_DestroyTexture(resources->enemyTextures[i]);
    for (int i = 0; i < 2; i++) if (resources->projectileTextures[i]) SDL_DestroyTexture(resources->projectileTextures[i]);
    for (int i = 0; i < 3; i++) if (resources->towerBaseTextures[i]) SDL_DestroyTexture(resources->towerBaseTextures[i]);
    for (int i = 0; i < 3; i++) if (resources->towerAttackTextures[i]) SDL_DestroyTexture(resources->towerAttackTextures[i]);
    for (int i = 0; i < 3; i++) if (resources->towerIconTextures[i]) SDL_DestroyTexture(resources->towerIconTextures[i]);

    // Font
    if (resources->font) TTF_CloseFont(resources->font);

    // Audio 
    if (audio->popSound) Mix_FreeChunk(audio->popSound);
    if (audio->bgm) Mix_FreeMusic(audio->bgm);

    memset(resources, 0, sizeof(GameResources));
    memset(audio, 0, sizeof(Audio));

    printf("Resources cleaned up.\n");
}

// Shuts down SDL subsystems
void cleanup_subsystems() {
    printf("Cleaning up SDL subsystems...\n");
    SDLNet_Quit();
    Mix_CloseAudio(); // Important to close audio before quitting SDL
    TTF_Quit();
    IMG_Quit();
    printf("SDL subsystems cleaned up.\n");
}

// Cleans up SDL Core, Renderer, and Window
void cleanup_sdl(SDL_Window *window, SDL_Renderer *renderer) {
    printf("Cleaning up SDL Core...\n");
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit(); // Final SDL shutdown
    printf("SDL Core cleaned up.\n");
}


// Audio Playback Functions
void play_sound(const Audio *audio, Mix_Chunk* sound) {
     if (sound) {
        Mix_PlayChannel(-1, sound, 0);
     }
}

void play_music(Mix_Music* music) {
    if (music) {
        if (Mix_PlayingMusic() == 0) { // Avoid restarting if already playing
            Mix_PlayMusic(music, -1); // Loop indefinitely
        }
    }
}

void stop_music() {
    Mix_HaltMusic();
}


// Helper Functions 

// Calculates distance between two points
float distance_between_points(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

// Calculates delta time since last call
float get_delta_time(Uint32 *last_time) {
    Uint32 current_time = SDL_GetTicks();
    // Prevent large dt spikes on first frame or after pauses
    Uint32 elapsed = current_time - *last_time;
    if (elapsed > 100) { // Clamp max dt to avoid physics issues (e.g., 100ms)
        elapsed = 100;
    }
    float dt = (float)elapsed / 1000.0f; // Convert ms to seconds
    *last_time = current_time;
    // Minimum dt to prevent division by zero or jitter?
    // if (dt < 0.001f) dt = 0.001f;
    return dt;
}