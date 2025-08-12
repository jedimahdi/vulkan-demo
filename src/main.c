#include <stdlib.h>
#include "input.h"
#include "render.h"
#include "window.h"

#define WIDTH 800
#define HEIGHT 600

int main(void) {
  Window* window = window_create(&(WindowDesc){
      .width = WIDTH,
      .height = HEIGHT,
      .title = "Vulkan",
      .resizable = false,
  });
  Input* input = input_create(window_get_handle(window));

  VkContext* ctx = malloc(sizeof(*ctx));
  vk_init(window, ctx);

  RenderContext render;
  render_init(&render, ctx);

  while (!window_should_close(window)) {
    window_poll_events(window);
    input_update(input);
    render_game(&render);
  }

  vk_cleanup(ctx);
  input_destroy(input);
  window_destroy(window);
  return 0;
}
