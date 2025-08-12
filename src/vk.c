#include "vk.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLAMP(x, a, b) (((x) < (a)) ? (a) : ((b) < (x)) ? (b) \
                                                        : (x))
#define COUNTOF(a) (sizeof(a) / sizeof(*(a)))
#define ALIGN_FORWARD(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#define VK_RETURN(expr)                  \
  do {                                   \
    VkResult _res = (expr);              \
    if (_res != VK_SUCCESS) return _res; \
  } while (0)

static void log_version() {
  uint32_t api_version;
  vkEnumerateInstanceVersion(&api_version);
  uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(api_version);
  uint32_t apiVersionMinor = VK_API_VERSION_MINOR(api_version);
  uint32_t apiVersionPatch = VK_API_VERSION_PATCH(api_version);
  fprintf(stderr, "Vulkan API %u.%u.%u\n", apiVersionMajor, apiVersionMinor, apiVersionPatch);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  const char* severity =
      (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)     ? "ERROR"
      : (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "WARNING"
      : (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    ? "INFO"
                                                                            : "VERBOSE";
  fprintf(stderr, "[%s] %s\n", severity, pCallbackData->pMessage);
  return VK_FALSE;
}

static void remove_duplicates(const char* arr[], uint32_t* count) {
  for (uint32_t i = 0; i < *count; ++i) {
    for (uint32_t j = i + 1; j < *count;) {
      if (strcmp(arr[i], arr[j]) == 0) {
        arr[j] = arr[--(*count)];
      } else {
        j++;
      }
    }
  }
}

static bool has_instance_extension(const char* name) {
  uint32_t count = 0;
  if (vkEnumerateInstanceExtensionProperties(NULL, &count, NULL) != VK_SUCCESS) return false;
  VkExtensionProperties* props = malloc(count * sizeof(*props));
  if (!props) return false;
  bool found = false;
  if (vkEnumerateInstanceExtensionProperties(NULL, &count, props) == VK_SUCCESS) {
    for (uint32_t i = 0; i < count; ++i) {
      if (strcmp(props[i].extensionName, name) == 0) {
        found = true;
        break;
      }
    }
  }
  free(props);
  return found;
}

static bool has_validation_layer(const char* name) {
  uint32_t count = 0;
  if (vkEnumerateInstanceLayerProperties(&count, NULL) != VK_SUCCESS) return false;
  VkLayerProperties* layers = malloc(count * sizeof(*layers));
  if (!layers) return false;
  bool found = false;
  if (vkEnumerateInstanceLayerProperties(&count, layers) == VK_SUCCESS) {
    for (uint32_t i = 0; i < count; ++i) {
      if (strcmp(layers[i].layerName, name) == 0) {
        found = true;
        break;
      }
    }
  }
  free(layers);
  return found;
}

static VkResult create_instance_sdl(VkContext* ctx) {
  uint32_t window_exts_count = 0;
  const char* const* window_exts = window_get_vulkan_required_extensions(&window_exts_count);
  if (!window_exts || window_exts_count == 0) {
    fprintf(stderr, "Failed to get SDL3 required extensions\n");
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  const char* portability_ext = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
  const char* debug_ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

  uint32_t max_extra = 2;
  uint32_t max_total = window_exts_count + max_extra;
  const char** exts = malloc(max_total * sizeof(*exts));
  if (!exts) return VK_ERROR_OUT_OF_HOST_MEMORY;

  uint32_t exts_count = 0;
  for (uint32_t i = 0; i < window_exts_count; ++i) {
    exts[exts_count++] = window_exts[i];
  }

  VkInstanceCreateFlags flags = 0;

  if (has_instance_extension(portability_ext)) {
    exts[exts_count++] = portability_ext;
    flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    fprintf(stderr, "Enabled instance extension: %s\n", portability_ext);
  }

#ifndef NDEBUG
  bool enable_validation = has_validation_layer("VK_LAYER_KHRONOS_validation");
  if (!enable_validation) {
    fprintf(stderr, "Warning: validation layer not available\n");
  }
  if (enable_validation && has_instance_extension(debug_ext)) {
    exts[exts_count++] = debug_ext;
    fprintf(stderr, "Enabled instance extension: %s\n", debug_ext);
  }
#else
  bool enable_validation = false;
#endif

  remove_duplicates(exts, &exts_count);

  uint32_t api_version = VK_API_VERSION_1_2;
  uint32_t loader_version = VK_API_VERSION_1_0;
  PFN_vkEnumerateInstanceVersion pEnumVer =
      (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
  if (!pEnumVer || pEnumVer(&loader_version) != VK_SUCCESS) {
    loader_version = VK_API_VERSION_1_0;
  }
  if (api_version > loader_version) api_version = loader_version;

  const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Hello Triangle",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = api_version,
  };
  VkInstanceCreateInfo instance_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = exts_count,
      .ppEnabledExtensionNames = exts,
      .enabledLayerCount = enable_validation ? 1u : 0u,
      .ppEnabledLayerNames = enable_validation ? validation_layers : NULL,
      .flags = flags,
  };
  VkResult res = vkCreateInstance(&instance_info, NULL, &ctx->instance);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create vulkan instance!\n");
  }
  ctx->enable_validation = enable_validation;
  free(exts);
  return res;
}

static VkResult setup_debug_utils(VkContext* ctx) {
  if (!ctx->enable_validation) return VK_SUCCESS;

  PFN_vkCreateDebugUtilsMessengerEXT pCreate =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkCreateDebugUtilsMessengerEXT");
  if (!pCreate) return VK_SUCCESS;

  VkDebugUtilsMessengerCreateInfoEXT ci = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = NULL,
      .flags = 0,
      .messageSeverity =
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
      .messageType =
          VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debug_callback,
      .pUserData = NULL,
  };

  return pCreate(ctx->instance, &ci, NULL, &ctx->debug_messenger);
}

