/*
Copyright (C) 2018 Christoph Schied
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef  _GLOBAL_UBO_DESCRIPTOR_SET_LAYOUT_H_
#define  _GLOBAL_UBO_DESCRIPTOR_SET_LAYOUT_H_

#include "constants.h"

#define GLOBAL_UBO_BINDING_IDX               0
#define GLOBAL_INSTANCE_BUFFER_BINDING_IDX   1

#define UBO_CVAR_LIST \
	UBO_CVAR_DO( flt_antilag_hf,					1		)	/* A-SVGF anti-lag filter strength, [0..inf) */ \
	UBO_CVAR_DO( flt_antilag_lf,					0.2		) \
	UBO_CVAR_DO( flt_antilag_spec,					2		) \
	UBO_CVAR_DO( flt_antilag_spec_motion,			0.004	) /* scaler for motion vector scaled specular anti-blur adjustment */ \
	UBO_CVAR_DO( flt_atrous_depth,					0.5		)	/* wavelet fitler sensitivity to depth, [0..inf) */ \
	UBO_CVAR_DO( flt_atrous_deflicker_lf,			2		)	/* max brightness difference between adjacent pixels in the LF channel, (0..inf) */ \
	UBO_CVAR_DO( flt_atrous_hf,						4		)	/* number of a-trous wavelet filter iterations on the LF channel, [0..4] */ \
	UBO_CVAR_DO( flt_atrous_lf,						4		) \
	UBO_CVAR_DO( flt_atrous_spec,					3		) \
	UBO_CVAR_DO( flt_atrous_lum_hf,					16		)	/* wavelet filter sensitivity to luminance, [0..inf) */ \
	UBO_CVAR_DO( flt_atrous_normal_hf,				64		)	/* wavelet filter sensitivity to normals, [0..inf) */ \
	UBO_CVAR_DO( flt_atrous_normal_lf,				8		) \
	UBO_CVAR_DO( flt_atrous_normal_spec,			1		) \
	UBO_CVAR_DO( flt_enable,						1		)	/* switch for the entire SVGF reconstruction, 0 or 1 */ \
	UBO_CVAR_DO( flt_fixed_albedo,					0		)	/* if nonzero, replaces surface albedo with that value after filtering */ \
	UBO_CVAR_DO( flt_min_alpha_color_hf,			0.02	)	/* minimum weight for the new frame data, color channel, (0..1] */ \
	UBO_CVAR_DO( flt_min_alpha_color_lf,			0.01	) \
	UBO_CVAR_DO( flt_min_alpha_color_spec,			0.01	) \
	UBO_CVAR_DO( flt_min_alpha_moments_hf,			0.01	)	/* minimum weight for the new frame data, moments channel, (0..1] */  \
	UBO_CVAR_DO( flt_scale_hf,						1		)	/* overall per-channel output scale, [0..inf) */ \
	UBO_CVAR_DO( flt_scale_lf,						1		) \
	UBO_CVAR_DO( flt_scale_overlay,					1.0		)	/* scale for transparent and emissive objects visible with primary rays */ \
	UBO_CVAR_DO( flt_scale_spec,					1		) \
	UBO_CVAR_DO( flt_show_gradients,				0		)	/* switch for showing the gradient values as overlay image, 0 or 1 */ \
	UBO_CVAR_DO( flt_taa,							1		)	/* temporal anti-aliasing mode: 0 = off, 1 = regular TAA, 2 = temporal upscale: AA_MODE_UPSCALE */ \
	UBO_CVAR_DO( flt_taa_anti_sparkle,				0.25	)	/* strength of the anti-sparkle filter of TAA, [0..1] */ \
	UBO_CVAR_DO( flt_taa_variance,					1.0		)	/* temporal AA variance window scale, 0 means disable NCC, [0..inf) */ \
	UBO_CVAR_DO( flt_taa_history_weight,			0.95	)	/* temporal AA weight of the history sample, [0..1) */ \
	UBO_CVAR_DO( flt_temporal_hf,					1		)	/* temporal filter strength, [0..1] */ \
	UBO_CVAR_DO( flt_temporal_lf,					1		) \
	UBO_CVAR_DO( flt_temporal_spec,					1		) \
	UBO_CVAR_DO( pt_aperture,						2.0		)	/* aperture size for the Depth of Field effect, in world units */ \
	UBO_CVAR_DO( pt_aperture_angle,					0		)	/* rotation of the polygonal aperture, [0..1] */ \
	UBO_CVAR_DO( pt_aperture_type,					0		)	/* number of aperture polygon edges, circular if less than 3 */ \
	UBO_CVAR_DO( pt_beam_softness,					1.0		)	/* beam softness */ \
	UBO_CVAR_DO( pt_bump_scale,						1.0		)	/* scale for normal maps [0..1] */ \
	UBO_CVAR_DO( pt_cameras,						1		)	/* switch for security cameras, 0 or 1 */ \
	UBO_CVAR_DO( pt_direct_polygon_lights,			1		)	/* switch for direct lighting from local polygon lights, 0 or 1 */ \
	UBO_CVAR_DO( pt_direct_roughness_threshold,		0.18	)	/* roughness value where the path tracer switches direct light specular sampling from NDF based to light based, [0..1] */ \
	UBO_CVAR_DO( pt_direct_dyn_lights,			1		)	/* switch for direct lighting from local sphere lights, 0 or 1 */ \
	UBO_CVAR_DO( pt_direct_sun_light,				1		)	/* switch for direct lighting from the sun, 0 or 1 */ \
	UBO_CVAR_DO( pt_explosion_brightness,			4.0		)	/* brightness factor for explosions */ \
	UBO_CVAR_DO( pt_fake_roughness_threshold,		0.20	)	/* roughness value where the path tracer starts switching indirect light specular sampling from NDF based to SH based, [0..1] */ \
	UBO_CVAR_DO( pt_focus,							200		)	/* focal distance for the Depth of Field effect, in world units */ \
	UBO_CVAR_DO( pt_indirect_polygon_lights,		1		)	/* switch for bounce lighting from local polygon lights, 0 or 1 */ \
	UBO_CVAR_DO( pt_indirect_dyn_lights,			1		)	/* switch for bounce lighting from local sphere lights, 0 or 1 */ \
	UBO_CVAR_DO( pt_light_stats,					1		)	/* switch for statistical light PDF correction, 0 or 1 */ \
	UBO_CVAR_DO( pt_max_log_sky_luminance,			-3		)	/* maximum sky luminance, log2 scale, used for polygon light selection, (-inf..inf) */ \
	UBO_CVAR_DO( pt_min_log_sky_luminance,			-10		)	/* minimum sky luminance, log2 scale, used for polygon light selection, (-inf..inf) */ \
	UBO_CVAR_DO( pt_metallic_override,				-1		)	/* overrides metallic parameter of all materials if non-negative, [0..1] */ \
	UBO_CVAR_DO( pt_ndf_trim,						0.9		)	/* trim factor for GGX NDF sampling (0..1] */ \
	UBO_CVAR_DO( pt_num_bounce_rays,				1		)	/* number of bounce rays, valid values are 0 (disabled), 0.5 (half-res diffuse), 1 (full-res diffuse + specular), 2 (two bounces) */ \
	UBO_CVAR_DO( pt_particle_softness,				1.0		)	/* particle softness */ \
	UBO_CVAR_DO( pt_reflect_refract,				2		)	/* number of reflection or refraction bounces: 0, 1 or 2 */ \
	UBO_CVAR_DO( pt_roughness_override,				-1		)	/* overrides roughness of all materials if non-negative, [0..1] */ \
	UBO_CVAR_DO( pt_specular_anti_flicker,			2		)	/* fade factor for rough reflections of surfaces far away, [0..inf) */ \
	UBO_CVAR_DO( pt_specular_mis,					1		)	/* enables the use of MIS between specular direct lighting and BRDF specular rays */ \
	UBO_CVAR_DO( pt_show_sky,						0		)	/* switch for showing the sky polygons, 0 or 1 */ \
	UBO_CVAR_DO( pt_sun_bounce_range,				2000	)	/* range limiter for indirect lighting from the sun, helps reduce noise, (0..inf) */ \
	UBO_CVAR_DO( pt_sun_specular,					1.0		)	/* scale for the direct specular reflection of the sun */ \
	UBO_CVAR_DO( pt_texture_lod_bias,				0		)	/* LOD bias for textures, (-inf..inf) */ \
	UBO_CVAR_DO( pt_toksvig,						1		)	/* intensity of Toksvig roughness correction, [0..inf) */ \
	UBO_CVAR_DO( pt_thick_glass,					0		)	/* switch for thick glass refraction: 0 (disabled), 1 (reference mode only), 2 (real-time mode) */ \
	UBO_CVAR_DO( pt_water_density,					0.5		)	/* scale for light extinction in water and other media, [0..inf) */ \
	UBO_CVAR_DO( tm_debug,							0		)	/* switch to show the histogram (1) or tonemapping curve (2) */ \
	UBO_CVAR_DO( tm_dyn_range_stops,				7.0		)	/* Effective display dynamic range in linear stops = log2((max+refl)/(darkest+refl)) (eqn. 6), (-inf..0) */ \
	UBO_CVAR_DO( tm_enable,							1		)	/* switch for tone mapping, 0 or 1 */ \
	UBO_CVAR_DO( tm_exposure_bias,					-1.0	)	/* exposure bias, log-2 scale */ \
	UBO_CVAR_DO( tm_exposure_speed_down,			1		)	/* speed of exponential eye adaptation when scene gets darker, 0 means instant */ \
	UBO_CVAR_DO( tm_exposure_speed_up,				2		)	/* speed of exponential eye adaptation when scene gets brighter, 0 means instant */ \
	UBO_CVAR_DO( tm_blend_scale_border,				1		)	/* scale factor for full screen blend intensity, at screen border */ \
	UBO_CVAR_DO( tm_blend_scale_center,				0		)	/* scale factor for full screen blend intensity, at screen center */ \
	UBO_CVAR_DO( tm_blend_scale_fade_exp,			4		)	/* exponent used to interpolate between "border" and "center" factors */ \
	UBO_CVAR_DO( tm_blend_distance_factor,			1.2		)	/* scale for the distance from the screen center when computing full screen blend intensity */ \
	UBO_CVAR_DO( tm_blend_max_alpha,				0.2		)	 /* maximum opacity for full screen blend effects */ \
	UBO_CVAR_DO( tm_high_percentile,				90		)	/* high percentile for computing histogram average, (0..100] */ \
	UBO_CVAR_DO( tm_knee_start,						0.6		)	/* where to switch from a linear to a rational function ramp in the post-tonemapping process, (0..1)  */ \
	UBO_CVAR_DO( tm_low_percentile,					70		)	/* low percentile for computing histogram average, [0..100) */ \
	UBO_CVAR_DO( tm_max_luminance,					1.0		)	/* auto-exposure maximum luminance, (0..inf) */ \
	UBO_CVAR_DO( tm_min_luminance,					0.0002	)	/* auto-exposure minimum luminance, (0..inf) */ \
	UBO_CVAR_DO( tm_noise_blend,					0.5		)	/* Amount to blend noise values between autoexposed and flat image [0..1] */ \
	UBO_CVAR_DO( tm_noise_stops,					-12		)	/* Absolute noise level in photographic stops, (-inf..inf) */ \
	UBO_CVAR_DO( tm_reinhard,						0.5		)	/* blend factor between adaptive curve tonemapper (0) and Reinhard curve (1) */ \
	UBO_CVAR_DO( tm_slope_blur_sigma,				12.0	)	/* sigma for Gaussian blur of tone curve slopes, (0..inf) */ \
	UBO_CVAR_DO( tm_white_point,					10.0	)	/* how bright colors can be before they become white, (0..inf) */ \
	UBO_CVAR_DO( tm_hdr_peak_nits,					800.0	)	/* Exposure value 0 is mapped to this display brightness (post tonemapping) */ \
	UBO_CVAR_DO( tm_hdr_saturation_scale,			100		)	/* HDR mode saturation adjustment, percentage [0..200], with 0% -> desaturated, 100% -> normal, 200% -> oversaturated */ \
	UBO_CVAR_DO( ui_hdr_nits,						300		)	/* HDR mode UI (stretch pic) brightness in nits */ \

