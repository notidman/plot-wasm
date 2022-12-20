#include "plot.h"
#include "common_function.h"
#include "global.h"
#include "common_function.h"
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>


plot_t* Plot_init(SDL_Point position) {
   SDL_Surface *sur = SDL_LoadBMP("res/plot_back.bmp");
   if (sur == NULL ) {
       display_error_sdl("Can't load plot_back.bmp");
       return NULL;
   }

   uint32_t color_key = SDL_MapRGB(sur->format, 0xFF, 0xFF, 0xFF);
   SDL_SetColorKey(sur, SDL_TRUE, color_key);

   SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sur);
   if (tex == NULL) {
       display_error_sdl("Can't create texture from surface-bmp");
       SDL_FreeSurface(sur);
       return NULL;
   }

   plot_t *new_plot = malloc(sizeof(*new_plot));
   new_plot->position.x = position.x;
   new_plot->position.y = position.y;
   new_plot->position.w = sur->w;
   new_plot->position.h = sur->h;
   new_plot->background = tex;

   SDL_FreeSurface(sur);


   return new_plot;
}

void Plot_free(plot_t * plot) {
    if(plot == NULL) {
        printf("plot == NULL");
        return;
    }

    SDL_DestroyTexture(plot->background);
    plot->background = NULL;

    free(plot);
    plot = NULL;
}