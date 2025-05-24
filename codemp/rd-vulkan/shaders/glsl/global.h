#define VK_DLIGHT_GPU

#ifdef VK_BINDLESS
	//#define VK_BINDLESS_VBO_ARRAY			// vbo array
	#define VK_BINDLESS_BATCHED_INDIRECT	// uses vbo array too

	#if defined(PER_PIXEL_LIGHTING) || defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV)
		#define USE_ENTITY_DATA
	#endif

	#if (defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV)) && (defined(VK_BINDLESS_VBO_ARRAY) || defined(VK_BINDLESS_BATCHED_INDIRECT))
		#define VK_BINDLESS_MODEL_VBO
	#endif
#endif

#define LIGHT_MODE_NONE		0
#define LIGHT_MODE_LIGHTMAP	1
#define LIGHT_MODE_VECTOR	2
#define LIGHT_MODE_VERTEX	3

#ifdef VK_BINDLESS
	#extension GL_EXT_nonuniform_qualifier : require
#endif

#define PI 3.1415926535897932384626433832795
#define M_PI 3.1415926535897932384626433832795

// 64 bytes
layout(push_constant) uniform Transform {
	mat4 mvp;
#ifdef VK_BINDLESS
	uint draw_id;
#endif
};

// general
#ifdef USE_TX2
#define USE_TX1
#endif

#ifdef USE_CL2
#define USE_CL1
#endif

struct NTB {
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
};

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
	#ifdef VK_BINDLESS_MODEL_VBO
		uint			vbo_model_index;
	#else
		uint			pad0;
	#endif
	vec4				SpecularScale;	
	vec4				NormalScale;
	uint				texture_idx[8];
	#ifdef VK_BINDLESS_BATCHED_INDIRECT
	mat4				mvp;
	#endif
};

struct vkUniformCamera_t {
	vec4 ViewOrigin;
};

struct vkUniformEntity_t {
	vec4 ambientLight;
	vec4 directedLight;
	vec4 LocalLightOrigin;
	vec4 localViewOrigin;
	mat4 ModelMatrix;
};


struct vkUniformLightEntry_t {
	vec4	origin;
	vec3	color;
	float	radius;
};

struct vkUniformLight_t {
	uint	num_lights;
	vkUniformLightEntry_t	light[64];
};

struct vkUniformBones_t {
	mat3x4 BoneMatrices[72];
};

#ifdef VK_BINDLESS
	// a shared_glsl_bindings.c would be neat! 
	// .. these bindings are also in vk_local.h
	#define VK_DESC_TEXTURES_BINDLESS	1
	#define VK_DESC_SSBO_BINDLESS		2
	#define VK_DESC_VBO_BINDLESS		3
	#define VK_DESC_IBO_BINDLESS		4

	// SSBO
	layout(std430, set = VK_DESC_SSBO_BINDLESS, binding = 0) readonly buffer EntityBuffer {
		vkUniformEntity_t entities[];
	};

	layout(std430, set = VK_DESC_SSBO_BINDLESS, binding = 1) readonly buffer BonesBuffer {
		vkUniformBones_t bones[];
	};

	layout(std430, set = VK_DESC_SSBO_BINDLESS, binding = 2) readonly buffer GlobalBuffer {
		vkUniformGlobal_t global[];
	};

	#ifdef PER_PIXEL_LIGHTING
	layout(std430, set = VK_DESC_SSBO_BINDLESS, binding = 3) readonly buffer CameraBuffer {
		vkUniformCamera_t camera;
	};
	#endif

	layout(std430, set = VK_DESC_SSBO_BINDLESS, binding = 4) readonly buffer LightBuffer {
		#ifdef VK_DLIGHT_GPU
			vkUniformLight_t lights;
		#else
			vec4 item;
		#endif
	};

	vkUniformGlobal_t global_data;
	#ifdef USE_ENTITY_DATA
		vkUniformEntity_t entity_data;
	#endif
	// VBO & IBO
