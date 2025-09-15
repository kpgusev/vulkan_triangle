#include <cstdlib>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <toml.hpp>
#include <vulkan/vulkan.hpp>

int main(int argc, char **argv) {
  glfwInit(); // TODO: RAII-wrap, `glfwSetErrorCallback`, not `GLFW_FALSE` check

  // TODO: set `GLFW_RESIZABLE`, `GLFW_DECORATED`, etc.
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window =
      glfwCreateWindow(512, 512, "", nullptr, nullptr); // TODO: RAII-wrap

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window); // TODO: RAII-wrap
  glfwTerminate();           // TODO: RAII-wrap
  return EXIT_SUCCESS;
}
