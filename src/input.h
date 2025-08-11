#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Input Input;

typedef enum {
  KEY_UNKNOWN = 0,
  KEY_W,
  KEY_A,
  KEY_S,
  KEY_D,
  KEY_ESCAPE,
  COUNT_KEYS
} KeyCode;

typedef enum {
  MOUSE_LEFT = 0,
  MOUSE_RIGHT,
  MOUSE_MIDDLE,
  COUNTS_MOUSE_BUTTONS
} MouseButton;

Input* input_create(void* window_handle);
void input_update(Input* input);
bool input_is_key_down(Input* input, KeyCode key);
bool input_is_mouse_button_down(Input* input, MouseButton button);
void input_get_mouse_position(Input* input, int* x, int* y);
void input_destroy(Input* input);

#endif
