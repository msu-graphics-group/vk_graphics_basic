#include "ImGuiRender.h"

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>
#include <etna/GlobalContext.hpp>
#include <etna/RenderTargetStates.hpp>


ImGuiRender::ImGuiRender(vk::Format a_target_format)
{
  CreateDescriptorPool();
  InitImGui(a_target_format);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
}

PFN_vkVoidFunction vulkanLoaderFunction(const char *function_name, void *)
{
  return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(etna::get_context().getInstance(), function_name);
}

void ImGuiRender::CreateDescriptorPool()
{
  using Sz   = vk::DescriptorPoolSize;
  using Type = vk::DescriptorType;

  std::array descrTypes{
    Sz{ Type::eSampler, 1000 },
    Sz{ Type::eCombinedImageSampler, 1000 },
    Sz{ Type::eSampledImage, 1000 },
    Sz{ Type::eStorageImage, 1000 },
    Sz{ Type::eUniformTexelBuffer, 1000 },
    Sz{ Type::eStorageTexelBuffer, 1000 },
    Sz{ Type::eUniformBuffer, 1000 },
    Sz{ Type::eStorageBuffer, 1000 },
    Sz{ Type::eUniformBufferDynamic, 1000 },
    Sz{ Type::eStorageBufferDynamic, 1000 },
    Sz{ Type::eInputAttachment, 100 }
  };

  vk::DescriptorPoolCreateInfo info{
    .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
    .maxSets       = descrTypes.size() * 1000,
    .poolSizeCount = descrTypes.size(),
    .pPoolSizes    = descrTypes.data(),
  };

  m_descriptorPool = etna::unwrap_vk_result(etna::get_context().getDevice().createDescriptorPoolUnique(info));
}

void ImGuiRender::InitImGui(vk::Format a_target_format)
{
  const auto &ctx = etna::get_context();

  std::array targetFormats{ static_cast<VkFormat>(a_target_format) };
  ImGui_ImplVulkan_InitInfo init_info{
    .Instance                    = ctx.getInstance(),
    .PhysicalDevice              = ctx.getPhysicalDevice(),
    .Device                      = ctx.getDevice(),
    .QueueFamily                 = ctx.getQueueFamilyIdx(),
    .Queue                       = ctx.getQueue(),
    .DescriptorPool              = m_descriptorPool.get(),
    // This is basically unused
    .MinImageCount               = 2,
    // This is mis-named
    .ImageCount                  = std::max(static_cast<uint32_t>(ctx.getMainWorkCount().multiBufferingCount()), uint32_t{2}),
    .UseDynamicRendering         = true,
    .PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfoKHR{
      .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount    = targetFormats.size(),
      .pColorAttachmentFormats = targetFormats.data(),
    },
    .CheckVkResultFn = nullptr
  };

  ImGui_ImplVulkan_LoadFunctions(vulkanLoaderFunction);
  ImGui_ImplVulkan_Init(&init_info);

  // Upload GUI fonts texture
  ImGui_ImplVulkan_CreateFontsTexture();
}

void ImGuiRender::NextFrame()
{
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
}

void ImGuiRender::Draw(vk::CommandBuffer a_cmdbuf, vk::Rect2D a_rect, vk::Image a_image, vk::ImageView a_view, ImDrawData *a_imgui_draw_data)
{
  etna::RenderTargetState renderTargets(a_cmdbuf, a_rect, { { .image = a_image, .view = a_view, .loadOp = vk::AttachmentLoadOp::eLoad } }, {});

  ImGui_ImplVulkan_RenderDrawData(a_imgui_draw_data, a_cmdbuf);
}

void ImGuiRender::CleanupImGui()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
}

ImGuiRender::~ImGuiRender()
{
  CleanupImGui();
}
