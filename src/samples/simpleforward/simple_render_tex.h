#ifndef SIMPLE_RENDER_TEX_H
#define SIMPLE_RENDER_TEX_H

#define VK_NO_PROTOTYPES

#include "simple_render.h"
#include <vk_images.h>

class SimpleRenderTexture : public SimpleRender
{
public:
  const std::string VERTEX_SHADER_PATH = "../resources/shaders/simple.vert";
  const std::string FRAGMENT_SHADER_PATH = "../resources/shaders/simple_tex.frag";

  SimpleRenderTexture(uint32_t a_width, uint32_t a_height);
  ~SimpleRenderTexture() override { Cleanup(); };

  void ProcessInput(const AppInput& input) override;
  void LoadScene(const char *path, bool transpose_inst_matrices) override;
  void DrawFrame(float a_time, DrawMode a_mode) override;

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
protected:

  bool m_textureNeedsReload = false;
  std::string m_texturePath = "../resources/textures/test_tex_1.png";

  vk_utils::VulkanImageMem m_texture {};
  VkSampler m_textureSampler = VK_NULL_HANDLE;

  void LoadTexture();

  void SetupGUIElements() override;
  void SetupSimplePipeline() override;
  void Cleanup();

};


#endif //SIMPLE_RENDER_TEX_H
