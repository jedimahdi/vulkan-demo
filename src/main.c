#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "base.h"
#include "vk.h"

#define WIDTH 800
#define HEIGHT 600

//
// void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index) {
//   VkCommandBufferBeginInfo begin_info = {
//       .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//       .flags = 0,
//       .pInheritanceInfo = NULL};
//
//   if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
//     fprintf(stderr, "Failed to begin recording command buffer!\n");
//     exit(1);
//   }
//
//   VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
//   VkRenderPassBeginInfo render_pass_info = {
//       .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
//       .renderPass = render_pass,
//       .framebuffer = swapchain_framebuffers[image_index],
//       .renderArea.offset = {0, 0},
//       .renderArea.extent = swapchain_extent,
//       .clearValueCount = 1,
//       .pClearValues = &clear_color};
//   vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
//   vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
//
//   VkViewport viewport = {
//       .x = 0.0f,
//       .y = 0.0f,
//       .width = (float)swapchain_extent.width,
//       .height = (float)swapchain_extent.height,
//       .minDepth = 0.0f,
//       .maxDepth = 1.0f};
//   vkCmdSetViewport(command_buffer, 0, 1, &viewport);
//
//   VkRect2D scissor = {
//       .offset = {0, 0},
//       .extent = swapchain_extent};
//   vkCmdSetScissor(command_buffer, 0, 1, &scissor);
//   vkCmdDraw(command_buffer, 3, 1, 0, 0);
//
//   vkCmdEndRenderPass(command_buffer);
//   if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
//     fprintf(stderr, "Failed to record command buffer!\n");
//     exit(1);
//   }
// }
//
//
// void draw_frame() {
//   vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
//   vkResetFences(device, 1, &in_flight_fences[current_frame]);
//
//   uint32_t image_index;
//   vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);
//
//   vkResetCommandBuffer(command_buffers[current_frame], 0);
//   record_command_buffer(command_buffers[current_frame], image_index);
//
//   VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame]};
//   VkSemaphore signal_semaphores[] = {render_finished_per_image_semaphores[image_index]};
//   VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
//   VkSubmitInfo submit_info = {
//       .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//       .waitSemaphoreCount = 1,
//       .pWaitSemaphores = wait_semaphores,
//       .pWaitDstStageMask = wait_stages,
//       .commandBufferCount = 1,
//       .pCommandBuffers = &command_buffers[current_frame],
//       .signalSemaphoreCount = 1,
//       .pSignalSemaphores = signal_semaphores,
//   };
//
//   if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS) {
//     fprintf(stderr, "Failed to submit draw command buffer!\n");
//     exit(1);
//   }
//
//   VkSwapchainKHR swapchains[] = {swapchain};
//   VkPresentInfoKHR present_info = {
//       .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
//       .waitSemaphoreCount = 1,
//       .pWaitSemaphores = signal_semaphores,
//       .swapchainCount = 1,
//       .pSwapchains = swapchains,
//       .pImageIndices = &image_index,
//       .pResults = NULL,
//   };
//   vkQueuePresentKHR(present_queue, &present_info);
//   current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
// }
//
// void init_window() {
//   if (!glfwInit()) {
//     fprintf(stderr, "Failed to initialize GLFW\n");
//     exit(1);
//   }
//
//   if (!glfwVulkanSupported()) {
//     fprintf(stderr, "Vulkan not supported\n");
//     glfwTerminate();
//     exit(1);
//   }
//
//   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
//
//   window = glfwCreateWindow(800, 600, "Vulkan Window", NULL, NULL);
//   if (!window) {
//     fprintf(stderr, "Failed to create GLFW window\n");
//     glfwTerminate();
//     exit(1);
//   }
// }
//
// void cleanup() {
//   for (uint32_t i = 0; i < swapchain_images_count; ++i) {
//     vkDestroyFramebuffer(device, swapchain_framebuffers[i], NULL);
//   }
//   for (uint32_t i = 0; i < swapchain_images_count; ++i) {
//     vkDestroyImageView(device, swapchain_image_views[i], NULL);
//   }
//   vkDestroyPipeline(device, graphics_pipeline, NULL);
//   vkDestroyPipelineLayout(device, pipeline_layout, NULL);
//   vkDestroyRenderPass(device, render_pass, NULL);
//   vkDestroyCommandPool(device, command_pool, NULL);
//   for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
//     vkDestroySemaphore(device, image_available_semaphores[i], NULL);
//     vkDestroyFence(device, in_flight_fences[i], NULL);
//   }
//   for (uint32_t i = 0; i < swapchain_images_count; ++i) {
//     vkDestroySemaphore(device, render_finished_per_image_semaphores[i], NULL);
//   }
//   vkDestroySwapchainKHR(device, swapchain, NULL);
//   // vkDestroySurfaceKHR(instance, surface, NULL);
//   vkDestroyDevice(device, NULL);
//   // vkDestroyInstance(instance, NULL);
// }
//
// int main2() {
//   init_window();
//
//   VkContext ctx;
//   VkResult res = vk_init(window, &ctx);
//
//   instance = ctx.instance;
//   surface = ctx.surface;
//
//   if (res != VK_SUCCESS) {
//     fprintf(stderr, "WTF!\n");
//     return 1;
//   }
//
//   // create_instance();
//   // create_surface();
//
//   pick_physical_device();
//   create_logical_device();
//   create_swap_chain();
//   create_image_views();
//   create_render_pass();
//   create_graphics_pipeline();
//   create_framebuffers();
//   create_command_pool();
//   create_command_buffer();
//   create_sync_objects();
//
//   uint32_t api_version;
//   LOG_INFO("GLFW %s", glfwGetVersionString());
//
//   LOG_INFO("test");
//
//   while (!glfwWindowShouldClose(window)) {
//     glfwPollEvents();
//     draw_frame();
//   }
//   vkDeviceWaitIdle(device);
//   cleanup();
//   vk_cleanup(&ctx);
//   glfwDestroyWindow(window);
//   glfwTerminate();
//
//   return 0;
// }

