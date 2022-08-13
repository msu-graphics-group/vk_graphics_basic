#include "shadowmap_render.h"

#include <geom/vk_mesh.h>
#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <iostream>

#include <etna/Context.hpp>

/// RESOURCE ALLOCATION

void SimpleShadowmapRender::AllocateResources()
{
  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM
  };
  vk_utils::getSupportedDepthFormat(m_physicalDevice, depthFormats, &m_depthBuffer.format);
  m_depthBuffer  = vk_utils::createDepthTexture(m_device, m_physicalDevice, m_width, m_height, m_depthBuffer.format);

  // create shadow map
  //
  m_pShadowMap2 = std::make_shared<vk_utils::RenderTarget>(m_device, VkExtent2D{2048, 2048});

  vk_utils::AttachmentInfo infoDepth;
  infoDepth.format           = VK_FORMAT_D16_UNORM;
  infoDepth.usage            = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  infoDepth.imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
  m_shadowMapId              = m_pShadowMap2->CreateAttachment(infoDepth);
  auto memReq                = m_pShadowMap2->GetMemoryRequirements()[0]; // we know that we have only one texture
  
  // memory for all shadowmaps (well, if you have them more than 1 ...)
  {
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext           = nullptr;
    allocateInfo.allocationSize  = memReq.size;
    allocateInfo.memoryTypeIndex = vk_utils::findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_physicalDevice);

    VK_CHECK_RESULT(vkAllocateMemory(m_device, &allocateInfo, NULL, &m_memShadowMap));
  }

  m_pShadowMap2->CreateViewAndBindMemory(m_memShadowMap, {0});
  m_pShadowMap2->CreateDefaultSampler();

  CreateUniformBuffer();
}