#define GLOBAL_UBO_VAR_LIST \
	GLOBAL_UBO_VAR_LIST_DO( MAT4,	 V								) \
	GLOBAL_UBO_VAR_LIST_DO( MAT4,	 invV							) \
	GLOBAL_UBO_VAR_LIST_DO( MAT4,	 V_prev							) \
	GLOBAL_UBO_VAR_LIST_DO( MAT4,	 P								) \
	GLOBAL_UBO_VAR_LIST_DO( MAT4,	 invP							) \
	GLOBAL_UBO_VAR_LIST_DO( MAT4,	 P_prev							) \
	GLOBAL_UBO_VAR_LIST_DO( MAT4,	 invP_prev						) \
	GLOBAL_UBO_VAR_LIST_DO( MAT4,	 shadow_map_VP					) \
	GLOBAL_UBO_VAR_LIST_DO( MAT4,	 environment_rotation_matrix	) \
	\
	GLOBAL_UBO_VAR_LIST_DO( DYNLIGHTDATA, dyn_light_data[MAX_LIGHT_SOURCES] ) \
	GLOBAL_UBO_VAR_LIST_DO( VEC4,	 cam_pos						) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC4,	 world_center					) \
	GLOBAL_UBO_VAR_LIST_DO( VEC4,	 world_size						) \
	GLOBAL_UBO_VAR_LIST_DO( VEC4,	 world_half_size_inv			) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC2,	 sub_pixel_jitter				) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,   prev_adapted_luminance			) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,   temporal_blend_factor			) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC4,	 fs_blend_color					) \
	GLOBAL_UBO_VAR_LIST_DO( VEC4,	 fs_colorize					) \
	\
	GLOBAL_UBO_VAR_LIST_DO( UINT,	 current_frame_idx				) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 width							) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 height							) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 current_gpu_slice_width		) \
	\
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 medium							) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 time							) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 first_person_model				) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 environment_type				) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC3,	 sun_direction					) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 sun_solid_angle				) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC3,	 sun_tangent					) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 sun_tan_half_angle				) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC3,	 sun_bitangent					) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 sun_bounce_scale				) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC3,	 sun_color						) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 sun_cos_half_angle				) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC3,	 sun_direction_envmap			) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 sun_visible					) \
	\
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 unscaled_width					) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 unscaled_height				) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 prev_gpu_slice_width			) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 render_mode					) \
	\
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,   inv_width						) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,   inv_height						) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 prev_width						) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 prev_height					) \
	\
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 cluster_debug_index			) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 screen_image_width				) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 screen_image_height			) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 pt_env_scale					) \
	\
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 pt_swap_checkerboard			) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 shadow_map_depth_scale			) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 god_rays_intensity				) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 god_rays_eccentricity			) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC2,	 pad0							) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 tonemap_hdr_clamp_strength		) \
	GLOBAL_UBO_VAR_LIST_DO( INT,	 num_dyn_lights					) \
	\
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 num_static_lights				) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 taa_image_width				) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 taa_image_height				) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 taa_output_width				) \
	\
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 taa_output_height				) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 prev_taa_output_width			) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 prev_taa_output_height			) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,	 bloom_intensity				) \
	\
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,   sky_transmittance				) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,   sky_phase_g					) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,   sky_amb_phase_g				) \
	GLOBAL_UBO_VAR_LIST_DO( FLOAT,   sky_scattering					) \
	\
	GLOBAL_UBO_VAR_LIST_DO( VEC3,	 physical_sky_ground_radiance	) \
	GLOBAL_UBO_VAR_LIST_DO( INT	,	 physical_sky_flags				) \
	\
	UBO_CVAR_LIST // WARNING: Do not put any other members into global_ubo after this: the CVAR list is not vec4-aligned

