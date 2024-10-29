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

#define IMG_WIDTH		            ( vk.extent_screen_images.width )
#define IMG_HEIGHT		            ( vk.extent_screen_images.height )
#define IMG_WIDTH_MGPU              ( vk.extent_screen_images.width / vk.device_count )    
#define IMG_WIDTH_UNSCALED			( vk.extent_unscaled.width )
#define IMG_HEIGHT_UNSCALED			( vk.extent_unscaled.height )

#define IMG_WIDTH_GRAD	            ( ( vk.extent_screen_images.width	+ GRAD_DWN - 1) / GRAD_DWN )
#define IMG_HEIGHT_GRAD	            ( ( vk.extent_screen_images.height	+ GRAD_DWN - 1) / GRAD_DWN )
#define IMG_WIDTH_GRAD_MGPU         ( ( vk.extent_screen_images.width	+ GRAD_DWN - 1) / GRAD_DWN / vk.device_count )    // only rendering from one device

#define IMG_WIDTH_TAA				( vk.extent_taa_images.width )
#define IMG_HEIGHT_TAA				( vk.extent_taa_images.height )

/* These are images that are to be used as render targets and buffers, but not textures. */
// binding = BINDING_OFFSET_IMAGES + _binding
#define LIST_IMAGES \
	IMG_DO(PT_MOTION,                  0, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_TRANSPARENT,             1, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_HF,        2, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PING_LF_SH,    3, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_ATROUS_PONG_LF_SH,    4, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_ATROUS_PING_LF_COCG,  5, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_ATROUS_PONG_LF_COCG,  6, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_ATROUS_PING_HF,       7, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PONG_HF,       8, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PING_SPEC,     9, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PONG_SPEC,    10, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PING_MOMENTS, 11, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PONG_MOMENTS, 12, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_COLOR,               13, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_GRAD_LF_PING,        14, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_LF_PONG,        15, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_HF_SPEC_PING,   16, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_HF_SPEC_PONG,   17, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(PT_SHADING_POSITION,       18, R32G32B32A32_SFLOAT, rgba32f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(FLAT_COLOR,                19, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(FLAT_MOTION,               20, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_GODRAYS_THROUGHPUT_DIST,21, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(BLOOM_HBLUR,               22, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_TAA / 4,   IMG_HEIGHT_TAA / 4 ) \
	IMG_DO(BLOOM_VBLUR,               23, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_TAA / 4,   IMG_HEIGHT_TAA / 4 ) \
	IMG_DO(TAA_OUTPUT,                24, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_TAA,       IMG_HEIGHT_TAA ) \
	IMG_DO(PT_VIEW_DIRECTION,         25, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_VIEW_DIRECTION2,        26, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_THROUGHPUT,             27, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_BOUNCE_THROUGHPUT,      28, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(HQ_COLOR_INTERLEAVED,      29, R32G32B32A32_SFLOAT, rgba32f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_COLOR_LF_SH,            30, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_COLOR_LF_COCG,          31, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_COLOR_HF,               32, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_COLOR_SPEC,             33, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_GEO_NORMAL2,            34, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(FSR_EASU_OUTPUT,           35, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(FSR_RCAS_OUTPUT,           36, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \

#define RTX_IMG_NUM_STATIC 37

#define LIST_IMAGES_A_B \
	IMG_DO( PT_VISBUF_PRIM_A,			  RTX_IMG_NUM_STATIC + 0,  R32G32_UINT,	rg32ui, IMG_WIDTH_MGPU,      IMG_HEIGHT ) \
	IMG_DO( PT_VISBUF_PRIM_B,			  RTX_IMG_NUM_STATIC + 1,  R32G32_UINT,	rg32ui, IMG_WIDTH_MGPU,      IMG_HEIGHT ) \
	IMG_DO(PT_VISBUF_BARY_A,          RTX_IMG_NUM_STATIC + 2,  R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_VISBUF_BARY_B,          RTX_IMG_NUM_STATIC + 3,  R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_CLUSTER_A,              RTX_IMG_NUM_STATIC + 4,  R16_UINT,            r16ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_CLUSTER_B,              RTX_IMG_NUM_STATIC + 5,  R16_UINT,            r16ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_BASE_COLOR_A,           RTX_IMG_NUM_STATIC + 6,  R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_BASE_COLOR_B,           RTX_IMG_NUM_STATIC + 7,  R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_METALLIC_A,             RTX_IMG_NUM_STATIC + 8,  R8G8_UNORM,          rg8,     IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_METALLIC_B,             RTX_IMG_NUM_STATIC + 9,  R8G8_UNORM,          rg8,     IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_VIEW_DEPTH_A,           RTX_IMG_NUM_STATIC + 10, R16_SFLOAT,          r32f,    IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_VIEW_DEPTH_B,           RTX_IMG_NUM_STATIC + 11, R16_SFLOAT,          r32f,    IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_NORMAL_A,               RTX_IMG_NUM_STATIC + 12, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_NORMAL_B,               RTX_IMG_NUM_STATIC + 13, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_GEO_NORMAL_A,           RTX_IMG_NUM_STATIC + 14, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_GEO_NORMAL_B,           RTX_IMG_NUM_STATIC + 15, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_FILTERED_SPEC_A,     RTX_IMG_NUM_STATIC + 16, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_FILTERED_SPEC_B,     RTX_IMG_NUM_STATIC + 17, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_MOMENTS_HF_A,   RTX_IMG_NUM_STATIC + 18, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_MOMENTS_HF_B,   RTX_IMG_NUM_STATIC + 19, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_TAA_A,               RTX_IMG_NUM_STATIC + 20, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_TAA,       IMG_HEIGHT_TAA ) \
	IMG_DO(ASVGF_TAA_B,               RTX_IMG_NUM_STATIC + 21, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_TAA,       IMG_HEIGHT_TAA ) \
	IMG_DO(ASVGF_RNG_SEED_A,          RTX_IMG_NUM_STATIC + 22, R32_UINT,            r32ui,   IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_RNG_SEED_B,          RTX_IMG_NUM_STATIC + 23, R32_UINT,            r32ui,   IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_SH_A,  RTX_IMG_NUM_STATIC + 24, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_SH_B,  RTX_IMG_NUM_STATIC + 25, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_COCG_A,RTX_IMG_NUM_STATIC + 26, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_COCG_B,RTX_IMG_NUM_STATIC + 27, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_GRAD_SMPL_POS_A,     RTX_IMG_NUM_STATIC + 28, R32_UINT,            r32ui,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_SMPL_POS_B,     RTX_IMG_NUM_STATIC + 29, R32_UINT,            r32ui,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO( PT_TANGENT_A,			  RTX_IMG_NUM_STATIC + 30, R32_UINT,			r32ui,	 IMG_WIDTH_MGPU,      IMG_HEIGHT ) \
	IMG_DO( PT_TANGENT_B,			  RTX_IMG_NUM_STATIC + 31, R32_UINT,			r32ui,	 IMG_WIDTH_MGPU,      IMG_HEIGHT ) \

#define LIST_IMAGES_B_A \
	IMG_DO( PT_VISBUF_PRIM_B,			  RTX_IMG_NUM_STATIC + 0,  R32G32_UINT,	rg32ui, IMG_WIDTH_MGPU,      IMG_HEIGHT ) \
	IMG_DO( PT_VISBUF_PRIM_A,			  RTX_IMG_NUM_STATIC + 1,  R32G32_UINT,	rg32ui, IMG_WIDTH_MGPU,      IMG_HEIGHT ) \
	IMG_DO(PT_VISBUF_BARY_B,          RTX_IMG_NUM_STATIC + 2,  R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_VISBUF_BARY_A,          RTX_IMG_NUM_STATIC + 3,  R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_CLUSTER_B,              RTX_IMG_NUM_STATIC + 4,  R16_UINT,            r16ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_CLUSTER_A,              RTX_IMG_NUM_STATIC + 5,  R16_UINT,            r16ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_BASE_COLOR_B,           RTX_IMG_NUM_STATIC + 6,  R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_BASE_COLOR_A,           RTX_IMG_NUM_STATIC + 7,  R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_METALLIC_B,             RTX_IMG_NUM_STATIC + 8,  R8G8_UNORM,          rg8,     IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_METALLIC_A,             RTX_IMG_NUM_STATIC + 9,  R8G8_UNORM,          rg8,     IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_VIEW_DEPTH_B,           RTX_IMG_NUM_STATIC + 10, R16_SFLOAT,          r32f,    IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_VIEW_DEPTH_A,           RTX_IMG_NUM_STATIC + 11, R16_SFLOAT,          r32f,    IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_NORMAL_B,               RTX_IMG_NUM_STATIC + 12, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_NORMAL_A,               RTX_IMG_NUM_STATIC + 13, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_GEO_NORMAL_B,           RTX_IMG_NUM_STATIC + 14, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_GEO_NORMAL_A,           RTX_IMG_NUM_STATIC + 15, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_FILTERED_SPEC_B,     RTX_IMG_NUM_STATIC + 16, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_FILTERED_SPEC_A,     RTX_IMG_NUM_STATIC + 17, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_MOMENTS_HF_B,   RTX_IMG_NUM_STATIC + 18, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_MOMENTS_HF_A,   RTX_IMG_NUM_STATIC + 19, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_TAA_B,               RTX_IMG_NUM_STATIC + 20, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_TAA,       IMG_HEIGHT_TAA ) \
	IMG_DO(ASVGF_TAA_A,               RTX_IMG_NUM_STATIC + 21, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_TAA,       IMG_HEIGHT_TAA ) \
	IMG_DO(ASVGF_RNG_SEED_B,          RTX_IMG_NUM_STATIC + 22, R32_UINT,            r32ui,   IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_RNG_SEED_A,          RTX_IMG_NUM_STATIC + 23, R32_UINT,            r32ui,   IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_SH_B,  RTX_IMG_NUM_STATIC + 24, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_SH_A,  RTX_IMG_NUM_STATIC + 25, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_COCG_B,RTX_IMG_NUM_STATIC + 26, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_COCG_A,RTX_IMG_NUM_STATIC + 27, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_GRAD_SMPL_POS_B,     RTX_IMG_NUM_STATIC + 28, R32_UINT,            r32ui,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_SMPL_POS_A,     RTX_IMG_NUM_STATIC + 29, R32_UINT,            r32ui,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO( PT_TANGENT_B,			  RTX_IMG_NUM_STATIC + 30, R32_UINT,			r32ui,	 IMG_WIDTH_MGPU,      IMG_HEIGHT ) \
	IMG_DO( PT_TANGENT_A,			  RTX_IMG_NUM_STATIC + 31, R32_UINT,			r32ui,	 IMG_WIDTH_MGPU,      IMG_HEIGHT ) \

#define NUM_RTX_IMAGES (RTX_IMG_NUM_STATIC + 32 ) /* this really sucks but I don't know how to fix it
													 counting with enum does not work in GLSL */

#define GLOBAL_TEXTURES_TEX_ARR_BINDING_IDX  0
#define BINDING_OFFSET_IMAGES				(1 + GLOBAL_TEXTURES_TEX_ARR_BINDING_IDX)
#define BINDING_OFFSET_TEXTURES				(BINDING_OFFSET_IMAGES				+ NUM_RTX_IMAGES)
#define BINDING_OFFSET_BLUE_NOISE			(BINDING_OFFSET_TEXTURES			+ NUM_RTX_IMAGES)
#define BINDING_OFFSET_ENVMAP				(BINDING_OFFSET_BLUE_NOISE			+ 1)
#define BINDING_OFFSET_PHYSICAL_SKY			(BINDING_OFFSET_ENVMAP				+ 1)
#define BINDING_OFFSET_PHYSICAL_SKY_IMG		(BINDING_OFFSET_PHYSICAL_SKY		+ 1)
#define BINDING_OFFSET_SKY_TRANSMITTANCE	(BINDING_OFFSET_PHYSICAL_SKY_IMG	+ 1)
#define BINDING_OFFSET_SKY_SCATTERING		(BINDING_OFFSET_SKY_TRANSMITTANCE	+ 1)
#define BINDING_OFFSET_SKY_IRRADIANCE		(BINDING_OFFSET_SKY_SCATTERING		+ 1)
#define BINDING_OFFSET_SKY_CLOUDS			(BINDING_OFFSET_SKY_IRRADIANCE		+ 1)
#define BINDING_OFFSET_TERRAIN_ALBEDO		(BINDING_OFFSET_SKY_CLOUDS			+ 1)
#define BINDING_OFFSET_TERRAIN_NORMALS		(BINDING_OFFSET_TERRAIN_ALBEDO		+ 1)
#define BINDING_OFFSET_TERRAIN_DEPTH		(BINDING_OFFSET_TERRAIN_NORMALS		+ 1)
#define BINDING_OFFSET_TERRAIN_SHADOWMAP	(BINDING_OFFSET_TERRAIN_DEPTH		+ 1)
#define NUM_PHYSICAL_SKY_IMAGES				10

#ifndef GLSL
enum VK_RTX_IMAGES {
#define IMG_DO(_name, ...) \
	RTX_IMG_##_name,
	LIST_IMAGES
	LIST_IMAGES_A_B
#undef IMG_DO
	NUM_VKPT_IMAGES
};
#endif

#ifdef GLSL

// global texture
layout( set = 2, binding = 0 ) uniform sampler2D texure_array[];

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

/* framebuffer images */
#define IMG_DO(_name, _binding, _vkformat, _glslformat, _w, _h) \
	layout(set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_IMAGES + _binding, _glslformat) \
	uniform IMAGE_##_glslformat IMG_##_name;
LIST_IMAGES
LIST_IMAGES_A_B
#undef IMG_DO

/* framebuffer textures */
#define IMG_DO(_name, _binding, _vkformat, _glslformat, _w, _h) \
	layout(set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_TEXTURES + _binding) \
	uniform SAMPLER_##_glslformat TEX_##_name;
LIST_IMAGES
LIST_IMAGES_A_B
#undef IMG_DO

// blue noise
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_BLUE_NOISE		 ) uniform sampler2DArray TEX_BLUE_NOISE;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_ENVMAP			 ) uniform samplerCube TEX_ENVMAP;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_PHYSICAL_SKY		 ) uniform samplerCube TEX_PHYSICAL_SKY;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_PHYSICAL_SKY_IMG, rgba16f ) uniform imageCube IMG_PHYSICAL_SKY;

// precomputed_, sky begin
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_SKY_TRANSMITTANCE  ) uniform sampler2D TEX_SKY_TRANSMITTANCE;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_SKY_SCATTERING	 ) uniform sampler3D TEX_SKY_SCATTERING;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_SKY_IRRADIANCE	 ) uniform sampler2D TEX_SKY_IRRADIANCE;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_SKY_CLOUDS		 ) uniform sampler3D TEX_SKY_CLOUDS;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_TERRAIN_ALBEDO	 ) uniform samplerCube IMG_TERRAIN_ALBEDO;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_TERRAIN_NORMALS	 ) uniform samplerCube IMG_TERRAIN_NORMALS;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_TERRAIN_DEPTH		 ) uniform samplerCube IMG_TERRAIN_DEPTH;
layout( set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_TERRAIN_SHADOWMAP	 ) uniform sampler2D TEX_TERRAIN_SHADOWMAP;
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