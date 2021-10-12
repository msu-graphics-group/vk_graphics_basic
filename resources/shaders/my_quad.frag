#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 color;

layout (binding = 0) uniform sampler2D colorTex;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;


void main()
{

	const int filterSize = 2;

	



	float x = surf.texCoord.x;
	float y = surf.texCoord.y;


	vec4 ixy =  textureLod(colorTex, surf.texCoord, 0);


	float smooth1 = 1.15;
	float smooth2 = 1.25;


	float dx = 1.0 / textureSize (colorTex, 0).x;
	float dy = 1.0 / textureSize (colorTex, 0).y;

	float totalWeight = 0;
	vec4 totalMain = vec4(0);


	for (int i = -filterSize;i<filterSize;i++)
		for (int j = -filterSize;j<filterSize;j++)
		{
			


			float ox = x + dx * i;
			float oy = y + dy * j;
			
			
			vec4 ioxy = textureLod(colorTex, vec2(ox, oy), 0);



			float ww = exp(-((pow(length(ioxy-ixy),2))/(2*pow(smooth1,2)))-(pow(length(vec2(ox-x,oy-y)),2))/(2*pow(smooth2,2)));


			totalMain+=ioxy*ww;

			totalWeight+=ww;

		}

	color = totalMain / totalWeight;

	//color = textureLod(colorTex, surf.texCoord, 0);
}
