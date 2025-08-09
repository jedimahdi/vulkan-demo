#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define MAX_FRAMES_IN_FLIGHT 5

#define CLAMP(x, a, b) (((x) < (a)) ? (a) : ((b) < (x)) ? (b) \
                                                        : (x))
#define COUNTOF(a) (sizeof(a) / sizeof(*(a)))

GLFWwindow* window;
VkInstance instance;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device;
VkQueue graphics_queue;
VkQueue present_queue;
VkSurfaceKHR surface;

VkSwapchainKHR swapchain;
VkFormat swapchain_image_format;
VkExtent2D swapchain_extent;
VkImage* swapchain_images;
uint32_t swapchain_images_count = 0;
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
uint32_t current_frame = 0;

void create_instance() {
  VkApplicationInfo app_info = {0};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Hello Triangle";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  uint32_t glfw_extensions_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

  const char** extensions = malloc(sizeof(char*) * (glfw_extensions_count + 1));
  uint32_t extensions_count = glfw_extensions_count;
  memcpy(extensions, glfw_extensions, sizeof(char*) * glfw_extensions_count);
  extensions[extensions_count++] = "VK_KHR_portability_enumeration";

  char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

  VkInstanceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount = extensions_count;
  create_info.ppEnabledExtensionNames = extensions;
  create_info.enabledLayerCount = 1;
  create_info.ppEnabledLayerNames = (const char* const*)validation_layers;
  create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  create_info.pNext = NULL;

  if (vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create vulkan instance!\n");
    exit(1);
  }
  free(extensions);
}

void create_surface() {
  if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create window surface!\n");
    exit(1);
  }
}

typedef struct {
  uint32_t graphics_family;
  uint32_t present_family;
  bool found_graphics_family;
  bool found_present_family;
} QueueFamilyIndices;

QueueFamilyIndices find_queue_families(VkPhysicalDevice device) {
  QueueFamilyIndices result = {0};
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
  VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

  for (uint32_t i = 0; i < queue_family_count; ++i) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      result.graphics_family = i;
      result.found_graphics_family = true;
    }
    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
    if (present_support) {
      result.present_family = i;
      result.found_present_family = true;
    }

    if (result.found_graphics_family && result.found_present_family) break;
  }

  return result;
}

typedef struct {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR* formats;
  uint32_t formats_count;
  VkPresentModeKHR* present_modes;
  uint32_t present_modes_count;
} SwapchainSupportDetails;

SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device) {
  SwapchainSupportDetails details = {0};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formats_count, NULL);
  if (details.formats_count != 0) {
    details.formats = malloc(sizeof(*details.formats) * details.formats_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formats_count, details.formats);
  }

  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_modes_count, NULL);
  if (details.present_modes_count != 0) {
    details.present_modes = malloc(sizeof(*details.present_modes) * details.present_modes_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_modes_count, details.present_modes);
  }

  return details;
}

bool is_device_suitable(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);

  QueueFamilyIndices queue_families = find_queue_families(device);

  bool swapchain_adequate = false;
  SwapchainSupportDetails swapchain_support = query_swapchain_support(device);
  swapchain_adequate = swapchain_support.formats_count != 0 && swapchain_support.present_modes_count;

  return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         queue_families.found_present_family &&
         queue_families.found_graphics_family &&
         swapchain_adequate;
}

void pick_physical_device() {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, NULL);
  if (device_count == 0) {
    fprintf(stderr, "Failed to find GPUs with Vulkan support!");
    exit(1);
  }
  VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices);

  for (uint32_t i = 0; i < device_count; ++i) {
    if (is_device_suitable(devices[i])) {
      physical_device = devices[i];
      break;
    }
  }
  free(devices);

  if (physical_device == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to find a suitable GPU!");
    exit(1);
  }
}