static VkResult create_sdl_surface(Window* window, VkContext* ctx) {
  if (!window_create_vulkan_surface(window, ctx->instance, NULL, &ctx->surface)) {
    fprintf(stderr, "Failed to create window surface\n");
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return VK_SUCCESS;
}

typedef struct {
  uint32_t graphics_family;
  uint32_t present_family;
  bool found_graphics_family;
  bool found_present_family;
} QueueFamilyIndices;

static QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkContext* ctx) {
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
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, ctx->surface, &present_support);
    if (present_support) {
      result.present_family = i;
      result.found_present_family = true;
    }

    if (result.found_graphics_family && result.found_present_family) break;
  }
  free(queue_families);
  return result;
}

typedef struct {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR* formats;
  uint32_t formats_count;
  VkPresentModeKHR* present_modes;
  uint32_t present_modes_count;
} SwapchainSupportDetails;

static SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkContext* ctx) {
  SwapchainSupportDetails details = {0};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, ctx->surface, &details.capabilities);

  vkGetPhysicalDeviceSurfaceFormatsKHR(device, ctx->surface, &details.formats_count, NULL);
  if (details.formats_count != 0) {
    details.formats = malloc(sizeof(*details.formats) * details.formats_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, ctx->surface, &details.formats_count, details.formats);
  }

  vkGetPhysicalDeviceSurfacePresentModesKHR(device, ctx->surface, &details.present_modes_count, NULL);
  if (details.present_modes_count != 0) {
    details.present_modes = malloc(sizeof(*details.present_modes) * details.present_modes_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, ctx->surface, &details.present_modes_count, details.present_modes);
  }

  return details;
}

static void free_swapchain_support(SwapchainSupportDetails* details) {
  free(details->formats);
  free(details->present_modes);
  details->formats = NULL;
  details->formats_count = 0;
  details->present_modes = NULL;
  details->present_modes_count = 0;
}

static bool is_device_suitable(VkPhysicalDevice device, VkContext* ctx) {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);

  QueueFamilyIndices queue_families = find_queue_families(device, ctx);

  bool swapchain_adequate = false;
  SwapchainSupportDetails swapchain_support = query_swapchain_support(device, ctx);
  swapchain_adequate = swapchain_support.formats_count != 0 && swapchain_support.present_modes_count;
  free_swapchain_support(&swapchain_support);

  return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         queue_families.found_present_family &&
         queue_families.found_graphics_family &&
         swapchain_adequate;
}

