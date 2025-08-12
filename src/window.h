#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Window Window;

typedef struct {
  int width;
  int height;
  char* title;
  bool resizable;
} WindowDesc;

Window* window_create(WindowDesc* desc);
bool window_should_close(Window* win);
void window_set_should_close(Window* win, bool should_close);
void window_poll_events(Window* win);
void window_get_size(Window* win, int* width, int* height);
void window_get_drawable_size(Window* win, int* width, int* height);
void* window_get_handle(Window* win);
void window_destroy(Window* win);

// --- Vulkan ---
struct VkInstance_T;
struct VkSurfaceKHR_T;
struct VkAllocationCallbacks;

bool window_create_vulkan_surface(
    Window* win,
    struct VkInstance_T* instance,
    const struct VkAllocationCallbacks* allocator,
    struct VkSurfaceKHR_T** out_surface);
const char** window_get_vulkan_required_extensions(uint32_t* count);

// --- OpenGL ---
bool window_create_gl_context(Window* w);
void window_make_gl_current(Window* w);
void window_swap_buffers(Window* w);
void* window_get_gl_proc_address(const char* name);

#endif
