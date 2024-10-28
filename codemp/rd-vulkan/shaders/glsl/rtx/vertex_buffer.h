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

#ifdef VERTEX_READONLY
#define BUFFER_T readonly buffer
#else
#define BUFFER_T buffer
#endif

// Buffer with indices and vertices
layout( set = 0, binding = BINDING_OFFSET_IDX_WORLD_STATIC )			BUFFER_T Indices_World_static { uint i[]; } indices_world_static;
layout( set = 0, binding = BINDING_OFFSET_XYZ_WORLD_STATIC )			BUFFER_T Vertices_World_static { VertexBuffer v[]; } vertices_world_static;

layout( set = 0, binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA )		BUFFER_T Indices_dynamic_data { uint i[]; } indices_dynamic_data;
layout( set = 0, binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA )		BUFFER_T Vertices_dynamic_data { VertexBuffer v[]; } vertices_dynamic_data;
layout( set = 0, binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS )		BUFFER_T Indices_dynamic_as { uint i[]; } indices_dynamic_as;
layout( set = 0, binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS )		BUFFER_T Vertices_dynamic_as { VertexBuffer v[]; } vertices_dynamic_as;

layout( set = 0, binding = BINDING_OFFSET_CLUSTER_WORLD_STATIC )		BUFFER_T Cluster_World_static { uint c[]; } cluster_world_static;
layout( set = 0, binding = BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_DATA )	BUFFER_T Cluster_World_dynamic_data { uint c[]; } cluster_world_dynamic_data;
layout( set = 0, binding = BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_AS )	BUFFER_T Cluster_World_dynamic_as { uint c[]; } cluster_world_dynamic_as;

layout( set = 0, binding = BINDING_OFFSET_READBACK_BUFFER )				buffer READBACK_BUFFER { ReadbackBuffer readback; };
layout( set = 0, binding = BINDING_OFFSET_DYNAMIC_VERTEX )				buffer MODEL_DYNAMIC_VERTEX_BUFFER { ModelDynamicVertexBuffer vbo_model_dynamic; };
layout( set = 0, binding = BINDING_OFFSET_LIGHT_BUFFER )				buffer LIGHT_BUFFER { LightBuffer lbo; };

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
layout( set = 0, binding = BINDING_LIGHT_COUNTS_HISTORY_BUFFER ) readonly buffer LIGHT_COUNTS_HISTORY_BUFFER {
	uint sample_light_counts[];
} light_counts_history[LIGHT_COUNT_HISTORY];

layout( set = 0, binding = BINDING_OFFSET_TONEMAP_BUFFER )				buffer TONE_MAPPING_BUFFER { ToneMappingBuffer tonemap_buffer; };
layout( set = 0, binding = BINDING_OFFSET_SUN_COLOR_BUFFER )			buffer SUN_COLOR_BUFFER { SunColorBuffer sun_color_buffer; };
layout( set = 0, binding = BINDING_OFFSET_LIGHT_STATS_BUFFER )			buffer LIGHT_STATS_BUFFERS { uint stats[]; } light_stats_bufers[3];

//prev
layout( set = 0, binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA_PREV ) BUFFER_T Indices_dynamic_data_prev { uint i[]; } indices_dynamic_data_prev;
layout( set = 0, binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA_PREV ) BUFFER_T Vertices_dynamic_data_prev { VertexBuffer v[]; } vertices_dynamic_data_prev;
layout( set = 0, binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS_PREV )	BUFFER_T Indices_dynamic_as_prev { uint i[]; } indices_dynamic_as_prev;
layout( set = 0, binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS_PREV )	BUFFER_T Vertices_dynamic_as_prev { VertexBuffer v[]; } vertices_dynamic_as_prev;

// layout( set = 0, binding = BINDING_OFFSET_CLUSTER_WORLD_STATIC_PREV )		buffer Cluster_World_static_prev { uint c[]; } cluster_world_static_prev;
// layout( set = 0, binding = BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_DATA_PREV )	buffer Cluster_World_dynamic_data_prev { uint c[]; } cluster_world_dynamic_data_prev;
// layout( set = 0, binding = BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_AS_PREV )	buffer Cluster_World_dynamic_as_prev { uint c[]; } cluster_world_dynamic_as_prev;

#undef BUFFER_T

struct NTB {
	vec3 normal, tangent, binormal;
};

vec3 QuatTransVec( in vec4 quat, in vec3 vec ) {
	vec3 tmp = 2.0 * cross( quat.xyz, vec );
	return vec + quat.w * tmp + cross( quat.xyz, tmp );
}

void QTangentToNTB( in vec4 qtangent, out NTB ntb ) {
	ntb.normal = QuatTransVec( qtangent, vec3( 0.0, 0.0, 1.0 ) );
	ntb.tangent = QuatTransVec( qtangent, vec3( 1.0, 0.0, 0.0 ) );
	ntb.tangent *= sign( qtangent.w );
	//ntb.binormal = QuatTransVec( qtangent, vec3( 0.0, 1.0, 0.0 ) );
}

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

	//t.alpha = 1.0;

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

	//set_alpha_instanced(prim_id, t.alpha);
}
#endif

