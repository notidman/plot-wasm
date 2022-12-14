#include "channel.h"
#include "colorscheme.h"
#include "common_function.h"
#include "global.h"
#include "plot.h"
#include "text.h"

#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>

static inline char const *get_relay_num(size_t num) {
  switch (num) {
  case 0:
    return RELAY_NUMBER_1_ON;
  case 1:
    return RELAY_NUMBER_2_ON;
  case 2:
    return RELAY_NUMBER_1_OFF;
  case 3:
    return RELAY_NUMBER_2_OFF;
  default:
    return "";
  }
}

static inline void channel_crealloc(struct channel *channel, SDL_Point position,
                                    char const *plot0_name,
                                    char const *plot1_name, SDL_Rect pos_num) {
  channel->position = position;
  // plots
  SDL_Point pos_plot0_body = {.x = 69 + 48 + pos_num.x, .y = pos_num.y - 25};
  channel->plot0 = plot_crealloc(pos_plot0_body);

  SDL_Point pos_plot1_body = {.x = pos_plot0_body.x +
                                   channel->plot0->position.w + 73,
                              .y = pos_num.y - 25};
  channel->plot1 = plot_crealloc(pos_plot1_body);

  int32_t const plot_size_name = 20;
  SDL_Rect pos_plot0_name = {.x = pos_plot0_body.x + 9,
                             .y = pos_plot0_body.y - 11};
  channel->plot0_name =
      text_crealloc(TEXT_FONT_BOLD, plot_size_name, COLOR_PLOT_NAME,
                    pos_plot0_name, plot0_name);

  SDL_Rect pos_plot1_name = {.x = pos_plot0_name.x + 280,
                             .y = pos_plot0_name.y};
  channel->plot1_name =
      text_crealloc(TEXT_FONT_BOLD, plot_size_name, COLOR_PLOT_NAME,
                    pos_plot1_name, plot1_name);
  channel->state = true;
}

static inline void channel_free(struct channel *channel) {
  text_free(channel->plot0_name);
  text_free(channel->plot1_name);
  plot_free(channel->plot0);
  plot_free(channel->plot1);
  channel->state = false;

  free(channel);
  channel = NULL;
}

struct channel *channel_service_crealloc(SDL_Point position,
                                         int32_t channel_number,
                                         char const *plot0_name,
                                         char const *plot1_name) {
  struct channel_service *new_channel = malloc(sizeof *new_channel);
  new_channel->channel = malloc(sizeof *new_channel->channel);

  SDL_Rect pos_num = {.x = position.x + 26, .y = position.y + 67};
  channel_crealloc(new_channel->channel, position, plot0_name, plot1_name,
                   pos_num);

  char channel_number_s[10] = {'\0'};
  sprintf(channel_number_s, "%d", channel_number);

  // body
  new_channel->channel_number = text_crealloc(
      TEXT_FONT_BOLD, 150, COLOR_CHANNEL_NUMBER_ON, pos_num, channel_number_s);

  return (struct channel *)new_channel;
}

void channel_service_free(struct channel *channel) {
  if (channel == NULL) {
    return;
  }

  struct channel_service *schannel = (struct channel_service *)channel;
  channel_free(schannel->channel);

  text_free(schannel->channel_number);
  free(schannel);
  channel = NULL;
}

struct channels *channels_service_crealloc(size_t count, SDL_Point position) {
  struct channels *channels = malloc(sizeof *channels);
  channels->channels_count = count;
  channels->channels = malloc(sizeof(struct channels *) * count);
  channels->states = malloc(sizeof(bool) * count);

  SDL_Point dpos = position;

  for (size_t i = 0; i < count; ++i) {
    channels->channels[i] = channel_service_crealloc(dpos, i + 1, "Tx", "Rx");
    channels->states[i] = ((struct channel_service*)channels->channels[i])->channel->state;

    if (!is_even(i)) {
      dpos.y += 244;
      dpos.x -= 654 * 2;
    }

    dpos.x += 654;
  }

  return channels;
}

