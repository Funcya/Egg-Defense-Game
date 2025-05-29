// paths.c
#include "paths.h"
#include <stdlib.h>

// Intern representation
struct Paths {
    int nmbrOfPoints;
    SDL_Point left[NUM_POINTS];
    SDL_Point right[NUM_POINTS];
};

Paths *createPaths(void) {
    Paths *p = malloc(sizeof *p);
    if (!p) return NULL;
    p->nmbrOfPoints = NUM_POINTS;

    // Initiera vänsterpunkter
    p->left[0]  = (SDL_Point){ (int)(WINDOW_WIDTH / 4.615), 0 };
    p->left[1]  = (SDL_Point){ (int)(WINDOW_WIDTH / 4.615), (int)(WINDOW_HEIGHT * 0.09) };
    p->left[2]  = (SDL_Point){ (int)(WINDOW_WIDTH / 10.67), (int)(WINDOW_HEIGHT * 0.09) };
    p->left[3]  = (SDL_Point){ (int)(WINDOW_WIDTH / 10.67), (int)(WINDOW_HEIGHT * 0.20) };
    p->left[4]  = (SDL_Point){ (int)(WINDOW_WIDTH / 6.857), (int)(WINDOW_HEIGHT * 0.20) };
    p->left[5]  = (SDL_Point){ (int)(WINDOW_WIDTH / 2.341), (int)(WINDOW_HEIGHT * 0.20) };
    p->left[6]  = (SDL_Point){ (int)(WINDOW_WIDTH / 2.341), (int)(WINDOW_HEIGHT * 0.35) };
    p->left[7]  = (SDL_Point){ (int)(WINDOW_WIDTH / 3.2),   (int)(WINDOW_HEIGHT * 0.35) };
    p->left[8]  = (SDL_Point){ (int)(WINDOW_WIDTH / 3.2),   (int)(WINDOW_HEIGHT * 0.61) };
    p->left[9]  = (SDL_Point){ (int)(WINDOW_WIDTH / 2.526), (int)(WINDOW_HEIGHT * 0.61) };
    p->left[10] = (SDL_Point){ (int)(WINDOW_WIDTH / 2.526), (int)(WINDOW_HEIGHT * 0.90) };
    p->left[11] = (SDL_Point){ (int)(WINDOW_WIDTH / 13.714),(int)(WINDOW_HEIGHT * 0.90) };
    p->left[12] = (SDL_Point){ (int)(WINDOW_WIDTH / 13.714),(int)(WINDOW_HEIGHT * 0.65) };
    p->left[13] = (SDL_Point){ (int)(WINDOW_WIDTH / 4.364), (int)(WINDOW_HEIGHT * 0.65) };
    p->left[14] = (SDL_Point){ (int)(WINDOW_WIDTH / 4.364), WINDOW_HEIGHT };

    // Skapa speglade högerpunkter
    for (int i = 0; i < NUM_POINTS; ++i) {
        p->right[i] = (SDL_Point){
            WINDOW_WIDTH - p->left[i].x,
            p->left[i].y
        };
    }
    return p;
}

void destroyPaths(Paths *paths) {
    free(paths);
}

int getNumPointsPaths(const Paths *paths) {
    return paths ? paths->nmbrOfPoints : 0;
}

SDL_Point leftPointPaths(const Paths *paths, int index) {
    return paths->left[index];
}

SDL_Point rightPointPaths(const Paths *paths, int index) {
    return paths->right[index];
}
