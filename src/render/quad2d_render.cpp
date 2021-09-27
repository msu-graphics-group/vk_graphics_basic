#include "quad2d_render.h"
#include "../utils/input_definitions.h"

#include <geom/vk_mesh.h>
#include <vk_pipeline.h>
#include <vk_buffers.h>

Quad2D_Render::Quad2D_Render(uint32_t a_width, uint32_t a_height) : m_width(a_width), m_height(a_height)
{
#ifdef NDEBUG
  m_enableValidation = false;
#else
  m_enableValidation = true;
#endif
}

void Quad2D_Render::SetupDeviceFeatures()
{
  // m_enabledDeviceFeatures.fillModeNonSolid = VK_TRUE;
}

void Quad2D_Render::SetupDeviceExtensions()
{
  m_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Quad2D_Render::SetupValidationLayers()
{
  m_validationLayers.push_back("VK_LAYER_KHRONOS_validation");
  m_validationLayers.push_back("VK_LAYER_LUNARG_monitor");
}

void Quad2D_Render::InitVulkan(std::vector<const char *> a_instanceExtensions, uint32_t a_deviceId)
{
  m_instanceExtensions = std::move(a_instanceExtensions);
  SetupValidationLayers();
  VK_CHECK_RESULT(volkInitialize());
  CreateInstance();
  volkLoadInstance(m_instance);

  CreateDevice(a_deviceId);
  volkLoadDevice(m_device);

  m_commandPool = vk_utils::createCommandPool(m_device, m_queueFamilyIDXs.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  m_cmdBuffersDrawMain.reserve(m_framesInFlight);
  m_cmdBuffersDrawMain = vk_utils::createCommandBuffers(m_device, m_commandPool, m_framesInFlight);

  m_frameFences.resize(m_framesInFlight);
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for (size_t i = 0; i < m_framesInFlight; i++)
  {
    VK_CHECK_RESULT(vkCreateFence(m_device, &fenceInfo, nullptr, &m_frameFences[i]));
  }
  
  m_pCopyHelper = std::make_shared<vk_utils::SimpleCopyHelper>(m_physicalDevice, m_device, m_transferQueue, m_queueFamilyIDXs.graphics, 8*1024*1024);
}

void Quad2D_Render::InitPresentation(VkSurfaceKHR &a_surface)
{
  m_surface = a_surface;

  m_presentationResources.queue = m_swapchain.CreateSwapChain(m_physicalDevice, m_device, m_surface,
                                                              m_width, m_height, m_vsync);
  m_presentationResources.currentFrame = 0;

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_presentationResources.imageAvailable));
  VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_presentationResources.renderingFinished));
  m_screenRenderPass = vk_utils::createDefaultRenderPass(m_device, m_swapchain.GetFormat());

  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM
  };
  vk_utils::getSupportedDepthFormat(m_physicalDevice, depthFormats, &m_depthBuffer.format);
  m_depthBuffer  = vk_utils::createDepthTexture(m_device, m_physicalDevice, m_width, m_height, m_depthBuffer.format);
  m_frameBuffers = vk_utils::createFrameBuffers(m_device, m_swapchain, m_screenRenderPass, m_depthBuffer.view);
  
  // create full screen quad for debug purposes
  //
  //m_pFSQuad = std::make_shared<vk_utils::FSQuad>();
  //m_pFSQuad->Create(m_device, "../resources/shaders/quad_vert.spv", "../resources/shaders/quad_frag.spv", 
  //                  vk_utils::RenderTargetInfo2D{ VkExtent2D{ m_width, m_height }, m_swapchain.GetFormat(),                                        // this is debug full scree quad
  //                                                VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }); // seems we need LOAD_OP_LOAD if we want to draw quad to part of screen
  
  m_pFSQuad = std::make_shared<vk_utils::QuadRenderer>(0,0, 1024, 1024);
  m_pFSQuad->Create(m_device, "../resources/shaders/quad3_vert.vert.spv", "../resources/shaders/my_quad.frag.spv", 
                    vk_utils::RenderTargetInfo2D{ VkExtent2D{ m_width, m_height }, m_swapchain.GetFormat(),                                        // this is debug full scree quad
                                                  VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }); // seems we need LOAD_OP_LOAD if we want to draw quad to part of screen
}

void Quad2D_Render::CreateInstance()
{
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = nullptr;
  appInfo.pApplicationName = "VkRender";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName = "SimpleForward";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

  m_instance = vk_utils::createInstance(m_enableValidation, m_validationLayers, m_instanceExtensions, &appInfo);
  if (m_enableValidation)
    vk_utils::initDebugReportCallback(m_instance, &debugReportCallbackFn, &m_debugReportCallback);
}

