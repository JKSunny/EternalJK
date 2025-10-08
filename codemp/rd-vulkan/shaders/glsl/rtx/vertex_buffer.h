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

#define PRIMITIVE_BUFFER_BINDING_IDX						1
#define POSITION_BUFFER_BINDING_IDX							2

// readback
#define BINDING_OFFSET_READBACK_BUFFER						3

// dynamic vertex
#define BINDING_OFFSET_DYNAMIC_VERTEX						4

// light and material
#define BINDING_OFFSET_LIGHT_BUFFER							5
#define BINDING_LIGHT_COUNTS_HISTORY_BUFFER					6

// tonemap
#define BINDING_OFFSET_TONEMAP_BUFFER						7

// sky
#define BINDING_OFFSET_SUN_COLOR_BUFFER						8
#define BINDING_OFFSET_SUN_COLOR_UBO						9

// light stats
#define BINDING_OFFSET_LIGHT_STATS_BUFFER					10

// debug relations
#define USE_MULTI_WORLD_BUFFERS

#define VERTEX_BUFFER_WORLD					0
#define VERTEX_BUFFER_SKY					1
#define VERTEX_BUFFER_WORLD_D_MATERIAL		2
#define VERTEX_BUFFER_WORLD_D_GEOMETRY		3
#define VERTEX_BUFFER_DEBUG_LIGHT_POLYS		4
#define VERTEX_BUFFER_INSTANCED				5
#define VERTEX_BUFFER_SUB_MODELS			6
#define VERTEX_BUFFER_FIRST_MODEL			7

#define SUN_COLOR_ACCUMULATOR_FIXED_POINT_SCALE 0x100000
#define SKY_COLOR_ACCUMULATOR_FIXED_POINT_SCALE 0x100

// sample_light_counts: light count in cluster used for sampling (may differ from actual light count!)

#define VERTEX_BUFFER_LIST_DO( type, dim, name, size ) \
type name[ ALIGN_SIZE_4( size, dim ) ];

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

STRUCT (
	VEC3    ( pos0 )
	UINT    ( material_id )

	VEC3    ( pos1 )
	INT     ( cluster )

	VEC3    ( pos2 )
	UINT    ( shell )

	UVEC3	( normals )
	UINT    ( instance )

	UVEC3   ( tangents )
	/* packed half2x16, with emissive in x component/low 16 bits and
	 * alpha in y component/high 16 bits */
	UINT    ( emissive_and_alpha )

	UVEC4    ( uv0 )
	UVEC4    ( uv1 )
	UVEC4    ( uv2 )

	UVEC2	( custom0 )  // The custom fields store motion for instanced meshes in the animated buffer,
	UVEC2	( custom1 )  // or blend indices and weights for skinned meshes before they're animated.
	UVEC2	( custom2 )

	UINT    ( color0[4] )
	UINT    ( color1[4] )
	UINT    ( color2[4] )

	UVEC2	( pad0 )
, VboPrimitive )

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
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = PRIMITIVE_BUFFER_BINDING_IDX )		buffer PRIMITIVE_BUFFER {
	VboPrimitive primitives[];
} primitive_buffers[];

// The buffer with just the position data for animated models.
layout(set = VERTEX_BUFFER_DESC_SET_IDX, binding = POSITION_BUFFER_BINDING_IDX) buffer POSITION_BUFFER {
	float positions[];
} instanced_position_buffer;

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
layout( set = VERTEX_BUFFER_DESC_SET_IDX, binding = BINDING_OFFSET_LIGHT_BUFFER )				buffer LIGHT_BUFFER { LightBuffer light_buffer; };

#undef BUFFER_T

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
	uint   shell;
	int cluster;
	uint material_id;
	uint   instance_index;
	uint   instance_prim;
	float  emissive_factor;
	float  alpha;
	//float  alpha;
};

vec2 get_uv(uvec4 packed_uvs, int stage) {
    return unpackHalf2x16(packed_uvs[stage]);
}

Triangle
load_triangle(uint buffer_idx, uint prim_id)
{
	// info
	// buffer_idx = gl_InstanceCustomIndexEXT = instance.instance_id = vbo_index
	
	VboPrimitive prim = primitive_buffers[nonuniformEXT(buffer_idx)].primitives[prim_id];

	Triangle t;
	t.positions[0] = prim.pos0;
	t.positions[1] = prim.pos1;
	t.positions[2] = prim.pos2;

	t.positions_prev[0] = t.positions[0] + unpackHalf4x16(prim.custom0).xyz;
	t.positions_prev[1] = t.positions[1] + unpackHalf4x16(prim.custom1).xyz;
	t.positions_prev[2] = t.positions[2] + unpackHalf4x16(prim.custom2).xyz;

	t.normals[0] = decode_normal(prim.normals.x);
	t.normals[1] = decode_normal(prim.normals.y);
	t.normals[2] = decode_normal(prim.normals.z);

	t.tangents[0] = decode_normal(prim.tangents.x);
	t.tangents[1] = decode_normal(prim.tangents.y);
	t.tangents[2] = decode_normal(prim.tangents.z);

	t.tex_coords0[0] = get_uv( prim.uv0, 0 );
	t.tex_coords0[1] = get_uv( prim.uv1, 0 );
	t.tex_coords0[2] = get_uv( prim.uv2, 0 );

	t.tex_coords1[0] = get_uv( prim.uv0, 1 );
	t.tex_coords1[1] = get_uv( prim.uv1, 1 );
	t.tex_coords1[2] = get_uv( prim.uv2, 1 );

	t.tex_coords2[0] = get_uv( prim.uv0, 2 );
	t.tex_coords2[1] = get_uv( prim.uv1, 2 );
	t.tex_coords2[2] = get_uv( prim.uv2, 2 );

	t.tex_coords3[0] = get_uv( prim.uv0, 3 );
	t.tex_coords3[1] = get_uv( prim.uv1, 3 );
	t.tex_coords3[2] = get_uv( prim.uv2, 3 );

	t.material_id = prim.material_id;
	t.shell = prim.shell;
	t.cluster = prim.cluster;
	t.instance_index = prim.instance;
	t.instance_prim = 0;
	
	vec2 emissive_and_alpha = unpackHalf2x16(prim.emissive_and_alpha);
	t.emissive_factor = emissive_and_alpha.x;
	t.alpha = emissive_and_alpha.y;

	return t;
}

