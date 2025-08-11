#include <GLFW/glfw3.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stdio.h>
#include <stdlib.h>

#include "window.h"

struct Window {
  SDL_Window* handle;
  SDL_GLContext gl_ctx;
  bool should_close;
};

Window* window_create(WindowDesc* desc) {
  if (!desc) return NULL;

  struct Window* win = malloc(sizeof(*win));
  if (!win) return NULL;

  const char* title = desc->title ? desc->title : "";

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Failed to initialize SDL3: %s\n", SDL_GetError());
    free(win);
    return NULL;
  }

  SDL_WindowFlags flags = SDL_WINDOW_VULKAN;
  if (desc->resizable) flags |= SDL_WINDOW_RESIZABLE;

  SDL_Window* handle = SDL_CreateWindow(title, desc->width, desc->height, flags);
  if (!handle) {
    fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
    SDL_Quit();
    free(win);
    return NULL;
  }
  SDL_Vulkan_LoadLibrary(NULL);

  win->handle = handle;
  win->should_close = false;
  return win;
}

bool window_should_close(Window* win) {
  return win->should_close;
}

void window_set_should_close(Window* win, bool should_close) {
  win->should_close = should_close;
}

void window_poll_events(Window* win) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_EVENT_QUIT:
        win->should_close = true;
        break;
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        win->should_close = true;
        break;
    }
  }
}

void window_get_size(Window* win, int* width, int* height) {
  SDL_GetWindowSize(win->handle, width, height);
}

void window_get_drawable_size(Window* win, int* width, int* height) {
  SDL_GetWindowSizeInPixels(win->handle, width, height);
}

void* window_get_handle(Window* win) {
  return win->handle;
}

void window_destroy(Window* win) {
  if (!win) return;
  if (win->handle) {
    SDL_DestroyWindow(win->handle);
    SDL_Quit();
    win->handle = NULL;
  }
  free(win);
}

bool window_create_gl_context(Window* w) {
  if (!w || !w->handle) return false;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  w->gl_ctx = SDL_GL_CreateContext(w->handle);
  if (!w->gl_ctx) {
    fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
    return false;
  }
  return true;
}

void window_make_gl_current(Window* w) {
  if (w && w->handle && w->gl_ctx)
    SDL_GL_MakeCurrent(w->handle, w->gl_ctx);
}

void window_swap_buffers(Window* w) {
  if (w && w->handle)
    SDL_GL_SwapWindow(w->handle);
}

void* window_get_gl_proc_address(const char* name) {
  return SDL_GL_GetProcAddress(name);
}

bool window_create_vulkan_surface(Window* w, VkInstance instance, const void* allocator, VkSurfaceKHR* out_surface) {
  if (!SDL_Vulkan_CreateSurface(w->handle, instance, allocator, out_surface)) {
    fprintf(stderr, "SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
    return false;
  }
  return true;
}

const char** window_get_vulkan_required_extensions(uint32_t* count) {
  return (const char**)SDL_Vulkan_GetInstanceExtensions(count);
}
