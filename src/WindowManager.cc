#include "WindowManager.hh"

WindowManager::WindowManager() {
  glfwSetErrorCallback([](int, const char *description) {
    throw std::runtime_error(description); // TODO: custom exceptions
  });

  if (!glfwInit())
    throw std::runtime_error("Failed to initialize GLFW");
  setParameter(GLFW_CLIENT_API, GLFW_NO_API);
}

WindowManager::~WindowManager() { glfwTerminate(); }

WindowManager &WindowManager::instance() {
  static WindowManager windowManager;
  return windowManager;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
GLFWwindow *WindowManager::createWindow(const int width, const int height,
                                        const char *title) const {
  return glfwCreateWindow(width, height, title, nullptr, nullptr);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void WindowManager::destroyWindow(GLFWwindow *window) const {
  glfwDestroyWindow(window);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void WindowManager::insertRequiredInstanceExtensions(
    std::vector<const char *> &extensions) const {
  uint32_t instanceExtensionCount = 0;
  const char **instanceExtensions =
      glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
  extensions.insert(extensions.end(), instanceExtensions,
                    instanceExtensions + instanceExtensionCount);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void WindowManager::setParameter(const int parameter, const int value) const {
  glfwWindowHint(parameter, value);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
VkSurfaceKHR WindowManager::createSurface(const vk::Instance &instance,
                                          GLFWwindow *window) const {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS)
    throw std::runtime_error(
        "Failed to create window surface"); // TODO: custom exceptions
  return surface;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool WindowManager::windowShouldClose(GLFWwindow *window) const {
  return glfwWindowShouldClose(window);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void WindowManager::pollEvents() const { glfwPollEvents(); }