// VkResult create_framebuffers(VkContext* ctx) {
//   ctx->swapchain_framebuffers = malloc(sizeof(VkFramebuffer) * ctx->swapchain_images_count);
//
//   for (uint32_t i = 0; i < ctx->swapchain_images_count; ++i) {
//     VkImageView attachments[] = {ctx->swapchain_image_views[i]};
//     VkFramebufferCreateInfo framebuffer_info = {
//         .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
//         .renderPass = ctx->render_pass,
//         .attachmentCount = 1,
//         .pAttachments = attachments,
//         .width = ctx->swapchain_extent.width,
//         .height = ctx->swapchain_extent.height,
//         .layers = 1};
//
//     if (vkCreateFramebuffer(ctx->device, &framebuffer_info, NULL, &ctx->swapchain_framebuffers[i]) != VK_SUCCESS) {
//       fprintf(stderr, "Failed to create framebuffer!\n");
//       exit(1);
//     }
//   }
// }

SDL_Window* window;

static VkResult record_command_buffer(VkContext* ctx, VkCommandBuffer cmd, uint32_t image_index) {
  // Make sure the command buffer is back to INITIAL state before re-recording
  VkResult res = vkResetCommandBuffer(cmd, 0);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "vkResetCommandBuffer failed: %d\n", res);
    return res;
  }

  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = NULL};
  res = vkBeginCommandBuffer(cmd, &begin_info);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "vkBeginCommandBuffer failed: %d\n", res);
    return res;
  }

  VkClearValue clear_color = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};

  VkRenderPassBeginInfo render_pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = ctx->render_pass,
      .framebuffer = ctx->swapchain_framebuffers[image_index],
      .renderArea = {
          .offset = {0, 0},
          .extent = ctx->swapchain_extent},
      .clearValueCount = 1,
      .pClearValues = &clear_color};

  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  // Graphics pipeline must match the render pass and subpass index.
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphics_pipeline);

  // If your pipeline declared viewport/scissor as dynamic, set them here:
  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)ctx->swapchain_extent.width,
      .height = (float)ctx->swapchain_extent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f};
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor = {
      .offset = {0, 0},
      .extent = ctx->swapchain_extent};
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  // Draw a fullscreen-ish triangle (pipeline with no vertex buffers / no vertex input)
  vkCmdDraw(cmd, 3, 1, 0, 0);

  vkCmdEndRenderPass(cmd);

  res = vkEndCommandBuffer(cmd);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "vkEndCommandBuffer failed: %d\n", res);
    return res;
  }

  return VK_SUCCESS;
}

void vulkan_init(VkContext* ctx) {
  if (vk_init(window, ctx) != VK_SUCCESS) {
    fprintf(stderr, "Failed to initialize vulkan!\n");
    SDL_Quit();
    exit(1);
  }
}

void sdl_init() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Failed to initialize SDL3: %s\n", SDL_GetError());
    exit(1);
  }

  window = SDL_CreateWindow("Hello", WIDTH, HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }
  SDL_Vulkan_LoadLibrary(NULL);
}

void sdl_cleanup() {
  SDL_DestroyWindow(window);
  SDL_Quit();
}

int main(void) {
  sdl_init();

  VkContext* ctx = malloc(sizeof(*ctx));
  vulkan_init(ctx);

  VkFence* images_in_flight = malloc(sizeof(VkFence) * ctx->swapchain_images_count);
  for (uint32_t i = 0; i < ctx->swapchain_images_count; i++) {
    images_in_flight[i] = VK_NULL_HANDLE;
  }

  bool should_close = false;
  SDL_Event e;
  while (!should_close) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) {
        should_close = true;
      }
    }
    // 1. Wait for the current frame's fence
    vkWaitForFences(ctx->device, 1, &ctx->in_flight_fences[ctx->current_frame],
                    VK_TRUE, UINT64_MAX);

    // 2. Acquire next image
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(
        ctx->device, ctx->swapchain,
        UINT64_MAX,
        ctx->image_available_semaphores[ctx->current_frame],
        VK_NULL_HANDLE,
        &image_index);

    // 3. If the image is already in use, wait for its fence
    if (images_in_flight[image_index] != VK_NULL_HANDLE) {
      vkWaitForFences(ctx->device, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }
    images_in_flight[image_index] = ctx->in_flight_fences[ctx->current_frame];

    // 4. Reset the fence for the current frame
    vkResetFences(ctx->device, 1, &ctx->in_flight_fences[ctx->current_frame]);

    // 5. Record command buffer using image_index
    record_command_buffer(ctx, ctx->command_buffers[ctx->current_frame], image_index);

    // 6. Submit work
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &ctx->image_available_semaphores[ctx->current_frame],
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &ctx->command_buffers[ctx->current_frame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &ctx->render_finished_semaphores[ctx->current_frame]};

    vkQueueSubmit(ctx->graphics_queue, 1, &submit_info, ctx->in_flight_fences[ctx->current_frame]);

    // 7. Present
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &ctx->render_finished_semaphores[ctx->current_frame],
        .swapchainCount = 1,
        .pSwapchains = &ctx->swapchain,
        .pImageIndices = &image_index};
    vkQueuePresentKHR(ctx->present_queue, &present_info);

    // 8. Move to next frame
    ctx->current_frame = (ctx->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  vk_cleanup(ctx);
  sdl_cleanup();
  return 0;
}
