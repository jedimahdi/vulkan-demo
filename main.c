#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

GLFWwindow* window;
VkInstance instance;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device;
VkQueue graphics_queue;
VkQueue present_queue;
VkSurfaceKHR surface;

VkSwapchainKHR swapchain;
VkImage* swapchain_images;
VkFormat swapchain_image_format;
VkExtent2D swapchain_extent;

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
  VkPresentModeKHR present_modes;
} SwapchainSupportDetails;

SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device) {
  SwapchainSupportDetails details = {0};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
  // vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, )

  return details;
}

bool is_device_suitable(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);

  QueueFamilyIndices queue_families = find_queue_families(device);

  return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         queue_families.found_present_family &&
         queue_families.found_graphics_family;
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

void create_swap_chain() {
  VkSwapchainCreateInfoKHR create_info = {0};
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

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);
  vkDestroyDevice(device, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