#define INSTANCE_BUFFER_VAR_LIST \
	INSTANCE_BUFFER_VAR_LIST_DO( INT,				model_indices				[SHADER_MAX_ENTITIES + SHADER_MAX_BSP_ENTITIES]) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				model_current_to_prev		[SHADER_MAX_ENTITIES]		) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				model_prev_to_current		[SHADER_MAX_ENTITIES]		) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				world_current_to_prev		[SHADER_MAX_BSP_ENTITIES]	) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				world_prev_to_current		[SHADER_MAX_BSP_ENTITIES]	) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				bsp_prim_offset				[SHADER_MAX_BSP_ENTITIES]	) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				model_idx_offset			[SHADER_MAX_ENTITIES]		) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				model_cluster_id			[SHADER_MAX_ENTITIES]		) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				model_cluster_id_prev		[SHADER_MAX_ENTITIES]		) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				bsp_cluster_id				[SHADER_MAX_BSP_ENTITIES]	) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				bsp_cluster_id_prev			[SHADER_MAX_BSP_ENTITIES]	) \
	INSTANCE_BUFFER_VAR_LIST_DO( MODELINSTANCE,		model_instances				[SHADER_MAX_ENTITIES]		) \
	INSTANCE_BUFFER_VAR_LIST_DO( MODELINSTANCE,		model_instances_prev		[SHADER_MAX_ENTITIES]		) \
	INSTANCE_BUFFER_VAR_LIST_DO( BSPMESHINSTANCE,	bsp_mesh_instances			[SHADER_MAX_BSP_ENTITIES]	) \
	INSTANCE_BUFFER_VAR_LIST_DO( BSPMESHINSTANCE,	bsp_mesh_instances_prev		[SHADER_MAX_BSP_ENTITIES]	) \
	/* stores the offset into the instance buffer in numberof primitives */ \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				model_instance_buf_offset	[SHADER_MAX_ENTITIES]		) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				model_instance_buf_size		[SHADER_MAX_ENTITIES]		) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				bsp_instance_buf_offset		[SHADER_MAX_BSP_ENTITIES]	) \
	INSTANCE_BUFFER_VAR_LIST_DO( UINT,				bsp_instance_buf_size		[SHADER_MAX_BSP_ENTITIES]	) \
	INSTANCE_BUFFER_VAR_LIST_DO( BONESREF,			model_mdxm_bones			[SHADER_MAX_ENTITIES]	) \

