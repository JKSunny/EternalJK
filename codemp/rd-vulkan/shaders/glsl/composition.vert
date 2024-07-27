#version 450

//layout(location = 0) in vec3 in_position;

layout (location = 0) out vec2 outUV;

void main() {
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	//gl_Position = mvp * vec4(in_position, 1.0);
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}
