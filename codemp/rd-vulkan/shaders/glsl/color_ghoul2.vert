#version 450

// 64 bytes
layout(push_constant) uniform Transform {
	mat4 mvp;
	float renderMode;
};

layout(set = 1, binding = 4) uniform Bones {
	mat3x4 u_BoneMatrices[72];
};	

layout(location = 0) in vec3 in_position;
layout(location = 10) in uvec4 in_bones;
layout(location = 11) in vec4 in_weights;

out gl_PerVertex {
	vec4 gl_Position;
};

mat4x3 GetBoneMatrix(uint index)
{
	mat3x4 bone = u_BoneMatrices[index];
	return mat4x3(
		bone[0].x, bone[1].x, bone[2].x,
		bone[0].y, bone[1].y, bone[2].y,
		bone[0].z, bone[1].z, bone[2].z,
		bone[0].w, bone[1].w, bone[2].w);

}
void main() {
	mat4x3 skin_matrix =
		GetBoneMatrix(in_bones.x) * in_weights.x +
        GetBoneMatrix(in_bones.y) * in_weights.y +
        GetBoneMatrix(in_bones.z) * in_weights.z +
        GetBoneMatrix(in_bones.w) * in_weights.w;

	vec3 position = skin_matrix * vec4(in_position, 1.0);

	gl_Position = mvp * vec4(position, 1.0);
}