// shared structures between GLSL and C
#define UBO_CVAR_DO( name, default_value ) GLOBAL_UBO_VAR_LIST_DO( FLOAT, name )
#define GLOBAL_UBO_VAR_LIST_DO( type, name ) type(name)
#define INSTANCE_BUFFER_VAR_LIST_DO( type, name ) type(name)

#ifdef GLSL
    #define STRUCT(content, name) struct name { content };
    #define BOOL(n)		bool n;
    #define INT(n)		int n;
    #define UINT(n)		uint n;
    #define FLOAT(n)	float n;
    #define VEC2(n)		vec2 n;
    #define VEC3(n)		vec3 n;
    #define VEC4(n)		vec4 n;
    #define MAT4(n)		mat4 n;
    #define MAT4X3(n)	mat4x3 n;
    #define MAT3X4(n)	mat3x4 n;
    #define UVEC2(n)	uvec2 n;
	#define IVEC3(n)	ivec3 n;
	#define IVEC4(n)	ivec4 n;
	#define uint32_t	uint
	#define MAX_LIGHT_STYLES		64
	#define BONESREF(n)	mat3x4 n[72];	
#else
    #define STRUCT(content, name) typedef struct { content } name;
    #define BOOL(n)		unsigned int n;
    #define INT(n)		int n;
    #define UINT(n)		unsigned int n;
    #define FLOAT(n)	float n;
    #define VEC2(n)		float n[2];
    #define VEC3(n)		float n[3];
    #define VEC4(n)		float n[4];
    #define MAT4(n)		float n[16];
    #define MAT4X3(n)	float n[12];
    #define MAT3X4(n)	float n[12];
    #define UVEC2(n)	unsigned int[2] n;
	#define IVEC3(n)	int n[3];
	#define IVEC4(n)	int n[4];
	#define BONESREF(n)	mat3x4_t n[72];	
