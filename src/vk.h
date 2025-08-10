#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

extern VkInstance instance;
extern VkSurfaceKHR surface;

VkResult init_vulkan(GLFWwindow* window);
