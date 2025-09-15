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
  // (`vk::enumerateInstanceExtensionProperties` and
  // `vk::enumerateInstanceLayerProperties`)

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

  auto physicalDevice =
      instance->enumeratePhysicalDevices().front(); // TODO: check is suitable

  auto queuePriority = 1.0f;
  auto deviceQueueCreateInfos = std::vector{vk::DeviceQueueCreateInfo{
      {},
      0, // TODO: `std::set` of  unique queue families indices
      1,
      &queuePriority}};
  auto deviceExtensions = std::vector{vk::KHRSwapchainExtensionName};
  auto device = physicalDevice.createDeviceUnique(
      {{},
       static_cast<uint32_t>(deviceQueueCreateInfos.size()),
       deviceQueueCreateInfos.data(),
       0,
       nullptr,
       static_cast<uint32_t>(deviceExtensions.size()),
       deviceExtensions.data(),
       nullptr});

  auto queueFamilyIndices = std::vector<uint32_t>{0};
  auto swapchain = device->createSwapchainKHRUnique(
      {{},
       *surface,
       4, // TODO: smart set
       vk::Format::eB8G8R8A8Unorm,        // TODO: find best
       vk::ColorSpaceKHR::eSrgbNonlinear, // TODO: find best
       physicalDevice.getSurfaceCapabilitiesKHR(*surface)
           .currentExtent, // TODO: smart get
       1,
       vk::ImageUsageFlagBits::eColorAttachment,
       vk::SharingMode::eExclusive, // TODO: smart set
       static_cast<uint32_t>(queueFamilyIndices.size()),
       queueFamilyIndices.data(),
       physicalDevice.getSurfaceCapabilitiesKHR(*surface).currentTransform,
       vk::CompositeAlphaFlagBitsKHR::eOpaque,
       vk::PresentModeKHR::eMailbox, // TODO: find best
       vk::True,
       nullptr});

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window); // TODO: RAII-wrap
  glfwTerminate();           // TODO: RAII-wrap
  return EXIT_SUCCESS;
}
