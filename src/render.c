#include "render.h"
#include <stdio.h>

static VkResult record_command_buffer(VkContext* ctx, VkCommandBuffer cmd, uint32_t image_index);

void render_init(RenderContext* render, VkContext* ctx) {
  render->ctx = ctx;
}

void render_draw_quad(RenderContext* render) {
}

void acquire(VkContext* ctx) {
  uint32_t current_frame = ctx->current_frame;
  VkFence* fence = &ctx->in_flight_fences[current_frame];

  vkWaitForFences(ctx->device, 1, fence, VK_TRUE, UINT64_MAX);
  vkResetFences(ctx->device, 1, fence);

  vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, ctx->image_available_semaphores[current_frame],
                        VK_NULL_HANDLE, &ctx->image_index);
}

void submit(VkContext* ctx) {
  uint32_t current_frame = ctx->current_frame;
  uint32_t image_index = ctx->image_index;

  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &ctx->image_available_semaphores[current_frame],
      .pWaitDstStageMask = wait_stages,
      .commandBufferCount = 1,
      .pCommandBuffers = &ctx->command_buffers[current_frame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &ctx->render_finished_semaphores[image_index]};

  vkQueueSubmit(ctx->graphics_queue, 1, &submit_info, ctx->in_flight_fences[current_frame]);
}

void present(VkContext* ctx) {
  uint32_t image_index = ctx->image_index;
  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &ctx->render_finished_semaphores[image_index],
      .swapchainCount = 1,
      .pSwapchains = &ctx->swapchain,
      .pImageIndices = &ctx->image_index};
  vkQueuePresentKHR(ctx->present_queue, &present_info);

  ctx->current_frame = (ctx->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void render_game(RenderContext* render) {
  VkContext* ctx = render->ctx;

  acquire(ctx);

  record_command_buffer(ctx, ctx->command_buffers[ctx->current_frame], ctx->image_index);
  submit(ctx);
  present(ctx);
}

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
