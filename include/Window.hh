#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {
public:
  Window(int width, int height, const char *title);
  ~Window();

  // TODO: copy- & move- constructor & operator implementation
  Window(const Window &) = delete;
  Window &operator=(Window &) = delete;
  Window(const Window &&) = delete;
  Window &operator=(const Window &&) = delete;

  [[nodiscard]] GLFWwindow *get() const;
  GLFWwindow *operator*() const;

  bool shouldClose() const;

private:
  GLFWwindow *window;
};