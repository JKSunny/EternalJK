#version 450

// 64 bytes
layout(push_constant) uniform Transform {
	mat4 mvp;
};

layout(set = 1, binding = 0) uniform UBO {
	// VERTEX
	vec4 eyePos;
	vec4 lightPos;
	//  VERTEX-FOG
	vec4 fogDistanceVector;
	vec4 fogDepthVector;
	vec4 fogEyeT;
	// FRAGMENT
	vec4 lightColor;
	vec4 fogColor;
	// linear dynamic light
	vec4 lightVector;
};

layout(set = 1, binding = 2) uniform UBOG3 {
	mat3x4 u_BoneMatrices[72];
};	

layout(location = 0) in vec3 in_position;
layout(location = 8) in uvec4 in_bones;
layout(location = 9) in vec4 in_weights;

layout(location = 4) out vec2 fog_tex_coord;

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
		GetBoneMatrix(uint(in_bones[0])) * in_weights[0] +
        GetBoneMatrix(uint(in_bones[1])) * in_weights[1] +
        GetBoneMatrix(uint(in_bones[2])) * in_weights[2] +
        GetBoneMatrix(uint(in_bones[3])) * in_weights[3];

	vec3 position = skin_matrix * vec4(in_position, 1.0);
	gl_Position = mvp * vec4(position, 1.0);

	float s = dot(in_position, fogDistanceVector.xyz) + fogDistanceVector.w;
	float t = dot(in_position, fogDepthVector.xyz) + fogDepthVector.w;

	if ( fogEyeT.y == 1.0 ) {
		if ( t < 0.0 ) {
			t = 1.0 / 32.0;
		} else {
			t = 31.0 / 32.0;
		}
	} else {
		if ( t < 1.0 ) {
			t = 1.0 / 32.0;
		} else {
			t = 1.0 / 32.0 + (30.0 / 32.0 * t) / ( t - fogEyeT.x );
		}
	}

	fog_tex_coord = vec2(s, t);
}