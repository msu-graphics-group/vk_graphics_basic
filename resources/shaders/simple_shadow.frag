#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout(location = 0) out vec4 out_fragColor;

layout (location = 0 ) in VS_OUT
{
  vec3 wPos;
  vec3 wNorm;
  vec3 wTangent;
  vec2 texCoord;
  vec4 color;
} surf;

layout(binding = 0, set = 0) uniform AppData
{
  UniformParams Params;
};

layout (binding = 1) uniform sampler2D shadowMap;



vec3 hsv_to_rgb(vec3 hsv) {
    
    float h = mod(hsv.x,1)*360;
    float s = mod(hsv.y,1);
    float v = mod(hsv.z,1);

    float c = v*s;
    float x = c*(1.0-abs(mod(h/60.0,2)-1));
    float m = v-c;

    float r = (300<=h || h<60) ? c : ((60<=h && h<120) || (240<=h && h<300) ? x: 0);
    float g = ((60<=h && h<180)? c : (0<=h && h<240) ? x: 0);
    float b = ((180<=h && h<300)? c : (120<=h && h<360) ? x: 0);
    

    r = r+m;
    g = g+m;
    b = b+m;

    return vec3(r,g,b);

}

void main()
{
  const vec4 posLightClipSpace = Params.lightMatrix*vec4(surf.wPos, 1.0f); // 
  const vec3 posLightSpaceNDC  = posLightClipSpace.xyz/posLightClipSpace.w;    // for orto matrix, we don't need perspective division, you can remove it if you want; this is general case;
  const vec2 shadowTexCoord    = posLightSpaceNDC.xy*0.5f + vec2(0.5f, 0.5f);  // just shift coords from [-1,1] to [0,1]               
    
  const bool  outOfView = (shadowTexCoord.x < 0.0001f || shadowTexCoord.x > 0.9999f || shadowTexCoord.y < 0.0091f || shadowTexCoord.y > 0.9999f);
  const float shadow    = ((posLightSpaceNDC.z < textureLod(shadowMap, shadowTexCoord, 0).x + 0.001f) || outOfView) ? 1.0f : 0.0f;

  const vec4 dark_violet = vec4(0.59f, 0.0f, 0.82f, 1.0f);
  const vec4 chartreuse  = vec4(0.5f, 1.0f, 0.0f, 1.0f);

  vec4 lightColor1 = mix(dark_violet, chartreuse, abs(sin(Params.time)));
  vec4 lightColor2 = vec4(1.0f, 1.0f, 1.0f, 1.0f);
  vec4 lightColor3 = surf.color;
  vec4 lightColor4 = vec4(hsv_to_rgb(vec3(length(surf.wPos*10),0.9f,0.9f)), 1.0f);
   
  vec3 lightDir   = normalize(Params.lightPos - surf.wPos);
  vec4 lightColor = max(dot(surf.wNorm, lightDir), 0.0f) * lightColor4;
  out_fragColor   = (lightColor*shadow + vec4(0.1f)) * vec4(Params.baseColor, 1.0f);
}