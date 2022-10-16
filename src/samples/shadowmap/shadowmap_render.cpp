#include "shadowmap_render.h"

#include <geom/vk_mesh.h>
#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <iostream>

#include <etna/GlobalContext.hpp>
#include <etna/Etna.hpp>
#include <vulkan/vulkan_core.h>


/// RESOURCE ALLOCATION

void SimpleShadowmapRender::AllocateResources()
{
  mainViewDepth = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{m_width, m_height, 1},
    .format = vk::Format::eD32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment
  });

  shadowMap = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{2048, 2048, 1},
    .format = vk::Format::eD16Unorm,
    .imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled
  });

  defaultSampler = etna::Sampler(etna::Sampler::CreateInfo{});
  constants = m_context->createBuffer(etna::Buffer::CreateInfo
  {
    .size = sizeof(UniformParams),
    .bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY
  });

  m_uboMappedMem = constants.map();
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
  mainViewDepth.reset(); // TODO: Make an etna method to reset all the resources
  shadowMap.reset();

  constants = etna::Buffer();
}





/// PIPELINES CREATION

void SimpleShadowmapRender::PreparePipelines()
{
  // create full screen quad for debug purposes
  // 
  m_pFSQuad = std::make_shared<vk_utils::QuadRenderer>(0,0, 512, 512);
  m_pFSQuad->Create(m_context->getDevice(), 
    VK_GRAPHICS_BASIC_ROOT "/resources/shaders/quad3_vert.vert.spv",
    VK_GRAPHICS_BASIC_ROOT "/resources/shaders/quad.frag.spv", 
    vk_utils::RenderTargetInfo2D{ VkExtent2D{ m_width, m_height }, m_swapchain.GetFormat(),                                        // this is debug full scree quad
                                                  VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }); // seems we need LOAD_OP_LOAD if we want to draw quad to part of screen

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
  etna::create_program("simple_material",
    {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/simple_shadow.frag.spv", VK_GRAPHICS_BASIC_ROOT"/resources/shaders/simple.vert.spv"});
  etna::create_program("simple_shadow", {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/simple.vert.spv"});
}

void SimpleShadowmapRender::SetupSimplePipeline()
{
  std::vector<std::pair<VkDescriptorType, uint32_t> > dtypes = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     2}
  };

  m_pBindings = std::make_shared<vk_utils::DescriptorMaker>(m_context->getDevice(), dtypes, 2);
  
  m_pBindings->BindBegin(VK_SHADER_STAGE_FRAGMENT_BIT);
  m_pBindings->BindImage(0, shadowMap.getView({}), defaultSampler.get(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  m_pBindings->BindEnd(&m_quadDS, &m_quadDSLayout);

  etna::VertexShaderInputDescription sceneVertexInputDesc
    {
      .bindings = {etna::VertexShaderInputDescription::Binding
        {
          .byteStreamDescription = m_pScnMgr->GetVertexStreamDescription()
        }}
    };

  auto& pipelineManager = etna::get_context().getPipelineManager();
  m_basicForwardPipeline = pipelineManager.createGraphicsPipeline("simple_material",
    {
      .vertexShaderInput = sceneVertexInputDesc,
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {static_cast<vk::Format>(m_swapchain.GetFormat())},
          .depthAttachmentFormat = vk::Format::eD32Sfloat
        }
    });
  m_shadowPipeline = pipelineManager.createGraphicsPipeline("simple_shadow",
    {
      .vertexShaderInput = sceneVertexInputDesc,
      .fragmentShaderOutput =
        {
          .depthAttachmentFormat = vk::Format::eD16Unorm
        }
    });

  print_prog_info("simple_material");
  print_prog_info("simple_shadow");
}

void SimpleShadowmapRender::DestroyPipelines()
{
  m_pFSQuad     = nullptr; // smartptr delete it's resources
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
    vkCmdPushConstants(a_cmdBuff, m_basicForwardPipeline.getVkPipelineLayout(),
      stageFlags, 0, sizeof(pushConst2M), &pushConst2M);

    auto mesh_info = m_pScnMgr->GetMeshInfo(inst.mesh_id);
    vkCmdDrawIndexed(a_cmdBuff, mesh_info.m_indNum, 1, mesh_info.m_indexOffset, mesh_info.m_vertexOffset, 0);
  }
}

void SimpleShadowmapRender::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkImage a_targetImage, VkImageView a_targetImageView)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

  {
    std::array barriers
      {
        // Transfer the shadowmap to depth write layout
        VkImageMemoryBarrier2
        {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
          .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
          .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = shadowMap.get(),
          .subresourceRange =
            {
              .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
            }
        },
      };
    VkDependencyInfo depInfo
      {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        .imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data(),
      };
    vkCmdPipelineBarrier2(a_cmdBuff, &depInfo);
  }


  //// draw scene to shadowmap
  //
  {
    vk::Extent2D ext{2048, 2048};
    vk::Viewport viewport
      {
        .x = 0.0f,
        .y = 0.0f,
        .width  = static_cast<float>(ext.width),
        .height = static_cast<float>(ext.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
      };
    vk::Rect2D scissor
      {
        .offset = {0, 0},
        .extent = ext
      };

    VkViewport vp = (VkViewport)viewport;
    VkRect2D scis = (VkRect2D)scissor;
    vkCmdSetViewport(a_cmdBuff, 0, 1, &vp);
    vkCmdSetScissor(a_cmdBuff, 0, 1, &scis);

    vk::RenderingAttachmentInfo shadowMapAttInfo {
      .imageView = shadowMap.getView({}),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearDepthStencilValue{1.0f, 0}
    };
    vk::RenderingInfo renderInfo {
      .renderArea = scissor,
      .layerCount = 1,
      .pDepthAttachment = &shadowMapAttInfo
    };
    VkRenderingInfo rInf = (VkRenderingInfo)renderInfo;
    vkCmdBeginRendering(a_cmdBuff, &rInf);

    {
      vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipeline.getVkPipeline());
      DrawSceneCmd(a_cmdBuff, m_lightMatrix);
    }
    vkCmdEndRendering(a_cmdBuff);
  }

  {
    std::array barriers
      {
        // Transfer the shadowmap from depth write to shader read 
        VkImageMemoryBarrier2
        {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
          .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
          .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
          .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = shadowMap.get(),
          .subresourceRange =
            {
              .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
            }
        },
        // Wait for the semaphore to signal that the swapchain image is available
        VkImageMemoryBarrier2
        {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          // Our semo signals this stage
          .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .srcAccessMask = 0,
          .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = a_targetImage,
          .subresourceRange =
            {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
            }
        },
      };
    VkDependencyInfo depInfo
      {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        .imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data(),
      };
    vkCmdPipelineBarrier2(a_cmdBuff, &depInfo);
  }

  //// draw final scene to screen
  //
  {
    vk::Extent2D ext{m_width, m_height};
    vk::Viewport viewport
      {
        .x = 0.0f,
        .y = 0.0f,
        .width  = static_cast<float>(ext.width),
        .height = static_cast<float>(ext.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
      };
    vk::Rect2D scissor
      {
        .offset = {0, 0},
        .extent = ext
      };

    VkViewport vp = (VkViewport)viewport;
    VkRect2D scis = (VkRect2D)scissor;
    vkCmdSetViewport(a_cmdBuff, 0, 1, &vp);
    vkCmdSetScissor(a_cmdBuff, 0, 1, &scis);

    auto simpleMaterialInfo = etna::get_shader_program("simple_material");

    auto set = etna::create_descriptor_set(simpleMaterialInfo.getDescriptorLayoutId(0), {
      etna::Binding {0, vk::DescriptorBufferInfo {constants.get(), 0, VK_WHOLE_SIZE}},
      etna::Binding {1, vk::DescriptorImageInfo {defaultSampler.get(), shadowMap.getView({}), vk::ImageLayout::eShaderReadOnlyOptimal}}
    });

    VkDescriptorSet vkSet = set.getVkSet();

    vk::RenderingAttachmentInfo depthBufferAttInfo = {
      .imageView = mainViewDepth.getView({}),
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearDepthStencilValue{1.0f, 0}
    };
    vk::RenderingAttachmentInfo swapchainImageAttInfo = {
      .imageView = a_targetImageView,
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearColorValue{std::array<float, 4>({0.0f, 0.0f, 0.0f, 1.0f})}
    };

    vk::RenderingInfo renderInfo {
      .renderArea = scissor,
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &swapchainImageAttInfo,
      .pDepthAttachment = &depthBufferAttInfo
    };
    VkRenderingInfo rInf = (VkRenderingInfo)renderInfo;
    vkCmdBeginRendering(a_cmdBuff, &rInf);

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_basicForwardPipeline.getVkPipeline());
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_basicForwardPipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, VK_NULL_HANDLE);

    DrawSceneCmd(a_cmdBuff, m_worldViewProj);

    vkCmdEndRendering(a_cmdBuff);
  }

  if(m_input.drawFSQuad)
  {
    float scaleAndOffset[4] = {0.5f, 0.5f, -0.5f, +0.5f};
    m_pFSQuad->SetRenderTarget(a_targetImageView);
    m_pFSQuad->DrawCmd(a_cmdBuff, m_quadDS, scaleAndOffset);
  }
  
  {
    std::array barriers
      {
        // Transfer swapchain to present layout
        VkImageMemoryBarrier2
        {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
          .dstAccessMask = 0,
          .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = a_targetImage,
          .subresourceRange =
            {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
            }
        },
      };
    VkDependencyInfo depInfo
      {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        .imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data(),
      };
    vkCmdPipelineBarrier2(a_cmdBuff, &depInfo);
  }

  VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}
