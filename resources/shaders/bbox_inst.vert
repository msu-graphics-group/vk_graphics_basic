#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 vPos;

layout(push_constant) uniform params_t
{
  mat4 mViewProj;
} params;

layout(binding = 0) readonly buffer Instances
{
  mat4 instances[];
};

void main() 
{
  gl_Position = params.mViewProj * vec4((instances[gl_InstanceIndex] * vPos).xyz, 1.0);
}
