#include <stdio.h>
#include <string.h>
#include <math.h>
#include "engine.h"

void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color, bool center) {
    if (!font || !text || !renderer || strlen(text) == 0) return;
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;
    SDL_Rect dstRect = {x, y, 0, 0};
    SDL_QueryTexture(texture, NULL, NULL, &dstRect.w, &dstRect.h);
    if (center) dstRect.x = x - dstRect.w / 2;
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_DestroyTexture(texture);
}

// Main Rendering Functions
void render_main_menu(SDL_Renderer *renderer, GameResources *resources, BuildMode mode) {
    if (!renderer || !resources || !resources->mainMenuBg || !resources->font) {
        printf("Error: Missing resources for main menu rendering.\n");
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        return;
    }
    SDL_Rect bgRect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderCopy(renderer, resources->mainMenuBg, NULL, &bgRect);
    SDL_Color white = {255, 255, 255, 255};
    int text_y = WINDOW_HEIGHT / 2 - 50;
    const char* menu_text = "Unknown Mode - Error?";
    if (mode == MODE_SINGLEPLAYER) menu_text = "Press SPACE to Play Singleplayer";
    else if (mode == MODE_SERVER) menu_text = "Press SPACE to Host Game";
    else if (mode == MODE_CLIENT) menu_text = "Press SPACE to Join Game";
    render_text(renderer, resources->font, menu_text, WINDOW_WIDTH / 2, text_y, white, true);
    SDL_RenderPresent(renderer);
}

