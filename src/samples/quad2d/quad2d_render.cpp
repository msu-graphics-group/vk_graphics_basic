#include "quad2d_render.h"

#include <geom/vk_mesh.h>
#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <iostream>

#include <etna/GlobalContext.hpp>
#include <etna/Etna.hpp>
#include <etna/RenderTargetStates.hpp>
#include <vulkan/vulkan_core.h>


static std::vector<unsigned> LoadBMP(const char *filename, unsigned *pW, unsigned *pH)
{
  FILE *f = fopen(filename, "rb");

  if (f == nullptr)
  {
    (*pW) = 0;
    (*pH) = 0;
    std::cout << "can't open file" << std::endl;
    return {};
  }

  unsigned char info[54];
  auto readRes = fread(info, sizeof(unsigned char), 54, f);// read the 54-byte header
  if (readRes != 54)
  {
    std::cout << "can't read 54 byte BMP header" << std::endl;
    return {};
  }

  int width  = *(int *)&info[18];
  int height = *(int *)&info[22];

  int row_padded = (width * 3 + 3) & (~3);
  auto data      = new unsigned char[row_padded];

  std::vector<unsigned> res(width * height);

  for (int i = 0; i < height; i++)
  {
    fread(data, sizeof(unsigned char), row_padded, f);
    for (int j = 0; j < width; j++)
      res[i * width + j] = (uint32_t(data[j * 3 + 0]) << 16) | (uint32_t(data[j * 3 + 1]) << 8) | (uint32_t(data[j * 3 + 2]) << 0);
  }

  fclose(f);
  delete[] data;

  (*pW) = unsigned(width);
  (*pH) = unsigned(height);
  return res;
}

void SimpleQuad2dRender::LoadScene(const char* path, bool transpose_inst_matrices)
{
  uint32_t texW, texH;
  auto texData = LoadBMP("../resources/textures/texture1.bmp", &texW, &texH);

  m_imageData = vk_utils::allocateColorTextureFromDataLDR(m_context->getDevice(), m_context->getPhysicalDevice(), (const unsigned char *)texData.data(), 
      texW, texH, 1, VK_FORMAT_R8G8B8A8_UNORM, m_pCopyHelper, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

  m_imageSampler = vk_utils::createSampler(m_context->getDevice(), VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);

  PreparePipelines();
}

void SimpleQuad2dRender::DeallocateResources()
{
  vkDestroyImageView(m_context->getDevice(), m_imageData.view, nullptr);
  vkDestroyImage(m_context->getDevice(), m_imageData.image, nullptr);
  vkFreeMemory(m_context->getDevice(), m_imageData.mem, nullptr);
  vkDestroySampler(m_context->getDevice(), m_imageSampler, nullptr);

  m_swapchain.Cleanup();
  vkDestroySurfaceKHR(GetVkInstance(), m_surface, nullptr);  
}

/// PIPELINES CREATION

void SimpleQuad2dRender::PreparePipelines()
{
  // create full screen quad
  // 
  m_pFSQuad.reset();
  m_pFSQuad = std::make_shared<vk_utils::QuadRenderer>(0,0, 1024, 1024);
  m_pFSQuad->Create(m_context->getDevice(),
    VK_GRAPHICS_BASIC_ROOT "/resources/shaders/quad3_vert.vert.spv",
    VK_GRAPHICS_BASIC_ROOT "/resources/shaders/my_quad.frag.spv",
    vk_utils::RenderTargetInfo2D{
      .size          = VkExtent2D{ m_width, m_height },
      .format        = m_swapchain.GetFormat(),
      .loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD,
      .initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 
    }
  );
  SetupSimplePipeline();
}

void SimpleQuad2dRender::SetupSimplePipeline()
{
  std::vector<std::pair<VkDescriptorType, uint32_t> > dtypes = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     1}
  };

  m_pBindings = std::make_shared<vk_utils::DescriptorMaker>(m_context->getDevice(), dtypes, 1);
  
  m_pBindings->BindBegin(VK_SHADER_STAGE_FRAGMENT_BIT);
  m_pBindings->BindImage(0, m_imageData.view, m_imageSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  m_pBindings->BindEnd(&m_quadDS, &m_quadDSLayout);
}

void SimpleQuad2dRender::DestroyPipelines()
{
  m_pFSQuad     = nullptr; // smartptr delete it's resources
}

/// COMMAND BUFFER FILLING

void SimpleQuad2dRender::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkImage a_targetImage, VkImageView a_targetImageView)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

  float scaleAndOffset[4] = { 0.5f, 0.5f, -0.5f, +0.5f };
  m_pFSQuad->SetRenderTarget(a_targetImageView);
  m_pFSQuad->DrawCmd(a_cmdBuff, m_quadDS, scaleAndOffset);

  etna::set_state(a_cmdBuff, a_targetImage, vk::PipelineStageFlagBits2::eBottomOfPipe,
    vk::AccessFlags2(), vk::ImageLayout::ePresentSrcKHR,
    vk::ImageAspectFlagBits::eColor);

  etna::finish_frame(a_cmdBuff);

  VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}