#ifdef VK_BINDLESS_MODEL_VBO
	#if defined(VK_BINDLESS_BATCHED_INDIRECT)
		#define NUM_BINDLESS_INIDIRECT_DRAW_ITEMS 4096
	#elif defined(VK_BINDLESS_VBO_ARRAY)
		#define NUM_BINDLESS_INIDIRECT_DRAW_ITEMS 20
	#endif

	struct vkUniformIndirectDraw_t {
		int first_index[NUM_BINDLESS_INIDIRECT_DRAW_ITEMS];
	};

	layout(std430, set = VK_DESC_SSBO_BINDLESS, binding = 5) readonly buffer IndirectBuffer {
		vkUniformIndirectDraw_t indirect[];
	};


	layout(set = VK_DESC_VBO_BINDLESS,	binding = 0) readonly buffer MODEL_VBO {
		uint data[];
	} model_vbos[];

	layout(set = VK_DESC_IBO_BINDLESS,	binding = 0) readonly buffer MODEL_IBO {
		uint data[];
	} model_ibos[];

	
	uint get_model_uint(uint model_id, uint offset)
	{
		return model_vbos[nonuniformEXT(model_id)].data[offset];
	}

	vec2 get_model_float2(uint model_id, uint offset)
	{
		vec2 result;
		result.x = uintBitsToFloat(model_vbos[nonuniformEXT(model_id)].data[offset + 0]);
		result.y = uintBitsToFloat(model_vbos[nonuniformEXT(model_id)].data[offset + 1]);
		return result;
	}

	vec3 get_model_float3(uint model_id, uint offset)
	{
		vec3 result;
		result.x = uintBitsToFloat(model_vbos[nonuniformEXT(model_id)].data[offset + 0]);
		result.y = uintBitsToFloat(model_vbos[nonuniformEXT(model_id)].data[offset + 1]);
		result.z = uintBitsToFloat(model_vbos[nonuniformEXT(model_id)].data[offset + 2]);
		return result;
	}

	vec4 get_model_float4(uint model_id, uint offset)
	{
		vec4 result;
		result.x = uintBitsToFloat(model_vbos[nonuniformEXT(model_id)].data[offset + 0]);
		result.y = uintBitsToFloat(model_vbos[nonuniformEXT(model_id)].data[offset + 1]);
		result.z = uintBitsToFloat(model_vbos[nonuniformEXT(model_id)].data[offset + 2]);
		result.w = uintBitsToFloat(model_vbos[nonuniformEXT(model_id)].data[offset + 3]);
		return result;
	}
#endif

	#define GET_BONE_BY_INDEX bones[global_data.bones_id].BoneMatrices[index];
#else
	#ifdef PER_PIXEL_LIGHTING
		layout(std140, set = 0, binding = 1) uniform Camera {
			vkUniformCamera_t camera;
		};
	#endif

	layout(std140, set = 0, binding = 2) uniform Light {
		#ifdef VK_DLIGHT_GPU
			vkUniformLight_t lights;
		#else
			vec4 item;
		#endif
	};

	layout(std140, set = 0, binding = 3) uniform Entity {
		vkUniformEntity_t entity_data;
	};

	layout(std140, set = 0, binding = 4) uniform Bones {
		vkUniformBones_t bones;
	};

	layout(std140, set = 0, binding = 5) uniform Global {
		vkUniformGlobal_t global_data;
	};

	#define GET_BONE_BY_INDEX bones.BoneMatrices[index]
#endif

#define u_ambientLight		entity_data.ambientLight
#define u_directedLight		entity_data.directedLight
#define u_LocalLightOrigin	entity_data.LocalLightOrigin
#define u_localViewOrigin	entity_data.localViewOrigin
#define u_ModelMatrix		entity_data.ModelMatrix

#define u_bundle			global_data.bundle
#define u_disintegration	global_data.disintegration
#define u_deform			global_data.deform
#define u_portalRange		global_data.portalRange
#define u_SpecularScale		global_data.SpecularScale
#define u_NormalScale		global_data.NormalScale

#define u_ViewOrigin		camera.ViewOrigin

vec3 QuatTransVec( in vec4 quat, in vec3 vec ) {
	vec3 tmp = 2.0 * cross( quat.xyz, vec );
	return vec + quat.w * tmp + cross( quat.xyz, tmp );
}

