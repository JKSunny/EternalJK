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

#ifndef _VERTEX_BUFFER_H_
#define _VERTEX_BUFFER_H_

#include "shader_structs.h"

// lights
#define MAX_LIGHT_LISTS         (1 << 14)
#define MAX_LIGHT_LIST_NODES    (1 << 19)
//#define LIGHT_COUNT_HISTORY	16
#define LIGHT_COUNT_HISTORY     3 // one for previous frame rendered, one for current frame rendering, one for upload

#define MAX_LIGHT_POLYS         4096
#define LIGHT_POLY_VEC4S        4
#define MATERIAL_UINTS			5

// should match the same constant declared in material.h
#define MAX_PBR_MATERIALS		4096

#define ALIGN_SIZE_4(x, n)  ((x * n + 3) & (~3))

// BINDING OFFSETS
// top level acceleration structure
#define RAY_GEN_DESCRIPTOR_SET_IDX							0

// world static
#define BINDING_OFFSET_XYZ_WORLD_STATIC                     0
#define BINDING_OFFSET_IDX_WORLD_STATIC                     1

// world dynamic data
#define BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA		        2
#define BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA_PREV          3
#define BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA		        4
#define BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA_PREV          5

// world dynamic as
#define BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS		            6
#define BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS_PREV            7
#define BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS		            8
#define BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS_PREV            9

// cluster data
#define BINDING_OFFSET_CLUSTER_WORLD_STATIC                 10
#define BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_DATA           11
#define BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_AS             12

// readback
#define BINDING_OFFSET_READBACK_BUFFER						13

// dynamic vertex
#define BINDING_OFFSET_DYNAMIC_VERTEX						14

// light and material
#define BINDING_OFFSET_LIGHT_BUFFER							15
#define BINDING_LIGHT_COUNTS_HISTORY_BUFFER					16

// tonemap
#define BINDING_OFFSET_TONEMAP_BUFFER						17

// sky
#define BINDING_OFFSET_SUN_COLOR_BUFFER						18
#define BINDING_OFFSET_SUN_COLOR_UBO						19

// light stats
#define BINDING_OFFSET_LIGHT_STATS_BUFFER					20

#define BINDING_OFFSET_XYZ_SKY								21
#define BINDING_OFFSET_IDX_SKY								22
#define BINDING_OFFSET_CLUSTER_SKY                			23

// world submodels
#define BINDING_OFFSET_XYZ_WORLD_SUBMODEL                   24
#define BINDING_OFFSET_IDX_WORLD_SUBMODEL                   25

#define SUN_COLOR_ACCUMULATOR_FIXED_POINT_SCALE 0x100000
#define SKY_COLOR_ACCUMULATOR_FIXED_POINT_SCALE 0x100

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

// sample_light_counts: light count in cluster used for sampling (may differ from actual light count!)

#define VERTEX_BUFFER_LIST_DO( type, dim, name, size ) \
type name[ ALIGN_SIZE_4( size, dim ) ];

struct ModelDynamicVertexBuffer
{
	MODEL_DYNAMIC_VERTEX_BUFFER_LIST
};

STRUCT ( 
	UINT	( material_table[MAX_PBR_MATERIALS * MATERIAL_UINTS] )
	VEC4	( light_polys[MAX_LIGHT_POLYS * LIGHT_POLY_VEC4S] )
	UINT	( light_list_offsets[MAX_LIGHT_LISTS] )
	UINT	( light_list_lights[MAX_LIGHT_LIST_NODES] )
	FLOAT	( light_styles[MAX_LIGHT_STYLES] )
	UINT	( cluster_debug_mask[MAX_LIGHT_LISTS / 32] )
	UINT	( sky_visibility[MAX_LIGHT_LISTS / 32] )
, LightBuffer )
// UINT	( sky_visibility[MAX_LIGHT_LISTS / 32] )

#undef VERTEX_BUFFER_LIST_DO

#ifndef USE_RTX_GLOBAL_MODEL_VBO
typedef struct {
	vec3_t position;
	vec3_t normal;
	vec2_t texcoord;
	vec4_t tangents;
} model_vertex_t;
#endif

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

