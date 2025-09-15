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
  auto window =
      glfwCreateWindow(512, 512, "", nullptr, nullptr); // TODO: RAII-wrap

  // TODO: `NDEBUG` check
  std::vector layers{"VK_LAYER_KHRONOS_validation"};
  std::vector extensions{vk::EXTDebugUtilsExtensionName};

  uint32_t instanceExtensionCount = 0;
  const char **instanceExtensions =
      glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
  extensions.insert(extensions.end(), instanceExtensions,
                    instanceExtensions + instanceExtensionCount);

  // TODO: verify required extensions and layers
  // (`enumerateInstanceExtensionProperties` and
  // `enumerateInstanceLayerProperties`)

  auto applicationInfo =
      vk::ApplicationInfo{"", 0, "", 0, vk::enumerateInstanceVersion()};

  auto instance =
      vk::createInstanceUnique({{},
                                &applicationInfo,
                                static_cast<uint32_t>(layers.size()),
                                layers.data(),
                                static_cast<uint32_t>(extensions.size()),
                                extensions.data()});

  VkSurfaceKHR rawSurface;
  glfwCreateWindowSurface(*instance, window, nullptr,
                          &rawSurface); // TODO: check `vk::Result::eSuccess`
  auto surface = vk::UniqueSurfaceKHR{
      rawSurface,
      vk::detail::ObjectDestroy<vk::Instance, vk::detail::DispatchLoaderStatic>{
          *instance}};

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window); // TODO: RAII-wrap
  glfwTerminate();           // TODO: RAII-wrap
  return EXIT_SUCCESS;
}
