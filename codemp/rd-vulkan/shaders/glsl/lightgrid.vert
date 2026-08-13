#version 460
#extension GL_GOOGLE_include_directive : enable

#include "global.h"

layout(push_constant) uniform Transform {
	mat4 mvp;
};

layout(set = VK_DESC_UNIFORM, binding = VK_DESC_UNIFORM_MAIN_BINDING) uniform UBO {
	vec4 eyePos;		// lightGridOrigin
	vec4 lightPos;		// lightGridSize
	vec4 lightColor;	// lightGridBounds
	vec4 lightVector;
	vec4 fogDistanceVector;
	vec4 fogDepthVector;
	vec4 fogEyeT;
	vec4 fogColor;
};

layout(set = 1, binding = 0) readonly buffer LightGrid {
	vkLightGridSample_t lightGrid[];
};

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

	uint bx = uint(lightColor.x);
	uint by = uint(lightColor.y);
	uint xy = bx * by;

	uint z = index / xy;
	uint rem = index - z * xy;
	uint y = rem / bx;
	uint x = rem - y * bx;

	vec3 gridPosition = eyePos.xyz + vec3(x, y, z) * lightPos.xyz;

	gridPosition += cubeVerts[gl_VertexIndex] * 2.0;

	gl_Position = mvp * vec4(gridPosition, 1.0);

	out_color = lightGrid[index].ambientLight.rgb / 255.0;
}