static VkResult pick_physical_device(VkContext* ctx) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(ctx->instance, &device_count, NULL);
  if (device_count == 0) {
    fprintf(stderr, "Failed to find GPUs with Vulkan support!");
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
  if (!devices) return VK_ERROR_OUT_OF_HOST_MEMORY;

  VkResult res = vkEnumeratePhysicalDevices(ctx->instance, &device_count, devices);
  if (res != VK_SUCCESS) {
    free(devices);
    return res;
  }

  for (uint32_t i = 0; i < device_count; ++i) {
    if (is_device_suitable(devices[i], ctx)) {
      ctx->physical_device = devices[i];
      break;
    }
  }
  free(devices);

  if (ctx->physical_device == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to find a suitable GPU!\n");
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return VK_SUCCESS;
}

static VkResult create_logical_device(VkContext* ctx) {
  QueueFamilyIndices queue_familiy_indicies = find_queue_families(ctx->physical_device, ctx);

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

  VkResult res = vkCreateDevice(ctx->physical_device, &create_info, NULL, &ctx->device);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create logical device!\n");
    return res;
  }

  vkGetDeviceQueue(ctx->device, queue_familiy_indicies.graphics_family, 0, &ctx->graphics_queue);
  vkGetDeviceQueue(ctx->device, queue_familiy_indicies.present_family, 0, &ctx->present_queue);
  free(queue_create_infos);
  free(unique_queue_families);
  return res;
}

static VkSurfaceFormatKHR choose_swap_surface_format(VkSurfaceFormatKHR* available_formats, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    VkSurfaceFormatKHR format = available_formats[i];
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  return available_formats[0];
}

static VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR* available_present_modes, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    VkPresentModeKHR mode = available_present_modes[i];
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return mode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_swap_extent(Window* window, VkSurfaceCapabilitiesKHR* capabilities) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  } else {
    int width, height;
    window_get_drawable_size(window, &width, &height);

    VkExtent2D actual_extent = {
        .width = (uint32_t)width,
        .height = (uint32_t)height};
    actual_extent.width = CLAMP(actual_extent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    actual_extent.height = CLAMP(actual_extent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);
    return actual_extent;
  }
}

static VkResult create_swapchain(Window* window, VkContext* ctx) {
  SwapchainSupportDetails swapchain_support = query_swapchain_support(ctx->physical_device, ctx);

  VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats, swapchain_support.formats_count);
  VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.present_modes, swapchain_support.present_modes_count);
  VkExtent2D extent = choose_swap_extent(window, &swapchain_support.capabilities);

  uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;

  if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
    image_count = swapchain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = ctx->surface;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = find_queue_families(ctx->physical_device, ctx);
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

  VkResult res = vkCreateSwapchainKHR(ctx->device, &create_info, NULL, &ctx->swapchain);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create swapchain!\n");
    return res;
  }
  vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_images_count, NULL);
  ctx->swapchain_images = malloc(sizeof(VkImage) * ctx->swapchain_images_count);
  vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_images_count, ctx->swapchain_images);

  fprintf(stderr, "SwapChain images count: %u\n", ctx->swapchain_images_count);

  ctx->swapchain_image_format = surface_format.format;
  ctx->swapchain_extent = extent;

  free_swapchain_support(&swapchain_support);
  return res;
}

static VkResult create_image_views(VkContext* ctx) {
  ctx->swapchain_image_views = malloc(sizeof(VkImageView) * ctx->swapchain_images_count);

  for (uint32_t i = 0; i < ctx->swapchain_images_count; ++i) {
    VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = ctx->swapchain_images[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = ctx->swapchain_image_format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };
    VkResult res = vkCreateImageView(ctx->device, &create_info, NULL, &ctx->swapchain_image_views[i]);
    if (res != VK_SUCCESS) {
      return res;
    }
  }
  return VK_SUCCESS;
}

static VkResult create_command_pool(VkContext* ctx) {
  QueueFamilyIndices queue_family_indicies = find_queue_families(ctx->physical_device, ctx);

  VkCommandPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queue_family_indicies.graphics_family};

  VkResult res = vkCreateCommandPool(ctx->device, &pool_info, NULL, &ctx->command_pool);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create command pool!\n");
  }
  return res;
}

static VkResult create_render_pass(VkContext* ctx) {
  VkAttachmentDescription color_attachment = {
      .format = ctx->swapchain_image_format,
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

  VkResult res = vkCreateRenderPass(ctx->device, &render_pass_info, NULL, &ctx->render_pass);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create render pass!\n");
  }
  return res;
}

