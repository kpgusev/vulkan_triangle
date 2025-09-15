#include <cstdlib>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <toml.hpp>
#include <vulkan/vulkan.hpp>

static std::vector<char> readBinaryFile(const std::filesystem::path &filepath) {
  std::ifstream file(filepath, std::ios::binary);
  const auto fileSize = std::filesystem::file_size(filepath);
  std::vector<char> buffer(fileSize);
  file.read(buffer.data(), fileSize);
  return buffer;
}

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

  auto surfaceFormat = vk::SurfaceFormatKHR{// TODO: find best
                                            vk::Format::eB8G8R8A8Unorm,
                                            vk::ColorSpaceKHR::eSrgbNonlinear};
  auto queueFamilyIndices = std::vector<uint32_t>{0};
  auto swapchain = device->createSwapchainKHRUnique(
      {{},
       *surface,
       4, // TODO: smart set
       surfaceFormat.format,
       surfaceFormat.colorSpace,
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

  auto swapchainImages = device->getSwapchainImagesKHR(*swapchain);
  auto swapchainImageViews =
      std::vector<vk::UniqueImageView>(swapchainImages.size());
  for (size_t i = 0; i < swapchainImages.size(); ++i) {
    swapchainImageViews[i] = device->createImageViewUnique(
        {{},
         swapchainImages[i],
         vk::ImageViewType::e2D,
         surfaceFormat.format,
         {},
         {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
  }

  auto colorAttachment =
      vk::AttachmentDescription{{},
                                surfaceFormat.format,
                                vk::SampleCountFlagBits::e1,
                                vk::AttachmentLoadOp::eClear,
                                vk::AttachmentStoreOp::eStore,
                                vk::AttachmentLoadOp::eDontCare,
                                vk::AttachmentStoreOp::eDontCare,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::ePresentSrcKHR};
  auto colorAttachmentReference =
      vk::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal};

  auto subpass =
      vk::SubpassDescription{{}, vk::PipelineBindPoint::eGraphics, 0, nullptr,
                             1,  &colorAttachmentReference};
  auto renderPass =
      device->createRenderPassUnique({{}, 1, &colorAttachment, 1, &subpass});

  auto vertexShaderCode = readBinaryFile("shaders/main.vert.spv");
  auto vertexShaderModule = device->createShaderModuleUnique(
      {{},
       vertexShaderCode.size(),
       reinterpret_cast<const uint32_t *>(vertexShaderCode.data())});

  auto fragmentShaderCode = readBinaryFile("shaders/main.frag.spv");
  auto fragmentShaderModule = device->createShaderModuleUnique(
      {{},
       fragmentShaderCode.size(),
       reinterpret_cast<const uint32_t *>(fragmentShaderCode.data())});

  auto shaderStages = std::array{
      vk::PipelineShaderStageCreateInfo{
          {}, vk::ShaderStageFlagBits::eVertex, *vertexShaderModule, "main"},
      vk::PipelineShaderStageCreateInfo{{},
                                        vk::ShaderStageFlagBits::eFragment,
                                        *fragmentShaderModule,
                                        "main"}};

  auto pipelineVertexInputStateCreateInfo =
      vk::PipelineVertexInputStateCreateInfo{};
  auto pipelineInputAssemblyStateCreateInfo =
      vk::PipelineInputAssemblyStateCreateInfo{
          {}, vk::PrimitiveTopology::eTriangleList, vk::False};
  auto viewport = vk::Viewport{
      0.0f,
      0.0f,
      static_cast<float>(physicalDevice.getSurfaceCapabilitiesKHR(*surface)
                             .currentExtent.width),
      static_cast<float>(physicalDevice.getSurfaceCapabilitiesKHR(*surface)
                             .currentExtent.height),
      0.0f,
      1.0f};
  auto scissor = vk::Rect2D{
      {0, 0}, physicalDevice.getSurfaceCapabilitiesKHR(*surface).currentExtent};
  auto pipelineViewportStateCreateInfo =
      vk::PipelineViewportStateCreateInfo{{}, 1, &viewport, 1, &scissor};
  auto rasterizer =
      vk::PipelineRasterizationStateCreateInfo{{},
                                               vk::False,
                                               vk::False,
                                               vk::PolygonMode::eFill,
                                               vk::CullModeFlagBits::eBack,
                                               vk::FrontFace::eClockwise,
                                               vk::False,
                                               {},
                                               {},
                                               {},
                                               1.0f};
  auto multisampling = vk::PipelineMultisampleStateCreateInfo{
      {}, vk::SampleCountFlagBits::e1, vk::False};
  auto pipelineColorBlendAttachmentState =
      vk::PipelineColorBlendAttachmentState{
          vk::False,
          {},
          {},
          {},
          {},
          {},
          {},
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
  auto pipelineColorBlendStateCreateInfo =
      vk::PipelineColorBlendStateCreateInfo{{},
                                            vk::False,
                                            vk::LogicOp::eCopy,
                                            1,
                                            &pipelineColorBlendAttachmentState};
  auto pipelineLayout = device->createPipelineLayoutUnique({});
  auto pipeline =
    device->createGraphicsPipelineUnique(nullptr, {{},
                                                   shaderStages,
                                                   &pipelineVertexInputStateCreateInfo,
                                                   &pipelineInputAssemblyStateCreateInfo,
                                                   nullptr,
                                                   &pipelineViewportStateCreateInfo,
                                                   &rasterizer,
                                                   &multisampling,
                                                   nullptr,
                                                   &pipelineColorBlendStateCreateInfo,
                                                   nullptr,
                                                   *pipelineLayout,
                                                   *renderPass,
                                                   0}).value;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window); // TODO: RAII-wrap
  glfwTerminate();           // TODO: RAII-wrap
  return EXIT_SUCCESS;
}