void Quad2D_Render::CreateDevice(uint32_t a_deviceId)
{
  SetupDeviceExtensions();
  m_physicalDevice = vk_utils::findPhysicalDevice(m_instance, true, a_deviceId, m_deviceExtensions);

  SetupDeviceFeatures();
  m_device = vk_utils::createLogicalDevice(m_physicalDevice, m_validationLayers, m_deviceExtensions,
                                           m_enabledDeviceFeatures, m_queueFamilyIDXs,
                                           VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);

  vkGetDeviceQueue(m_device, m_queueFamilyIDXs.graphics, 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, m_queueFamilyIDXs.transfer, 0, &m_transferQueue);
}


void Quad2D_Render::SetupSimplePipeline()
{
  std::vector<std::pair<VkDescriptorType, uint32_t> > dtypes = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     1}
  };

  m_pBindings = std::make_shared<vk_utils::DescriptorMaker>(m_device, dtypes, 2);

  m_pBindings->BindBegin(VK_SHADER_STAGE_FRAGMENT_BIT);
  m_pBindings->BindImage(0, m_imageData.view, m_imageSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  m_pBindings->BindEnd(&m_quadDS, &m_quadDSLayout);                      
}

void Quad2D_Render::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkFramebuffer a_frameBuff,
                                             VkImageView a_targetImageView, VkPipeline a_pipeline)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

  //// draw final scene to screen
  //
  {
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_screenRenderPass;
    renderPassInfo.framebuffer = a_frameBuff;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchain.GetExtent();

    VkClearValue clearValues[2] = {};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues    = &clearValues[0];

    vkCmdBeginRenderPass(a_cmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(a_cmdBuff);
  }

  float scaleAndOffset[4] = {0.5f, 0.5f, -0.5f, +0.5f};
  m_pFSQuad->SetRenderTarget(a_targetImageView);
  m_pFSQuad->DrawCmd(a_cmdBuff, m_quadDS, scaleAndOffset);

  VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}


void Quad2D_Render::CleanupPipelineAndSwapchain()
{
  if (!m_cmdBuffersDrawMain.empty())
  {
    vkFreeCommandBuffers(m_device, m_commandPool, static_cast<uint32_t>(m_cmdBuffersDrawMain.size()),
                         m_cmdBuffersDrawMain.data());
    m_cmdBuffersDrawMain.clear();
  }

  for (size_t i = 0; i < m_frameFences.size(); i++)
  {
    vkDestroyFence(m_device, m_frameFences[i], nullptr);
  }

  vkDestroyImageView(m_device, m_depthBuffer.view, nullptr);
  vkDestroyImage(m_device, m_depthBuffer.image, nullptr);

  vkDestroyImageView(m_device, m_imageData.view, nullptr);
  vkDestroyImage(m_device, m_imageData.image, nullptr);
  vkFreeMemory(m_device, m_imageData.mem, nullptr);

  for (size_t i = 0; i < m_frameBuffers.size(); i++)
  {
    vkDestroyFramebuffer(m_device, m_frameBuffers[i], nullptr);
  }

  vkDestroyRenderPass(m_device, m_screenRenderPass, nullptr);

  //m_swapchain.Cleanup();
}

void Quad2D_Render::RecreateSwapChain()
{
  vkDeviceWaitIdle(m_device);

  CleanupPipelineAndSwapchain();
  m_presentationResources.queue = m_swapchain.CreateSwapChain(m_physicalDevice, m_device, m_surface, m_width, m_height,
                                                              m_vsync);
  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM
  };                                                            
  vk_utils::getSupportedDepthFormat(m_physicalDevice, depthFormats, &m_depthBuffer.format);

  m_screenRenderPass = vk_utils::createDefaultRenderPass(m_device, m_swapchain.GetFormat());
  m_depthBuffer      = vk_utils::createDepthTexture(m_device, m_physicalDevice, m_width, m_height, m_depthBuffer.format);
  m_frameBuffers     = vk_utils::createFrameBuffers(m_device, m_swapchain, m_screenRenderPass, m_depthBuffer.view);

  m_frameFences.resize(m_framesInFlight);
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for (size_t i = 0; i < m_framesInFlight; i++)
  {
    VK_CHECK_RESULT(vkCreateFence(m_device, &fenceInfo, nullptr, &m_frameFences[i]));
  }

  m_cmdBuffersDrawMain = vk_utils::createCommandBuffers(m_device, m_commandPool, m_framesInFlight);
  for (size_t i = 0; i < m_swapchain.GetImageCount(); ++i)
  {
    BuildCommandBufferSimple(m_cmdBuffersDrawMain[i], m_frameBuffers[i], m_swapchain.GetAttachment(i).view, nullptr);
  }
}

void Quad2D_Render::Cleanup()
{
  m_pFSQuad     = nullptr; // smartptr delete it's resources
  CleanupPipelineAndSwapchain();


  if (m_presentationResources.imageAvailable != VK_NULL_HANDLE)
    vkDestroySemaphore(m_device, m_presentationResources.imageAvailable, nullptr);
  
  if (m_presentationResources.renderingFinished != VK_NULL_HANDLE)
    vkDestroySemaphore(m_device, m_presentationResources.renderingFinished, nullptr);

  if (m_commandPool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
  }
}