#endif

#define MODEL_DYNAMIC_VERTEX_BUFFER_LIST \
	VERTEX_BUFFER_LIST_DO( float,	 3,					positions_instanced,		(MAX_VERT_MODEL) ) \
	VERTEX_BUFFER_LIST_DO( float,    3,					pos_prev_instanced,			(MAX_VERT_MODEL) ) \
	VERTEX_BUFFER_LIST_DO( uint32_t, 1,					normals_instanced,			(MAX_VERT_MODEL) ) \
	VERTEX_BUFFER_LIST_DO( uint32_t, 1,					tangents_instanced,			(MAX_VERT_MODEL) ) \
	VERTEX_BUFFER_LIST_DO( float,    2,					tex_coords_instanced,		(MAX_VERT_MODEL) ) \
	VERTEX_BUFFER_LIST_DO( float,    1,					alpha_instanced,			(MAX_PRIM_MODEL) ) \
	VERTEX_BUFFER_LIST_DO( uint32_t, 1,					clusters_instanced,			(MAX_PRIM_MODEL) ) \
	VERTEX_BUFFER_LIST_DO( uint32_t, 1,					materials_instanced,		(MAX_PRIM_MODEL) ) \
	VERTEX_BUFFER_LIST_DO( uint32_t, 1,					instance_id_instanced,		(MAX_PRIM_MODEL) ) \

