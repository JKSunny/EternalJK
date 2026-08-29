#version 460
#extension GL_GOOGLE_include_directive : enable

#include "global.h"

layout(push_constant) uniform Transform {
	mat4 mvp;
};

layout(set = VK_DESC_UNIFORM, binding = VK_DESC_UNIFORM_MAIN_BINDING) uniform UBO {
	vec4 lightGridOrigin;	// eyePos		~sunny, use another union?
	vec4 lightGridSize;		// lightPos
	vec4 lightGridBounds;	// lightColor
	vec4 lightVector;
	vec4 fogDistanceVector;
	vec4 fogDepthVector;
	vec4 fogEyeT;
	vec4 fogColor;
};

layout(set = 1, binding = 0) readonly buffer LightGrid {
	vkLightGridSample_t lightGrid[];
};

layout (constant_id = 0) const int mode = 0;

layout(location = 0) out vec3 out_color;

const vec3 cubeVerts[36] = vec3[](
	vec3(-1,-1,-1), vec3( 1,-1,-1), vec3( 1, 1,-1),
	vec3(-1,-1,-1), vec3( 1, 1,-1), vec3(-1, 1,-1),
	vec3(-1,-1, 1), vec3(-1, 1, 1), vec3( 1, 1, 1),
	vec3(-1,-1, 1), vec3( 1, 1, 1), vec3( 1,-1, 1),
	vec3(-1,-1,-1), vec3(-1,-1, 1), vec3( 1,-1, 1),
	vec3(-1,-1,-1), vec3( 1,-1, 1), vec3( 1,-1,-1),
	vec3(-1, 1,-1), vec3( 1, 1,-1), vec3( 1, 1, 1),
	vec3(-1, 1,-1), vec3( 1, 1, 1), vec3(-1, 1, 1),
	vec3(-1,-1,-1), vec3(-1, 1,-1), vec3(-1, 1, 1),
	vec3(-1,-1,-1), vec3(-1, 1, 1), vec3(-1,-1, 1),
	vec3( 1,-1,-1), vec3( 1,-1, 1), vec3( 1, 1, 1),
	vec3( 1,-1,-1), vec3( 1, 1, 1), vec3( 1, 1,-1)
);

void main()
{
	uint index = gl_InstanceIndex;

	uint bx = uint(lightGridBounds.x);
	uint by = uint(lightGridBounds.y);
	uint xy = bx * by;

	uint z = index / xy;
	uint rem = index - z * xy;
	uint y = rem / bx;
	uint x = rem - y * bx;

	vec3 gridPosition = lightGridOrigin.xyz + vec3(x, y, z) * lightGridSize.xyz;

	gridPosition += cubeVerts[gl_VertexIndex] * 2.0;

	gl_Position = mvp * vec4(gridPosition, 1.0);

	switch ( mode )
	{
		case LIGHTGRID_DEBUG_MODE_AMBIENT:
			out_color = lightGrid[index].ambientLight.rgb / 255.0;
			break;

		case LIGHTGRID_DEBUG_MODE_DIRECTED:
			out_color = lightGrid[index].directedLight.rgb / 255.0;
			break;

		case LIGHTGRID_DEBUG_MODE_DIRECTION:
			//out_color = lightGrid[index].lightDir.rgb * 0.5 + 0.5;
			out_color = lightGrid[index].lightDir.rgb;
			break;

		default: 
			out_color = vec3(1.0, 0.0, 0.0);
	}
}