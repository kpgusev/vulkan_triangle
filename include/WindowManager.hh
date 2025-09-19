#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

class WindowManager {
public:
  WindowManager(WindowManager &) = delete;
  WindowManager &operator=(WindowManager &) = delete;
  WindowManager(WindowManager &&) = delete;
  WindowManager &operator=(WindowManager &&) = delete;

  static WindowManager &instance();

  GLFWwindow *createWindow(int width, int height, const char *title) const;
  void destroyWindow(GLFWwindow *window) const;
  void
  insertRequiredInstanceExtensions(std::vector<const char *> &extensions) const;
  void setParameter(int parameter, int value) const;
  VkSurfaceKHR createSurface(const vk::Instance &instance,
                             GLFWwindow *window) const;
  void pollEvents() const;

  bool windowShouldClose(GLFWwindow *window) const;

private:
  WindowManager();
  ~WindowManager();
};