void SimpleShadowmapRender::CreateUniformBuffer()
{
  VkMemoryRequirements memReq;
  m_ubo = vk_utils::createBuffer(m_device, sizeof(UniformParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &memReq);

  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext = nullptr;
  allocateInfo.allocationSize = memReq.size;
  allocateInfo.memoryTypeIndex = vk_utils::findMemoryType(memReq.memoryTypeBits,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                          m_physicalDevice);
  VK_CHECK_RESULT(vkAllocateMemory(m_device, &allocateInfo, nullptr, &m_uboAlloc));
  VK_CHECK_RESULT(vkBindBufferMemory(m_device, m_ubo, m_uboAlloc, 0));

  vkMapMemory(m_device, m_uboAlloc, 0, sizeof(m_uniforms), 0, &m_uboMappedMem);
}

void SimpleShadowmapRender::LoadScene(const char* path, bool transpose_inst_matrices)
{
  m_pScnMgr->LoadSceneXML(path, transpose_inst_matrices);

  // TODO: Make a separate stage
  loadShaders();
  PreparePipelines();

  auto loadedCam = m_pScnMgr->GetCamera(0);
  m_cam.fov = loadedCam.fov;
  m_cam.pos = float3(loadedCam.pos);
  m_cam.up  = float3(loadedCam.up);
  m_cam.lookAt = float3(loadedCam.lookAt);
  m_cam.tdist  = loadedCam.farPlane;
}

void SimpleShadowmapRender::DeallocateResources()
{
  vkDestroyImageView(m_device, m_depthBuffer.view, nullptr);
  vkDestroyImage(m_device, m_depthBuffer.image, nullptr);

  m_pShadowMap2 = nullptr;
  
  if(m_memShadowMap != VK_NULL_HANDLE)
  {
    vkFreeMemory(m_device, m_memShadowMap, VK_NULL_HANDLE);
    m_memShadowMap = VK_NULL_HANDLE;
  }

  vkUnmapMemory(m_device, m_uboAlloc);
  vkFreeMemory(m_device, m_uboAlloc, nullptr);
  vkDestroyBuffer(m_device, m_ubo, nullptr);
}





/// PIPELINES CREATION

void SimpleShadowmapRender::PreparePipelines()
{
  m_screenRenderPass = vk_utils::createDefaultRenderPass(m_device, m_swapchain.GetFormat());

  m_frameBuffers = vk_utils::createFrameBuffers(m_device, m_swapchain, m_screenRenderPass, m_depthBuffer.view);
  
  // create full screen quad for debug purposes
  // 
  m_pFSQuad = std::make_shared<vk_utils::QuadRenderer>(0,0, 512, 512);
  m_pFSQuad->Create(m_device, "../../resources/shaders/quad3_vert.vert.spv", "../../resources/shaders/quad.frag.spv", 
                    vk_utils::RenderTargetInfo2D{ VkExtent2D{ m_width, m_height }, m_swapchain.GetFormat(),                                        // this is debug full scree quad
                                                  VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }); // seems we need LOAD_OP_LOAD if we want to draw quad to part of screen

  m_pShadowMap2->CreateDefaultRenderPass();

  SetupSimplePipeline();
}

static void print_prog_info(const std::string &name)
{
  auto info = etna::get_shader_program(name);
  std::cout << "Program Info " << name << "\n";

  for (uint32_t set = 0u; set < etna::MAX_PROGRAM_DESCRIPTORS; set++)
  {
    if (!info.isDescriptorSetUsed(set))
      continue;
    auto setInfo = info.getDescriptorSetInfo(set);
    for (uint32_t binding = 0; binding < etna::MAX_DESCRIPTOR_BINDINGS; binding++)
    {
      if (!setInfo.isBindingUsed(binding))
        continue;
      auto &vkBinding = setInfo.getBinding(binding);

      std::cout << "Binding " << binding << " " << vk::to_string(vkBinding.descriptorType) << ", count = " << vkBinding.descriptorCount << " ";
      std::cout << " " << vk::to_string(vkBinding.stageFlags) << "\n"; 
    }
  }

  auto pc = info.getPushConst();
  if (pc.size)
  {
    std::cout << "PushConst " << " size = " << pc.size << " stages = " << vk::to_string(pc.stageFlags) << "\n";
  }
}

void SimpleShadowmapRender::loadShaders()
{
  etna::create_program("simple_material", {"../../resources/shaders/simple_shadow.frag.spv", "../../resources/shaders/simple.vert.spv"});
  etna::create_program("simple_shadow", {"../../resources/shaders/simple.vert.spv"});
}

void SimpleShadowmapRender::createDescriptorSets()
{

}

vk::Pipeline SimpleShadowmapRender::createGraphicsPipeline(const std::string &prog_name, uint32_t width, uint32_t height, 
                                                           const VkPipelineVertexInputStateCreateInfo &vinput,
                                                           VkRenderPass renderpass)
{
  std::vector<vk::VertexInputAttributeDescription> vertexAttribures;
  std::vector<vk::VertexInputBindingDescription> vertexBindings;
  
  vertexAttribures.reserve(vinput.vertexAttributeDescriptionCount);
  vertexBindings.reserve(vinput.vertexBindingDescriptionCount);

  for (uint32_t i = 0; i < vinput.vertexAttributeDescriptionCount; i++)
  {
    vertexAttribures.push_back(vinput.pVertexAttributeDescriptions[i]);
  }

  for (uint32_t i = 0; i < vinput.vertexBindingDescriptionCount; i++)
  {
    vertexBindings.push_back(vinput.pVertexBindingDescriptions[i]);
  }

  vk::PipelineVertexInputStateCreateInfo vertexInput {};
  vertexInput.flags = static_cast<vk::PipelineVertexInputStateCreateFlags>(vinput.flags);
  vertexInput.setVertexAttributeDescriptions(vertexAttribures);
  vertexInput.setVertexBindingDescriptions(vertexBindings);

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly {};
  inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);
  inputAssembly.setPrimitiveRestartEnable(false);

  vk::Viewport viewport {};
  viewport.setWidth(float(width));
  viewport.setHeight(float(height));
  viewport.setX(0.f);
  viewport.setY(0.f);
  viewport.setMinDepth(0.f);
  viewport.setMaxDepth(1.f);

  vk::Rect2D scissors {};
  scissors.offset = vk::Offset2D {0, 0};
  scissors.extent = vk::Extent2D {width, height};

  vk::PipelineViewportStateCreateInfo viewportState {};
  viewportState.setViewportCount(1);
  viewportState.setPViewports(&viewport);
  viewportState.setScissorCount(1);
  viewportState.setPScissors(&scissors);

  vk::PipelineRasterizationStateCreateInfo rasterization {};
  rasterization.depthClampEnable = false;
  rasterization.rasterizerDiscardEnable = false;
  rasterization.polygonMode = vk::PolygonMode::eFill;
  rasterization.lineWidth = 1.f;
  rasterization.cullMode = vk::CullModeFlagBits::eNone;
  rasterization.frontFace = vk::FrontFace::eClockwise;
  rasterization.depthBiasEnable = false;

  vk::PipelineMultisampleStateCreateInfo multisampleState {};
  multisampleState.setSampleShadingEnable(false);
  multisampleState.setRasterizationSamples(vk::SampleCountFlagBits::e1);

  std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments;
  blendAttachments.resize(1);
  blendAttachments[0] = vk::PipelineColorBlendAttachmentState {};
  blendAttachments[0].colorWriteMask = vk::ColorComponentFlagBits::eR|vk::ColorComponentFlagBits::eG|vk::ColorComponentFlagBits::eB|vk::ColorComponentFlagBits::eA;
  blendAttachments[0].blendEnable = false;

  vk::PipelineColorBlendStateCreateInfo blendState {};
  blendState.setAttachments(blendAttachments);
  blendState.setLogicOpEnable(false);
  blendState.setLogicOp(vk::LogicOp::eClear);

  vk::PipelineDepthStencilStateCreateInfo depthState {};
  depthState.setDepthTestEnable(true);
  depthState.setDepthWriteEnable(true);
  depthState.setDepthCompareOp(vk::CompareOp::eLessOrEqual);
  depthState.setMaxDepthBounds(1.f);

  auto &m_pShaderPrograms = etna::get_context().getShaderManager();
  auto progId = m_pShaderPrograms.getProgram(prog_name);
  auto stages = m_pShaderPrograms.getShaderStages(progId);

  vk::GraphicsPipelineCreateInfo pipelineInfo {};
  pipelineInfo.setPVertexInputState(&vertexInput);
  pipelineInfo.setPInputAssemblyState(&inputAssembly);
  pipelineInfo.setPViewportState(&viewportState);
  pipelineInfo.setPRasterizationState(&rasterization);
  pipelineInfo.setPMultisampleState(&multisampleState);
  pipelineInfo.setPColorBlendState(&blendState);
  pipelineInfo.setPDepthStencilState(&depthState);
  pipelineInfo.setStages(stages);
  pipelineInfo.setLayout(m_pShaderPrograms.getProgramLayout(progId));
  pipelineInfo.setRenderPass(renderpass);
  
  auto vkdevice = vk::Device {m_device};
  auto res = vkdevice.createGraphicsPipeline(nullptr, pipelineInfo);
  if (res.result != vk::Result::eSuccess)
    ETNA_RUNTIME_ERROR("Pipeline creation error");
  return res.value;
}

