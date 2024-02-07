#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vPos;

layout(push_constant) uniform params_t
{
  mat4 mViewProj;
  mat4 mModel;
} params;

void main() 
{
  gl_Position = params.mViewProj * vec4((params.mModel * vPos).xyz, 1.0);
}
