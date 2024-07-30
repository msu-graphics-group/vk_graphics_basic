#include "../../utils/input_definitions.h"

#include "etna/Etna.hpp"
#include "shadowmap_render.h"
#include <glm/ext.hpp>


void SimpleShadowmapRender::UpdateCamera(const Camera* cams, uint32_t a_camsNumber)
{
  m_cam = cams[0];
  if(a_camsNumber >= 2)
    m_light.cam = cams[1];
  UpdateView();
}

void SimpleShadowmapRender::UpdateView()
{
  ///// calc camera matrix

  // TODO: coordinate systems are all over the place in the sample, investigate and fix properly
  glm::mat4x4 mFlipY = glm::identity<glm::mat4x4>();
  mFlipY[1][1] = -1;

  glm::mat4x4 mFlipX = glm::identity<glm::mat4x4>();
  mFlipX[0][0] = -1;

  {
    const float aspect = float(m_width) / float(m_height);
    const auto mProj = glm::perspectiveLH_ZO(m_cam.fov, aspect, m_cam.zNear, m_cam.zFar);
    m_worldViewProj = mFlipY * mFlipX * mProj * m_cam.viewTm();
  }

  ///// calc light matrix

  {
    const auto mProj =
      m_light.usePerspectiveM ?
        glm::perspectiveLH_ZO(m_light.cam.fov, 1.0f, 1.0f, m_light.lightTargetDist*2.0f) :
        glm::orthoLH_ZO(-m_light.radius, +m_light.radius, -m_light.radius, +m_light.radius, 0.0f, m_light.lightTargetDist);

    m_lightMatrix = mFlipX * mProj * m_light.cam.viewTm();
  }
}

void SimpleShadowmapRender::UpdateUniformBuffer(float a_time)
{
  m_uniforms.lightMatrix = m_lightMatrix;
  m_uniforms.lightPos    = m_light.cam.position;
  m_uniforms.time        = a_time;

  memcpy(m_uboMappedMem, &m_uniforms, sizeof(m_uniforms));
}

void SimpleShadowmapRender::ProcessInput(const AppInput &input)
{
  // add keyboard controls here
  // camera movement is processed separately
  //
  if(input.keyReleased[GLFW_KEY_Q])
    m_input.drawFSQuad = !m_input.drawFSQuad;

  if(input.keyReleased[GLFW_KEY_P])
    m_light.usePerspectiveM = !m_light.usePerspectiveM;

  // recreate pipeline to reload shaders
  if(input.keyPressed[GLFW_KEY_B])
  {
#ifdef WIN32
    std::system("cd ../resources/shaders && python compile_shadowmap_shaders.py");
#else
    std::system("cd ../resources/shaders && python3 compile_shadowmap_shaders.py");
#endif

    etna::reload_shaders();
  }
}