void render_game(SDL_Renderer *renderer, GameState *gameState, GameResources *resources,
                 float birdRotations[], bool placingBird, int selectedOption, int localPlayerIndex) {
    if (!renderer || !gameState || !resources) return;

    // Map Rendering (remains the same)
    if (resources->mapTexture) {
        SDL_Rect mapRect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
        SDL_RenderCopy(renderer, resources->mapTexture, NULL, &mapRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);
    }

    // Enemy Rendering (remains the same)
    SDL_Rect baseEnemyRect; if (resources->enemyTextures[0]) {
        int w,h;
        SDL_QueryTexture(resources->enemyTextures[0], NULL, NULL, &w, &h);
        baseEnemyRect.w = (int)(w * ENEMY_RENDER_SCALE_WIDTH);
        baseEnemyRect.h = (int)(h * ENEMY_RENDER_SCALE_HEIGHT);
    } else {
        baseEnemyRect.w = WINDOW_WIDTH * ENEMY_RENDER_SCALE_WIDTH;
        baseEnemyRect.h = WINDOW_HEIGHT * ENEMY_RENDER_SCALE_HEIGHT;
    }
    for (int i = 0; i < gameState->numEnemiesActive; i++) {
        Enemy *e = &gameState->enemies[i];
        if (!e->active || !e->texture) continue;
        int shadowW = (int)(baseEnemyRect.w * 1.0f);
        int shadowH = (int)(baseEnemyRect.h * 0.3f);
        SDL_Rect shadowRect = {
            (int)(e->x - shadowW / 2),
            (int)(e->y + baseEnemyRect.h * 0.07f),
            shadowW,
            shadowH
        };
        SDL_SetTextureAlphaMod(resources->shadow, 140); 
        SDL_RenderCopy(renderer, resources->shadow, NULL, &shadowRect);
        SDL_Rect r = { (int)(e->x - baseEnemyRect.w / 2.0f), (int)(e->y - baseEnemyRect.h / 2.0f), baseEnemyRect.w, baseEnemyRect.h };
        SDL_Point p = { baseEnemyRect.w / 2, baseEnemyRect.h / 2 };
        SDL_RenderCopyEx(renderer, e->texture, NULL, &r, e->angle+180, &p, SDL_FLIP_NONE);
    }

    // Tower (Bird) Rendering (remains the same)
    SDL_Rect baseBirdRect;
    if (resources->towerBaseTextures[0]) {
        int w,h;
        SDL_QueryTexture(resources->towerBaseTextures[0], NULL, NULL, &w, &h);
        baseBirdRect.w = (int)(w * BIRD_RENDER_SCALE);
        baseBirdRect.h = (int)(h * BIRD_RENDER_SCALE);
    } else {
        baseBirdRect.w = 40; baseBirdRect.h = 40;
    }
    for (int i = 0; i < gameState->numPlacedBirds; i++) {
        Bird *b = &gameState->placedBirds[i];
        if (!b->active || !b->texture) continue;
        int shadowW = (int)(baseBirdRect.w * 1.0f);
        int shadowH = (int)(baseBirdRect.h * 0.4f);
        SDL_Rect shadowRect = {
            (int)(b->x - shadowW / 2),
            (int)(b->y + baseBirdRect.h * 0.1f),
            shadowW,
            shadowH
        };
        SDL_SetTextureAlphaMod(resources->shadow, 160);
        SDL_RenderCopy(renderer, resources->shadow, NULL, &shadowRect);
        SDL_Rect r = { (int)(b->x - baseBirdRect.w / 2.0f), (int)(b->y - baseBirdRect.h / 2.0f), baseBirdRect.w, baseBirdRect.h };
        SDL_Point p = { baseBirdRect.w / 2, baseBirdRect.h / 2 };
        SDL_RenderCopyEx(renderer, b->texture, NULL, &r, birdRotations[i], &p, SDL_FLIP_NONE);
    }

    // Projectile Rendering (remains the same)
    SDL_Rect baseProjRect;
    if (resources->projectileTextures[0]) {
        int w,h;
        SDL_QueryTexture(resources->projectileTextures[0], NULL, NULL, &w, &h);
        baseProjRect.w = (int)(w * PROJECTILE_RENDER_SCALE);
        baseProjRect.h = (int)(h * PROJECTILE_RENDER_SCALE);
    } else {
        baseProjRect.w = 10; baseProjRect.h = 10;
    }
    for (int i = 0; i < gameState->numProjectiles; i++) {
        Projectile *p = &gameState->projectiles[i];
        if (!p->active || !p->texture) continue;
        SDL_Rect r = { (int)(p->x - baseProjRect.w / 2.0f), (int)(p->y - baseProjRect.h / 2.0f), baseProjRect.w, baseProjRect.h };
        SDL_Point pv = { baseProjRect.w / 2, baseProjRect.h / 2 };
        SDL_RenderCopyEx(renderer, p->texture, NULL, &r, p->angle, &pv, SDL_FLIP_NONE);
    }

    // UI Rendering (Updated parts)
    if (resources->font) {
        char buf[64];
        SDL_Color w = {255,255,255,255}; // white
        SDL_Color y = {255,255,0,255}; // yellow
        SDL_Color r = {255,0,0,255}; // red
        SDL_Color g = {144, 238, 144, 255}; // green

        int uy=10; // 10px från toppen

        Team team = (localPlayerIndex == 0 || localPlayerIndex == 2) ? TEAM_LEFT : TEAM_RIGHT;
        int currentMoney = money_manager_get_balance(gameState->team_money[team]);
        snprintf(buf, sizeof(buf), "Money: $%d", currentMoney);
        render_text(renderer, resources->font, buf, WINDOW_WIDTH / 2, uy, y, true);

        // Wave text 
        int wy = uy + 30; // 30px under money 
        snprintf(buf, sizeof(buf), "Wave: %d", gameState->currentWave);
        render_text(renderer, resources->font, buf, WINDOW_WIDTH / 2, wy, w, true);

        // Team text 
        if (localPlayerIndex >= 0) {
            const char *teamStr = (team == TEAM_LEFT) ? "Team Left" : "Team Right";
            int ty = wy + 30;
            render_text(renderer, resources->font, teamStr, WINDOW_WIDTH/2, ty, w, true);
        }

        // HP Bars 
        int bw = (int)(WINDOW_WIDTH * 0.1f);
        int bh = (int)(WINDOW_HEIGHT * 0.02f);
        int hy = uy + 30;
        int xo = 50;
        // Left HP Bar
        SDL_Rect ol = { xo, hy, bw, bh };
        float frl = (gameState->leftPlayerHP > 0)?(float)gameState->leftPlayerHP/PLAYER_START_HP:0.0f;
        if (frl < 0.0f) {frl = 0.0f;}
        if (frl > 1.0f) {frl = 1.0f;}
        SDL_Rect fl = { ol.x+1, ol.y+1, (int)(bw * frl)-2, bh-2 };
        SDL_SetRenderDrawColor(renderer, 255,255,255,255); SDL_RenderDrawRect(renderer, &ol);
        SDL_SetRenderDrawColor(renderer, 0,200,0,255); if(fl.w>0&&fl.h>0) SDL_RenderFillRect(renderer, &fl);
        snprintf(buf, sizeof(buf), "%d/%d", gameState->leftPlayerHP<0?0:gameState->leftPlayerHP, PLAYER_START_HP);
        render_text(renderer, resources->font, buf, xo+bw+5, hy, w, false);
        // Right HP Bar
        SDL_Rect or_rect = { WINDOW_WIDTH-xo-bw, hy, bw, bh };
        float frr = (gameState->rightPlayerHP>0)?(float)gameState->rightPlayerHP/PLAYER_START_HP:0.0f;
        if (frr < 0.0f) {frr = 0.0f;}
        if (frr > 1.0f) {frr = 1.0f;}
        SDL_Rect fillr = { or_rect.x+1, or_rect.y+1, (int)(bw * frr)-2, bh-2 };
        SDL_SetRenderDrawColor(renderer, 255,255,255,255); SDL_RenderDrawRect(renderer, &or_rect);
        SDL_SetRenderDrawColor(renderer, 0,200,0,255); if(fillr.w>0&&fillr.h>0) SDL_RenderFillRect(renderer, &fillr);
        snprintf(buf, sizeof(buf), "%d/%d", gameState->rightPlayerHP<0?0:gameState->rightPlayerHP, PLAYER_START_HP);
        int tw, th; TTF_SizeText(resources->font, buf, &tw, &th);
        render_text(renderer, resources->font, buf, or_rect.x-tw-5, hy, w, false);

        // Tower Icons 
        for(int i=0; i<3; i++){
            const TowerOption *o = &resources->towerOptions[i];
            if(o->iconTexture){
                Team team = (localPlayerIndex == 0 || localPlayerIndex == 2) ? TEAM_LEFT : TEAM_RIGHT; 
                bool canAfford = (money_manager_get_balance(gameState->team_money[team]) >= o->prototype.cost);

                SDL_SetTextureColorMod(o->iconTexture, canAfford?255:100, canAfford?255:100, canAfford?255:100);
                SDL_RenderCopy(renderer, o->iconTexture, NULL, &o->iconRect);
                SDL_SetTextureColorMod(o->iconTexture, 255,255,255); 
                snprintf(buf, sizeof(buf), "$%d", o->prototype.cost);
                render_text(renderer, resources->font, buf, o->iconRect.x+o->iconRect.w/2, o->iconRect.y+o->iconRect.h+2, canAfford?g:r, true);
            }
        }
    }

    // Placement Preview 
     if (placingBird && selectedOption != -1) {
        int mx,my; SDL_GetMouseState(&mx,&my);
        int w,h; SDL_GetRendererOutputSize(renderer,&w,&h);
        mx = (int)(mx * ((float)WINDOW_WIDTH / w));
        my = (int)(my * ((float)WINDOW_HEIGHT / h));
        render_placement_preview(renderer, resources, selectedOption, mx, my);
    }

     // Reset draw color before returning 
     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}

