#version 450

layout(set = 0, binding = 0) uniform sampler2D ssaoTex;

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 0) out vec4 out_color;

void main()
{
	float ao = textureLod(ssaoTex, frag_tex_coord, 0.0).r;
	out_color = vec4(ao, ao, ao, 1.0);
}