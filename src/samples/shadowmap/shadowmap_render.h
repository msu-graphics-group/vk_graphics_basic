#ifndef SIMPLE_SHADOWMAP_RENDER_H
#define SIMPLE_SHADOWMAP_RENDER_H

#include "../../render/scene_mgr.h"
#include "../../render/render_common.h"
#include "../../../resources/shaders/common.h"
#include <geom/vk_mesh.h>
#include <vk_descriptor_sets.h>
#include <vk_fbuf_attachment.h>
#include <vk_images.h>
#include <vk_swapchain.h>
#include <vk_quad.h>

#include <string>
#include <iostream>

#include <etna/GlobalContext.hpp>


class SimpleShadowmapRender : public IRender
{
public:
  SimpleShadowmapRender(uint32_t a_width, uint32_t a_height);
  ~SimpleShadowmapRender();

  inline uint32_t     GetWidth()      const override { return m_width; }
  inline uint32_t     GetHeight()     const override { return m_height; }
  VkInstance   GetVkInstance() const override { return m_context->getInstance(); }

  void InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId) override;

  void InitPresentation(VkSurfaceKHR &a_surface, bool initGUI) override;

  void ProcessInput(const AppInput& input) override;
  void UpdateCamera(const Camera* cams, uint32_t a_camsNumber) override;
  Camera GetCurrentCamera() override {return m_cam;}
  void UpdateView();

  void LoadScene(const char *path, bool transpose_inst_matrices) override;
  void DrawFrame(float a_time, DrawMode a_mode) override;

private:
  etna::GlobalContext* m_context;
  etna::Image mainViewDepth;

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

  UniformParams m_uniforms {};
  VkBuffer m_ubo = VK_NULL_HANDLE;
  VkDeviceMemory m_uboAlloc = VK_NULL_HANDLE;
  void* m_uboMappedMem = nullptr;

  pipeline_data_t m_basicForwardPipeline {};
  pipeline_data_t m_shadowPipeline {};

  VkRenderPass m_screenRenderPass = VK_NULL_HANDLE; // main renderpass

  std::shared_ptr<vk_utils::DescriptorMaker> m_pBindings = nullptr;
  
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VulkanSwapChain m_swapchain;
  std::vector<VkFramebuffer> m_frameBuffers;

  Camera   m_cam;
  uint32_t m_width  = 1024u;
  uint32_t m_height = 1024u;
  uint32_t m_framesInFlight = 2u;
  bool m_vsync = false;

  vk::PhysicalDeviceFeatures m_enabledDeviceFeatures = {};
  std::vector<const char*> m_deviceExtensions;
  std::vector<const char*> m_instanceExtensions;

  std::shared_ptr<SceneManager>     m_pScnMgr;
  
  // objects and data for shadow map
  //
  std::shared_ptr<vk_utils::IQuad>               m_pFSQuad;
  //std::shared_ptr<vk_utils::RenderableTexture2D> m_pShadowMap;
  std::shared_ptr<vk_utils::RenderTarget>        m_pShadowMap2;
  uint32_t                                       m_shadowMapId = 0;
  
  VkDeviceMemory        m_memShadowMap = VK_NULL_HANDLE;
  VkDescriptorSet       m_quadDS; 
  VkDescriptorSetLayout m_quadDSLayout = nullptr;

  struct InputControlMouseEtc
  {
    bool drawFSQuad = false;
  } m_input;

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

  void BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkFramebuffer a_frameBuff,
                                VkImageView a_targetImageView, VkPipeline a_pipeline);

  void DrawSceneCmd(VkCommandBuffer a_cmdBuff, const float4x4& a_wvp);

  void loadShaders();
  void createDescriptorSets();

  /*todo: move pipeline creation to etna*/
  vk::Pipeline createGraphicsPipeline(const std::string &prog_name, uint32_t width, uint32_t height,
    const VkPipelineVertexInputStateCreateInfo &vinput, VkRenderPass *renderpass = nullptr);

  void SetupSimplePipeline();
  void RecreateSwapChain();

  void CreateUniformBuffer();
  void UpdateUniformBuffer(float a_time);


  void SetupDeviceExtensions();

  void AllocateResources();
  void PreparePipelines();

  void DestroyPipelines();
  void DeallocateResources();

  void InitPresentStuff();
  void ResetPresentStuff();
};


#endif //CHIMERA_SIMPLE_RENDER_H
