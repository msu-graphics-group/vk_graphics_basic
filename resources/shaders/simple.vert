#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "unpack_attributes.h"


layout(location = 0) in vec4 vPosNorm;
layout(location = 1) in vec4 vTexCoordAndTang;

layout(push_constant) uniform params_t
{
    mat4 mProjView;
    mat4 mModel;
} params;


layout (location = 0 ) out VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;

    vec4 color;

} vOut;


out gl_PerVertex { vec4 gl_Position; };




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

void main(void)
{
    const vec4 wNorm = vec4(DecodeNormal(floatBitsToInt(vPosNorm.w)),         0.0f);
    const vec4 wTang = vec4(DecodeNormal(floatBitsToInt(vTexCoordAndTang.z)), 0.0f);

    vOut.wPos     = (params.mModel * vec4(vPosNorm.xyz, 1.0f)).xyz;
    vOut.wNorm    = normalize(mat3(transpose(inverse(params.mModel))) * wNorm.xyz);
    vOut.wTangent = normalize(mat3(transpose(inverse(params.mModel))) * wTang.xyz);
    vOut.texCoord = vTexCoordAndTang.xy;

    gl_Position   = params.mProjView * vec4(vOut.wPos, 1.0);

    vOut.color = vec4(hsv_to_rgb(vec3(length(vOut.wPos*10),0.9f,0.9f)), 1.0f);
}