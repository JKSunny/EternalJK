#version 450
#extension GL_GOOGLE_include_directive : require

#include "global.h"

layout(set = 0, binding = 0) uniform sampler2D ssao_texture;

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 0) out vec4 out_color;

void main()
{
	float ao = textureLod(ssao_texture, frag_tex_coord, 0.0).r;
	out_color = vec4( ao, ao, ao, 1.0 );
}