// holds all bsp vertex data
STRUCT ( 
    VEC3    ( pos )
    UINT    ( material )

	// using packed qtangent instead, this can be removed or commented for now
    VEC4    ( normal )
    VEC4    ( qtangent )
    UINT    ( color[4] )

    VEC2    ( uv[4] )

    UINT	( texIdx0 )
    UINT    ( texIdx1 )
    INT     ( cluster )
    UINT    ( buff2 )
,VertexBuffer )


#ifdef GLSL

#ifdef VERTEX_READONLY
#define BUFFER_T readonly buffer
#else
#define BUFFER_T buffer
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

struct LightPolygon
{
	/* Meaning of positions depends on light type:
	 * - static/poly/triangle light: actual positions of vertices
	 * - dynamic light:
	 *   - all types:
	 *       positions[0]: light origin
	 *       positions[1].x: radius
	 *   - spot light:
	 *       positions[1].y: emission profile (uint reinterepreted as float)
	 *       positions[1].z: spot light data, meaning depending on emission profile:
	 *         DYNLIGHT_SPOT_EMISSION_PROFILE_FALLOFF -> contains packed2x16 with cosTotalWidth, cosFalloffStart
	 *         DYNLIGHT_SPOT_EMISSION_PROFILE_AXIS_ANGLE_TEXTURE -> contains a half with cosTotalWidth and the texture index
	 *       positions[2]: direction
	 */
	mat3 positions;
	vec3 color;
	float light_style_scale;
	float prev_style_scale;
	float type;
};

struct TextureData {
	int tex0;
	int tex1;
	uint tex0Blend;
	uint tex1Blend;
	bool tex0Color;
	bool tex1Color;
};


// Buffer with indices and vertices
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_IDX_SKY )					BUFFER_T Indices_Sky_static { uint i[]; } indices_sky_static;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_XYZ_SKY )					BUFFER_T Vertices_Sky_static { VertexBuffer v[]; } vertices_sky_static;

layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_IDX_WORLD_STATIC )			BUFFER_T Indices_World_static { uint i[]; } indices_world_static;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_XYZ_WORLD_STATIC )			BUFFER_T Vertices_World_static { VertexBuffer v[]; } vertices_world_static;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA )		BUFFER_T Indices_dynamic_data { uint i[]; } indices_dynamic_data;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA )		BUFFER_T Vertices_dynamic_data { VertexBuffer v[]; } vertices_dynamic_data;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS )		BUFFER_T Indices_dynamic_as { uint i[]; } indices_dynamic_as;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS )		BUFFER_T Vertices_dynamic_as { VertexBuffer v[]; } vertices_dynamic_as;

layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_CLUSTER_SKY )				BUFFER_T Cluster_Sky_static { uint c[]; } cluster_sky_static;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_CLUSTER_WORLD_STATIC )		BUFFER_T Cluster_World_static { uint c[]; } cluster_world_static;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_DATA )	BUFFER_T Cluster_World_dynamic_data { uint c[]; } cluster_world_dynamic_data;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_AS )	BUFFER_T Cluster_World_dynamic_as { uint c[]; } cluster_world_dynamic_as;

//prev
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA_PREV ) BUFFER_T Indices_dynamic_data_prev { uint i[]; } indices_dynamic_data_prev;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA_PREV ) BUFFER_T Vertices_dynamic_data_prev { VertexBuffer v[]; } vertices_dynamic_data_prev;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS_PREV )	BUFFER_T Indices_dynamic_as_prev { uint i[]; } indices_dynamic_as_prev;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS_PREV )	BUFFER_T Vertices_dynamic_as_prev { VertexBuffer v[]; } vertices_dynamic_as_prev;

// world submodels
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_IDX_WORLD_SUBMODEL )			BUFFER_T Indices_World_submodel { uint i[]; } indices_world_submodel;
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_XYZ_WORLD_SUBMODEL )			BUFFER_T Vertices_World_submodel { VertexBuffer v[]; } vertices_world_submodel;

/* History of light count in cluster, used for sampling.
 * This is used to make gradient estimation work correctly:
 * "The A-SVGF algorithm uses old random numbers to select lights for a subset of pixels,
 * and expects to get the same result if the lighting didn't change."
 * (quoted from discussion on GH PR 227).
 * One way to achieve this is to also keep a history of light numbers and use that for
 * sampling "old" data.
 *
 * We have multiple buffers b/c we may need to access the history for the current or any previous frame.
 * That'd be harder to do with a light_buffer member since that is backed by alternating buffers */
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_LIGHT_COUNTS_HISTORY_BUFFER ) readonly buffer LIGHT_COUNTS_HISTORY_BUFFER {
	uint sample_light_counts[];
} light_counts_history[LIGHT_COUNT_HISTORY];

layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_TONEMAP_BUFFER )				buffer TONE_MAPPING_BUFFER { ToneMappingBuffer tonemap_buffer; };
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_SUN_COLOR_BUFFER )			buffer SUN_COLOR_BUFFER { SunColorBuffer sun_color_buffer; };
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_SUN_COLOR_UBO, std140 )		uniform SUN_COLOR_UBO { SunColorBuffer sun_color_ubo; };
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_LIGHT_STATS_BUFFER )			buffer LIGHT_STATS_BUFFERS { uint stats[]; } light_stats_bufers[3];
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_READBACK_BUFFER )			buffer READBACK_BUFFER { ReadbackBuffer readback; };
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_DYNAMIC_VERTEX )				buffer MODEL_DYNAMIC_VERTEX_BUFFER { ModelDynamicVertexBuffer vbo_model_dynamic; };
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_LIGHT_BUFFER )				buffer LIGHT_BUFFER { LightBuffer light_buffer; };

#undef BUFFER_T

#define GET_float_1(buf,name) \
float \
get_##name(uint idx) \
{ \
	return buf.name[idx]; \
}

#define GET_float_2(buf,name) \
vec2 \
get_##name(uint idx) \
{ \
	return vec2(buf.name[idx * 2 + 0], buf.name[idx * 2 + 1]); \
}

#define GET_float_3(buf,name) \
vec3 \
get_##name(uint idx) \
{ \
	return vec3(buf.name[idx * 3 + 0], buf.name[idx * 3 + 1], buf.name[idx * 3 + 2]); \
}

#define GET_float_4(buf,name) \
vec4 \
get_##name(uint idx) \
{ \
	return vec4(buf.name[idx * 4 + 0], buf.name[idx * 4 + 1], buf.name[idx * 4 + 2], buf.name[idx * 4 + 3]); \
}

#define GET_uint32_t_1(buf,name) \
uint \
get_##name(uint idx) \
{ \
	return buf.name[idx]; \
}

#define GET_uint32_t_3(buf,name) \
uvec3 \
get_##name(uint idx) \
{ \
	return uvec3(buf.name[idx * 3 + 0], buf.name[idx * 3 + 1], buf.name[idx * 3 + 2]); \
}

#define GET_uint32_t_4(buf,name) \
uvec4 \
get_##name(uint idx) \
{ \
	return uvec4(buf.name[idx * 4 + 0], buf.name[idx * 4 + 1], buf.name[idx * 4 + 2], buf.name[idx * 4 + 3]); \
}

#ifndef VERTEX_READONLY
#define SET_float_1(buf,name) \
void \
set_##name(uint idx, float v) \
{ \
	buf.name[idx] = v; \
}

#define SET_float_2(buf,name) \
void \
set_##name(uint idx, vec2 v) \
{ \
	buf.name[idx * 2 + 0] = v[0]; \
	buf.name[idx * 2 + 1] = v[1]; \
}

#define SET_float_3(buf,name) \
void \
set_##name(uint idx, vec3 v) \
{ \
	buf.name[idx * 3 + 0] = v[0]; \
	buf.name[idx * 3 + 1] = v[1]; \
	buf.name[idx * 3 + 2] = v[2]; \
}

#define SET_float_4(buf,name) \
void \
set_##name(uint idx, vec4 v) \
{ \
	buf.name[idx * 4 + 0] = v[0]; \
	buf.name[idx * 4 + 1] = v[1]; \
	buf.name[idx * 4 + 2] = v[2]; \
	buf.name[idx * 4 + 3] = v[3]; \
}

#define SET_uint32_t_1(buf,name) \
void \
set_##name(uint idx, uint u) \
{ \
	buf.name[idx] = u; \
}

#define SET_uint32_t_3(buf,name) \
void \
set_##name(uint idx, uvec3 v) \
{ \
	buf.name[idx * 3 + 0] = v[0]; \
	buf.name[idx * 3 + 1] = v[1]; \
	buf.name[idx * 3 + 2] = v[2]; \
}
#endif

