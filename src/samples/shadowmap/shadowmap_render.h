#ifndef SIMPLE_SHADOWMAP_RENDER_H
#define SIMPLE_SHADOWMAP_RENDER_H

#include "../../render/scene_mgr.h"
#include "../../render/render_common.h"
#include "../../render/quad_renderer.h"
#include "../../../resources/shaders/common.h"
#include "etna/GraphicsPipeline.hpp"
#include <geom/vk_mesh.h>
#include <vk_descriptor_sets.h>
#include <vk_fbuf_attachment.h>
#include <vk_images.h>

#include <etna/GlobalContext.hpp>
#include <etna/Sampler.hpp>


class ImGuiRender;

class SimpleShadowmapRender : public IRender
{
public:
  SimpleShadowmapRender(uint32_t a_width, uint32_t a_height);
  ~SimpleShadowmapRender();

  uint32_t     GetWidth()      const override { return m_width; }
  uint32_t     GetHeight()     const override { return m_height; }
  VkInstance   GetVkInstance() const { return m_context->getInstance(); }

  void InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount);

  void InitPresentation(vk::UniqueSurfaceKHR a_surface, etna::ResolutionProvider a_res_provider);

  void ProcessInput(const AppInput& input) override;
  void UpdateCamera(const Camera* cams, uint32_t a_camsNumber) override;
  Camera GetCurrentCamera() override {return m_cam; }
  void DrawFrame(float a_time, DrawMode a_mode) override;


  void UpdateView();

  void LoadScene(const char *path, bool transpose_inst_matrices);

private:
  etna::GlobalContext* m_context;
  etna::Image mainViewDepth;
  etna::Image shadowMap;
  etna::Sampler defaultSampler;
  etna::Buffer constants;

  std::unique_ptr<etna::Window> window;
  std::unique_ptr<etna::CommandManager> commandManager;

  struct
  {
    float4x4 projView;
    float4x4 model;
  } pushConst2M;

  float4x4 m_worldViewProj;
  float4x4 m_lightMatrix;

  UniformParams m_uniforms {};
  void* m_uboMappedMem = nullptr;

  etna::GraphicsPipeline m_basicForwardPipeline {};
  etna::GraphicsPipeline m_shadowPipeline {};


  Camera   m_cam;
  uint32_t m_width  = 1024u;
  uint32_t m_height = 1024u;

  vk::PhysicalDeviceFeatures m_enabledDeviceFeatures = {};

  std::shared_ptr<SceneManager> m_pScnMgr;
  std::unique_ptr<ImGuiRender> m_pGUIRender;

  std::unique_ptr<QuadRenderer> m_pQuad;

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

  void DrawFrameSimple(bool draw_gui);

  void BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkImage a_targetImage, VkImageView a_targetImageView);

  void DrawSceneCmd(VkCommandBuffer a_cmdBuff, const float4x4& a_wvp, VkPipelineLayout a_pipelineLayout = VK_NULL_HANDLE);

  void loadShaders();

  void SetupSimplePipeline();
  void RecreateSwapChain();

  void UpdateUniformBuffer(float a_time);

  void AllocateResources();
  void PreparePipelines();

  void DrawGui();
};


#endif //CHIMERA_SIMPLE_RENDER_H