void SimpleShadowmapRender::SetupSimplePipeline()
{
  std::vector<std::pair<VkDescriptorType, uint32_t> > dtypes = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     2}
  };

  m_pBindings = std::make_shared<vk_utils::DescriptorMaker>(m_device, dtypes, 2);
  auto shadowMap = m_pShadowMap2->m_attachments[m_shadowMapId];
  
  m_pBindings->BindBegin(VK_SHADER_STAGE_FRAGMENT_BIT);
  m_pBindings->BindImage(0, shadowMap.view, m_pShadowMap2->m_sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  m_pBindings->BindEnd(&m_quadDS, &m_quadDSLayout);

  auto forwardProgInfo = etna::get_shader_program("simple_material");
  auto shadowProgInfo = etna::get_shader_program("simple_shadow");
  
  m_basicForwardPipeline.layout = forwardProgInfo.getPipelineLayout();
  m_basicForwardPipeline.pipeline = createGraphicsPipeline("simple_material", m_width, m_height,
    m_pScnMgr->GetPipelineVertexInputStateCreateInfo(), m_screenRenderPass);
  
  m_shadowPipeline.layout = shadowProgInfo.getPipelineLayout();
  m_shadowPipeline.pipeline = createGraphicsPipeline("simple_shadow", uint32_t(m_pShadowMap2->m_resolution.width), uint32_t(m_pShadowMap2->m_resolution.height),
    m_pScnMgr->GetPipelineVertexInputStateCreateInfo(), m_pShadowMap2->m_renderPass);

  print_prog_info("simple_material");
  print_prog_info("simple_shadow");
}

void SimpleShadowmapRender::DestroyPipelines()
{
  m_pFSQuad     = nullptr; // smartptr delete it's resources

  for (size_t i = 0; i < m_frameBuffers.size(); i++)
  {
    vkDestroyFramebuffer(m_device, m_frameBuffers[i], nullptr);
  }

  vkDestroyRenderPass(m_device, m_screenRenderPass, nullptr);

  if(m_basicForwardPipeline.pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(m_device, m_basicForwardPipeline.pipeline, nullptr);
    m_basicForwardPipeline.pipeline = VK_NULL_HANDLE;
  }

  if(m_shadowPipeline.pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(m_device, m_shadowPipeline.pipeline, nullptr);
    m_shadowPipeline.pipeline = VK_NULL_HANDLE;
  }
}



