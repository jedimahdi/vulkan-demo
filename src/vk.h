#pragma once

#include <vulkan/vulkan.h>
#include "window.h"

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct {
  VkInstance instance;
  VkSurfaceKHR surface;
  bool enable_validation;
  VkDebugUtilsMessengerEXT debug_messenger;

  VkPhysicalDevice physical_device;
  VkDevice device;
  VkQueue graphics_queue;
  VkQueue present_queue;

  VkSwapchainKHR swapchain;
  VkFormat swapchain_image_format;
  VkExtent2D swapchain_extent;
  VkImage* swapchain_images;
  uint32_t swapchain_images_count;
  VkImageView* swapchain_image_views;
  VkFramebuffer* swapchain_framebuffers;

  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  VkCommandPool command_pool;
  VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
  VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
  uint32_t current_frame;

  VkFence frame_fences[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore acquire_semaphores[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore* submit_semaphores;  // size = swapchain_image_count
  uint32_t frame_index;
} VkContext;

VkResult vk_init(Window* window, VkContext* ctx);
VkResult create_shader_module(VkContext* ctx, const char* path, VkShaderModule* module);
void vk_cleanup(VkContext* ctx);