void create_logical_device() {
  QueueFamilyIndices queue_familiy_indicies = find_queue_families(physical_device);

  uint32_t* unique_queue_families = malloc(sizeof(uint32_t) * 2);
  uint32_t unique_queue_family_count = 0;
  unique_queue_families[unique_queue_family_count++] = queue_familiy_indicies.graphics_family;
  if (queue_familiy_indicies.graphics_family != queue_familiy_indicies.present_family) {
    unique_queue_families[unique_queue_family_count++] = queue_familiy_indicies.present_family;
  }

  VkDeviceQueueCreateInfo* queue_create_infos = malloc(sizeof(VkDeviceQueueCreateInfo) * unique_queue_family_count);
  uint32_t queue_create_info_count = 0;

  float queue_priority = 1.0f;
  for (uint32_t i = 0; i < unique_queue_family_count; ++i) {
    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = unique_queue_families[i];
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos[queue_create_info_count++] = queue_create_info;
  }

  VkPhysicalDeviceFeatures device_features = {0};

  const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
  const char* device_extensions[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkDeviceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = queue_create_infos;
  create_info.queueCreateInfoCount = queue_create_info_count;
  create_info.pEnabledFeatures = &device_features;
  create_info.pNext = NULL;
  create_info.ppEnabledLayerNames = validation_layers;
  create_info.enabledLayerCount = 1;
  create_info.enabledExtensionCount = 1;
  create_info.ppEnabledExtensionNames = device_extensions;

  if (vkCreateDevice(physical_device, &create_info, NULL, &device) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create logical device!\n");
    exit(1);
  }

  vkGetDeviceQueue(device, queue_familiy_indicies.graphics_family, 0, &graphics_queue);
  vkGetDeviceQueue(device, queue_familiy_indicies.present_family, 0, &present_queue);
  free(queue_create_infos);
  free(unique_queue_families);
}

VkSurfaceFormatKHR choose_swap_surface_format(VkSurfaceFormatKHR* available_formats, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    VkSurfaceFormatKHR format = available_formats[i];
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  return available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR* available_present_modes, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    VkPresentModeKHR mode = available_present_modes[i];
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_extent(VkSurfaceCapabilitiesKHR* capabilities) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {
        .width = (uint32_t)width,
        .height = (uint32_t)height};
    actual_extent.width = CLAMP(actual_extent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    actual_extent.height = CLAMP(actual_extent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);
    return actual_extent;
  }
}

void create_swap_chain() {
  SwapchainSupportDetails swapchain_support = query_swapchain_support(physical_device);

  VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats, swapchain_support.formats_count);
  VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.present_modes, swapchain_support.present_modes_count);
  VkExtent2D extent = choose_swap_extent(&swapchain_support.capabilities);

  uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;

  if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
    image_count = swapchain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = find_queue_families(physical_device);
  uint32_t queue_family_indicies[] = {indices.graphics_family, indices.present_family};
  if (indices.graphics_family != indices.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indicies;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = NULL;
  }

  create_info.preTransform = swapchain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create swapchain!\n");
    exit(1);
  }
  vkGetSwapchainImagesKHR(device, swapchain, &swapchain_images_count, NULL);
  swapchain_images = malloc(sizeof(VkImage) * swapchain_images_count);
  vkGetSwapchainImagesKHR(device, swapchain, &swapchain_images_count, swapchain_images);

  swapchain_image_format = surface_format.format;
  swapchain_extent = extent;
}

void create_image_views() {
  swapchain_image_views = malloc(sizeof(VkImageView) * swapchain_images_count);

  for (uint32_t i = 0; i < swapchain_images_count; ++i) {
    VkImageViewCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swapchain_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swapchain_image_format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &create_info, NULL, &swapchain_image_views[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create image views!\n");
      exit(1);
    }
  }
}

VkShaderModule create_shader_module(const char* path) {
  FILE* fp = fopen(path, "rb");
  fseek(fp, 0, SEEK_END);
  long len = ftell(fp);
  rewind(fp);

  char* code = aligned_alloc(8, len + 1);
  fread(code, 1, len, fp);
  code[len] = '\0';

  VkShaderModuleCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = (size_t)len,
      .pCode = (uint32_t*)code,
  };
  VkShaderModule shader_module;
  if (vkCreateShaderModule(device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create shader module!\n");
    exit(1);
  }
  fclose(fp);
  free(code);
  return shader_module;
}

void create_render_pass() {
  VkAttachmentDescription color_attachment = {
      .format = swapchain_image_format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

  VkAttachmentReference color_attachment_ref = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_ref};

  VkSubpassDependency dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
  VkRenderPassCreateInfo render_pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &color_attachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency};

  if (vkCreateRenderPass(device, &render_pass_info, NULL, &render_pass) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create render pass!\n");
    exit(1);
  }
}

void create_graphics_pipeline() {
  VkShaderModule vert_shader_module = create_shader_module("shaders/vert.spv");
  VkShaderModule frag_shader_module = create_shader_module("shaders/frag.spv");

  VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vert_shader_module,
      .pName = "main"};

  VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = frag_shader_module,
      .pName = "main"};

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = NULL,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = NULL};

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE};

  VkPipelineViewportStateCreateInfo viewport_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1};

  VkPipelineRasterizationStateCreateInfo rasterizer = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .lineWidth = 1.0f,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE};

  VkPipelineMultisampleStateCreateInfo multisampling = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable = VK_FALSE,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

  VkPipelineColorBlendAttachmentState color_blend_attachment = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_FALSE};
  VkPipelineColorBlendStateCreateInfo color_blending = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment,
      .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}};

  VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = (uint32_t)COUNTOF(dynamic_states),
      .pDynamicStates = dynamic_states};

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pushConstantRangeCount = 0};

  if (vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &pipeline_layout) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create pipeline layout!\n");
    exit(1);
  }

  VkGraphicsPipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = NULL,
      .pColorBlendState = &color_blending,
      .pDynamicState = &dynamic_state,
      .layout = pipeline_layout,
      .renderPass = render_pass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = -1};

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &graphics_pipeline) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create graphics pipeline!\n");
    exit(1);
  }

  vkDestroyShaderModule(device, vert_shader_module, NULL);
  vkDestroyShaderModule(device, frag_shader_module, NULL);
}

