// paths.h
#ifndef PATHS_H
#define PATHS_H

#include <SDL2/SDL.h>
#include "defs.h"

// Opaque type for path data
typedef struct Paths Paths;

Paths *createPaths(void);
void destroyPaths(Paths *paths);
int getNumPointsPaths(const Paths *paths);
SDL_Point leftPointPaths(const Paths *paths, int index);
SDL_Point rightPointPaths(const Paths *paths, int index);

#endif // PATHS_H