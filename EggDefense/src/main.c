#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h> 
#include <SDL2/SDL_image.h>

#include "engine.h"     
#include "defs.h"  
#include <SDL2/SDL_thread.h>

// Wrapper för run_server till SDL-tråd
int server_thread_func(void* data) {
    return run_server();
}


void render_menu_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color, bool center) {
    if (!font || !text || !renderer) return;
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;
    SDL_Rect dstRect = {x, y, 0, 0};
    SDL_QueryTexture(texture, NULL, NULL, &dstRect.w, &dstRect.h);
    if (center) {
        dstRect.x = x - dstRect.w / 2;
    }
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_DestroyTexture(texture);
}

static char* get_ip_address_sdl_window(TTF_Font* font_to_use, SDL_Window* parent_menu_window) {
    SDL_Window* ip_window = NULL;
    SDL_Renderer* ip_renderer = NULL;
    char input_ip[100] = {0};
    bool done = false;
    char* result = NULL;

    int ip_win_width = 500;
    int ip_win_height = 300;

    if (parent_menu_window) {
        SDL_HideWindow(parent_menu_window);
    }

    ip_window = SDL_CreateWindow("Enter Server IP-adress",
                                 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 ip_win_width, ip_win_height, SDL_WINDOW_SHOWN);
    if (!ip_window) {
        printf("Misslyckades skapa IP-inmatningsfönster: %s\n", SDL_GetError());
        if (parent_menu_window) SDL_ShowWindow(parent_menu_window); 
        return NULL;
    }

    ip_renderer = SDL_CreateRenderer(ip_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ip_renderer) {
        printf("Misslyckades skapa IP-inmatningsrenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(ip_window);
        if (parent_menu_window) SDL_ShowWindow(parent_menu_window); 
        return NULL;
    }

    SDL_Color text_color_prompt = {200, 200, 200, 255};
    SDL_Color text_color_input = {255, 255, 255, 255}; 
    SDL_Color bg_color = {30, 30, 50, 255};        

    SDL_StartTextInput();

    Uint32 cursor_blink_time = 0;
    bool show_cursor = true;

    while (!done) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                done = true;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                    if (strlen(input_ip) > 0) {
                        result = strdup(input_ip);
                    }
                    done = true;
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                    done = true;
                } else if (e.key.keysym.sym == SDLK_BACKSPACE && strlen(input_ip) > 0) {
                    input_ip[strlen(input_ip) - 1] = '\0';
                }
            } else if (e.type == SDL_TEXTINPUT) {
                if (strlen(input_ip) + strlen(e.text.text) < sizeof(input_ip) -1) {
                    for (char *c = e.text.text; *c != '\0'; ++c) {
                        if ((*c >= '0' && *c <= '9') || *c == '.' || *c == ':' ||
                            (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '-') {
                            char temp_char_str[2] = {*c, '\0'};
                            strcat(input_ip, temp_char_str);
                            if (strlen(input_ip) >= sizeof(input_ip) -1) break;
                        }
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(ip_renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderClear(ip_renderer);

        render_menu_text(ip_renderer, font_to_use, "Enter Server IP:", ip_win_width / 2, 20, text_color_prompt, true);

        char display_text[sizeof(input_ip) + 1] = {0}; 
        strcpy(display_text, input_ip);

        if (SDL_GetTicks() - cursor_blink_time > 500) {
            show_cursor = !show_cursor;
            cursor_blink_time = SDL_GetTicks();
        }
        if (show_cursor) {
            strcat(display_text, "_");
        }

        if (strlen(display_text) > 0) {
            render_menu_text(ip_renderer, font_to_use, display_text, ip_win_width / 2, 60, text_color_input, true);
        } else {
             if (show_cursor) render_menu_text(ip_renderer, font_to_use, "_", ip_win_width / 2, 60, text_color_input, true);
        }

        render_menu_text(ip_renderer, font_to_use, "Press ENTER to confirm ", ip_win_width / 2, 110, text_color_prompt, true);
        render_menu_text(ip_renderer, font_to_use, "Press ESC to cancel", ip_win_width / 2, 140, text_color_prompt, true);

        SDL_RenderPresent(ip_renderer);
        SDL_Delay(10);
    }

    SDL_StopTextInput();

    if (ip_renderer) SDL_DestroyRenderer(ip_renderer);
    if (ip_window) SDL_DestroyWindow(ip_window);

    if (parent_menu_window) {
        SDL_ShowWindow(parent_menu_window);
    }


    return result;
}

int main(int argc, char *argv[]) {
    (void)argc; 
    (void)argv;

    SDL_Window *menu_window = NULL;
    SDL_Renderer *menu_renderer = NULL;
    TTF_Font *menu_font = NULL;
    SDL_Texture *menu_bg = NULL;
    bool quit_menu = false;
    int choice = 0;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init Error (Menu): %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() == -1) {
        printf("TTF_Init Error (Menu): %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("IMG_Init Error (Menu): %s\n", IMG_GetError());
    }


    menu_window = SDL_CreateWindow("Choose gamemode", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 0);
    if (!menu_window) {
        printf("SDL_CreateWindow Error (Menu): %s\n", SDL_GetError());
        TTF_Quit();
        IMG_Quit(); 
        SDL_Quit();
        return 1;
    }

    menu_renderer = SDL_CreateRenderer(menu_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!menu_renderer) {
        printf("SDL_CreateRenderer Error (Menu): %s\n", SDL_GetError());
        SDL_DestroyWindow(menu_window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    menu_font = TTF_OpenFont("resources/font.ttf", 24); 
    if (!menu_font) {
        printf("TTF_OpenFont Error (Menu) for 'resources/font.ttf': %s\n", TTF_GetError());
        SDL_DestroyRenderer(menu_renderer);
        SDL_DestroyWindow(menu_window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    menu_bg = load_texture(menu_renderer, "resources/MainMenuPic3.png");
    if (!menu_bg) {
        printf("Warning: Kunde inte ladda menybakgrund.\n");
    }

    while (!quit_menu && choice == 0) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit_menu = true;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_1:
                    case SDLK_KP_1:
                        choice = 1; 
                        break;
                    case SDLK_2:
                    case SDLK_KP_2:
                        choice = 2; 
                        break;
                    case SDLK_3:
                    case SDLK_KP_3:
                        choice = 3;
                        break;
                    case SDLK_ESCAPE:
                        quit_menu = true;
                        break;
                    default:
                        break;
                }
            }
        }

        SDL_SetRenderDrawColor(menu_renderer, 0, 0, 0, 255);
        SDL_RenderClear(menu_renderer);

        if (menu_bg) {
             SDL_Rect bgRect = {0, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2}; 
             SDL_RenderCopy(menu_renderer, menu_bg, NULL, &bgRect);
        }


        SDL_Color white = {255, 255, 255, 255};
        int center_x = (WINDOW_WIDTH / 2) / 2; 
        int start_y = (WINDOW_HEIGHT / 2) / 4; 
        int line_height = 40;

        render_menu_text(menu_renderer, menu_font, "Choose gamemode:", center_x, start_y, white, true);
        render_menu_text(menu_renderer, menu_font, "1. Singleplayer", center_x, start_y + line_height * 1, white, true);
        render_menu_text(menu_renderer, menu_font, "2. Host Game (Server)", center_x, start_y + line_height * 2, white, true);
        render_menu_text(menu_renderer, menu_font, "3. Join Game (Client)", center_x, start_y + line_height * 3, white, true);
        render_menu_text(menu_renderer, menu_font, "ESC. Quit", center_x, start_y + line_height * 5, white, true);


        SDL_RenderPresent(menu_renderer);
        SDL_Delay(10); 
    }

    char* server_ip_from_sdl_window = NULL;
    if (choice == 3) {
        server_ip_from_sdl_window = get_ip_address_sdl_window(menu_font, menu_window);
    }

    if (menu_bg) SDL_DestroyTexture(menu_bg);
    if (menu_font) TTF_CloseFont(menu_font);
    if (menu_renderer) SDL_DestroyRenderer(menu_renderer);
    if (menu_window) SDL_DestroyWindow(menu_window); 
    
    TTF_Quit();
    IMG_Quit();
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER); 
    SDL_Quit();

    int result = 0;

    if (choice == 1) {
        printf("Startar Singleplayer...\n");
        run_singleplayer(); 
    } else if (choice == 2) {
        printf("Startar Server i bakgrund...\n");

    // Starta servern i en tråd
    SDL_Thread* server_thread = SDL_CreateThread(server_thread_func, "ServerThread", NULL);
    if (!server_thread) {
        printf("Kunde inte skapa server-tråd: %s\n", SDL_GetError());
        return 1;
    }

    // Starta fönster som väntar på SPACE
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* wait_window = SDL_CreateWindow("Press SPACE to join as client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 400, 200, SDL_WINDOW_SHOWN);
    SDL_Renderer* wait_renderer = SDL_CreateRenderer(wait_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    bool wait_for_space = true;
    while (wait_for_space) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                wait_for_space = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                wait_for_space = false;
            }
        }

        SDL_SetRenderDrawColor(wait_renderer, 30, 30, 30, 255);
        SDL_RenderClear(wait_renderer);
        SDL_RenderPresent(wait_renderer);
        SDL_Delay(10);
    }

    SDL_DestroyRenderer(wait_renderer);
    SDL_DestroyWindow(wait_window);

    if (TTF_Init() == -1) {
        printf("TTF_Init Error (Client Phase): %s\n", TTF_GetError());
        return 1;
    }

    TTF_Font* client_font = TTF_OpenFont("resources/font.ttf", 24); 
    if (!client_font) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        return 1;
    }

    char* ip = get_ip_address_sdl_window(client_font, NULL);
    if (ip != NULL && strlen(ip) > 0) {
        printf("Startar Client (ansluter till %s)...\n", ip);
        result = run_client(ip);
        free(ip);
    } else {
        printf("IP-inmatning avbruten eller ogiltig.\n");
        result = 1;
    }

    TTF_CloseFont(client_font);
    TTF_Quit();
    SDL_Quit();
    } else if (choice == 3) {
        if (server_ip_from_sdl_window != NULL && strlen(server_ip_from_sdl_window) > 0) {
             printf("Startar Client (ansluter till %s)...\n", server_ip_from_sdl_window);
             result = run_client(server_ip_from_sdl_window); 
             free(server_ip_from_sdl_window); 
        } else {
             printf("IP-inmatning avbruten eller ingen IP angiven. Programmet avslutas.\n");
             result = 1; 
        }
    } else {
        printf("Inget val gjort eller avslutat från menyn.\n");
    }

    printf("Programmet avslutas med kod %d.\n", result);
    return result;
}