#ifdef VERTEX_READONLY
	#define VERTEX_BUFFER_LIST_DO(type, dim, name, size) \
		GET_##type##_##dim(vbo_model_dynamic,name)
	MODEL_DYNAMIC_VERTEX_BUFFER_LIST
	#undef VERTEX_BUFFER_LIST_DO
#else
	#define VERTEX_BUFFER_LIST_DO(type, dim, name, size) \
		GET_##type##_##dim(vbo_model_dynamic,name) \
		SET_##type##_##dim(vbo_model_dynamic,name)
	MODEL_DYNAMIC_VERTEX_BUFFER_LIST
	#undef VERTEX_BUFFER_LIST_DO
#endif

struct Triangle {
	mat3 positions;		// mat3x3
	mat3 positions_prev;// mat3x3
	mat3x2 tex_coords0;
	mat3x2 tex_coords1;
	mat3x2 tex_coords2;
	mat3x2 tex_coords3;
	mat3x3 normals;
	mat3x3 tangents;
	//mat3x3 binormal;
	mat3x4 color0;
	mat3x4 color1;
	mat3x4 color2;
	mat3x4 color3;
	uint tex0;
	uint tex1;
	uint cluster;
	uint material_id;
	float  emissive_factor;
	float  alpha;
	//float  alpha;
};

Triangle
get_instanced_triangle( uint prim_id )
{
	Triangle t;
	t.positions[0] = get_positions_instanced(prim_id * 3 + 0);
	t.positions[1] = get_positions_instanced(prim_id * 3 + 1);
	t.positions[2] = get_positions_instanced(prim_id * 3 + 2);

	t.positions_prev[0] = get_pos_prev_instanced(prim_id * 3 + 0);
	t.positions_prev[1] = get_pos_prev_instanced(prim_id * 3 + 1);
	t.positions_prev[2] = get_pos_prev_instanced(prim_id * 3 + 2);

	t.normals[0] = decode_normal(get_normals_instanced(prim_id * 3 + 0));
	t.normals[1] = decode_normal(get_normals_instanced(prim_id * 3 + 1));
	t.normals[2] = decode_normal(get_normals_instanced(prim_id * 3 + 2));

	t.tangents[0] = decode_normal(get_tangents_instanced(prim_id * 3 + 0));
	t.tangents[1] = decode_normal(get_tangents_instanced(prim_id * 3 + 1));
	t.tangents[2] = decode_normal(get_tangents_instanced(prim_id * 3 + 2));

	t.tex_coords0[0] = get_tex_coords_instanced(prim_id * 3 + 0);
	t.tex_coords0[1] = get_tex_coords_instanced(prim_id * 3 + 1);
	t.tex_coords0[2] = get_tex_coords_instanced(prim_id * 3 + 2);

	t.material_id = get_materials_instanced(prim_id);

	t.cluster = get_clusters_instanced(prim_id);
		
	t.alpha = get_alpha_instanced(prim_id);
	
	t.emissive_factor = 1.0;

	return t;
}

#ifndef VERTEX_READONLY
void
store_instanced_triangle(Triangle t, uint instance_id, uint prim_id)
{
	set_positions_instanced(prim_id * 3 + 0, t.positions[0]);
	set_positions_instanced(prim_id * 3 + 1, t.positions[1]);
	set_positions_instanced(prim_id * 3 + 2, t.positions[2]);

	set_pos_prev_instanced(prim_id * 3 + 0, t.positions_prev[0]);
	set_pos_prev_instanced(prim_id * 3 + 1, t.positions_prev[1]);
	set_pos_prev_instanced(prim_id * 3 + 2, t.positions_prev[2]);

	set_normals_instanced(prim_id * 3 + 0, encode_normal(t.normals[0]));
	set_normals_instanced(prim_id * 3 + 1, encode_normal(t.normals[1]));
	set_normals_instanced(prim_id * 3 + 2, encode_normal(t.normals[2]));

	set_tangents_instanced(prim_id * 3 + 0, encode_normal(t.tangents[0]));
	set_tangents_instanced(prim_id * 3 + 1, encode_normal(t.tangents[1]));
	set_tangents_instanced(prim_id * 3 + 2, encode_normal(t.tangents[2]));

	set_tex_coords_instanced(prim_id * 3 + 0, t.tex_coords0[0]);
	set_tex_coords_instanced(prim_id * 3 + 1, t.tex_coords0[1]);
	set_tex_coords_instanced(prim_id * 3 + 2, t.tex_coords0[2]);

	set_materials_instanced(prim_id, t.material_id);

	set_instance_id_instanced(prim_id, instance_id);

	set_clusters_instanced(prim_id, t.cluster);

	set_alpha_instanced(prim_id, t.alpha);
}
#endif

