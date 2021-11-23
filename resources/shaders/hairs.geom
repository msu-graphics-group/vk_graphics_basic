#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout (triangles) in;
layout (invocations = 4) in;

layout (triangle_strip, max_vertices = 12) out;

layout(push_constant) uniform params_t
{
	mat4 mProjView;
	mat4 mModel;
} params;

layout (location = 0 ) in VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
} surfInp[3];

layout (location = 0 ) out VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
} surfOut;

layout (binding = 0, set=0) uniform AppData
{
    UniformParams Params;
};

vec3 rotateZaxis(vec3 v, float deg) {
    return vec3(v.x*cos(deg)-v.y*sin(deg),v.x*sin(deg)+v.y*cos(deg),v.z);
}

void main() {
    vec3 center1 = vec3(0);
    for (uint i=0; i<3; ++i)
    {
        center1 += surfInp[i].wPos;
    }

    center1 /= 3.0;




    float deg = 3.14f*sin(Params.time)/8.0f;

    vec3 rig2 = rotateZaxis(surfInp[1].wNorm,deg)/10.0f;


    vec3[3][3] resV;
    
    vec3 offw =(surfInp[0].wPos-center1)/5.0f;
    resV[0] = vec3[3](center1+offw,center1-offw,center1+offw+rig2);
    resV[1] = vec3[3](center1-offw+rig2,center1-offw,center1+offw+rig2);

    vec3 center2 = center1+rig2;
    vec3 rig3 = rotateZaxis(rig2,deg);

    resV[2] = vec3[3](center2-offw,center2+offw,center2+rig3);





    for (uint i=0;i<3;++i) {
            surfOut.wPos = surfInp[i].wPos;
            surfOut.wNorm = surfInp[i].wNorm;
            surfOut.wTangent = surfInp[i].wTangent;
            surfOut.texCoord = surfInp[i].texCoord;


            if (gl_InvocationID>0)
                surfOut.wPos = resV[gl_InvocationID-1][i];
           

            gl_Position = params.mProjView * vec4(surfOut.wPos, 1.0);
            EmitVertex();
            }
    EndPrimitive();
    

    

}