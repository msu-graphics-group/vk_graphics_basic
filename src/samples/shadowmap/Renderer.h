#pragma once

#include "../../render/scene_mgr.h"
#include "../../render/quad_renderer.h"
#include "../../../resources/shaders/common.h"
#include <geom/vk_mesh.h>
#include "utils/Camera.h"
#include "wsi/Keyboard.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/Sampler.hpp>


class ImGuiRender;

class Renderer
{
public:
  Renderer(glm::uvec2 resolution);
  ~Renderer();

  void initVulkan(std::span<const char*> instance_extensions);
  void initPresentation(vk::UniqueSurfaceKHR surface, etna::ResolutionProvider res_provider);

  void debugInput(const Keyboard& kb);

  void updateView(const Camera &main, const Camera &shadow);

  void drawFrame(float dt);

  void loadScene(const char *path, bool transpose_inst_matrices);

private:
  etna::GlobalContext* m_context;
  etna::Image mainViewDepth;
  etna::Image shadowMap;
  etna::Sampler defaultSampler;
  etna::Buffer constants;

  std::unique_ptr<etna::Window> window;
  std::unique_ptr<etna::PerFrameCmdMgr> commandManager;

  struct
  {
    glm::mat4x4 projView;
    glm::mat4x4 model;
  } pushConst2M;

  glm::mat4x4 m_worldViewProj;
  glm::mat4x4 m_lightMatrix;
  glm::vec3 lightPos;

  UniformParams m_uniforms {};
  void* m_uboMappedMem = nullptr;

  etna::GraphicsPipeline m_basicForwardPipeline {};
  etna::GraphicsPipeline m_shadowPipeline {};

  glm::uvec2 resolution;

  vk::PhysicalDeviceFeatures m_enabledDeviceFeatures = {};

  std::shared_ptr<SceneManager> m_pScnMgr;
  std::unique_ptr<ImGuiRender> m_pGUIRender;

  std::unique_ptr<QuadRenderer> m_pQuad;

  bool drawDebugFSQuad = false;

  struct ShadowMapCam
  {
    float  radius = 5;              ///!< ignored when usePerspectiveM == true
    float  lightTargetDist = 20;    ///!< identify depth range
    bool   usePerspectiveM = true;  ///!< use perspective matrix if true and ortographics otherwise
  } m_light;

  void drawFrame(bool draw_gui);

  void renderWorld(VkCommandBuffer a_cmdBuff, VkImage a_targetImage, VkImageView a_targetImageView);

  void renderScene(VkCommandBuffer a_cmdBuff, const glm::mat4x4& a_wvp, VkPipelineLayout a_pipelineLayout = VK_NULL_HANDLE);

  void loadShaders();

  void setupPipelines();
  void recreateSwapchain();

  void updateUniformBuffer(float a_time);

  void allocateResources();
  void preparePipelines();

  void DrawGui();
};