uint get_material_uint( in uint material_index, in uint offset ) {
	return light_buffer.material_table[nonuniformEXT(material_index * MATERIAL_UINTS + offset)];
	//return lbo.material_table[nonuniformEXT(idx)] & 0xffff;
	//return (lbo.material_table[nonuniformEXT(idx)] + 0) >> 16;
}

TextureData unpackTextureData(in uint data){
	TextureData d;
	d.tex0 = int(data);
	
	//d.tex1 = int((data & TEX1_IDX_MASK) >> TEX_SHIFT_BITS);
	d.tex1 = TEX0_IDX_MASK;
	
	if(d.tex0 == TEX0_IDX_MASK) 
		d.tex0 = -1;
	if(d.tex1 == TEX0_IDX_MASK) 
		d.tex1 = -1;
#if 0
	d.tex0Blend = (data & TEX0_BLEND_MASK);
	d.tex1Blend = (data & TEX1_BLEND_MASK);
	d.tex0Color = (data & TEX0_COLOR_MASK) != 0;
	d.tex1Color = (data & TEX1_COLOR_MASK) != 0;
#else
	d.tex0Blend = TEX0_NORMAL_BLEND_MASK;
	d.tex1Blend = TEX1_NORMAL_BLEND_MASK;
	d.tex0Color = false;
	d.tex1Color = false;	
#endif
	return d;
}

MaterialInfo get_material_info( uint material_id ) 
{
	uint material_index = material_id & MATERIAL_INDEX_MASK;

	uint data[MATERIAL_UINTS];

	uint remappedIndex = get_material_uint( material_index, 4 );

	if ( remappedIndex > 0 )
		material_index = remappedIndex;

	data[0] = get_material_uint( material_index, 0 );
	data[1] = get_material_uint( material_index, 1 );
	data[2] = get_material_uint( material_index, 2 );
	data[3] = get_material_uint( material_index, 3 );

	MaterialInfo minfo;
	minfo.base_texture		= data[0] & 0xffff;	// albedo
	minfo.emissive_texture	= data[0] >> 16;	// emissive/glow
	minfo.normals_texture	= data[1] & 0xffff;	// normalmap
	minfo.physical_texture	= data[1] >> 16;	// rmo (physical)

	// swizzles to zwxy
	// keep it for compat with non rtx pbr implementation (readability)
	minfo.specular_scale[0] = unpackHalf2x16(data[2]).x;
	minfo.specular_scale[1] = unpackHalf2x16(data[2]).y;
	minfo.specular_scale[2] = unpackHalf2x16(data[3]).x;
	minfo.specular_scale[3] = unpackHalf2x16(data[3]).y;

	minfo.emissive_factor 	= 1.0f;
	minfo.base_factor		= 1.0f;

	return minfo;
}

LightPolygon
get_light_polygon(uint index)
{
	vec4 p0 = vec4( light_buffer.light_polys[ index * LIGHT_POLY_VEC4S + 0 ] );
	vec4 p1 = vec4( light_buffer.light_polys[ index * LIGHT_POLY_VEC4S + 1 ] );
	vec4 p2 = vec4( light_buffer.light_polys[ index * LIGHT_POLY_VEC4S + 2 ] );
	vec4 p3 = vec4( light_buffer.light_polys[ index * LIGHT_POLY_VEC4S + 3 ] );

	LightPolygon light;

	light.positions			= mat3x3( p0.xyz, p1.xyz, p2.xyz );
	light.color				= vec3( p0.w, p1.w, p2.w );
	light.light_style_scale	= p3.x;
	light.prev_style_scale	= p3.y;
	light.type = p3.z;

	return light;
}

vec4 unpackColor(in uint color) {
	return vec4(
		color & 0xff,
		(color & (0xff << 8)) >> 8,
		(color & (0xff << 16)) >> 16,
		(color & (0xff << 24)) >> 24
	);
}

