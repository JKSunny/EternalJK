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
	mat4 modelMatrix;
	mat4 boneMatrices[72];
};	

layout(location = 0) in vec3 in_position;
layout(location = 8) in vec4 in_bones;
layout(location = 9) in vec4 in_weights;

layout(location = 4) out vec2 fog_tex_coord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	mat4 skin_matrix = 
		in_weights.x * boneMatrices[int(in_bones.x)] +
		in_weights.y * boneMatrices[int(in_bones.y)] +
		in_weights.z * boneMatrices[int(in_bones.z)] +
		in_weights.w * boneMatrices[int(in_bones.w)];

	gl_Position = mvp * skin_matrix * vec4(in_position, 1.0);

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