void Quad2D_Render::ProcessInput(const AppInput &input)
{
  
}

void Quad2D_Render::UpdateCamera(const Camera* cams, uint32_t a_camsNumber)
{

}


static std::vector<unsigned> LoadBMP(const char* filename, unsigned* pW, unsigned* pH)
{
  FILE* f = fopen(filename, "rb");

  if(f == NULL)
  {
    (*pW) = 0;
    (*pH) = 0;
    std::cout << "can't open file" << std::endl;
    return std::vector<unsigned>();
  }

  unsigned char info[54];
  auto readRes = fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header
  if(readRes != 54)
  {
    std::cout << "can't read 54 byte BMP header" << std::endl;
    return std::vector<unsigned>();
  }

  int width  = *(int*)&info[18];
  int height = *(int*)&info[22];

  int row_padded      = (width*3 + 3) & (~3);
  unsigned char* data = new unsigned char[row_padded];

  std::vector<unsigned> res(width*height);

  for(int i = 0; i < height; i++)
  {
    fread(data, sizeof(unsigned char), row_padded, f);
    for(int j = 0; j < width; j++)
      res[i*width+j] = (uint32_t(data[j*3+0]) << 16) | (uint32_t(data[j*3+1]) << 8)  | (uint32_t(data[j*3+2]) << 0);
  }

  fclose(f);
  delete [] data;

  (*pW) = unsigned(width);
  (*pH) = unsigned(height);
  return res;
}

void Quad2D_Render::LoadScene(const std::string &path, bool transpose_inst_matrices)
{
  uint32_t texW, texH;
  auto texData = LoadBMP("../resources/textures/texture1.bmp", &texW, &texH);
  
  m_imageData    = vk_utils::allocateColorTextureFromDataLDR(m_device, m_physicalDevice, (const unsigned char*)texData.data(), texW, texH, 1, VK_FORMAT_R8G8B8A8_UNORM,
                                                             m_pCopyHelper, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  m_imageSampler = vk_utils::createSampler(m_device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);
  
  // transfer our texture layout from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
  //
  VkCommandBuffer commandBuffer = vk_utils::createCommandBuffer(m_device, m_commandPool);
    
  VkCommandBufferBeginInfo beginCommandBufferInfo = {};
  beginCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginCommandBufferInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  vkBeginCommandBuffer(commandBuffer, &beginCommandBufferInfo);
  {
    VkImageMemoryBarrier imgBar = {};
    {
      imgBar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imgBar.pNext = nullptr;
      imgBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imgBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  
      imgBar.srcAccessMask = 0;
      imgBar.dstAccessMask = 0;
      imgBar.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      imgBar.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imgBar.image         = m_imageData.image;
  
      imgBar.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      imgBar.subresourceRange.baseMipLevel   = 0;
      imgBar.subresourceRange.levelCount     = 1;
      imgBar.subresourceRange.baseArrayLayer = 0;
      imgBar.subresourceRange.layerCount     = 1;
    };
  
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &imgBar);
  }

  vkEndCommandBuffer(commandBuffer);  
  vk_utils::executeCommandBufferNow(commandBuffer, m_transferQueue, m_device);

  SetupSimplePipeline();

  for (size_t i = 0; i < m_framesInFlight; ++i)
    BuildCommandBufferSimple(m_cmdBuffersDrawMain[i], m_frameBuffers[i], m_swapchain.GetAttachment(i).view, nullptr);
}

void Quad2D_Render::DrawFrameSimple()
{
  vkWaitForFences(m_device, 1, &m_frameFences[m_presentationResources.currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(m_device, 1, &m_frameFences[m_presentationResources.currentFrame]);

  uint32_t imageIdx;
  m_swapchain.AcquireNextImage(m_presentationResources.imageAvailable, &imageIdx);

  auto currentCmdBuf = m_cmdBuffersDrawMain[imageIdx];

  VkSemaphore waitSemaphores[] = {m_presentationResources.imageAvailable};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  BuildCommandBufferSimple(currentCmdBuf, m_frameBuffers[imageIdx], m_swapchain.GetAttachment(imageIdx).view, nullptr);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &currentCmdBuf;

  VkSemaphore signalSemaphores[] = {m_presentationResources.renderingFinished};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VK_CHECK_RESULT(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_frameFences[m_presentationResources.currentFrame]));

  VkResult presentRes = m_swapchain.QueuePresent(m_presentationResources.queue, imageIdx,
                                                 m_presentationResources.renderingFinished);

  if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR)
  {
    RecreateSwapChain();
  }
  else if (presentRes != VK_SUCCESS)
  {
    RUN_TIME_ERROR("Failed to present swapchain image");
  }

  m_presentationResources.currentFrame = (m_presentationResources.currentFrame + 1) % m_framesInFlight;

  vkQueueWaitIdle(m_presentationResources.queue);
}

void Quad2D_Render::DrawFrame(float a_time)
{
  DrawFrameSimple();
}
