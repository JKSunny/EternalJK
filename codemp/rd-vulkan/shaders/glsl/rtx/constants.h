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

// blas instance type/id
#define AS_TYPE_DEFAULT                                 (0)
#define AS_TYPE_WORLD_STATIC                            (1)
#define AS_TYPE_WORLD_DYNAMIC_MATERIAL                      (2)
#define AS_TYPE_WORLD_DYNAMIC_GEOMETRY                        (3)
#define AS_TYPE_ENTITY_STATIC                           (4)
#define AS_TYPE_ENTITY_DYNAMIC                          (5)
#define AS_TYPE_SKY                                     (6)
#define AS_TYPE_WORLD_SUBMODEL                          (7)

#define AS_INSTANCE_FLAG_DYNAMIC					(1 << 23)
#define AS_INSTANCE_FLAG_SKY						(1 << 22)
#define AS_INSTANCE_MASK_OFFSET (AS_INSTANCE_FLAG_SKY - 1)

// cullMask
#define AS_FLAG_OPAQUE          (1 << 0)
#define AS_FLAG_TRANSPARENT     (1 << 1)
#define AS_FLAG_VIEWER_MODELS   (1 << 2)
#define AS_FLAG_VIEWER_WEAPON   (1 << 3)
#define AS_FLAG_SKY             (1 << 4)
#define AS_FLAG_CUSTOM_SKY      (1 << 5)

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
#define MATERIAL_KIND_TRANSP_MODEL					0xe0000000 // Transparent models. No distortion, just "see through".

#define MATERIAL_FLAG_MASK          				0x000ffff0
#define MATERIAL_FLAG_LIGHT 						0x00000010
#define MATERIAL_FLAG_TRANSPARENT  					0x00000020
#define MATERIAL_FLAG_SEE_THROUGH  					0x00000040
#define MATERIAL_FLAG_MIRROR 						0x00000080
#define MATERIAL_FLAG_NEEDSCOLOR 					0x00000100
#define MATERIAL_FLAG_CORRECT_ALBEDO 				0x00000200
#define MATERIAL_FLAG_PORTAL						0x00000400
#define MATERIAL_FLAG_BULLET_MARK					0x00000800
#define MATERIAL_FLAG_WEAPON			    		0x00001000
#define MATERIAL_FLAG_SEE_THROUGH_ADD  				0x00002000
#define MATERIAL_FLAG_SEE_THROUGH_NO_ALPHA  	    0x00004000
#define MATERIAL_FLAG_IGNORE_LUMINANCE 				0x00008000

#define MATERIAL_LIGHT_STYLE_MASK    0x0003f000
#define MATERIAL_LIGHT_STYLE_SHIFT   12
#define MATERIAL_INDEX_MASK			 0x00000fff	// just 4095 material indexes? can cause issues some day?

#define CHECKERBOARD_FLAG_PRIMARY    1
#define CHECKERBOARD_FLAG_REFLECTION 2
#define CHECKERBOARD_FLAG_REFRACTION 4

// Combines the PRIMARY, REFLECTION, REFRACTION fields
#define CHECKERBOARD_FLAG_FIELD_MASK 7 
// Not really a checkerboard flag, but it's stored in the same channel.
// Signals that the surface is a first-person weapon.
#define CHECKERBOARD_FLAG_WEAPON     8

#define MEDIUM_NONE  0
#define MEDIUM_WATER 1
#define MEDIUM_SLIME 2
#define MEDIUM_LAVA  3
#define MEDIUM_GLASS 4

#define ENVIRONMENT_NONE		0
#define ENVIRONMENT_STATIC		1
#define ENVIRONMENT_DYNAMIC		2

#define MAX_MODEL_INSTANCES      8192 // SHADER_MAX_ENTITIES/MAX_ENTITIESTOTAL + SHADER_MAX_BSP_ENTITIES? * (some number of geometries per model, usually 1)
#define MAX_RESERVED_INSTANCES   16   // TLAS instances reserved for skinned geometry, particles and the like
#define MAX_TLAS_INSTANCES       (MAX_MODEL_INSTANCES + MAX_RESERVED_INSTANCES)

#define MAX_LIGHT_SOURCES       32
#define MAX_LIGHT_STYLES        64
#define MAX_MODEL_LIGHTS		16384

// Variables that have "_lf", "_hf" or "_spec" suffix apply to the low-frequency, high-frequency or specular lighting channels, respectively.

// shader groups
#define SBT_RGEN				0
#define SBT_RMISS_EMPTY			1
#define SBT_RCHIT_GEOMETRY		2
#define SBT_ENTRIES_PER_PIPELINE 3

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

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

// Light types
#define LIGHT_POLYGON        0
#define LIGHT_SPHERE         1
#define LIGHT_SPOT           2

//
// Spotlight styles (emission profiles)
//
// spotlight emission profile is smooth falloff between two angle values
#define DYNLIGHT_SPOT_EMISSION_PROFILE_FALLOFF              0
// spotlight emission profile given by an 1D texture, indexed by the cosine of the angle from the axis
#define DYNLIGHT_SPOT_EMISSION_PROFILE_AXIS_ANGLE_TEXTURE   1

// Projection modes
#define PROJECTION_RECTILINEAR      0
#define PROJECTION_PANINI           1
#define PROJECTION_STEREOGRAPHIC    2
#define PROJECTION_CYLINDRICAL      3
#define PROJECTION_EQUIRECTANGULAR  4
#define PROJECTION_MERCATOR         5



// Projection constants
#define STEREOGRAPHIC_ANGLE			0.5
#define PANINI_D			        1.0		// 0.0 -> rectilinear, 1.0 -> cylindrical stereographic, +inf -> cylindrical orthographic

#endif // _CONSTANTS_H_