uint get_material_uint( in uint material_index, in uint offset ) {
	return lbo.material_table[nonuniformEXT(material_index * MATERIAL_UINTS + offset)];
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

vec4 unpackColor(in uint color) {
	return vec4(
		color & 0xff,
		(color & (0xff << 8)) >> 8,
		(color & (0xff << 16)) >> 16,
		(color & (0xff << 24)) >> 24
	);
}

ivec3 get_bsp_triangle_indices_and_data( in uint instance_id, in uint prim_id, out VertexBuffer triangle[3] )
{
	uint prim = prim_id * 3;
	ivec3 indices;

	if ( instance_buffer.tlas_instance_type[instance_id] == BAS_WORLD_STATIC )
	{
		indices = ivec3(	
			indices_world_static.i[prim + 0], 
			indices_world_static.i[prim + 1], 
			indices_world_static.i[prim + 2] );

		triangle[0] = vertices_world_static.v[indices.x];
		triangle[1] = vertices_world_static.v[indices.y];
		triangle[2] = vertices_world_static.v[indices.z];
	}
	
	else if ( instance_buffer.tlas_instance_type[instance_id] == BAS_WORLD_DYNAMIC_DATA )
	{
		indices = ivec3(	
			indices_dynamic_data.i[prim + 0], 
			indices_dynamic_data.i[prim + 1], 
			indices_dynamic_data.i[prim + 2] );
		
		triangle[0] = vertices_dynamic_data.v[indices.x];
		triangle[1] = vertices_dynamic_data.v[indices.y];
		triangle[2] = vertices_dynamic_data.v[indices.z];
	}
	
	else if ( instance_buffer.tlas_instance_type[instance_id] == BAS_WORLD_DYNAMIC_AS )
	{
		indices = ivec3(	
			indices_dynamic_as.i[prim + 0], 
			indices_dynamic_as.i[prim + 1], 
			indices_dynamic_as.i[prim + 2] );

		triangle[0] = vertices_dynamic_as.v[indices.x];
		triangle[1] = vertices_dynamic_as.v[indices.y];
		triangle[2] = vertices_dynamic_as.v[indices.z];
	}
	
	return indices;
}

mat3x3 get_dynamic_bsp_positions_prev( in uint instance_id, in uint prim_id  )
{
	uint prim = prim_id * 3;
	mat3x3 position;

	if ( instance_buffer.tlas_instance_type[instance_id] == BAS_WORLD_DYNAMIC_DATA )
	{
		ivec3 indices = ivec3( 
			indices_dynamic_data_prev.i[prim + 0], 
			indices_dynamic_data_prev.i[prim + 1], 
			indices_dynamic_data_prev.i[prim + 2] );
		
		position[0] = vertices_dynamic_data_prev.v[indices.x].pos.xyz;
		position[1] = vertices_dynamic_data_prev.v[indices.y].pos.xyz;
		position[2] = vertices_dynamic_data_prev.v[indices.z].pos.xyz;
	}
	
	else if ( instance_buffer.tlas_instance_type[instance_id] == BAS_WORLD_DYNAMIC_AS )
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

Triangle get_bsp_triangle( in uint instance_id, in uint prim_id )
{
	VertexBuffer triangle[3];
	ivec3 indices = get_bsp_triangle_indices_and_data( instance_id, prim_id, triangle );

	Triangle t;

	t.positions[0] = triangle[0].pos.xyz;
	t.positions[1] = triangle[1].pos.xyz;
	t.positions[2] = triangle[2].pos.xyz;
	
	if ( instance_buffer.tlas_instance_type[instance_id] == BAS_WORLD_STATIC )
		t.positions_prev = t.positions;
	else 
		t.positions_prev = get_dynamic_bsp_positions_prev( instance_id, prim_id ); 

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

#if 0
	// using packed qtangent instead, this can be removed or commented for now
	t.normals[0] = triangle[0].normal.xyz, 0;
	t.normals[1] = triangle[1].normal.xyz, 0;
	t.normals[2] = triangle[2].normal.xyz, 0;
#else
	NTB ntb[3];

	QTangentToNTB( triangle[0].qtangent, ntb[0] );
	QTangentToNTB( triangle[1].qtangent, ntb[1] );
	QTangentToNTB( triangle[2].qtangent, ntb[2] );
	
	t.normals[0] = ntb[0].normal.xyz;
	t.normals[1] = ntb[1].normal.xyz;
	t.normals[2] = ntb[2].normal.xyz;
	
	t.tangents[0] = ntb[0].tangent.xyz;
	t.tangents[1] = ntb[1].tangent.xyz;
	t.tangents[2] = ntb[2].tangent.xyz;

	//t.binormal[0] = ntb[0].binormal.xyz;
	//t.binormal[1] = ntb[1].binormal.xyz;
	//t.binormal[2] = ntb[2].binormal.xyz;	
#endif

	switch( instance_buffer.tlas_instance_type[instance_id] )
	{
		case BAS_WORLD_STATIC:
			t.tex0 = vertices_world_static.v[indices.x].texIdx0;
			t.tex1 = vertices_world_static.v[indices.x].texIdx1;

			t.cluster = cluster_world_static.c[prim_id];
			t.material_id = vertices_world_static.v[indices.x].material;	// same as triangle[0].material ?
			break;
		case BAS_WORLD_DYNAMIC_DATA:
			t.tex0 = vertices_dynamic_data.v[indices.x].texIdx0;
			t.tex1 = vertices_dynamic_data.v[indices.x].texIdx1;

			t.cluster = cluster_world_dynamic_data.c[prim_id];
			t.material_id = vertices_dynamic_data.v[indices.x].material;
			break;
		case BAS_WORLD_DYNAMIC_AS:
			t.tex0 = vertices_dynamic_as.v[indices.x].texIdx0;
			t.tex1 = vertices_dynamic_as.v[indices.x].texIdx1;

			t.cluster = cluster_world_dynamic_as.c[prim_id];
			t.material_id = vertices_dynamic_as.v[indices.x].material;
			break;
		default:
			t.tex0 = 0;
			t.tex1 = 0;
			t.cluster = 0;
			t.material_id = 0;
	}

	return t;
}
#endif