void channels_service_free(struct channels *channels) {
  if (channels == NULL) {
    return;
  }

  for (size_t i = 0; i < channels->channels_count; ++i) {
    if (channels->channels[i] == NULL)
      continue;
    channel_service_free(channels->channels[i]);
    channels->channels[i] = NULL;
  }

  free(channels->states);
  free(channels->channels);
  channels->channels = NULL;

  free(channels);
  channels = NULL;
}

struct channel *channel_relay_crealloc(SDL_Point position,
                                       int32_t channel_number,
                                       char const *plot0_name,
                                       char const *plot1_name) {
  struct channel_relay *new_channel = malloc(sizeof *new_channel);
  new_channel->channel = malloc(sizeof *new_channel->channel);
  new_channel->channel->position = position;
  new_channel->channel_number_count = channel_number;

  // body
  SDL_Rect pos_num = {.x = position.x + 26, .y = position.y + 67};
  channel_crealloc(new_channel->channel, position, plot0_name, plot1_name,
                   pos_num);

  SDL_Surface *sur_num = IMG_Load(get_relay_num(channel_number));
  if (sur_num == NULL) {
    display_error_img("Can't load rim_n.png");
    return NULL;
  }

  new_channel->channel_number = SDL_CreateTextureFromSurface(renderer, sur_num);
  if (new_channel->channel_number == NULL) {
    display_error_sdl("Can't create texture from surface sur_num");
    SDL_FreeSurface(sur_num);
    return NULL;
  }
  new_channel->channel_number_pos = (SDL_Rect){
      .w = sur_num->w, .h = sur_num->h, .x = pos_num.x, .y = pos_num.y};

  SDL_FreeSurface(sur_num);

  return (struct channel *)new_channel;
}

void channel_relay_free(struct channel *channel) {
  if (channel == NULL) {
    return;
  }

  struct channel_relay *rchannel = (struct channel_relay *)channel;
  channel_free(rchannel->channel);

  SDL_DestroyTexture(rchannel->channel_number);
  free(rchannel);
  channel = NULL;
}

struct channels *channels_relay_crealloc(size_t count, SDL_Point position) {
  struct channels *channels = malloc(sizeof *channels);
  channels->channels_count = count;
  channels->channels = malloc(sizeof(struct channel *) * count);
  channels->states = malloc(sizeof(bool) * count);

  SDL_Point dpos = position;

  for (size_t i = 0; i < count; ++i) {
    channels->channels[i] = channel_relay_crealloc(dpos, i, "Tx", "Rx");
    channels->states[i] = ((struct channel_relay*)channels->channels[i])->channel->state;

    if (!is_even(i)) {
      dpos.y += 244;
      dpos.x -= 654 * 2;
    }

    dpos.x += 654;
  }

  return channels;
}

void channels_relay_free(struct channels *channels) {
  if (channels == NULL) {
    return;
  }

  for (size_t i = 0; i < channels->channels_count; ++i) {
    if (channels->channels[i] == NULL)
      continue;
    channel_relay_free(channels->channels[i]);
    channels->channels[i] = NULL;
  }

  free(channels->states);
  free(channels->channels);
  channels->channels = NULL;

  free(channels);
  channels = NULL;
}


void channel_relay_switch_number(struct channel_relay* channel) {

  size_t num = 0;
  switch(channel->channel_number_count) {
  case 0: num = 2; break;
  case 1: num = 3; break;
  case 2: num = 0; break;
  case 3: num = 1; break;
  }

  SDL_Surface *sur_num = IMG_Load(get_relay_num(num));
  if (sur_num == NULL) {
    display_error_img("Can't load rim_n.png");
    return;
  }

  if(channel->channel_number != NULL) {
    SDL_DestroyTexture(channel->channel_number);
    channel->channel_number = NULL;
  }

  channel->channel_number = SDL_CreateTextureFromSurface(renderer, sur_num);
  if (channel->channel_number == NULL) {
    display_error_sdl("Can't create texture from surface sur_num");
    SDL_FreeSurface(sur_num);
    return;
  }

  SDL_FreeSurface(sur_num);
}