ivec3 get_bsp_triangle_indices_and_data( in uint instance_type, in uint prim_id, out VertexBuffer triangle[3] )
{
	uint prim = prim_id * 3;
	ivec3 indices;

	if ( instance_type == AS_TYPE_WORLD_STATIC )
	{
		indices = ivec3(	
			indices_world_static.i[prim + 0], 
			indices_world_static.i[prim + 1], 
			indices_world_static.i[prim + 2] );

		triangle[0] = vertices_world_static.v[indices.x];
		triangle[1] = vertices_world_static.v[indices.y];
		triangle[2] = vertices_world_static.v[indices.z];
	}
	
	else if ( instance_type == AS_TYPE_SKY )
	{
		indices = ivec3(	
			indices_sky_static.i[prim + 0], 
			indices_sky_static.i[prim + 1], 
			indices_sky_static.i[prim + 2] );

		triangle[0] = vertices_sky_static.v[indices.x];
		triangle[1] = vertices_sky_static.v[indices.y];
		triangle[2] = vertices_sky_static.v[indices.z];
	}
	
	else if ( instance_type == AS_TYPE_WORLD_DYNAMIC_MATERIAL )
	{
		indices = ivec3(	
			indices_dynamic_data.i[prim + 0], 
			indices_dynamic_data.i[prim + 1], 
			indices_dynamic_data.i[prim + 2] );
		
		triangle[0] = vertices_dynamic_data.v[indices.x];
		triangle[1] = vertices_dynamic_data.v[indices.y];
		triangle[2] = vertices_dynamic_data.v[indices.z];
	}
	
	else if ( instance_type == AS_TYPE_WORLD_DYNAMIC_GEOMETRY )
	{
		indices = ivec3(	
			indices_dynamic_as.i[prim + 0], 
			indices_dynamic_as.i[prim + 1], 
			indices_dynamic_as.i[prim + 2] );

		triangle[0] = vertices_dynamic_as.v[indices.x];
		triangle[1] = vertices_dynamic_as.v[indices.y];
		triangle[2] = vertices_dynamic_as.v[indices.z];
	}
	
	else if ( instance_type == AS_TYPE_WORLD_SUBMODEL )
	{
		indices = ivec3(	
			indices_world_submodel.i[prim + 0], 
			indices_world_submodel.i[prim + 1], 
			indices_world_submodel.i[prim + 2] );

		triangle[0] = vertices_world_submodel.v[indices.x];
		triangle[1] = vertices_world_submodel.v[indices.y];
		triangle[2] = vertices_world_submodel.v[indices.z];
	}
	
	
	return indices;
}

mat3x3 get_dynamic_bsp_positions_prev( in uint instance_type, in uint prim_id  )
{
	uint prim = prim_id * 3;
	mat3x3 position;

	if ( instance_type == AS_TYPE_WORLD_DYNAMIC_MATERIAL )
	{
		ivec3 indices = ivec3( 
			indices_dynamic_data_prev.i[prim + 0], 
			indices_dynamic_data_prev.i[prim + 1], 
			indices_dynamic_data_prev.i[prim + 2] );
		
		position[0] = vertices_dynamic_data_prev.v[indices.x].pos.xyz;
		position[1] = vertices_dynamic_data_prev.v[indices.y].pos.xyz;
		position[2] = vertices_dynamic_data_prev.v[indices.z].pos.xyz;
	}
	
	else if ( instance_type == AS_TYPE_WORLD_DYNAMIC_GEOMETRY )
	{
		ivec3 indices = ivec3(
			indices_dynamic_as_prev.i[prim + 0], 
			indices_dynamic_as_prev.i[prim + 1], 
			indices_dynamic_as_prev.i[prim + 2]);
		
		position[0] = vertices_dynamic_as_prev.v[indices.x].pos.xyz;
		position[1] = vertices_dynamic_as_prev.v[indices.y].pos.xyz;
		position[2] = vertices_dynamic_as_prev.v[indices.z].pos.xyz;
	}

	return position;
}