void render_placement_preview(SDL_Renderer *renderer, GameResources *resources, int selectedOption, int mouseX, int mouseY) {
    if (selectedOption < 0 || selectedOption >= 3 || !resources) return;
    const TowerOption *o = &resources->towerOptions[selectedOption];
    SDL_Texture *pt = o->prototype.baseTexture;
    if (!pt) return;
    SDL_Rect pr;
    int w,h;
    SDL_QueryTexture(pt, NULL, NULL, &w, &h);
    pr.w = (int)(w * BIRD_RENDER_SCALE);
    pr.h = (int)(h * BIRD_RENDER_SCALE);
    pr.x = mouseX - pr.w/2;
    pr.y = mouseY - pr.h/2;
    SDL_SetTextureAlphaMod(pt, 150);
    SDL_RenderCopy(renderer, pt, NULL, &pr);
    SDL_SetTextureAlphaMod(pt, 255);
    int range = (int)o->prototype.range;
    SDL_SetRenderDrawColor(renderer, 255,0,0,130);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    int cx = mouseX;
    int cy = mouseY;
    for(int ad=0; ad<360; ad+=5){
        float ar=ad*M_PI/180.0f;
        int x=cx+(int)(range*cosf(ar));
        int y=cy+(int)(range*sinf(ar));
        int dotSize = 3;
        SDL_Rect dot = {x - dotSize/2,y - dotSize/2,dotSize,dotSize};
        SDL_RenderFillRect(renderer, &dot);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
}

void render_game_over(SDL_Renderer* renderer, GameResources* resources, const char* message) {
    if (!renderer || !resources || !resources->font || !message) return;
    SDL_Color r={255,0,0,255};
    SDL_Color w={255,255,255,255};
    SDL_Color bg={50,50,50,200};
    int tw,th;
    TTF_SizeText(resources->font, message, &tw, &th);
    int x=WINDOW_WIDTH/2-tw/2;
    int y=WINDOW_HEIGHT/2-th/2-30;
    SDL_Rect bgr = { x-20, y-10, tw+40, th+20 };
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(renderer, &bgr);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    render_text(renderer, resources->font, message, x, y, r, false);
    const char* qm = "Press ESC to return to menu or quit";
    TTF_SizeText(resources->font, qm, &tw, &th);
    render_text(renderer, resources->font, qm, WINDOW_WIDTH/2, bgr.y+bgr.h+10, w, true);
}