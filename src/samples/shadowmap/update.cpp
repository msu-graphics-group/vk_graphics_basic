#include "Renderer.h"

#include <etna/Etna.hpp>
#include <glm/ext.hpp>


void Renderer::updateView(const Camera &main, const Camera &shadow)
{
  ///// calc camera matrix

  // TODO: coordinate systems are all over the place in the sample, investigate and fix properly
  glm::mat4x4 mFlipY = glm::identity<glm::mat4x4>();
  mFlipY[1][1] = -1;

  glm::mat4x4 mFlipX = glm::identity<glm::mat4x4>();
  mFlipX[0][0] = -1;

  {
    const float aspect = float(resolution.x) / float(resolution.y);
    m_worldViewProj = mFlipY * mFlipX * main.projTm(aspect) * main.viewTm();
  }

  ///// calc light matrix

  {
    const auto mProj =
      m_light.usePerspectiveM ?
        glm::perspectiveLH_ZO(shadow.fov, 1.0f, 1.0f, m_light.lightTargetDist*2.0f) :
        glm::orthoLH_ZO(-m_light.radius, +m_light.radius, -m_light.radius, +m_light.radius, 0.0f, m_light.lightTargetDist);

    m_lightMatrix = mFlipX * mProj * shadow.viewTm();

    lightPos = shadow.position;
  }
}

void Renderer::updateUniformBuffer(float time)
{
  m_uniforms.lightMatrix = m_lightMatrix;
  m_uniforms.lightPos    = lightPos;
  m_uniforms.time        = time;

  memcpy(m_uboMappedMem, &m_uniforms, sizeof(m_uniforms));
}

void Renderer::debugInput(const Keyboard &kb)
{
  if(kb[KeyboardKey::kQ] == ButtonState::Falling)
    drawDebugFSQuad = !drawDebugFSQuad;

  if(kb[KeyboardKey::kP] == ButtonState::Falling)
    m_light.usePerspectiveM = !m_light.usePerspectiveM;

  if(kb[KeyboardKey::kB] == ButtonState::Falling)
  {
#ifdef WIN32
    std::system("cd " VK_GRAPHICS_BASIC_ROOT "/resources/shaders && python compile_shadowmap_shaders.py");
#else
    std::system("cd " VK_GRAPHICS_BASIC_ROOT "/resources/shaders && python3 compile_shadowmap_shaders.py");
#endif
    ETNA_CHECK_VK_RESULT(m_context->getDevice().waitIdle());
    etna::reload_shaders();
  }
}