Triangle get_bsp_triangle( in uint instance_type, in uint prim_id )
{
	VertexBuffer triangle[3];
	ivec3 indices = get_bsp_triangle_indices_and_data( instance_type, prim_id, triangle );

	Triangle t;

	t.positions[0] = triangle[0].pos.xyz;
	t.positions[1] = triangle[1].pos.xyz;
	t.positions[2] = triangle[2].pos.xyz;
	

	if ( instance_type == AS_TYPE_WORLD_STATIC || instance_type == AS_TYPE_WORLD_SUBMODEL )
		t.positions_prev = t.positions;
	else 
		t.positions_prev = get_dynamic_bsp_positions_prev( instance_type, prim_id ); 

	t.color0[0] = unpackColor(triangle[0].color[0]);
	t.color0[1] = unpackColor(triangle[1].color[0]);
	t.color0[2] = unpackColor(triangle[2].color[0]);

	t.color1[0] = unpackColor(triangle[0].color[1]);
	t.color1[1] = unpackColor(triangle[1].color[1]);
	t.color1[2] = unpackColor(triangle[2].color[1]);

	t.color2[0] = unpackColor(triangle[0].color[2]);
	t.color2[1] = unpackColor(triangle[1].color[2]);
	t.color2[2] = unpackColor(triangle[2].color[2]);
	
	t.color3[0] = unpackColor(triangle[0].color[3]);
	t.color3[1] = unpackColor(triangle[1].color[3]);
	t.color3[2] = unpackColor(triangle[2].color[3]);

	t.tex_coords0[0] = triangle[0].uv[0].xy;
	t.tex_coords0[1] = triangle[1].uv[0].xy;
	t.tex_coords0[2] = triangle[2].uv[0].xy;

	t.tex_coords1[0] = triangle[0].uv[1].xy;
	t.tex_coords1[1] = triangle[1].uv[1].xy;
	t.tex_coords1[2] = triangle[2].uv[1].xy;

	t.tex_coords2[0] = triangle[0].uv[2].xy;
	t.tex_coords2[1] = triangle[1].uv[2].xy;
	t.tex_coords2[2] = triangle[2].uv[2].xy;

	t.tex_coords3[0] = triangle[0].uv[3].xy;
	t.tex_coords3[1] = triangle[1].uv[3].xy;
	t.tex_coords3[2] = triangle[2].uv[3].xy;

	t.normals[0] = triangle[0].normal.xyz;
	t.normals[1] = triangle[1].normal.xyz;
	t.normals[2] = triangle[2].normal.xyz;
	
	t.tangents[0] = triangle[0].qtangent.xyz;
	t.tangents[1] = triangle[1].qtangent.xyz;
	t.tangents[2] = triangle[2].qtangent.xyz;

	switch( instance_type )
	{
		case AS_TYPE_WORLD_STATIC:
			t.tex0 = vertices_world_static.v[indices.x].texIdx0;
			t.tex1 = vertices_world_static.v[indices.x].texIdx1;

			t.cluster = cluster_world_static.c[prim_id];
			t.material_id = vertices_world_static.v[indices.x].material;	// same as triangle[0].material ?
			break;
		case AS_TYPE_SKY:
			t.tex0 = vertices_sky_static.v[indices.x].texIdx0;
			t.tex1 = vertices_sky_static.v[indices.x].texIdx1;

			t.cluster = cluster_sky_static.c[prim_id];
			t.material_id = vertices_sky_static.v[indices.x].material;	// same as triangle[0].material ?
			break;
		case AS_TYPE_WORLD_DYNAMIC_MATERIAL:
			t.tex0 = vertices_dynamic_data.v[indices.x].texIdx0;
			t.tex1 = vertices_dynamic_data.v[indices.x].texIdx1;

			t.cluster = cluster_world_dynamic_data.c[prim_id];
			t.material_id = vertices_dynamic_data.v[indices.x].material;
			break;
		case AS_TYPE_WORLD_DYNAMIC_GEOMETRY:
			t.tex0 = vertices_dynamic_as.v[indices.x].texIdx0;
			t.tex1 = vertices_dynamic_as.v[indices.x].texIdx1;

			t.cluster = cluster_world_dynamic_as.c[prim_id];
			t.material_id = vertices_dynamic_as.v[indices.x].material;
			break;
		case AS_TYPE_WORLD_SUBMODEL:
			t.tex0 = vertices_world_submodel.v[indices.x].texIdx0;
			t.tex1 = vertices_world_submodel.v[indices.x].texIdx1;

			t.cluster = -1;
			t.material_id = vertices_world_submodel.v[indices.x].material;
			break;
		default:
			t.tex0 = 0;
			t.tex1 = 0;
			t.cluster = 0;
			t.material_id = 0;
	}

	t.emissive_factor = 1.0;
	t.alpha = 1.0;

	return t;
}
#endif
#endif