Triangle
load_and_transform_triangle(int instance_idx, uint buffer_idx, uint prim_id)
{
	Triangle t = load_triangle(buffer_idx, prim_id);

	if (instance_idx >= 0)
	{
		// Instance of a static mesh: transform the vertices.

		ModelInstance mi = instance_buffer.model_instances[instance_idx];
		
		t.positions[0] = vec3(mi.transform * vec4(t.positions[0], 1.0));
		t.positions[1] = vec3(mi.transform * vec4(t.positions[1], 1.0));
		t.positions[2] = vec3(mi.transform * vec4(t.positions[2], 1.0));

		t.positions_prev[0] = vec3(mi.transform_prev * vec4(t.positions_prev[0], 1.0));
		t.positions_prev[1] = vec3(mi.transform_prev * vec4(t.positions_prev[1], 1.0));
		t.positions_prev[2] = vec3(mi.transform_prev * vec4(t.positions_prev[2], 1.0));

		t.normals[0] = normalize(vec3(mi.transform * vec4(t.normals[0], 0.0)));
		t.normals[1] = normalize(vec3(mi.transform * vec4(t.normals[1], 0.0)));
		t.normals[2] = normalize(vec3(mi.transform * vec4(t.normals[2], 0.0)));

		t.tangents[0] = normalize(vec3(mi.transform * vec4(t.tangents[0], 0.0)));
		t.tangents[1] = normalize(vec3(mi.transform * vec4(t.tangents[1], 0.0)));
		t.tangents[2] = normalize(vec3(mi.transform * vec4(t.tangents[2], 0.0)));

		if (mi.material != 0) {
			t.material_id = mi.material;
			//t.shell = mi.shell;
		}
		int frame = int(mi.alpha_and_frame >> 16);
		//t.material_id = animate_material(t.material_id, frame);
		t.cluster = mi.cluster;
		t.emissive_factor = 1.0;
		t.alpha *= unpackHalf2x16(mi.alpha_and_frame).x;

		// Store the index of that instance and the prim offset relative to the instance.
		t.instance_index = uint(instance_idx);
		t.instance_prim = prim_id - mi.render_prim_offset;
	}
	else if (buffer_idx == VERTEX_BUFFER_INSTANCED)
	{
		// Instance of an animated or skinned mesh, coming from the primbuf.
		// In this case, `instance_idx` is -1 because it's not a static mesh, 
		// so load the original animated instance to find out its prim offset.

		ModelInstance mi = instance_buffer.model_instances[t.instance_index];
		t.instance_prim = prim_id - mi.render_prim_offset;
	}
	else if (buffer_idx >= VERTEX_BUFFER_WORLD && buffer_idx <= VERTEX_BUFFER_WORLD_D_GEOMETRY)
	{
		// Static BSP primitive.
#ifdef USE_MULTI_WORLD_BUFFERS
		t.instance_index = ~buffer_idx;
#else
		t.instance_index = ~0u;
#endif
		t.instance_prim = prim_id;

		if ( buffer_idx == VERTEX_BUFFER_WORLD_D_MATERIAL )
		{
			//t.material_id = 0;
		}
	}

	return t;
}


#ifndef VERTEX_READONLY
void
store_triangle(Triangle t, uint buffer_idx, uint prim_id)
{
	VboPrimitive prim;

	prim.pos0 = t.positions[0];
	prim.pos1 = t.positions[1];
	prim.pos2 = t.positions[2];

	prim.custom0 = packHalf4x16(vec4(t.positions_prev[0] - t.positions[0], 0));
	prim.custom1 = packHalf4x16(vec4(t.positions_prev[1] - t.positions[1], 0));
	prim.custom2 = packHalf4x16(vec4(t.positions_prev[2] - t.positions[2], 0));

	prim.normals.x = encode_normal(t.normals[0]);
	prim.normals.y = encode_normal(t.normals[1]);
	prim.normals.z = encode_normal(t.normals[2]);

	prim.tangents.x = encode_normal(t.tangents[0]);
	prim.tangents.y = encode_normal(t.tangents[1]);
	prim.tangents.z = encode_normal(t.tangents[2]);

	prim.uv0[0] = packHalf2x16(t.tex_coords0[0]);
	prim.uv1[0] = packHalf2x16(t.tex_coords0[1]);
	prim.uv2[0] = packHalf2x16(t.tex_coords0[2]);

	prim.material_id = t.material_id;
	prim.shell = t.shell;
	prim.cluster = t.cluster;
	prim.instance = t.instance_index;
	prim.emissive_and_alpha = packHalf2x16(vec2(t.emissive_factor, t.alpha));
	
	primitive_buffers[nonuniformEXT(buffer_idx)].primitives[prim_id] = prim;

	if (buffer_idx == VERTEX_BUFFER_INSTANCED)
	{
		for (int vert = 0; vert < 3; vert++)
		{
			for (int axis = 0; axis < 3; axis++)
			{
				instanced_position_buffer.positions[prim_id * 9 + vert * 3 + axis] 
					= t.positions[vert][axis];
			}
		}
	}
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
#endif
#endif
