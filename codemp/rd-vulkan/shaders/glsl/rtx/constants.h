/*
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

#ifndef  _CONSTANTS_H_
#define  _CONSTANTS_H_

// sunny
#define USE_RTX_GLOBAL_MODEL_VBO
#ifdef USE_VK_IMGUI
#define USE_RTX_INSPECT_TANGENTS
#endif

#define ALIGN_SIZE_4(x, n)  ((x * n + 3) & (~3))

//
#define BAS_DEFAULT                                 (0)
#define BAS_WORLD_STATIC                            (1)
#define BAS_WORLD_DYNAMIC_DATA                      (2)
#define BAS_WORLD_DYNAMIC_AS                        (3)
#define BAS_ENTITY_STATIC                           (4)
#define BAS_ENTITY_DYNAMIC                          (5)

#define AS_INSTANCE_FLAG_DYNAMIC					(1 << 23)
#define AS_INSTANCE_FLAG_SKY						(1 << 22)
#define AS_INSTANCE_MASK_OFFSET (AS_INSTANCE_FLAG_SKY - 1)

#define GRAD_DWN (3)

#define SHADOWMAP_SIZE 4096

#define HISTOGRAM_BINS 128

#define EMISSIVE_TRANSFORM_BIAS -0.001

#define MAX_MIRROR_ROUGHNESS 0.02

#define NUM_BLUE_NOISE_TEX							(128 * 4)
#define BLUE_NOISE_RES								(256)

#define NUM_LIGHT_STATS_BUFFERS 3

#define PRIMARY_RAY_T_MAX 10000

#define AA_MODE_OFF 0
#define AA_MODE_TAA 1
#define AA_MODE_UPSCALE 2

// Scaling factors for lighting components when they are stored in textures.
// FP16 and RGBE textures have very limited range, and these factors help bring the signal within that range.
#define STORAGE_SCALE_LF 1024
#define STORAGE_SCALE_HF 32
#define STORAGE_SCALE_SPEC 32
#define STORAGE_SCALE_HDR 128

#define MAX_RTX_STAGES 4

// texture
#define TEXTURE_DEFAULT 							0x00000000
#define TEXTURE_ADD 								0x00000001
#define TEXTURE_MUL 								0x00000002

// cullMask
#define RAY_FIRST_PERSON_OPAQUE_VISIBLE 			0x00000001
#define RAY_MIRROR_OPAQUE_VISIBLE 					0x00000002
#define RAY_FIRST_PERSON_MIRROR_OPAQUE_VISIBLE 		0x00000003

#define RAY_FIRST_PERSON_PARTICLE_VISIBLE 			0x00000004
#define RAY_MIRROR_PARTICLE_VISIBLE 				0x00000008
#define RAY_FIRST_PERSON_MIRROR_PARTICLE_VISIBLE 	0x0000000c

#define RAY_GLASS_VISIBLE 	                        0x00000010

#define MATERIAL_KIND_MASK							0xf0000000
#define MATERIAL_KIND_INVALID						0x00000000
#define MATERIAL_KIND_REGULAR						0x10000000
#define MATERIAL_KIND_CHROME						0x20000000
#define MATERIAL_KIND_WATER							0x30000000
#define MATERIAL_KIND_LAVA							0x40000000
#define MATERIAL_KIND_SLIME							0x50000000
#define MATERIAL_KIND_GLASS							0x60000000
#define MATERIAL_KIND_SKY							0x70000000
#define MATERIAL_KIND_INVISIBLE						0x80000000
#define MATERIAL_KIND_EXPLOSION						0x90000000
#define MATERIAL_KIND_TRANSPARENT					0xa0000000
#define MATERIAL_KIND_SCREEN						0xb0000000
#define MATERIAL_KIND_CAMERA						0xc0000000
#define MATERIAL_KIND_CHROME_MODEL					0xd0000000

#define MATERIAL_FLAG_MASK          				0x000ffff0
#define MATERIAL_FLAG_LIGHT 						0x00000010
#define MATERIAL_FLAG_TRANSPARENT  					0x00000020
#define MATERIAL_FLAG_SEE_THROUGH  					0x00000040
#define MATERIAL_FLAG_MIRROR 						0x00000080
#define MATERIAL_FLAG_NEEDSCOLOR 					0x00000100
#define MATERIAL_FLAG_CORRECT_ALBEDO 				0x00000200
#define MATERIAL_FLAG_PORTAL						0x00000400
#define MATERIAL_FLAG_BULLET_MARK					0x00000800
#define MATERIAL_FLAG_PLAYER_OR_WEAPON			    0x00001000
#define MATERIAL_FLAG_SEE_THROUGH_ADD  				0x00002000
#define MATERIAL_FLAG_SEE_THROUGH_NO_ALPHA  	    0x00004000
#define MATERIAL_FLAG_IGNORE_LUMINANCE 				0x00008000

// Q2RTX
#define CHECKERBOARD_FLAG_PRIMARY    1
#define CHECKERBOARD_FLAG_REFLECTION 2
#define CHECKERBOARD_FLAG_REFRACTION 4

#define MEDIUM_NONE  0
#define MEDIUM_WATER 1
#define MEDIUM_SLIME 2
#define MEDIUM_LAVA  3
#define MEDIUM_GLASS 4

#define ENVIRONMENT_NONE		0
#define ENVIRONMENT_STATIC		1
#define ENVIRONMENT_DYNAMIC		2

#define MAX_LIGHT_SOURCES        32
#define MAX_LIGHT_STYLES         64

// Variables that have "_lf", "_hf" or "_spec" suffix apply to the low-frequency, high-frequency or specular lighting channels, respectively.

// BINDING OFFSETS
// top level acceleration structure
#define BINDING_OFFSET_AS							        0

// world static
#define BINDING_OFFSET_XYZ_WORLD_STATIC                     1
#define BINDING_OFFSET_IDX_WORLD_STATIC                     2

// world dynamic data
#define BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA		        3
#define BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA_PREV          4
#define BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA		        5
#define BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA_PREV          6

// world dynamic as
#define BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS		            7
#define BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS_PREV            8
#define BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS		            9
#define BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS_PREV            10

// cluster data
#define BINDING_OFFSET_CLUSTER_WORLD_STATIC                 11
#define BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_DATA           12
#define BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_AS             13

// readback
#define BINDING_OFFSET_READBACK_BUFFER						14

// dynamic vertex
#define BINDING_OFFSET_DYNAMIC_VERTEX						15

// light and material
#define BINDING_OFFSET_LIGHT_BUFFER							16
#define BINDING_LIGHT_COUNTS_HISTORY_BUFFER					17

// tonemap
#define BINDING_OFFSET_TONEMAP_BUFFER						18

// sky
#define BINDING_OFFSET_SUN_COLOR_BUFFER						19

// light stats
#define BINDING_OFFSET_LIGHT_STATS_BUFFER					20

// precomputed sky
#define BINDING_OFFSET_PRECOMPUTED_SKY_UBO					21

#define NUM_BUFFERS											22	// used as offset for storage images and textures

// shader groups
#define SBT_RGEN_PRIMARY_RAYS				0
#define SBT_RGEN_REFLECT_REFRACT1			1
#define SBT_RGEN_REFLECT_REFRACT2			2
#define SBT_RGEN_DIRECT_LIGHTING			3
#define SBT_RGEN_DIRECT_LIGHTING_CAUSTICS	4
#define SBT_RGEN_INDIRECT_LIGHTING_FIRST	5
#define SBT_RGEN_INDIRECT_LIGHTING_SECOND	6
#define SBT_RMISS_PATH_TRACER				7
#define SBT_RMISS_SHADOW					8
#define SBT_RCHIT_OPAQUE					9
#define SBT_RCHIT_EMPTY						10

#define UINT_MAX                                    0xffffffff
#define UINT_TOP_16BITS_MASK                        0xffff0000
#define UINT_BOTTOM_16BITS_MASK                     0x0000ffff

#define TEX_SHIFT_BITS                              (16)
#define TEX0_IDX_MASK                               0x000001ff
#define TEX1_IDX_MASK                               0x01ff0000

#define TEX0_COLOR_MASK                             0x00000200
#define TEX1_COLOR_MASK                             0x02000000
#define TEX0_NORMAL_BLEND_MASK                      0x00000400
#define TEX0_MUL_BLEND_MASK                         0x00000800
#define TEX0_ADD_BLEND_MASK                         0x00001000
#define TEX1_NORMAL_BLEND_MASK                      0x04000000
#define TEX1_MUL_BLEND_MASK                         0x08000000
#define TEX1_ADD_BLEND_MASK                         0x10000000
#define TEX0_BLEND_MASK                             0x00001C00
#define TEX1_BLEND_MASK                             0x1C000000


#define MAX_VERT_MODEL          (1 << 23)
#define MAX_IDX_MODEL           (1 << 22)
#define MAX_PRIM_MODEL          (MAX_IDX_MODEL / 3)

#define SHADER_MAX_ENTITIES		1024
#define SHADER_MAX_BSP_ENTITIES	128

// lights
#define MAX_LIGHT_LISTS         (1 << 14)
#define MAX_LIGHT_LIST_NODES    (1 << 19)
#define LIGHT_COUNT_HISTORY 16

#define MAX_LIGHT_POLYS         4096
#define LIGHT_POLY_VEC4S        4

// should match the same constant declared in material.h
#define MATERIAL_INDEX_MASK		0x00000fff	// just 4095 material indexes? can cause issues some day?

#define MATERIAL_UINTS			5
#define MAX_PBR_MATERIALS		4096

#ifdef GLSL
	#ifndef M_PI
		#define M_PI 3.14159265358979323846264338327950288f
	#endif
#endif

// Dynamic light types
#define DYNLIGHT_SPHERE         0
#define DYNLIGHT_SPOT           1

//
// Spotlight styles (emission profiles)
//
// spotlight emission profile is smooth falloff between two angle values
#define DYNLIGHT_SPOT_EMISSION_PROFILE_FALLOFF              0
// spotlight emission profile given by an 1D texture, indexed by the cosine of the angle from the axis
#define DYNLIGHT_SPOT_EMISSION_PROFILE_AXIS_ANGLE_TEXTURE   1

#endif // _CONSTANTS_H_