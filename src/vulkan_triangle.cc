#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <png.h>
#include <vulkan/vulkan.hpp>

// TODO: RAII-wraps, exceptions, error handling
void savePNG(const char *filename, const int width, const int height,
             const void *buffer) {
  FILE *fp = fopen(filename, "wb");
  png_structp png_ptr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info_ptr = png_create_info_struct(png_ptr);
  png_init_io(png_ptr, fp);
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);
  std::vector<png_bytep> row_pointers(height);
  int stride = width * 4;
  for (int y = 0; y < height; ++y) {
    row_pointers[y] = (png_bytep)buffer + y * stride;
  }
  png_write_image(png_ptr, row_pointers.data());
  png_write_end(png_ptr, NULL);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(fp);
}

static std::vector<char> readBinaryFile(const std::filesystem::path &filepath) {
  std::ifstream file(filepath, std::ios::binary);
  const auto fileSize = std::filesystem::file_size(filepath);
  std::vector<char> buffer(fileSize);
  file.read(buffer.data(), fileSize);
  return buffer;
}

uint32_t findMemoryType(const vk::PhysicalDevice &physicalDevice,
                        const uint32_t &typeFilter,
                        const vk::MemoryPropertyFlags &properties) {
  auto memoryProperties = physicalDevice.getMemoryProperties();

  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memoryProperties.memoryTypes[i].propertyFlags & properties) ==
            properties)
      return i;
  }

  throw std::runtime_error("Failed to find suitable memory type!");
}

int main(int argc, char **argv) {
  const auto WIDTH = static_cast<uint32_t>(2048);
  const auto HEIGHT = WIDTH;
  const auto IMAGE_FORMAT = vk::Format::eR8G8B8A8Unorm;

  // TODO: `NDEBUG` check
  std::vector instanceLayers{"VK_LAYER_KHRONOS_validation"};
  std::vector instanceExtensions{vk::EXTDebugUtilsExtensionName};

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

  auto physicalDevice =
      instance->enumeratePhysicalDevices().front(); // TODO: check is suitable

  auto queuePriorities = 1.0f;
  auto deviceQueueCreateInfos = std::vector{vk::DeviceQueueCreateInfo{
      {},
      0, // TODO: `std::set` of  unique queue families indices
      1,
      &queuePriorities}};
  auto device = physicalDevice.createDeviceUnique(
      {{},
       static_cast<uint32_t>(deviceQueueCreateInfos.size()),
       deviceQueueCreateInfos.data(),
       0,
       nullptr,
       0,
       nullptr,
       nullptr});

  auto colorImage =
      device->createImageUnique({{},
                                 vk::ImageType::e2D,
                                 IMAGE_FORMAT,
                                 {WIDTH, HEIGHT, 1},
                                 1,
                                 1,
                                 vk::SampleCountFlagBits::e1,
                                 vk::ImageTiling::eOptimal,
                                 vk::ImageUsageFlagBits::eColorAttachment |
                                     vk::ImageUsageFlagBits::eTransferSrc,
                                 vk::SharingMode::eExclusive,
                                 {},
                                 vk::ImageLayout::eUndefined});

  auto imageMemoryRequirements =
      device->getImageMemoryRequirements(*colorImage);
  auto memoryAllocateInfo = vk::MemoryAllocateInfo{
      imageMemoryRequirements.size,
      findMemoryType(physicalDevice, imageMemoryRequirements.memoryTypeBits,
                     vk::MemoryPropertyFlagBits::eDeviceLocal)};
  auto colorImageMemory = device->allocateMemoryUnique(memoryAllocateInfo);
  device->bindImageMemory(*colorImage, *colorImageMemory, 0);

  auto colorImageView = device->createImageViewUnique(
      {{},
       *colorImage,
       vk::ImageViewType::e2D,
       IMAGE_FORMAT,
       {},
       {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});

  auto bufferSize = vk::DeviceSize{WIDTH * HEIGHT * 4};
  auto stagingBufferCreateInfo =
      vk::BufferCreateInfo{{},
                           bufferSize,
                           vk::BufferUsageFlagBits::eTransferDst,
                           vk::SharingMode::eExclusive};
  auto stagingBuffer = device->createBufferUnique(stagingBufferCreateInfo);
  auto bufferMemoryRequirements =
      device->getBufferMemoryRequirements(*stagingBuffer);
  memoryAllocateInfo = vk::MemoryAllocateInfo{
      bufferMemoryRequirements.size,
      findMemoryType(physicalDevice, bufferMemoryRequirements.memoryTypeBits,
                     vk::MemoryPropertyFlagBits::eHostVisible |
                         vk::MemoryPropertyFlagBits::eHostCoherent)};
  auto stagingBufferMemory = device->allocateMemoryUnique(memoryAllocateInfo);
  device->bindBufferMemory(*stagingBuffer, *stagingBufferMemory, 0);

  auto colorAttachment =
      vk::AttachmentDescription{{},
                                IMAGE_FORMAT,
                                vk::SampleCountFlagBits::e1,
                                vk::AttachmentLoadOp::eClear,
                                vk::AttachmentStoreOp::eStore,
                                vk::AttachmentLoadOp::eDontCare,
                                vk::AttachmentStoreOp::eDontCare,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eTransferSrcOptimal};
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
  auto viewport = vk::Viewport{
      0.0f, 0.0f, static_cast<float>(WIDTH), static_cast<float>(HEIGHT),
      0.0f, 1.0f};
  auto scissor = vk::Rect2D{{0, 0}, {WIDTH, HEIGHT}};
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

  auto attachments = std::vector{vk::ImageView{*colorImageView}};
  auto framebufferInfo = vk::FramebufferCreateInfo{
      {}, *renderPass, 1, attachments.data(), WIDTH, HEIGHT, 1};
  auto framebuffer = device->createFramebufferUnique(framebufferInfo);

  auto commandPool = device->createCommandPoolUnique(
      {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
       0}); // TODO: smart queue family index
  auto commandBuffer = std::move(device->allocateCommandBuffersUnique(
      {*commandPool, vk::CommandBufferLevel::ePrimary, 1})[0]);

  commandBuffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

  auto clearColor = vk::ClearValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}};
  auto renderPassBeginInfo = vk::RenderPassBeginInfo{
      *renderPass, *framebuffer, {{0, 0}, {WIDTH, HEIGHT}}, 1, &clearColor};
  commandBuffer->beginRenderPass(renderPassBeginInfo,
                                 vk::SubpassContents::eInline);
  commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
  commandBuffer->draw(3, 1, 0, 0);
  commandBuffer->endRenderPass();
  auto copyRegion =
      vk::BufferImageCopy{0,         0,
                          0,         {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                          {0, 0, 0}, {WIDTH, HEIGHT, 1}};
  commandBuffer->copyImageToBuffer(*colorImage,
                                   vk::ImageLayout::eTransferSrcOptimal,
                                   *stagingBuffer, {copyRegion});
  commandBuffer->end();

  auto graphicsQueue = device->getQueue(0, 0);
  auto submitInfo = vk::SubmitInfo{0, nullptr, nullptr, 1, &(*commandBuffer)};
  graphicsQueue.submit({submitInfo}, nullptr);
  device->waitIdle();

  void *mappedMemory =
      device->mapMemory(*stagingBufferMemory, 0, bufferSize, {});

  savePNG("result.png", WIDTH, HEIGHT, mappedMemory);

  device->unmapMemory(*stagingBufferMemory);
  return EXIT_SUCCESS;
}