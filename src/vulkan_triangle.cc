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
  // TODO: try-catch
  glfwSetErrorCallback([](int, const char *description) {
    throw std::runtime_error(description);
  });

  if (glfwInit() == GLFW_FALSE) // TODO: RAII-wrap
    throw std::runtime_error("Failed to initialize GLFW");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  auto window =
      glfwCreateWindow(512, 512, "", nullptr, nullptr); // TODO: RAII-wrap

  // TODO: `NDEBUG` check
  std::vector instanceLayers{"VK_LAYER_KHRONOS_validation"};
  std::vector instanceExtensions{vk::EXTDebugUtilsExtensionName};

  uint32_t instanceExtensionCount = 0;
  const char **glfwRequiredInstanceExtensions =
      glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
  instanceExtensions.insert(
      instanceExtensions.end(), glfwRequiredInstanceExtensions,
      glfwRequiredInstanceExtensions + instanceExtensionCount);

  // TODO: verify required extensions and layers
  // (`vk::enumerateInstanceExtensionProperties` and
  // `vk::enumerateInstanceLayerProperties`)

  auto applicationInfo =
      vk::ApplicationInfo{"", 0, "", 0, vk::enumerateInstanceVersion()};

  auto instance = vk::createInstanceUnique(
      {{},
       &applicationInfo,
       static_cast<uint32_t>(instanceLayers.size()),
       instanceLayers.data(),
       static_cast<uint32_t>(instanceExtensions.size()),
       instanceExtensions.data()});

  VkSurfaceKHR rawSurface;
  glfwCreateWindowSurface(*instance, window, nullptr,
                          &rawSurface); // TODO: check `vk::Result::eSuccess`
  auto surface = vk::UniqueSurfaceKHR{
      rawSurface,
      vk::detail::ObjectDestroy<vk::Instance, vk::detail::DispatchLoaderStatic>{
          *instance}};

  auto physicalDevice =
      instance->enumeratePhysicalDevices().front(); // TODO: check is suitable

  auto queuePriorities = 1.0f;
  auto deviceQueueCreateInfos = std::vector{vk::DeviceQueueCreateInfo{
      {},
      0, // TODO: `std::set` of  unique queue families indices
      1,
      &queuePriorities}};
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
  auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
  auto queueFamilyIndices = std::vector<uint32_t>{0}; // TODO: smart find
  auto swapchain = device->createSwapchainKHRUnique(
      {{},
       *surface,
       4, // TODO: smart set
       surfaceFormat.format,
       surfaceFormat.colorSpace,
       surfaceCapabilities.currentExtent,
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

  // TODO: per-module load
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
  auto viewport =
      vk::Viewport{0.0f,
                   0.0f,
                   static_cast<float>(surfaceCapabilities.currentExtent.width),
                   static_cast<float>(surfaceCapabilities.currentExtent.height),
                   0.0f,
                   1.0f};
  auto scissor = vk::Rect2D{{0, 0}, surfaceCapabilities.currentExtent};
  auto pipelineViewportStateCreateInfo =
      vk::PipelineViewportStateCreateInfo{{}, 1, &viewport, 1, &scissor};
  auto rasterizer = // TODO: config
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
  auto pipeline = device
                      ->createGraphicsPipelineUnique(
                          nullptr, {{},
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
                                    0})
                      .value;

  auto framebuffers =
      std::vector<vk::UniqueFramebuffer>(swapchainImageViews.size());
  for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
    vk::ImageView attachments[] = {*swapchainImageViews[i]};
    auto framebufferInfo =
        vk::FramebufferCreateInfo{{},
                                  *renderPass,
                                  1,
                                  attachments,
                                  surfaceCapabilities.currentExtent.width,
                                  surfaceCapabilities.currentExtent.height,
                                  1};
    framebuffers[i] = device->createFramebufferUnique(framebufferInfo);
  }

  auto commandPool = device->createCommandPoolUnique(
      {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
       0}); // TODO: smart queue family index
  auto commandBuffers = device->allocateCommandBuffersUnique(
      {*commandPool, vk::CommandBufferLevel::ePrimary,
       static_cast<uint32_t>(framebuffers.size())});

  for (size_t i = 0; i < commandBuffers.size(); ++i) {
    auto &commandBuffer = commandBuffers[i];
    commandBuffer->begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});
    vk::ClearValue clearColor{std::array{0.0f, 0.0f, 0.0f, 1.0f}};
    auto renderPassBeginInfo =
        vk::RenderPassBeginInfo{*renderPass,
                                *framebuffers[i],
                                {{0, 0}, surfaceCapabilities.currentExtent},
                                1,
                                &clearColor};
    commandBuffer->beginRenderPass(renderPassBeginInfo,
                                   vk::SubpassContents::eInline);
    commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    commandBuffer->draw(3, 1, 0, 0);
    commandBuffer->endRenderPass();
    commandBuffer->end();
  }

  auto MAX_FRAMES_IN_FLIGHT = 2;

  auto imageAvailableSemaphores =
      std::vector<vk::UniqueSemaphore>(MAX_FRAMES_IN_FLIGHT);
  auto inFlightFences = std::vector<vk::UniqueFence>(MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    imageAvailableSemaphores[i] = device->createSemaphoreUnique({});
    inFlightFences[i] =
        device->createFenceUnique({vk::FenceCreateFlagBits::eSignaled});
  }

  auto renderFinishedSemaphores =
      std::vector<vk::UniqueSemaphore>(swapchainImages.size());
  for (size_t i = 0; i < swapchainImages.size(); ++i) {
    renderFinishedSemaphores[i] = device->createSemaphoreUnique({});
  }

  auto imagesInFlight = std::vector<vk::Fence>(swapchainImages.size(), nullptr);

  auto graphicsQueue = device->getQueue(0, 0);
  auto presentQueue = device->getQueue(0, 0);

  auto currentFrame = static_cast<size_t>(0);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    device->waitForFences({*inFlightFences[currentFrame]}, vk::True,
                          std::numeric_limits<uint64_t>::max()); // TODO: add result variable

    uint32_t imageIndex =
        device
            ->acquireNextImageKHR(
                *swapchain, std::numeric_limits<uint64_t>::max(),
                *imageAvailableSemaphores[currentFrame], nullptr)
            .value;

    if (imagesInFlight[imageIndex] != nullptr) {
      device->waitForFences({imagesInFlight[imageIndex]}, vk::True,
                            std::numeric_limits<uint64_t>::max()); // TODO: add result variable
    }
    imagesInFlight[imageIndex] = *inFlightFences[currentFrame];

    device->resetFences({*inFlightFences[currentFrame]});

    vk::Semaphore waitSemaphores[] = {*imageAvailableSemaphores[currentFrame]};
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput};

    auto submitInfo = vk::SubmitInfo{1,
                                     waitSemaphores,
                                     waitStages,
                                     1,
                                     &(*commandBuffers[imageIndex]),
                                     1,
                                     &(*renderFinishedSemaphores[imageIndex])};
    graphicsQueue.submit({submitInfo}, *inFlightFences[currentFrame]);

    vk::SwapchainKHR swapchains[] = {*swapchain};
    presentQueue.presentKHR({1, &(*renderFinishedSemaphores[imageIndex]), 1,
                             swapchains, &imageIndex}); // TODO: add result variable

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  device->waitIdle();

  glfwDestroyWindow(window); // TODO: RAII-wrap
  glfwTerminate();           // TODO: RAII-wrap
  return EXIT_SUCCESS;
}
