#pragma once

#include <vulkan/vulkan.h>
#include "vk.h"
#include "window.h"

typedef struct {
  VkContext* ctx;
} RenderContext;

void render_init(RenderContext* render, VkContext* ctx);
void render_draw_quad(RenderContext* render);
void render_game(RenderContext* render);
