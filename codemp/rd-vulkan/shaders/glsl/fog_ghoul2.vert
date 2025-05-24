#version 450
#define VK_BINDLESS

// 64 bytes
layout(push_constant) uniform Transform {
	mat4 mvp;
#ifdef VK_BINDLESS
	uint draw_id;
#endif
};

layout(set = 0, binding = 0) uniform UBO {
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

#ifdef VK_BINDLESS
	struct tcMod_t {
		vec4	matrix;
		vec4	offTurb;
	};

	struct tcGen_t {
		vec3	vector0;
		vec3	vector1;
		int		type;
	};

	struct vkBundle_t {
		vec4	baseColor;
		vec4	vertColor;
		tcMod_t	tcMod;
		tcGen_t	tcGen;
		int		rgbGen;
		int		alphaGen;
		int		numTexMods;
	};

	struct vkDisintegration_t {
		vec3	origin;
		float	threshold;
	};

	struct vkDeform_t {
		float	base;
		float	amplitude;
		float	phase;
		float	frequency;

		vec3	vector;
		float	time;

		int		type;
		int		func;
	};
	
	struct vkUniformGlobal_t {
		vkBundle_t			bundle[3];
		vkDisintegration_t	disintegration;
		vkDeform_t			deform;
		float				portalRange;
		uint				entity_id;
		uint				bones_id;
		uint				pad0;
		vec4				SpecularScale;	
		vec4				NormalScale;
		uint				texture_idx[8];
	};
	
	struct vkUniformBones_t {
		mat3x4 BoneMatrices[72];
	};

	layout(set = 2, binding = 2) readonly buffer GlobalBuffer {
		vkUniformGlobal_t global[];
	};
	
	layout(set = 2, binding = 1) readonly buffer BonesBuffer {
		vkUniformBones_t bones[];
	};
	
	vkUniformGlobal_t global_data = global[draw_id];
#else
	layout(set = 0, binding = 4) uniform Bones {
		mat3x4 u_BoneMatrices[72];
	};
#endif

layout(location = 0) in vec3 in_position;
layout(location = 10) in uvec4 in_bones;
layout(location = 11) in vec4 in_weights;

layout(location = 4) out vec2 fog_tex_coord;

out gl_PerVertex {
	vec4 gl_Position;
};

mat4x3 GetBoneMatrix(uint index)
{
#ifdef VK_BINDLESS
	mat3x4 bone = bones[global_data.bones_id].BoneMatrices[index];
#else
	mat3x4 bone = u_BoneMatrices[index];
#endif
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
