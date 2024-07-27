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

#ifndef  _TEXTURES_H_
#define  _TEXTURES_H_

#include "constants.h"

#define IMG_W		            ( vk.extent_screen_images.width )
#define IMG_H		            ( vk.extent_screen_images.height )
#define IMG_W_MGPU              ( vk.extent_screen_images.width / vk.device_count )    
#define IMG_W_UNSCALED			( vk.extent_unscaled.width )
#define IMG_H_UNSCALED			( vk.extent_unscaled.height )

#define IMG_W_GRAD	            ( ( vk.extent_screen_images.width	+ GRAD_DWN - 1) / GRAD_DWN )
#define IMG_H_GRAD	            ( ( vk.extent_screen_images.height	+ GRAD_DWN - 1) / GRAD_DWN )
#define IMG_W_GRAD_MGPU         ( ( vk.extent_screen_images.width	+ GRAD_DWN - 1) / GRAD_DWN / vk.device_count )    // only rendering from one device

#define IMG_W_TAA				( vk.extent_taa_images.width )
#define IMG_H_TAA				( vk.extent_taa_images.height )

/* These are images that are to be used as render targets and buffers, but not textures. */
// binding = BINDING_OFFSET_IMAGES + _binding
#define LIST_IMAGES \
	IMG_DO( PT_MOTION,						0,	R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W,           IMG_H      ) \
	IMG_DO( PT_TRANSPARENT,					1,	R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( ASVGF_HIST_COLOR_HF,			2,	R32_UINT,            	r32ui,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( ASVGF_ATROUS_PING_LF_SH,		3,	R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_GRAD_MGPU, IMG_H_GRAD ) \
	IMG_DO( ASVGF_ATROUS_PONG_LF_SH,		4,	R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_GRAD_MGPU, IMG_H_GRAD ) \
	IMG_DO( ASVGF_ATROUS_PING_LF_COCG,  	5,	R16G16_SFLOAT,       	rg16f,		IMG_W_GRAD_MGPU, IMG_H_GRAD ) \
	IMG_DO( ASVGF_ATROUS_PONG_LF_COCG,  	6,	R16G16_SFLOAT,       	rg16f,		IMG_W_GRAD_MGPU, IMG_H_GRAD ) \
	IMG_DO( ASVGF_ATROUS_PING_HF,       	7,	R32_UINT,            	r32ui,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( ASVGF_ATROUS_PONG_HF,       	8,	R32_UINT,            	r32ui,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( ASVGF_ATROUS_PING_SPEC,    		9, R32_UINT,            	r32ui,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( ASVGF_ATROUS_PONG_SPEC,    		10, R32_UINT,            	r32ui,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( ASVGF_ATROUS_PING_MOMENTS, 		11, R16G16_SFLOAT,       	rg16f,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( ASVGF_ATROUS_PONG_MOMENTS, 		12, R16G16_SFLOAT,       	rg16f,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( ASVGF_COLOR,               		13, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W,           IMG_H      ) \
	IMG_DO( ASVGF_GRAD_LF_PING,        		14, R16G16_SFLOAT,       	rg16f,		IMG_W_GRAD_MGPU, IMG_H_GRAD ) \
	IMG_DO( ASVGF_GRAD_LF_PONG,        		15, R16G16_SFLOAT,       	rg16f,		IMG_W_GRAD_MGPU, IMG_H_GRAD ) \
	IMG_DO( ASVGF_GRAD_HF_SPEC_PING,   		16, R16G16_SFLOAT,       	rg16f,		IMG_W_GRAD_MGPU, IMG_H_GRAD ) \
	IMG_DO( ASVGF_GRAD_HF_SPEC_PONG,   		17, R16G16_SFLOAT,       	rg16f,		IMG_W_GRAD_MGPU, IMG_H_GRAD ) \
	IMG_DO( PT_SHADING_POSITION,       		18, R32G32B32A32_SFLOAT, 	rgba32f,	IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( FLAT_COLOR,                		19, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W,           IMG_H      ) \
	IMG_DO( FLAT_MOTION,               		20, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W,           IMG_H      ) \
	IMG_DO( PT_GODRAYS_THROUGHPUT_DIST,		21, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( BLOOM_DOWNSCALE_MIP_1,     		22, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_TAA / 2,   IMG_H_TAA / 2  ) \
	IMG_DO( BLOOM_HBLUR,               		23, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_TAA / 4,   IMG_H_TAA / 4  ) \
	IMG_DO( BLOOM_VBLUR,               		24, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_TAA / 4,   IMG_H_TAA / 4  ) \
	IMG_DO( TAA_OUTPUT,                		25, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_TAA,       IMG_H_TAA  ) \
	IMG_DO( PT_VIEW_DIRECTION,         		26, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( PT_VIEW_DIRECTION2,        		27, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( PT_THROUGHPUT,             		28, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( PT_BOUNCE_THROUGHPUT,      		29, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( HQ_COLOR_INTERLEAVED,      		30, R32G32B32A32_SFLOAT, 	rgba32f,	IMG_W,           IMG_H      ) \
	IMG_DO( PT_COLOR_LF_SH,            		31, R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( PT_COLOR_LF_COCG,          		32, R16G16_SFLOAT,			rg16f,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( PT_COLOR_HF,               		33, R32_UINT,				r32ui,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( PT_COLOR_SPEC,             		34, R32_UINT,				r32ui,		IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( PT_GEO_NORMAL2,					35, R32_UINT,				r32ui,		IMG_W_MGPU,      IMG_H      ) \

#define RTX_IMG_NUM_STATIC 36

// binding (A) = BINDING_OFFSET_IMAGES + RTX_IMG_NUM_STATIC + _binding
// binding (B) = BINDING_OFFSET_IMAGES + RTX_IMG_NUM_STATIC + _binding + RTX_IMG_NUM_A_B
//#ifdef USE_RTX_INSPECT_TANGENTS
// if this isnt used, PT_TANGENT can be removed
//#endif

#define LIST_IMAGES_A_B \
	IMG_DO( PT_VISBUF,						0,	R32G32B32A32_SFLOAT,	rgba32f,	IMG_W_MGPU,      IMG_H ) \
	IMG_DO( PT_CLUSTER,				        1,	R16_UINT,				r16ui,		IMG_W_MGPU,      IMG_H ) \
	IMG_DO( PT_BASE_COLOR,					2,	R16G16B16A16_SFLOAT, 	rgba16f,	IMG_W_MGPU,      IMG_H      ) \
	IMG_DO( PT_METALLIC,					3,	R8G8_UNORM,				rg8,		IMG_W_MGPU,      IMG_H   ) \
	IMG_DO( PT_VIEW_DEPTH,					4,	R16_SFLOAT,				r32f,		IMG_W,			 IMG_H   ) \
	IMG_DO( PT_NORMAL,				        5,	R32_UINT,				r32ui,		IMG_W_MGPU,      IMG_H ) \
	IMG_DO( PT_GEO_NORMAL,				    6,	R32_UINT,				r32ui,		IMG_W_MGPU,      IMG_H ) \
    IMG_DO( ASVGF_FILTERED_SPEC,            7,	R16G16B16A16_SFLOAT,	rgba16f,	IMG_W_MGPU,      IMG_H ) \
	IMG_DO( ASVGF_HIST_MOMENTS_HF,          8,	R16G16B16A16_SFLOAT,	rgba16f,	IMG_W_MGPU,      IMG_H ) \
	IMG_DO( ASVGF_TAA,                      9,	R16G16B16A16_SFLOAT,	rgba16f,	IMG_W_TAA,       IMG_H_TAA ) \
	IMG_DO( ASVGF_RNG_SEED,                 10,	R32_UINT,				r32ui ,		IMG_W,           IMG_H  ) \
	IMG_DO( ASVGF_HIST_COLOR_LF_SH,         11,	R16G16B16A16_SFLOAT,	rgba16f,	IMG_W_MGPU,      IMG_H ) \
	IMG_DO( ASVGF_HIST_COLOR_LF_COCG,       12,	R16G16_SFLOAT,			rg16f,		IMG_W_MGPU,      IMG_H   ) \
	IMG_DO( ASVGF_GRAD_SMPL_POS,			13,	R32_UINT,			    r32ui,		IMG_W_GRAD_MGPU, IMG_H_GRAD   ) \
	IMG_DO( PT_TANGENT,				        14,	R32_UINT,				r32ui,		IMG_W_MGPU,      IMG_H ) \

#define RTX_IMG_NUM_A_B 15

#if 1
#define NUM_RTX_IMAGES RTX_IMG_NUM_STATIC + ( RTX_IMG_NUM_A_B * 2 )
//#define NUM_RTX_IMAGES 63  /* this really sucks but I don't know how to fix it
//                         counting with enum does not work in GLSL */
#else	
// jank ;)
#define IMG_DO( _handle, _binding, _format, _glsl_format, _w, _h ) + 1
const int           define_num_images = ( 0 LIST_IMAGES + ( ( 0 LIST_IMAGES_A_B ) * 2 ) );
#define NUM_RTX_IMAGES  define_num_images
#undef IMG_DO
#endif

#define BINDING_OFFSET_IMAGES				NUM_BUFFERS											// storage
#define BINDING_OFFSET_TEXTURES				( BINDING_OFFSET_IMAGES				+ NUM_RTX_IMAGES )  // sampler
#define BINDING_OFFSET_PHYSICAL_SKY			( BINDING_OFFSET_TEXTURES			+ NUM_RTX_IMAGES )
#define BINDING_OFFSET_PHYSICAL_SKY_IMG		( BINDING_OFFSET_PHYSICAL_SKY		+ 1 )
#define BINDING_OFFSET_SKY_TRANSMITTANCE	( BINDING_OFFSET_PHYSICAL_SKY_IMG	+ 1 )
#define BINDING_OFFSET_SKY_SCATTERING		( BINDING_OFFSET_SKY_TRANSMITTANCE	+ 1 )
#define BINDING_OFFSET_SKY_IRRADIANCE		( BINDING_OFFSET_SKY_SCATTERING		+ 1 )
#define BINDING_OFFSET_SKY_CLOUDS			( BINDING_OFFSET_SKY_IRRADIANCE		+ 1 )
#define BINDING_OFFSET_TERRAIN_ALBEDO		( BINDING_OFFSET_SKY_CLOUDS			+ 1 )
#define BINDING_OFFSET_TERRAIN_NORMALS		( BINDING_OFFSET_TERRAIN_ALBEDO		+ 1 )
#define BINDING_OFFSET_TERRAIN_DEPTH		( BINDING_OFFSET_TERRAIN_NORMALS	+ 1 )
#define BINDING_OFFSET_TERRAIN_SHADOWMAP	( BINDING_OFFSET_TERRAIN_DEPTH		+ 1 )

#define NUM_PHYSICAL_SKY_IMAGES 10

#ifndef GLSL
enum VK_RTX_IMAGES {
#define IMG_DO(_handle, ...) RTX_IMG_##_handle,
    LIST_IMAGES
#undef IMG_DO
#define IMG_DO(_handle, ...) RTX_IMG_##_handle##_A,
    LIST_IMAGES_A_B
#undef IMG_DO
#define IMG_DO(_handle, ...) RTX_IMG_##_handle##_B,
    LIST_IMAGES_A_B
#undef IMG_DO
};
#endif

#ifdef GLSL
#define SAMPLER_r16ui   usampler2D
#define SAMPLER_r32ui   usampler2D
#define SAMPLER_rg32ui  usampler2D
#define SAMPLER_r32i    isampler2D
#define SAMPLER_r32f    sampler2D
#define SAMPLER_rg32f   sampler2D
#define SAMPLER_rg16f   sampler2D
#define SAMPLER_rgba32f sampler2D
#define SAMPLER_rgba16f sampler2D
#define SAMPLER_rgba8   sampler2D
#define SAMPLER_r8      sampler2D
#define SAMPLER_rg8     sampler2D

#define IMAGE_r16ui   uimage2D
#define IMAGE_r32ui   uimage2D
#define IMAGE_rg32ui  uimage2D
#define IMAGE_r32i    iimage2D
#define IMAGE_r32f    image2D
#define IMAGE_rg32f   image2D
#define IMAGE_rg16f   image2D
#define IMAGE_rgba32f image2D
#define IMAGE_rgba16f image2D
#define IMAGE_rgba8   image2D
#define IMAGE_r8      image2D
#define IMAGE_rg8     image2D

// storage images and sampelrs
#define IMG_DO( _handle, _binding, _format, _glsl_format, _w, _h ) \
	layout( set = 0, binding = BINDING_OFFSET_IMAGES    + _binding, _glsl_format )  uniform IMAGE_##_glsl_format     IMG_##_handle; \
    /* next is *optional for rtx, rtx and compute descriptors differ, try define guard in glsl::constants.h to not bind sampler and B images */ \
	layout( set = 0, binding = BINDING_OFFSET_TEXTURES  + _binding )                uniform SAMPLER_##_glsl_format   TEX_##_handle;
    LIST_IMAGES
#undef IMG_DO

// A_B
#define IMG_DO( _handle, _binding, _format, _glsl_format, _w, _h ) \
	layout( set = 0, binding = BINDING_OFFSET_IMAGES    + RTX_IMG_NUM_STATIC + _binding,     _glsl_format )					uniform IMAGE_##_glsl_format     IMG_##_handle##_A; \
    /* next is *optional for rtx, rtx and compute descriptors differ, try define guard in glsl::constants.h to not bind sampler and B images */ \
	layout( set = 0, binding = BINDING_OFFSET_IMAGES    + RTX_IMG_NUM_STATIC + _binding + RTX_IMG_NUM_A_B, _glsl_format )	uniform IMAGE_##_glsl_format     IMG_##_handle##_B; \
	layout( set = 0, binding = BINDING_OFFSET_TEXTURES  + RTX_IMG_NUM_STATIC + _binding      )								uniform SAMPLER_##_glsl_format   TEX_##_handle##_A; \
	layout( set = 0, binding = BINDING_OFFSET_TEXTURES  + RTX_IMG_NUM_STATIC + _binding + RTX_IMG_NUM_A_B  )				uniform SAMPLER_##_glsl_format   TEX_##_handle##_B;
    LIST_IMAGES_A_B
#undef IMG_DO

// global texture
layout( binding = 0, set = 1 ) uniform sampler2D texure_array[];

// blue noise
layout( binding = BINDING_OFFSET_BLUE_NOISE,		set = 0 ) uniform sampler2DArray TEX_BLUE_NOISE;
layout( binding = BINDING_OFFSET_ENVMAP,			set = 0 ) uniform samplerCube TEX_ENVMAP;
layout( binding = BINDING_OFFSET_PHYSICAL_SKY,		set = 0 ) uniform samplerCube TEX_PHYSICAL_SKY;
layout( binding = BINDING_OFFSET_PHYSICAL_SKY_IMG,	set = 0, rgba16f ) uniform imageCube IMG_PHYSICAL_SKY;

// precomputed_sky begin
layout( binding = BINDING_OFFSET_SKY_TRANSMITTANCE, set = 0 ) uniform sampler2D TEX_SKY_TRANSMITTANCE;
layout( binding = BINDING_OFFSET_SKY_SCATTERING,	set = 0 ) uniform sampler3D TEX_SKY_SCATTERING;
layout( binding = BINDING_OFFSET_SKY_IRRADIANCE,	set = 0 ) uniform sampler2D TEX_SKY_IRRADIANCE;
layout( binding = BINDING_OFFSET_SKY_CLOUDS,		set = 0 ) uniform sampler3D TEX_SKY_CLOUDS;
layout( binding = BINDING_OFFSET_TERRAIN_ALBEDO,	set = 0 ) uniform samplerCube IMG_TERRAIN_ALBEDO;
layout( binding = BINDING_OFFSET_TERRAIN_NORMALS,	set = 0 ) uniform samplerCube IMG_TERRAIN_NORMALS;
layout( binding = BINDING_OFFSET_TERRAIN_DEPTH,		set = 0 ) uniform samplerCube IMG_TERRAIN_DEPTH;
layout( binding = BINDING_OFFSET_TERRAIN_SHADOWMAP,	set = 0 ) uniform sampler2D TEX_TERRAIN_SHADOWMAP;
// precomputed_sky end

vec4 global_texture( in uint idx, in vec2 tex_coord )
{
	return	texture( texure_array[idx], tex_coord );
}

vec4
global_textureLod(uint idx, vec2 tex_coord, float lod)
{
	return textureLod(texure_array[nonuniformEXT(idx)], tex_coord, lod);
}

vec4
global_textureGrad(uint idx, vec2 tex_coord, vec2 d_x, vec2 d_y)
{
	return textureGrad(texure_array[nonuniformEXT(idx)], tex_coord, d_x, d_y);
}

ivec2
global_textureSize(uint idx, int level)
{
	return textureSize(texure_array[nonuniformEXT(idx)], level);
}

#endif

#endif /*_TEXTURES_H_*/
// vim: shiftwidth=4 noexpandtab tabstop=4 cindent