static VkResult create_graphics_pipeline(VkContext* ctx) {
  VkResult res = VK_SUCCESS;
  VkShaderModule vert_shader_module, frag_shader_module;
  if ((res = create_shader_module(ctx, "shaders/vert.spv", &vert_shader_module)) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create vertex shader module!\n");
    return res;
  }
  if ((res = create_shader_module(ctx, "shaders/frag.spv", &frag_shader_module)) != VK_SUCCESS) {
    vkDestroyShaderModule(ctx->device, vert_shader_module, NULL);
    fprintf(stderr, "Failed to create vertex shader module!\n");
    return res;
  }

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

  res = vkCreatePipelineLayout(ctx->device, &pipeline_layout_info, NULL, &ctx->pipeline_layout);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create pipeline layout!\n");
    goto cleanup;
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
      .layout = ctx->pipeline_layout,
      .renderPass = ctx->render_pass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = -1};

  res = vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &ctx->graphics_pipeline);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create graphics pipeline!\n");
  }

cleanup:
  vkDestroyShaderModule(ctx->device, vert_shader_module, NULL);
  vkDestroyShaderModule(ctx->device, frag_shader_module, NULL);
  return res;
}

static VkResult create_framebuffers(VkContext* ctx) {
  ctx->swapchain_framebuffers = malloc(sizeof(VkFramebuffer) * ctx->swapchain_images_count);

  for (uint32_t i = 0; i < ctx->swapchain_images_count; ++i) {
    VkImageView attachments[] = {ctx->swapchain_image_views[i]};
    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = ctx->render_pass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = ctx->swapchain_extent.width,
        .height = ctx->swapchain_extent.height,
        .layers = 1};

    VkResult res = vkCreateFramebuffer(ctx->device, &framebuffer_info, NULL, &ctx->swapchain_framebuffers[i]);
    if (res != VK_SUCCESS) {
      fprintf(stderr, "Failed to create framebuffer!\n");
      for (uint32_t j = 0; j < i; ++j) {
        vkDestroyFramebuffer(ctx->device, ctx->swapchain_framebuffers[j], NULL);
      }
      return res;
    }
  }
  return VK_SUCCESS;
}