void QTangentToNTB( in vec4 qtangent, out NTB ntb ) {
	ntb.normal = QuatTransVec( qtangent, vec3( 0.0, 0.0, 1.0 ) );
	ntb.tangent = QuatTransVec( qtangent, vec3( 1.0, 0.0, 0.0 ) );
	ntb.tangent *= sign( qtangent.w );
	ntb.binormal = QuatTransVec( qtangent, vec3( 0.0, 1.0, 0.0 ) );
}

// vertex
vec2 GenTexCoords(const in vec2 tc, int TCGen, vec3 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1)
{
	vec2 tex = tc;

	if ( TCGen == 9 ) // TCGEN_VECTOR
		tex = vec2(dot(position, TCGenVector0), dot(position, TCGenVector1));

	return tex;
}

vec2 ModTexCoords(vec2 st, vec3 position, vec4 texMatrix, vec4 offTurb)
{
	float amplitude = offTurb.z;
	float phase = offTurb.w * 2.0 * M_PI;
	vec2 st2;
	st2.x = st.x * texMatrix.x + (st.y * texMatrix.z + offTurb.x);
	st2.y = st.x * texMatrix.y + (st.y * texMatrix.w + offTurb.y);

	vec2 offsetPos = vec2(position.x + position.z, position.y);
	
	vec2 texOffset = sin(offsetPos * (2.0 * M_PI / 1024.0) + vec2(phase));
	
	return st2 + texOffset * amplitude;	
}

#if defined(USE_VBO_GHOUL2)
	mat4x3 GetBoneMatrix(uint index)
	{
		mat3x4 bone = GET_BONE_BY_INDEX;

		return mat4x3(
			bone[0].x, bone[1].x, bone[2].x,
			bone[0].y, bone[1].y, bone[2].y,
			bone[0].z, bone[1].z, bone[2].z,
			bone[0].w, bone[1].w, bone[2].w);
	}
#endif

