#include "vk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXTRA_EXTENSIONS_COUNT 2

#define VK_CHECK(expr)                                   \
  do {                                                   \
    VkResult _res = (expr);                              \
    if (_res != VK_SUCCESS) {                            \
      fprintf(stderr, "Vulkan error %d at %s:%d — %s\n", \
              _res, __FILE__, __LINE__, #expr);          \
      return _res;                                       \
    }                                                    \
  } while (0)

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
  fprintf(stderr, "Vulkan API %u.%u.%u", apiVersionMajor, apiVersionMinor, apiVersionPatch);
}

static VkResult create_instance(VkContext* ctx) {
  uint32_t glfw_extensions_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
  if (!glfw_extensions || glfw_extensions_count == 0) {
    fprintf(stderr, "Failed to get GLFW required extensions\n");
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  uint32_t total_extensions = glfw_extensions_count + EXTRA_EXTENSIONS_COUNT;
  const char** extensions = malloc(sizeof(char*) * total_extensions);
  if (!extensions) return VK_ERROR_OUT_OF_HOST_MEMORY;

  memcpy(extensions, glfw_extensions, sizeof(char*) * glfw_extensions_count);
  uint32_t extensions_count = glfw_extensions_count;
  extensions[extensions_count++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;

  const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Hello Triangle",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_2,
  };
  VkInstanceCreateInfo instance_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = extensions_count,
      .ppEnabledExtensionNames = extensions,
      .enabledLayerCount = 1,
      .ppEnabledLayerNames = validation_layers,
      .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
  };
  VkResult res = vkCreateInstance(&instance_info, NULL, &ctx->instance);
  free(extensions);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create vulkan instance!\n");
  }
  return res;
}

static VkResult create_glfw_surface(GLFWwindow* window, VkContext* ctx) {
  VkResult res = glfwCreateWindowSurface(ctx->instance, window, NULL, &ctx->surface);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Failed to create window surface!\n");
  }
  return res;
}

VkResult vk_init(GLFWwindow* window, VkContext* ctx) {
  ctx->instance = VK_NULL_HANDLE;
  ctx->surface = VK_NULL_HANDLE;

  log_version();
  VK_RETURN(create_instance(ctx));
  VK_RETURN(create_glfw_surface(window, ctx));

  return VK_SUCCESS;
}

void vk_cleanup(VkContext* ctx) {
  if (ctx->surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    ctx->surface = VK_NULL_HANDLE;
  }
  if (ctx->instance != VK_NULL_HANDLE) {
    vkDestroyInstance(ctx->instance, NULL);
    ctx->instance = VK_NULL_HANDLE;
  }
}