#define LIGHT_BUFFER_LIST \
	VERTEX_BUFFER_LIST_DO( uint32_t, MATERIAL_UINTS,	material_table,				(MAX_PBR_MATERIALS)		) \
	VERTEX_BUFFER_LIST_DO( float,    4,					light_polys,				(MAX_LIGHT_POLYS * LIGHT_POLY_VEC4S) ) \
	VERTEX_BUFFER_LIST_DO( uint32_t, 1,					light_list_offsets,			(MAX_LIGHT_LISTS     )	) \
	VERTEX_BUFFER_LIST_DO( uint32_t, 1,					light_list_lights,			(MAX_LIGHT_LIST_NODES)	) \
	VERTEX_BUFFER_LIST_DO( float,    1,					light_styles,				(MAX_LIGHT_STYLES    )	) \
	VERTEX_BUFFER_LIST_DO( uint32_t, 1,					cluster_debug_mask,			(MAX_LIGHT_LISTS / 32)	) \

// sample_light_counts: light count in cluster used for sampling (may differ from actual light count!)

#define VERTEX_BUFFER_LIST_DO( type, dim, name, size ) \
	type name[ ALIGN_SIZE_4( size, dim ) ];

struct ModelDynamicVertexBuffer
{
	MODEL_DYNAMIC_VERTEX_BUFFER_LIST
};

struct LightBuffer
{
	LIGHT_BUFFER_LIST
};

STRUCT ( 
	VEC3	( center )
	FLOAT	( radius )
	VEC3	( color )
	UINT	( type )			// Combines type (sphere vs spot) and "style" of light (eg spotlight emission profile)
	VEC3	( spot_direction )
	/* spot_data depends on spotlight emssion profile:
	 * DYNLIGHT_SPOT_EMISSION_PROFILE_FALLOFF -> contains packed2x16 with cosTotalWidth, cosFalloffStart
	 * DYNLIGHT_SPOT_EMISSION_PROFILE_AXIS_ANGLE_TEXTURE -> contains a half with cosTotalWidth and the texture index
	 */
	UINT	( spot_data )
, DynLightData )
#define DYNLIGHTDATA(n) DynLightData n;

#undef VERTEX_BUFFER_LIST_DO

#ifdef GLSL
struct LightPolygon
{
	mat3 positions;
	vec3 color;
	float light_style_scale;
	float prev_style_scale;
};
#else
typedef struct {
	vec3_t position;
	vec3_t normal;
	vec2_t texcoord;
	vec4_t tangents;
} model_vertex_t;
#endif