void create_framebuffers() {
  swapchain_framebuffers = malloc(sizeof(VkFramebuffer) * swapchain_images_count);

  for (uint32_t i = 0; i < swapchain_images_count; ++i) {
    VkImageView attachments[] = {swapchain_image_views[i]};
    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = swapchain_extent.width,
        .height = swapchain_extent.height,
        .layers = 1};

    if (vkCreateFramebuffer(device, &framebuffer_info, NULL, &swapchain_framebuffers[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create framebuffer!\n");
      exit(1);
    }
  }
}

void create_command_pool() {
  QueueFamilyIndices queue_family_indicies = find_queue_families(physical_device);

  VkCommandPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queue_family_indicies.graphics_family};

  if (vkCreateCommandPool(device, &pool_info, NULL, &command_pool) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create command pool!\n");
    exit(1);
  }
}

void create_command_buffer() {
  VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT};

  if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create command buffer!\n");
    exit(1);
  }
}

void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index) {
  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = 0,
      .pInheritanceInfo = NULL};

  if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
    fprintf(stderr, "Failed to begin recording command buffer!\n");
    exit(1);
  }

  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  VkRenderPassBeginInfo render_pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = render_pass,
      .framebuffer = swapchain_framebuffers[image_index],
      .renderArea.offset = {0, 0},
      .renderArea.extent = swapchain_extent,
      .clearValueCount = 1,
      .pClearValues = &clear_color};
  vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)swapchain_extent.width,
      .height = (float)swapchain_extent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f};
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor = {
      .offset = {0, 0},
      .extent = swapchain_extent};
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  vkCmdDraw(command_buffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(command_buffer);
  if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
    fprintf(stderr, "Failed to record command buffer!\n");
    exit(1);
  }
}

void create_sync_objects() {
  VkSemaphoreCreateInfo semaphore_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (vkCreateSemaphore(device, &semaphore_info, NULL, &image_available_semaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphore_info, NULL, &render_finished_semaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fence_info, NULL, &in_flight_fences[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create semaphores!\n");
      exit(1);
    }
  }
}

void draw_frame() {
  vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &in_flight_fences[current_frame]);

  uint32_t image_index;
  vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

  vkResetCommandBuffer(command_buffers[current_frame], 0);
  record_command_buffer(command_buffers[current_frame], image_index);

  VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame]};
  VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};
  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = wait_semaphores,
      .pWaitDstStageMask = wait_stages,
      .commandBufferCount = 1,
      .pCommandBuffers = &command_buffers[current_frame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signal_semaphores,
  };

  if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS) {
    fprintf(stderr, "Failed to submit draw command buffer!\n");
    exit(1);
  }

  VkSwapchainKHR swapchains[] = {swapchain};
  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signal_semaphores,
      .swapchainCount = 1,
      .pSwapchains = swapchains,
      .pImageIndices = &image_index,
      .pResults = NULL,
  };
  vkQueuePresentKHR(present_queue, &present_info);
  current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int main() {
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return EXIT_FAILURE;
  }

  if (!glfwVulkanSupported()) {
    fprintf(stderr, "Vulkan not supported\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(800, 600, "Vulkan Window", NULL, NULL);
  if (!window) {
    fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }

  create_instance();
  create_surface();
  pick_physical_device();
  create_logical_device();
  create_swap_chain();
  create_image_views();
  create_render_pass();
  create_graphics_pipeline();
  create_framebuffers();
  create_command_pool();
  create_command_buffer();
  create_sync_objects();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    draw_frame();
  }
  vkDeviceWaitIdle(device);

  for (uint32_t i = 0; i < swapchain_images_count; ++i) {
    vkDestroyFramebuffer(device, swapchain_framebuffers[i], NULL);
  }
  for (uint32_t i = 0; i < swapchain_images_count; ++i) {
    vkDestroyImageView(device, swapchain_image_views[i], NULL);
  }
  vkDestroyPipeline(device, graphics_pipeline, NULL);
  vkDestroyPipelineLayout(device, pipeline_layout, NULL);
  vkDestroyRenderPass(device, render_pass, NULL);
  vkDestroyCommandPool(device, command_pool, NULL);
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(device, image_available_semaphores[i], NULL);
    vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
    vkDestroyFence(device, in_flight_fences[i], NULL);
  }
  vkDestroySwapchainKHR(device, swapchain, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyDevice(device, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
