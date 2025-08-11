#include <SDL3/SDL.h>
#include <stdlib.h>
#include "input.h"

struct Input {
  SDL_Window* window_handle;
  bool keys[COUNT_KEYS];
  bool mouse_buttons[COUNTS_MOUSE_BUTTONS];
  int mouse_x, mouse_y;
};

Input* input_create(void* window_handle) {
  Input* input = malloc(sizeof(*input));
  if (!input) return NULL;
  memset(input, 0, sizeof(*input));
  input->window_handle = window_handle;
  return input;
}

void input_update(Input* input) {
  int n = 0;
  const bool* ks = SDL_GetKeyboardState(&n);
#define SC(sc) ((int)(sc) < n ? ks[(int)(sc)] : 0)
  input->keys[KEY_W] = SC(SDL_SCANCODE_W);
  input->keys[KEY_A] = SC(SDL_SCANCODE_A);
  input->keys[KEY_S] = SC(SDL_SCANCODE_S);
  input->keys[KEY_D] = SC(SDL_SCANCODE_D);
  input->keys[KEY_ESCAPE] = SC(SDL_SCANCODE_ESCAPE);
#undef SC

  float x = 0, y = 0;
  Uint32 mask = SDL_GetMouseState(&x, &y);
  input->mouse_x = (int)x;
  input->mouse_y = (int)y;
  // input->mouse_buttons[MOUSE_LEFT] = (mask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
  // input->mouse_buttons[MOUSE_RIGHT] = (mask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
  // input->mouse_buttons[MOUSE_MIDDLE] = (mask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
}

bool input_is_key_down(Input* in, KeyCode k) {
  return in && k >= 0 && k < COUNT_KEYS ? in->keys[k] : false;
}

bool input_is_mouse_button_down(Input* in, MouseButton b) {
  return in && b >= 0 && b < COUNTS_MOUSE_BUTTONS ? in->mouse_buttons[b] : false;
}

void input_get_mouse_position(Input* in, int* x, int* y) {
  if (x) *x = in->mouse_x;
  if (y) *y = in->mouse_y;
}

void input_destroy(Input* in) {
  free(in);
}
