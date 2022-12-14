#include "graphics.h"
#include "channel.h"
#include "colorscheme.h"
#include "common_function.h"
#include "global.h"
#include "parse.h"
#include "plot.h"
#include "text.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void set_fps(int fps) { g_graphics->fps = fps; }

static inline size_t index_plot_switch(size_t idx) {
  switch (idx) {
    case 0:  return 1;
    case 1:  return 3;
    case 2:  return 5;
    case 3:  return 7;
    case 4:  return 0;
    case 5:  return 2;
    case 6:  return 4;
    case 7:  return 6;
    case 8:  return 8;
    case 9:  return 9;
    case 10: return 10;
    case 11: return 11;
    default: return 0;
  }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void trans_plot_data(int plot_idx, float *data, int length, float dx,
                     float x0) {
  if (plot_idx < 0 || plot_idx > (int)g_plots_count)
    return;

  plot_idx = index_plot_switch(plot_idx);

  free(g_graphics->plots[plot_idx]->fft.data);
  g_graphics->plots[plot_idx]->fft.data = malloc(sizeof(float) * length);

  for(size_t i = 0; i < length; ++i) {
    g_graphics->plots[plot_idx]->fft.data[length - 1 - i] = data[i];
  }

  /* g_graphics->plots[plot_idx]->fft.data = data; */
  g_graphics->plots[plot_idx]->fft.length = length;
  g_graphics->plots[plot_idx]->fft.dx = dx;
  g_graphics->plots[plot_idx]->fft.x0 = x0;
}

char buffer_logger[100];
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
char *logger(void) {
  sprintf(buffer_logger, "%s", "Some log..");
  return buffer_logger;
}

static inline void off_channel_relay(int channel_idx) {
    struct channels* channels = g_graphics->relay_channel;

    if (channels->states[channel_idx] == false) return;
    channels->states[channel_idx] = false;

    struct channel_relay* rel = (struct channel_relay*)channels->channels[channel_idx];
    channel_relay_switch_number(rel);
}

static inline void off_channel_service(int channel_idx) {
    struct channels* channels = g_graphics->service_channel;

    if (channels->states[channel_idx] == false) return;
    channels->states[channel_idx] = false;

    struct channel_service* ser = (struct channel_service*)channels->channels[channel_idx];
    text_change_color(ser->channel_number, COLOR_CHANNEL_NUMBER_OFF);
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void off_channel(int channel_idx) {
  if (channel_idx < 0 || channel_idx >= (int)g_plots_count/2)
    return;

  size_t const serv_count = g_graphics->service_channel->channels_count - 1;
  if(channel_idx <= serv_count) {
    off_channel_service(channel_idx);
  }
  else {
    off_channel_relay(channel_idx-4);
  }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void on_channel(int channel_idx) {
  if (channel_idx < 0 || channel_idx > (int)g_plots_count)
    return;

  /* if(g_graphics->plots[plot_idx]->state == true) return; */
  /* g_graphics->plots[plot_idx]->state = true; */

  /* if(plot_idx <= g_graphics->service_channel->count) { */
  /*   text_change_color(((struct channel_service*)g_graphics->service_channel->channels[plot_idx])->channel_number, */
  /*                     COLOR_CHANNEL_NUMBER_ON); */
  /* } */
  /* else { */
  /*   channel_relay_switch_number(((struct channel_relay*)g_graphics->relay_channel->channels[plot_idx])); */
  /* } */
}

static inline void graphics_plots_crealloc(struct graphics *graphics);

struct graphics *graphics_crealloc(int32_t width, int32_t height, int32_t fps) {
  struct graphics *new_graphics = malloc(sizeof *new_graphics);
  assert(new_graphics);
  new_graphics->width = width;
  new_graphics->height = height;
  new_graphics->width_mid = width / 2;
  new_graphics->height_mid = height / 2;
  new_graphics->fps = fps;
  new_graphics->plots = NULL;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    display_error_sdl("sdl could not init");
    return NULL;
  }

#define _win_and_ren_ 1

#if _win_and_ren_
  SDL_CreateWindowAndRenderer(new_graphics->width, new_graphics->height, 0,
                              &window, &renderer);
#else
  window = SDL_CreateWindow("Odas", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, new_graphics->width,
                            new_graphics->height, SDL_WINDOW_SHOWN);
#endif

  if (window == NULL) {
    display_error_sdl("window could not be created");
    return NULL;
  }

#if !_win_and_ren_
  renderer = SDL_CreateRenderer(
      window, -1, 0 SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#endif

  if (renderer == NULL) {
    display_error_sdl("renderer could not be created");
    return NULL;
  }

  if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
    puts("Warning: linear texture filtering not enabled!");
  }

  if (TTF_Init() < 0) {
    display_error_ttf("TTF could not init");
    return NULL;
  }

  int32_t flags = IMG_INIT_PNG;
  if (!IMG_Init(flags) && flags) {
    display_error_img("Image could not init");
    return NULL;
  }

  new_graphics->service_channel =
      channels_service_crealloc(4, (SDL_Point){.x = 0, .y = 0});
  new_graphics->relay_channel =
      channels_relay_crealloc(2, (SDL_Point){.x = 0, .y = 244 * 2});

  graphics_plots_crealloc(new_graphics);

  return new_graphics;
}

static inline void graphics_plots_crealloc(struct graphics *graphics) {
  graphics->plots = malloc(sizeof(struct plot *) * g_plots_count);

  size_t plot_i = 0;
  for (size_t i = 0; i < graphics->service_channel->channels_count; ++i) {
    graphics->plots[plot_i] =
        ((struct channel_service *)graphics->service_channel->channels[i])
            ->channel->plot0;
    graphics->plots[plot_i + 1] =
        ((struct channel_service *)graphics->service_channel->channels[i])
            ->channel->plot1;
    plot_i += 2;
  }

  for (size_t i = 0; i < graphics->relay_channel->channels_count; ++i) {
    graphics->plots[plot_i] =
        ((struct channel_relay *)graphics->relay_channel->channels[i])
            ->channel->plot0;
    graphics->plots[plot_i + 1] =
        ((struct channel_relay *)graphics->relay_channel->channels[i])
            ->channel->plot1;
    plot_i += 2;
  }
}

void graphics_free(struct graphics *graphics) {
  channels_service_free(graphics->service_channel);
  channels_relay_free(graphics->relay_channel);

  free(graphics->plots);
  free(graphics);
  graphics = NULL;

  SDL_DestroyRenderer(renderer);
  renderer = NULL;

  SDL_DestroyWindow(window);
  window = NULL;

  TTF_Quit();
  SDL_Quit();
}
