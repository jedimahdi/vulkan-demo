#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define CLAMP(x, a, b) (((x) < (a)) ? (a) : ((b) < (x)) ? (b) \
                                                        : (x))

GLFWwindow* window;
VkInstance instance;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device;
VkQueue graphics_queue;
VkQueue present_queue;
VkSurfaceKHR surface;

VkSwapchainKHR swapchain;
VkImage* swapchain_images;
uint32_t swapchain_images_count = 0;
VkFormat swapchain_image_format;
VkExtent2D swapchain_extent;
VkImageView* swapchain_image_views;

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
    actual_extent.width = CLAMP(actual_extent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);
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

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);
  vkDestroySwapchainKHR(device, swapchain, NULL);

  for (uint32_t i = 0; i < swapchain_images_count; ++i) {
    vkDestroyImageView(device, swapchain_image_views[i], NULL);
  }

  vkDestroyDevice(device, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
