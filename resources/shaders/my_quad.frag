#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 color;

layout (binding = 0) uniform sampler2D colorTex;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

void cmpxch(in out vec4 c1, in out vec4 c2) {
  vec4 min = min(c1, c2);
  vec4 max = max(c1, c2);

  c1 = min;
  c2 = max;
}

void main()
{
  vec4 colors[9];

  colors[0] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(-1, -1));
  colors[1] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(-1, +0));
  colors[2] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(-1, +1));
  colors[3] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(+0, -1));
  colors[4] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(+0, +0));
  colors[5] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(+0, +1));
  colors[6] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(+1, -1));
  colors[7] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(+1, +0));
  colors[8] = textureLodOffset(colorTex, surf.texCoord, 0, ivec2(+1, +1));

  // Hardcoded bitonic sort for 9 elements
  // Operations that won't affect the fourth
  // element were commented out.
  // See https://www.researchgate.net/figure/Structure-of-9-input-sorting-network-bitonic-sorting-network-Each-vertical-line_fig1_4341819
  cmpxch(colors[0], colors[1]);
  cmpxch(colors[3], colors[2]);
  cmpxch(colors[5], colors[4]);
  cmpxch(colors[7], colors[8]);

  cmpxch(colors[2], colors[0]);
  cmpxch(colors[3], colors[1]);
  cmpxch(colors[6], colors[8]);

  cmpxch(colors[1], colors[0]);
  cmpxch(colors[3], colors[2]);
  cmpxch(colors[4], colors[8]);
  cmpxch(colors[6], colors[7]);

  cmpxch(colors[0], colors[8]);
  cmpxch(colors[4], colors[6]);
  cmpxch(colors[5], colors[7]);

  cmpxch(colors[4], colors[5]);
  cmpxch(colors[6], colors[7]);

  cmpxch(colors[0], colors[4]);
  cmpxch(colors[1], colors[5]);
  cmpxch(colors[2], colors[6]);
//  cmpxch(colors[3], colors[7]);

//  cmpxch(colors[0], colors[2]);
//  cmpxch(colors[1], colors[3]);
  cmpxch(colors[4], colors[6]);
  cmpxch(colors[5], colors[6]);

//  cmpxch(colors[0], colors[1]);
//  cmpxch(colors[2], colors[3]);
  cmpxch(colors[4], colors[5]);
//  cmpxch(colors[6], colors[7]);

  color = colors[4];
}
