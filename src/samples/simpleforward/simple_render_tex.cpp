#include <vk_pipeline.h>
#include "simple_render_tex.h"
#include "loader_utils/images.h"
#include "imgui/misc/cpp/imgui_stdlib.h"


SimpleRenderTexture::SimpleRenderTexture(uint32_t a_width, uint32_t a_height) : SimpleRender(a_width, a_height)
{
}


void SimpleRenderTexture::LoadScene(const char* path, bool transpose_inst_matrices)
{
  m_pScnMgr->LoadSceneXML(path, transpose_inst_matrices);

  CreateUniformBuffer();
  LoadTexture();
  SetupSimplePipeline();

  auto loadedCam = m_pScnMgr->GetCamera(0);
  m_cam.fov = loadedCam.fov;
  m_cam.pos = float3(loadedCam.pos);
  m_cam.up  = float3(loadedCam.up);
  m_cam.lookAt = float3(loadedCam.lookAt);
  m_cam.tdist  = loadedCam.farPlane;
  UpdateView();

  for (size_t i = 0; i < m_framesInFlight; ++i)
  {
    BuildCommandBufferSimple(m_cmdBuffersDrawMain[i], m_frameBuffers[i],
      m_swapchain.GetAttachment(i).view, m_basicForwardPipeline.pipeline);
  }
}

void SimpleRenderTexture::LoadTexture()
{
  int w, h, channels;
  auto pixels = loadImageLDR(m_texturePath.c_str(), w, h, channels);

  if(pixels == nullptr)
  {
    std::stringstream ss;
    ss << "Failed loading texture from " << m_texturePath;
    vk_utils::logWarning(ss.str());
    return;
  }

  vk_utils::deleteImg(m_device, &m_texture);
  if(m_textureSampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(m_device, m_textureSampler, VK_NULL_HANDLE);
  }

  // in this sample we simply reallocate memory every time
  // in more practical scenario you would try to reuse the same memory
  // or even better utilize some sort of allocator
  if(m_texture.mem != VK_NULL_HANDLE)
  {
    vkFreeMemory(m_device, m_texture.mem, nullptr);
    m_texture.mem = VK_NULL_HANDLE;
  }

  int mipLevels = 1;
  m_texture = allocateColorTextureFromDataLDR(m_device, m_physicalDevice, pixels, w, h, mipLevels,
           VK_FORMAT_R8G8B8A8_UNORM, m_pScnMgr->GetCopyHelper());
  m_textureSampler = vk_utils::createSampler(m_device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK);

  freeImageMemLDR(pixels);

  // after texture is loaded it's in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout
  // we need to change the layout suited for sampling
  auto imgCmdBuf = vk_utils::createCommandBuffer(m_device, m_commandPool);
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(imgCmdBuf, &beginInfo);
  {
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = m_texture.aspectMask;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 1;
    vk_utils::setImageLayout(
      imgCmdBuf,
      m_texture.image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      subresourceRange);
  }
  vkEndCommandBuffer(imgCmdBuf);
  vk_utils::executeCommandBufferNow(imgCmdBuf, m_graphicsQueue, m_device);
}

void SimpleRenderTexture::SetupSimplePipeline()
{
  std::vector<std::pair<VkDescriptorType, uint32_t> > dtypes = {
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1}
  };

  if(m_pBindings == nullptr)
    m_pBindings = std::make_shared<vk_utils::DescriptorMaker>(m_device, dtypes, 1000); // high max sets to allow recreation when texture is updated

  m_pBindings->BindBegin(VK_SHADER_STAGE_FRAGMENT_BIT);
  m_pBindings->BindBuffer(0, m_ubo, VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  m_pBindings->BindImage(1, m_texture.view, m_textureSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  m_pBindings->BindEnd(&m_dSet, &m_dSetLayout);

  // if we are recreating pipeline (for example, to reload shaders)
  // we need to cleanup old pipeline
  if(m_basicForwardPipeline.layout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(m_device, m_basicForwardPipeline.layout, nullptr);
    m_basicForwardPipeline.layout = VK_NULL_HANDLE;
  }
  if(m_basicForwardPipeline.pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(m_device, m_basicForwardPipeline.pipeline, nullptr);
    m_basicForwardPipeline.pipeline = VK_NULL_HANDLE;
  }

  vk_utils::GraphicsPipelineMaker maker;

  std::unordered_map<VkShaderStageFlagBits, std::string> shader_paths;
  shader_paths[VK_SHADER_STAGE_FRAGMENT_BIT] = FRAGMENT_SHADER_PATH + ".spv";
  shader_paths[VK_SHADER_STAGE_VERTEX_BIT]   = VERTEX_SHADER_PATH + ".spv";

  maker.LoadShaders(m_device, shader_paths);

  m_basicForwardPipeline.layout = maker.MakeLayout(m_device, {m_dSetLayout}, sizeof(pushConst2M));
  maker.SetDefaultState(m_width, m_height);

  m_basicForwardPipeline.pipeline = maker.MakePipeline(m_device, m_pScnMgr->GetPipelineVertexInputStateCreateInfo(),
    m_screenRenderPass, {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});
}

void SimpleRenderTexture::DrawFrame(float a_time, DrawMode a_mode)
{
  if(m_textureNeedsReload)
  {
    LoadTexture();
    SetupSimplePipeline();
    m_textureNeedsReload = false;
  }

  UpdateUniformBuffer(a_time);
  switch (a_mode)
  {
  case DrawMode::WITH_GUI:
    SetupGUIElements();
    DrawFrameWithGUI();
    break;
  case DrawMode::NO_GUI:
    DrawFrameSimple();
    break;
  default:
    DrawFrameSimple();
  }
}

void SimpleRenderTexture::ProcessInput(const AppInput &input)
{
  // recreate pipeline to reload shaders
  if(input.keyPressed[GLFW_KEY_B])
  {
#ifdef WIN32
    std::system("cd ../resources/shaders && python compile_simple_render_shaders.py");
#else
    std::system("cd ../resources/shaders && python3 compile_simple_texture_shaders.py");
#endif

    SetupSimplePipeline();

    for (size_t i = 0; i < m_framesInFlight; ++i)
    {
      BuildCommandBufferSimple(m_cmdBuffersDrawMain[i], m_frameBuffers[i],
        m_swapchain.GetAttachment(i).view, m_basicForwardPipeline.pipeline);
    }
  }

}

void SimpleRenderTexture::Cleanup()
{
  vk_utils::deleteImg(m_device, &m_texture);
  if(m_textureSampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(m_device, m_textureSampler, VK_NULL_HANDLE);
  }
  if(m_texture.mem != VK_NULL_HANDLE)
  {
    vkFreeMemory(m_device, m_texture.mem, nullptr);
    m_texture.mem = VK_NULL_HANDLE;
  }
}

void SimpleRenderTexture::SetupGUIElements()
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  {
    ImGui::Begin("Simple render settings");

    ImGui::InputText("Texture path", &m_texturePath);
    if(ImGui::Button("Load texture"))
    {
      m_textureNeedsReload = true;
    }

    ImGui::NewLine();

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::NewLine();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),"Press 'B' to recompile and reload shaders");
    ImGui::Text("Changing bindings is not supported.");
    ImGui::Text("Vertex shader path: %s", VERTEX_SHADER_PATH.c_str());
    ImGui::Text("Fragment shader path: %s", FRAGMENT_SHADER_PATH.c_str());
    ImGui::End();
  }

  // Rendering
  ImGui::Render();
}