/// COMMAND BUFFER FILLING

void SimpleShadowmapRender::DrawSceneCmd(VkCommandBuffer a_cmdBuff, const float4x4& a_wvp)
{
  VkShaderStageFlags stageFlags = (VK_SHADER_STAGE_VERTEX_BIT);

  VkDeviceSize zero_offset = 0u;
  VkBuffer vertexBuf = m_pScnMgr->GetVertexBuffer();
  VkBuffer indexBuf  = m_pScnMgr->GetIndexBuffer();
  
  vkCmdBindVertexBuffers(a_cmdBuff, 0, 1, &vertexBuf, &zero_offset);
  vkCmdBindIndexBuffer(a_cmdBuff, indexBuf, 0, VK_INDEX_TYPE_UINT32);

  pushConst2M.projView = a_wvp;
  for (uint32_t i = 0; i < m_pScnMgr->InstancesNum(); ++i)
  {
    auto inst         = m_pScnMgr->GetInstanceInfo(i);
    pushConst2M.model = m_pScnMgr->GetInstanceMatrix(i);
    vkCmdPushConstants(a_cmdBuff, m_basicForwardPipeline.layout, stageFlags, 0, sizeof(pushConst2M), &pushConst2M);

    auto mesh_info = m_pScnMgr->GetMeshInfo(inst.mesh_id);
    vkCmdDrawIndexed(a_cmdBuff, mesh_info.m_indNum, 1, mesh_info.m_indexOffset, mesh_info.m_vertexOffset, 0);
  }
}

void SimpleShadowmapRender::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkFramebuffer a_frameBuff,
                                                     VkImageView a_targetImageView, VkPipeline a_pipeline)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

  VkViewport viewport{};
  VkRect2D scissor{};
  VkExtent2D ext;
  ext.height = m_height;
  ext.width  = m_width;
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width  = static_cast<float>(ext.width);
  viewport.height = static_cast<float>(ext.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  scissor.offset = {0, 0};
  scissor.extent = ext;

  std::vector<VkViewport> viewports = {viewport};
  std::vector<VkRect2D> scissors = {scissor};
  vkCmdSetViewport(a_cmdBuff, 0, 1, viewports.data());
  vkCmdSetScissor(a_cmdBuff, 0, 1, scissors.data());

  //// draw scene to shadowmap
  //
  VkClearValue clearDepth = {};
  clearDepth.depthStencil.depth   = 1.0f;
  clearDepth.depthStencil.stencil = 0;
  std::vector<VkClearValue> clear =  {clearDepth};
  VkRenderPassBeginInfo renderToShadowMap = m_pShadowMap2->GetRenderPassBeginInfo(0, clear);
  vkCmdBeginRenderPass(a_cmdBuff, &renderToShadowMap, VK_SUBPASS_CONTENTS_INLINE);
  {
    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipeline.pipeline);
    DrawSceneCmd(a_cmdBuff, m_lightMatrix);
  }
  vkCmdEndRenderPass(a_cmdBuff);

  //// draw final scene to screen
  //
  {
    auto simpleMaterialInfo = etna::get_shader_program("simple_material");
    auto shadowMap = m_pShadowMap2->m_attachments[m_shadowMapId];

    auto set = etna::create_descriptor_set(simpleMaterialInfo.getDescriptorLayoutId(0), {
      etna::Binding {0, vk::DescriptorBufferInfo {m_ubo, 0, VK_WHOLE_SIZE}},
      etna::Binding {1, vk::DescriptorImageInfo {m_pShadowMap2->m_sampler, shadowMap.view, vk::ImageLayout::eShaderReadOnlyOptimal}}
    });
    
    VkDescriptorSet vkSet = set.getVkSet();

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

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, a_pipeline);
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_basicForwardPipeline.layout, 0, 1, &vkSet, 0, VK_NULL_HANDLE);

    DrawSceneCmd(a_cmdBuff, m_worldViewProj);

    vkCmdEndRenderPass(a_cmdBuff);
  }

  if(m_input.drawFSQuad)
  {
    float scaleAndOffset[4] = {0.5f, 0.5f, -0.5f, +0.5f};
    m_pFSQuad->SetRenderTarget(a_targetImageView);
    m_pFSQuad->DrawCmd(a_cmdBuff, m_quadDS, scaleAndOffset);
  }

  VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}
