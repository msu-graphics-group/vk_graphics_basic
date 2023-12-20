#ifndef SIMPLE_QUAD2D_RENDER_H
#define SIMPLE_QUAD2D_RENDER_H

#include "../../render/scene_mgr.h"
#include "../../render/render_common.h"
#include "../../../resources/shaders/common.h"
#include "etna/GraphicsPipeline.hpp"
#include <geom/vk_mesh.h>
#include <vk_descriptor_sets.h>
#include <vk_fbuf_attachment.h>
#include <vk_images.h>
#include <vk_swapchain.h>
#include <vk_quad.h>

#include <string>
#include <iostream>

#include <etna/GlobalContext.hpp>
#include <etna/Sampler.hpp>


class IRenderGUI;

class SimpleQuad2dRender : public IRender
{
public:
  SimpleQuad2dRender(uint32_t a_width, uint32_t a_height);
  ~SimpleQuad2dRender();

  uint32_t     GetWidth()      const override { return m_width; }
  uint32_t     GetHeight()     const override { return m_height; }
  VkInstance   GetVkInstance() const override { return m_context->getInstance(); }

  void InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId) override;

  void InitPresentation(VkSurfaceKHR &a_surface, bool initGUI) override;

  void ProcessInput(const AppInput& input) override;
  void UpdateCamera(const Camera* cams, uint32_t a_camsNumber) override;
  Camera GetCurrentCamera() override {return m_cam;}

  void LoadScene(const char *path, bool transpose_inst_matrices) override;
  void DrawFrame(float a_time, DrawMode a_mode) override;

private:
  etna::GlobalContext* m_context;
  etna::Sampler defaultSampler;

  VkCommandPool    m_commandPool    = VK_NULL_HANDLE;

  struct
  {
    uint32_t    currentFrame      = 0u;
    VkQueue     queue             = VK_NULL_HANDLE;
    VkSemaphore imageAvailable    = VK_NULL_HANDLE;
    VkSemaphore renderingFinished = VK_NULL_HANDLE;
  } m_presentationResources;

  std::vector<VkFence> m_frameFences;
  std::vector<VkCommandBuffer> m_cmdBuffersDrawMain;

  struct
  {
    float4x4 projView;
    float4x4 model;
  } pushConst2M;

  float4x4 m_worldViewProj;
  float4x4 m_lightMatrix;    

  etna::GraphicsPipeline m_basicForwardPipeline {};
  etna::GraphicsPipeline m_shadowPipeline {};

  std::shared_ptr<vk_utils::DescriptorMaker> m_pBindings = nullptr;
  
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VulkanSwapChain m_swapchain;

  Camera   m_cam;
  uint32_t m_width  = 1024u;
  uint32_t m_height = 1024u;
  uint32_t m_framesInFlight = 2u;
  bool m_vsync = false;

  vk::PhysicalDeviceFeatures m_enabledDeviceFeatures = {};
  std::vector<const char*> m_deviceExtensions;
  std::vector<const char*> m_instanceExtensions;

  std::shared_ptr<vk_utils::ICopyEngine> m_pCopyHelper;
  std::shared_ptr<vk_utils::IQuad>               m_pFSQuad;
  VkDescriptorSet       m_quadDS; 
  VkDescriptorSetLayout m_quadDSLayout = nullptr;

  vk_utils::VulkanImageMem m_imageData;
  VkSampler m_imageSampler;
  /**
  \brief basic parameters that you usually need for shadow mapping
  */
  struct ShadowMapCam
  {
    ShadowMapCam() 
    {  
      cam.pos    = float3(4.0f, 4.0f, 4.0f);
      cam.lookAt = float3(0, 0, 0);
      cam.up     = float3(0, 1, 0);
  
      radius          = 5.0f;
      lightTargetDist = 20.0f;
      usePerspectiveM = true;
    }

    float  radius;           ///!< ignored when usePerspectiveM == true 
    float  lightTargetDist;  ///!< identify depth range
    Camera cam;              ///!< user control for light to later get light worldViewProj matrix
    bool   usePerspectiveM;  ///!< use perspective matrix if true and ortographics otherwise
  
  } m_light;

 
  void DrawFrameSimple();

  void CreateInstance();
  void CreateDevice(uint32_t a_deviceId);

  void BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkImage a_targetImage, VkImageView a_targetImageView);

  void SetupSimplePipeline();
  void RecreateSwapChain();


  void SetupDeviceExtensions();

  void PreparePipelines();

  void DestroyPipelines();
  void DeallocateResources();

  void InitPresentStuff();
  void ResetPresentStuff();
};


#endif //CHIMERA_SIMPLE_RENDER_H