STRUCT (  
	MAT4	( M )

	UINT	( material )
	INT		( offset_curr )
	INT		( offset_prev )
	FLOAT	( backlerp )

	FLOAT	( alpha )
	INT		( idx_offset )
	INT		( model_index )
	INT		( pad )
, ModelInstance )
#define MODELINSTANCE(n) ModelInstance n;
	
STRUCT (  
	MAT4	( M )
	INT		( frame )
	FLOAT	( padding[3] )
, BspMeshInstance )
#define BSPMESHINSTANCE(n) BspMeshInstance n;

#ifdef GLSL
struct TextureData {
	int tex0;
	int tex1;
	uint tex0Blend;
	uint tex1Blend;
	bool tex0Color;
	bool tex1Color;
};
#endif

STRUCT (  
	UINT	( base_texture )
	UINT	( normals_texture )
	UINT	( emissive_texture )
	UINT	( physical_texture )
	UINT	( mask_texture )
	//FLOAT	( bump_scale )
	//FLOAT	( roughness_override )
	//FLOAT	( metalness_factor )
	//FLOAT	( specular_factor )
	VEC4	( specular_scale )	// contains the commented above see stage->specularScale
	FLOAT	( emissive_factor )
	FLOAT	( base_factor )
	FLOAT	( light_style_scale )
	UINT	( num_frames )
	UINT	( next_frame )
, MaterialInfo )

STRUCT (  
	INT		( accumulator[HISTOGRAM_BINS] )
	FLOAT	( curve[HISTOGRAM_BINS] )
	FLOAT	( normalized[HISTOGRAM_BINS] )
	FLOAT	( adapted_luminance )
	FLOAT	( tonecurve )
, ToneMappingBuffer )

STRUCT ( 
	UINT	( material )
	UINT	( cluster )
	FLOAT	( sun_luminance )
	FLOAT	( sky_luminance )
	VEC3	( hdr_color )
	FLOAT	( adapted_luminance )
, ReadbackBuffer )

STRUCT ( 
	IVEC3	( accum_sun_color )
	INT		( pad0 )
	IVEC4	( accum_sky_color )
	VEC3	( sun_color )
	FLOAT	( sun_luminance )
	VEC3	( sky_color )
	FLOAT	( sky_luminance )
, SunColorBuffer )

// holds material/offset/etc data for each AS Instance
STRUCT ( 
    MAT4    ( modelmat )
    UINT    ( currInstanceID )
    UINT    ( prevInstanceID )
    UINT    ( type )
	UINT    ( offsetIDX )
	UINT    ( offsetXYZ )
    UINT    ( texIdx0 )
    UINT    ( texIdx1 )
	BOOL    ( isBrushModel )
	BOOL    ( isPlayer )
	UINT    ( cluster )
    UINT    ( buff0 )
    UINT    ( buff1 )
, ASInstanceData )

// holds all bsp vertex data
STRUCT ( 
    VEC3    ( pos )
    UINT    ( material )

	// using packed qtangent instead, this can be removed or commented for now
    //VEC4    ( normal )
    VEC4    ( qtangent )
    UINT    ( color[4] )

    VEC2    ( uv[4] )

    UINT	( texIdx0 )
    UINT    ( texIdx1 )
    INT     ( cluster )
    UINT    ( buff2 )
,VertexBuffer )

// global ubo
STRUCT ( 
	GLOBAL_UBO_VAR_LIST
, vkUniformRTX_t )

// model instance
STRUCT ( 
	INSTANCE_BUFFER_VAR_LIST
, vkInstanceRTX_t )

#undef UBO_CVAR_DO

#undef STRUCT
#undef BOOL
#undef UINT
#undef FLOAT
#undef VEC2
#undef VEC3
#undef VEC4

#ifdef GLSL
// bindings
layout( binding = GLOBAL_UBO_BINDING_IDX, set = 3 ) uniform UBO { vkUniformRTX_t global_ubo; };
layout( binding = GLOBAL_INSTANCE_BUFFER_BINDING_IDX, set = 3 ) readonly buffer InstanceUBO { vkInstanceRTX_t instance_buffer; };
#endif

#endif /*_GLOBAL_UBO_DESCRIPTOR_SET_LAYOUT_H_*/