static VkResult create_sync_objects(VkContext* ctx) {
  VkSemaphoreCreateInfo semaphore_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  ctx->render_finished_semaphores = malloc(sizeof(VkSemaphore) * ctx->swapchain_images_count);

  for (uint32_t i = 0; i < ctx->swapchain_images_count; ++i) {
    if (vkCreateSemaphore(ctx->device, &semaphore_info, NULL, &ctx->render_finished_semaphores[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create semaphores!\n");
      return VK_ERROR_INITIALIZATION_FAILED;
    }
  }

  ctx->image_available_semaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (vkCreateSemaphore(ctx->device, &semaphore_info, NULL, &ctx->image_available_semaphores[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create semaphores!\n");
      return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (vkCreateFence(ctx->device, &fence_info, NULL,
                      &ctx->in_flight_fences[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create in_flight fence for frame %u\n", i);
      return VK_ERROR_INITIALIZATION_FAILED;
    }
  }

  return VK_SUCCESS;
}

static VkResult create_command_buffer(VkContext* ctx) {
  VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = ctx->command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT};

  VkResult res = vkAllocateCommandBuffers(ctx->device, &alloc_info, ctx->command_buffers);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create command buffer!\n");
  }
  return res;
}

VkResult vk_init(Window* window, VkContext* ctx) {
  memset(ctx, 0, sizeof(*ctx));
  ctx->instance = VK_NULL_HANDLE;
  ctx->debug_messenger = VK_NULL_HANDLE;
  ctx->surface = VK_NULL_HANDLE;
  ctx->physical_device = VK_NULL_HANDLE;
  ctx->device = VK_NULL_HANDLE;
  ctx->graphics_queue = VK_NULL_HANDLE;
  ctx->present_queue = VK_NULL_HANDLE;
  ctx->swapchain = VK_NULL_HANDLE;
  ctx->swapchain_images_count = 0;
  ctx->command_pool = VK_NULL_HANDLE;
  ctx->render_pass = VK_NULL_HANDLE;
  ctx->graphics_pipeline = VK_NULL_HANDLE;
  ctx->pipeline_layout = VK_NULL_HANDLE;

  log_version();

  VkResult res = VK_SUCCESS;

  if ((res = create_instance_sdl(ctx)) != VK_SUCCESS) goto fail;
  if ((res = setup_debug_utils(ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_sdl_surface(window, ctx)) != VK_SUCCESS) goto fail;
  if ((res = pick_physical_device(ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_logical_device(ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_swapchain(window, ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_image_views(ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_command_pool(ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_render_pass(ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_framebuffers(ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_graphics_pipeline(ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_sync_objects(ctx)) != VK_SUCCESS) goto fail;
  if ((res = create_command_buffer(ctx)) != VK_SUCCESS) goto fail;

  return res;

fail:
  vk_cleanup(ctx);
  return res;
}

void vk_cleanup(VkContext* ctx) {
  uint32_t images_count = ctx->swapchain_images_count;

  if (ctx->device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(ctx->device);
  }

  for (uint32_t j = 0; j < MAX_FRAMES_IN_FLIGHT; ++j) {
    if (ctx->image_available_semaphores[j] != VK_NULL_HANDLE) {
      vkDestroySemaphore(ctx->device, ctx->image_available_semaphores[j], NULL);
      ctx->image_available_semaphores[j] = VK_NULL_HANDLE;
    }
    if (ctx->in_flight_fences[j] != VK_NULL_HANDLE) {
      vkDestroyFence(ctx->device, ctx->in_flight_fences[j], NULL);
      ctx->in_flight_fences[j] = VK_NULL_HANDLE;
    }
  }

  for (uint32_t i = 0; i < images_count; ++i) {
    if (ctx->render_finished_semaphores[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(ctx->device, ctx->render_finished_semaphores[i], NULL);
      ctx->render_finished_semaphores[i] = VK_NULL_HANDLE;
    }
  }

  if (ctx->swapchain_framebuffers) {
    for (uint32_t i = 0; i < ctx->swapchain_images_count; ++i) {
      vkDestroyFramebuffer(ctx->device, ctx->swapchain_framebuffers[i], NULL);
    }
    free(ctx->swapchain_framebuffers);
    ctx->swapchain_framebuffers = NULL;
  }

  if (ctx->graphics_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(ctx->device, ctx->graphics_pipeline, NULL);
    ctx->graphics_pipeline = VK_NULL_HANDLE;
  }

  if (ctx->pipeline_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(ctx->device, ctx->pipeline_layout, NULL);
    ctx->pipeline_layout = VK_NULL_HANDLE;
  }

  if (ctx->render_pass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(ctx->device, ctx->render_pass, NULL);
    ctx->render_pass = VK_NULL_HANDLE;
  }

  if (ctx->swapchain_image_views) {
    for (uint32_t i = 0; i < ctx->swapchain_images_count; ++i) {
      vkDestroyImageView(ctx->device, ctx->swapchain_image_views[i], NULL);
    }
    free(ctx->swapchain_image_views);
    ctx->swapchain_image_views = NULL;
  }

  if (ctx->command_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(ctx->device, ctx->command_pool, NULL);
    ctx->command_pool = VK_NULL_HANDLE;
  }

  if (ctx->swapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
    ctx->swapchain = VK_NULL_HANDLE;
  }

  if (ctx->device != VK_NULL_HANDLE) {
    vkDestroyDevice(ctx->device, NULL);
    ctx->device = VK_NULL_HANDLE;
    ctx->graphics_queue = VK_NULL_HANDLE;
    ctx->present_queue = VK_NULL_HANDLE;
  }
  if (ctx->surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    ctx->surface = VK_NULL_HANDLE;
  }
  if (ctx->debug_messenger != VK_NULL_HANDLE) {
    PFN_vkDestroyDebugUtilsMessengerEXT pDestroy =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (pDestroy) {
      pDestroy(ctx->instance, ctx->debug_messenger, NULL);
      ctx->debug_messenger = VK_NULL_HANDLE;
    }
    vkDestroyInstance(ctx->instance, NULL);
    ctx->instance = VK_NULL_HANDLE;
  }
  if (ctx->instance != VK_NULL_HANDLE) {
    vkDestroyInstance(ctx->instance, NULL);
    ctx->instance = VK_NULL_HANDLE;
  }
}

VkResult create_shader_module(VkContext* ctx, const char* path, VkShaderModule* module) {
  FILE* fp = fopen(path, "rb");
  fseek(fp, 0, SEEK_END);
  long len = ftell(fp);
  rewind(fp);

  char* code = aligned_alloc(16, ALIGN_FORWARD(len + 1, 16));
  fread(code, 1, len, fp);
  code[len] = '\0';

  VkShaderModuleCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = (size_t)len,
      .pCode = (uint32_t*)code,
  };
  VkResult res = vkCreateShaderModule(ctx->device, &create_info, NULL, module);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create shader module!\n");
  }
  fclose(fp);
  free(code);
  return res;
}