#if defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV)
	vec4 CalcColor( int index, in vec3 position, in vec3 normal ) {
		vec4 color = ( u_bundle[index].vertColor * vec4( 0.0 ) ) + u_bundle[index].baseColor;		// skip vertColor?

		switch ( u_bundle[index].rgbGen ) {
			case 14:// CGEN_DISINTEGRATION_1
			{
				vec3 delta = u_disintegration.origin - position;
				float sqrDistance = dot( delta, delta );

				if ( sqrDistance < u_disintegration.threshold )
					color *= 0.0;
				else if ( sqrDistance < u_disintegration.threshold + 60.0 )
					color *= vec4( 0.0, 0.0, 0.0, 1.0 );
				else if ( sqrDistance < u_disintegration.threshold + 150.0 )
					color *= vec4( 0.435295, 0.435295, 0.435295, 1.0 );
				else if ( sqrDistance < u_disintegration.threshold + 180.0 )
					color *= vec4( 0.6862745, 0.6862745, 0.6862745, 1.0 );

				return color;
			}
			case 15:// CGEN_DISINTEGRATION_2
			{
				vec3 delta = u_disintegration.origin - position;
				float sqrDistance = dot( delta, delta );

				if ( sqrDistance < u_disintegration.threshold )
					return vec4(0.0);

				return color;
			}
		}

		switch ( u_bundle[index].alphaGen ) {
			case 6: // AGEN_LIGHTING_SPECULAR
			{
				vec3 viewer = normalize( u_localViewOrigin.xyz - position );
				vec3 lightDirection = ( transpose(u_ModelMatrix) * vec4( u_LocalLightOrigin.xyz, 0.0 ) ).xyz;

				vec3 reflected = -reflect( lightDirection, normal );
				color.a = clamp( dot( reflected, normalize( viewer ) ), 0.0, 1.0 );
				color.a *= color.a;
				color.a *= color.a;
				break;
			}
			case 8: // AGEN_PORTAL
			{
				vec3 viewer = normalize( u_localViewOrigin.xyz - position );

				color.a = clamp( length( viewer ) / u_portalRange, 0.0, 1.0 );
				break;
			}
		}

		return color;
	}

	float GetNoiseValue( float x, float y, float z, float t )
	{
		// Variation on the 'one-liner random function'.
		// Not sure if this is still 'correctly' random
		return fract( sin( dot(
			vec4( x, y, z, t ),
			vec4( 12.9898, 78.233, 12.9898, 78.233 )
		)) * 43758.5453 );
	}

	float CalculateDeformScale( in int func, in float time, in float phase, in float frequency )
	{
		float value = phase + time * frequency;

		switch ( func ) {
			case 1: // GF_SIN
				return sin(value * 2.0 * M_PI);
			case 2: // GF_SQUARE
				return sign(0.5 - fract(value));
			case 3: // GF_TRIANGLE
				return abs(fract(value + 0.75) - 0.5) * 4.0 - 1.0;
			case 4: // GF_SAWTOOTH
				return fract(value);
			case 5:	// GF_INVERSE_SAWTOOTH
				return 1.0 - fract(value);
			default:// GF_NONE
				return 0.0;
		}
	}

	vec3 DeformPosition( const vec3 pos, const vec3 normal, const vec2 st )
	{
		switch ( u_deform.type ) {
			default:
			{
				return pos;
			}

			case 3: // DEFORM_BULGE
			{
				float bulgeHeight	= u_deform.amplitude;
				float bulgeWidth	= u_deform.phase;
				float bulgeSpeed	= u_deform.frequency;

				float scale = CalculateDeformScale( 1, u_deform.time, bulgeWidth * st.x, bulgeSpeed );

				return pos + normal * scale * bulgeHeight;
			}

			case 4: // DEFORM_BULGE_UNIFORM
			{
				float bulgeHeight = u_deform.amplitude;

				return pos + normal * bulgeHeight;
			}

			case 1: // DEFORM_WAVE
			{
				float base		= u_deform.base;
				float amplitude = u_deform.amplitude;
				float phase		= u_deform.phase;
				float frequency = u_deform.frequency;
				float spread	= u_deform.vector.x;

				float offset = dot( pos.xyz, vec3( spread ) );
				float scale = CalculateDeformScale( u_deform.func, u_deform.time, phase + offset, frequency );

				return pos + normal * (base + scale * amplitude);
			}

			case 5: // DEFORM_MOVE
			{
				float base		= u_deform.base;
				float amplitude = u_deform.amplitude;
				float phase		= u_deform.phase;
				float frequency = u_deform.frequency;
				vec3 direction	= u_deform.vector;

				float scale = CalculateDeformScale( u_deform.func, u_deform.time, phase, frequency );

				return pos + direction * (base + scale * amplitude);
			}

			case 6: // DEFORM_PROJECTION_SHADOW
			{
				vec3 ground = vec3(
					u_deform.base,
					u_deform.amplitude,
					u_deform.phase
				);

				float groundDist	= u_deform.frequency;
				vec3 lightDir		= u_deform.vector;

				float d = 1.0 / dot( lightDir, ground );
				vec3 lightPos = lightDir * d;
				return pos - lightPos * (dot( pos, ground ) + groundDist);
			}

			case 17: // DEFORM_DISINTEGRATION
			{
				vec3 delta = u_disintegration.origin - pos;
				float sqrDistance = dot(delta, delta);
				vec3 normalScale = vec3(-0.01);
				if ( sqrDistance < u_disintegration.threshold )
				{
					normalScale = vec3(2.0, 2.0, 0.5);
				}
				else if ( sqrDistance < u_disintegration.threshold + 50 )
				{
					normalScale = vec3(1.0, 1.0, 0.0);
				}
				return pos + normal * normalScale;
			}
		}
	}

	vec3 DeformNormal( const in vec3 position, const in vec3 normal )
	{
		if ( u_deform.type != 2 ) // DEFORM_NORMALS
			return normal;

		float amplitude = u_deform.amplitude;
		float frequency = u_deform.frequency;

		vec3 outNormal = normal;
		const float scale = 0.98;

		outNormal.x += amplitude * GetNoiseValue(
			position.x * scale,
			position.y * scale,
			position.z * scale,
			u_deform.time * frequency );

		outNormal.y += amplitude * GetNoiseValue(
			100.0 * position.x * scale,
			position.y * scale,
			position.z * scale,
			u_deform.time * frequency );

		outNormal.z += amplitude * GetNoiseValue(
			200.0 * position.x * scale,
			position.y * scale,
			position.z * scale,
			u_deform.time * frequency );

		return outNormal;
	}
#endif

// fragment