/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "tr_local.h"


#define MAX_SABER_LIGHTS 128

#define TR_PARTICLE_MAX_NUM    16384
#define TR_BEAM_MAX_NUM        1024
#define TR_SPRITE_MAX_NUM      1024
#define TR_VERTEX_MAX_NUM      ((TR_PARTICLE_MAX_NUM + TR_SPRITE_MAX_NUM) * 4)
#define TR_INDEX_MAX_NUM       ((TR_PARTICLE_MAX_NUM + TR_SPRITE_MAX_NUM) * 6)
#define TR_BEAM_AABB_SIZE      sizeof(VkAabbPositionsKHR)
#define TR_POSITION_SIZE       (3 * sizeof(float))
#define TR_COLOR_SIZE          (4 * sizeof(float))
#define TR_BEAM_INTERSECT_SIZE (12 * sizeof(float))
#define TR_SPRITE_INFO_SIZE    (2 * sizeof(float))

#define LENGTH(a) ((sizeof (a)) / (sizeof(*(a))))

#define VectorAdd3(a,b,c,d) \
        ((d)[0]=(a)[0]+(b)[0]+(c)[0], \
         (d)[1]=(a)[1]+(b)[1]+(c)[1], \
         (d)[2]=(a)[2]+(b)[2]+(c)[2])

struct
{
	size_t vertex_position_host_offset;
	size_t beam_aabb_host_offset;
	size_t particle_color_host_offset;
	size_t beam_color_host_offset;
	size_t sprite_info_host_offset;
	size_t current_upload_size;

	size_t beam_intersect_host_offset;

	size_t sprite_vertex_device_offset;

	size_t host_buffer_size;
	size_t host_frame_size;
	unsigned int particle_num;
	unsigned int beam_num;
	unsigned int sprite_num;
	unsigned int host_frame_index;
	unsigned int host_buffered_frame_num;
	char* mapped_host_buffer;
	char* host_buffer_shadow;
	vkbuffer_t vertex_buffer;
	vkbuffer_t index_buffer;
	vkbuffer_t beam_aabb_buffer;
	vkbuffer_t particle_color_buffer;
	vkbuffer_t beam_color_buffer;
	vkbuffer_t sprite_info_buffer;
	vkbuffer_t beam_intersect_buffer;
	VkBufferView particle_color_buffer_view;
	VkBufferView beam_color_buffer_view;
	VkBufferView sprite_info_buffer_view;
	VkBufferView beam_intersect_buffer_view;
	VkBuffer host_buffer;
	VkDeviceMemory host_buffer_memory;
	VkBufferMemoryBarrier transfer_barriers[6];
} transparency;

// initialization
static void create_buffers(void);
static bool allocate_and_bind_memory_to_buffers(void);
static void create_buffer_views(void);
static void fill_index_buffer(void);


// update
//static void write_particle_geometry(const float* view_matrix, const particle_t* particles, int particle_num);
//static void write_beam_geometry(const entity_t* entities, int entity_num);
static void write_sprite_geometry(const float* view_matrix, const trRefdef_t *refdef);
static void upload_geometry(VkCommandBuffer command_buffer);

cvar_t* cvar_pt_particle_size = NULL;
cvar_t* cvar_pt_beam_width = NULL;
cvar_t* cvar_pt_beam_lights = NULL;
extern cvar_t* cvar_pt_enable_particles;
extern cvar_t* cvar_pt_particle_emissive;
extern cvar_t* cvar_pt_projection;

//void cast_u32_to_f32_color(int color_index, const color_t* pcolor, float* color_f32, float hdr_factor)
//{
//	color_t color;
//	if (color_index < 0)
//		color.u32 = pcolor->u32;
//	else
//		color.u32 = d_8to24table[color_index & 0xff];
//
//	for (int i = 0; i < 3; i++)
//		color_f32[i] = hdr_factor * decode_srgb(color.u8[i]);
//}

bool initialize_transparency()
{
	cvar_pt_particle_size = ri.Cvar_Get("pt_particle_size", "0.5", 0, "");
	cvar_pt_beam_width = ri.Cvar_Get("pt_beam_width", "1.0", 0, "");
	cvar_pt_beam_lights = ri.Cvar_Get("pt_beam_lights", "1.0", 0, "");

	memset(&transparency, 0, sizeof(transparency));

	const size_t particle_vertex_position_max_size = TR_VERTEX_MAX_NUM * TR_POSITION_SIZE;
	const size_t particle_color_size = TR_PARTICLE_MAX_NUM * TR_COLOR_SIZE;
	const size_t particle_data_size = particle_vertex_position_max_size + particle_color_size;

	const size_t beam_aabb_max_size = TR_BEAM_MAX_NUM * TR_BEAM_AABB_SIZE;
	const size_t beam_color_size = TR_BEAM_MAX_NUM * TR_COLOR_SIZE;
	const size_t beam_intersect_size = TR_BEAM_MAX_NUM * TR_BEAM_INTERSECT_SIZE;
	const size_t beam_data_size = beam_aabb_max_size + beam_color_size + beam_intersect_size;

	const size_t sprite_vertex_position_max_size = TR_SPRITE_MAX_NUM * TR_POSITION_SIZE;
	const size_t sprite_info_size = TR_SPRITE_MAX_NUM * TR_SPRITE_INFO_SIZE;
	const size_t sprite_data_size = sprite_vertex_position_max_size + sprite_info_size;
	
	transparency.host_buffered_frame_num = VK_MAX_SWAPCHAIN_SIZE;
	transparency.host_frame_size = particle_data_size + beam_data_size + sprite_data_size;
	transparency.host_buffer_size = transparency.host_buffered_frame_num * transparency.host_frame_size;

	create_buffers();

	if (allocate_and_bind_memory_to_buffers() != VK_TRUE)
		return false;

	create_buffer_views();
	fill_index_buffer();

	return true;
}

void destroy_transparency()
{
	qvkDestroyBufferView(vk.device, transparency.particle_color_buffer_view, NULL);
	qvkDestroyBufferView(vk.device, transparency.beam_color_buffer_view, NULL);
	qvkDestroyBufferView(vk.device, transparency.sprite_info_buffer_view, NULL);
	qvkDestroyBufferView(vk.device, transparency.beam_intersect_buffer_view, NULL);
	vk_rtx_buffer_destroy(&transparency.vertex_buffer);
	vk_rtx_buffer_destroy(&transparency.index_buffer);
	vk_rtx_buffer_destroy(&transparency.beam_aabb_buffer);
	vk_rtx_buffer_destroy(&transparency.particle_color_buffer);
	vk_rtx_buffer_destroy(&transparency.beam_color_buffer);
	vk_rtx_buffer_destroy(&transparency.sprite_info_buffer);
	vk_rtx_buffer_destroy(&transparency.beam_intersect_buffer);

	qvkDestroyBuffer(vk.device, transparency.host_buffer, NULL);
	qvkFreeMemory(vk.device, transparency.host_buffer_memory, NULL);

	if (transparency.host_buffer_shadow)
	{
		Z_Free((void**)&transparency.host_buffer_shadow);
	}
}

void update_transparency(VkCommandBuffer command_buffer, const trRefdef_t *refdef, const float* view_matrix )
{
	transparency.host_frame_index = (transparency.host_frame_index + 1) % transparency.host_buffered_frame_num;
	//particle_num = MIN(particle_num, TR_PARTICLE_MAX_NUM);

	const int particle_num = 0;
	uint32_t beam_num = 0;
	uint32_t sprite_num = 0;
	float radius = 0;

	for ( int i = 0; i < refdef->num_entities; i++ )
	{
		if ( !r_drawentities->integer )
			break;

		trRefEntity_t *entity = refdef->entities + i;
		
		if (entity->e.reType == RT_BEAM)
		{
			//++beam_num;
		}
		else if (entity->e.reType == RT_LINE || entity->e.reType == RT_SPRITE)
		{
			++sprite_num;
		}
		else if (entity->e.reType == RT_SABER_GLOW)
		{
			radius = entity->e.radius;

			for (float j = entity->e.saberLength; j > 0; j -= radius * 0.65f)
			{
				++sprite_num;
				radius += 0.017f;
			}

			++sprite_num; 
		}
	}
	beam_num = MIN(beam_num, TR_BEAM_MAX_NUM);
	sprite_num = MIN(sprite_num, TR_SPRITE_MAX_NUM);

	transparency.beam_num = beam_num;
	transparency.particle_num = particle_num;
	transparency.sprite_num = sprite_num;

	const size_t particle_vertices_size = particle_num * (4 * TR_POSITION_SIZE);
	const size_t sprite_vertices_size = sprite_num * (4 * TR_POSITION_SIZE);

	transparency.vertex_position_host_offset	= 0;
	transparency.particle_color_host_offset		= transparency.vertex_position_host_offset + particle_vertices_size + sprite_vertices_size;
	transparency.sprite_info_host_offset		= transparency.particle_color_host_offset + particle_num * TR_COLOR_SIZE;
	transparency.beam_aabb_host_offset			= transparency.sprite_info_host_offset + sprite_num * TR_SPRITE_INFO_SIZE;
	transparency.beam_color_host_offset			= transparency.beam_aabb_host_offset + beam_num * TR_BEAM_AABB_SIZE;
	transparency.beam_intersect_host_offset		= transparency.beam_color_host_offset + beam_num * TR_COLOR_SIZE;
	transparency.current_upload_size			= transparency.beam_intersect_host_offset + beam_num * TR_BEAM_INTERSECT_SIZE;

	if (particle_num > 0 || beam_num > 0 || sprite_num > 0)
	{
		//write_particle_geometry(view_matrix, particles, particle_num);
		//write_beam_geometry(entities, entity_num);
		write_sprite_geometry(view_matrix, refdef);
		upload_geometry(command_buffer);
	}
}

void vkpt_get_transparency_buffers(
	vkpt_transparency_t ttype,
	vkbuffer_t** vertex_buffer,
	uint64_t* vertex_offset,
	vkbuffer_t** index_buffer,
	uint64_t* index_offset,
	uint32_t* num_vertices,
	uint32_t* num_indices)
{
	*vertex_buffer = &transparency.vertex_buffer;
	*index_buffer = &transparency.index_buffer;
	*index_offset = 0;

	switch (ttype)
	{
	case VKPT_TRANSPARENCY_PARTICLES:
		*vertex_offset = 0;
		*num_vertices = transparency.particle_num * 4;
		*num_indices = transparency.particle_num * 6;
		return;

	case VKPT_TRANSPARENCY_SPRITES:
		*vertex_offset = transparency.sprite_vertex_device_offset;
		*num_vertices = transparency.sprite_num * 4;
		*num_indices = transparency.sprite_num * 6;
		return;

	default:
		*vertex_offset = transparency.sprite_vertex_device_offset;
		*num_vertices = 0;
		*num_indices = 0;
		return;
	}
}

void vkpt_get_beam_aabb_buffer(
	vkbuffer_t** aabb_buffer,
	uint64_t* aabb_offset,
	uint32_t* num_aabbs)
{
	*aabb_buffer = &transparency.beam_aabb_buffer;
	*aabb_offset = 0;
	*num_aabbs = transparency.beam_num;
}

VkBufferView get_transparency_particle_color_buffer_view()
{
	return transparency.particle_color_buffer_view;
}

VkBufferView get_transparency_beam_color_buffer_view()
{
	return transparency.beam_color_buffer_view;
}

VkBufferView get_transparency_sprite_info_buffer_view()
{
	return transparency.sprite_info_buffer_view;
}

VkBufferView get_transparency_beam_intersect_buffer_view()
{
	return transparency.beam_intersect_buffer_view;
}

void get_transparency_counts(int* particle_num, int* beam_num, int* sprite_num)
{
	*particle_num = transparency.particle_num;
	*beam_num = transparency.beam_num;
	*sprite_num = transparency.sprite_num;
}

bool vkpt_build_cylinder_light(light_poly_t* light_list, int* num_lights, int max_lights, world_t *worldData, vec3_t begin, vec3_t end, vec3_t color, float radius, entity_hash_t hash, int *light_entity_ids)
{
	vec3_t dir, norm_dir;
	VectorSubtract(end, begin, dir);
	VectorCopy(dir, norm_dir);
	VectorNormalize(norm_dir);

	vec3_t up = { 0.f, 0.f, 1.f };
	vec3_t left = { 1.f, 0.f, 0.f };
	if (fabsf(norm_dir[2]) < 0.9f)
	{
		CrossProduct(up, norm_dir, left);
		VectorNormalize(left);
		CrossProduct(norm_dir, left, up);
		VectorNormalize(up);
	}
	else
	{
		CrossProduct(norm_dir, left, up);
		VectorNormalize(up);
		CrossProduct(up, norm_dir, left);
		VectorNormalize(left);
	}


	vec3_t vertices[6] = {
		{ 0.f, 1.f, 0.f },
		{ 0.866f, -0.5f, 0.f },
		{ -0.866f, -0.5f, 0.f },
		{ 0.f, -1.f, 1.f },
		{ -0.866f, 0.5f, 1.f },
		{ 0.866f, 0.5f, 1.f },
	};

	const int indices[18] = {
		0, 4, 2,
		2, 4, 3,
		2, 3, 1,
		1, 3, 5,
		1, 5, 0,
		0, 5, 4
	};

	for (int vert = 0; vert < 6; vert++)
	{
		vec3_t transformed;
		VectorCopy(begin, transformed);
		VectorMA(transformed, vertices[vert][0] * radius, up, transformed);
		VectorMA(transformed, vertices[vert][1] * radius, left, transformed);
		VectorMA(transformed, vertices[vert][2], dir, transformed);
		VectorCopy(transformed, vertices[vert]);
	}

	for (int tri = 0; tri < 6; tri++)
	{
		if (*num_lights >= max_lights)
			return false;

		int i0 = indices[tri * 3 + 0];
		int i1 = indices[tri * 3 + 1];
		int i2 = indices[tri * 3 + 2];

		light_poly_t* light = light_list + *num_lights;

		VectorCopy(vertices[i0], light->positions + 0);
		VectorCopy(vertices[i1], light->positions + 3);
		VectorCopy(vertices[i2], light->positions + 6);
		get_triangle_off_center(light->positions, light->off_center, NULL, 1.f);

		light->cluster = BSP_PointLeaf(worldData->nodes, light->off_center)->cluster;
		light->material = NULL;
		light->style = 0;
		light->type = LIGHT_POLYGON;

		VectorCopy(color, light->color);

		if (light->cluster >= 0)
		{
			hash.mesh = tri;
			light_entity_ids[(*num_lights)] = *(uint32_t*)&hash;
			(*num_lights)++;
		}
	}

	return true;
}


static void do_sprite( vec3_t *vertex_positions, vec3_t origin, float radius )
{
	float	s, c;
	float	ang;
	vec3_t up, down, left, right;
		
	const float rotation = 0.0f;

	ang = M_PI * rotation / 180.0f;
	s = sin( ang );
	c = cos( ang );

	VectorScale( backEnd.viewParms.ori.axis[1], c * radius, left );
	VectorMA( left, -s * radius, backEnd.viewParms.ori.axis[2], left );

	VectorScale( backEnd.viewParms.ori.axis[2], c * radius, up );
	VectorMA( up, s * radius, backEnd.viewParms.ori.axis[1], up );

	VectorScale( left, -1.0f, right );
	VectorScale( up, -1.0f, down );

	VectorAdd3( origin, up,   left,  vertex_positions[0] );
	VectorAdd3( origin, down, left,  vertex_positions[1] );
	VectorAdd3( origin, down, right, vertex_positions[2] );
	VectorAdd3( origin, up,   right, vertex_positions[3] );
}

static void do_line( vec3_t *vertex_positions, const vec3_t start, const vec3_t end, const vec3_t up, float spanWidth )
{
	const float spanWidth2 = -spanWidth;

    VectorMA( start, spanWidth2, up, vertex_positions[0] );
    VectorMA( start, spanWidth,  up, vertex_positions[1] );
    VectorMA( end,   spanWidth,  up, vertex_positions[2] );
    VectorMA( end,   spanWidth2, up, vertex_positions[3] );
}

static inline void write_sprite_info( uint32_t *sprite_info, const rtx_material_t *mat, const refEntity_t *e )
{
    sprite_info[0] = mat->flags;
    sprite_info[1] =
        ((uint32_t)e->shaderRGBA[0]      ) |
        ((uint32_t)e->shaderRGBA[1] <<  8) |
        ((uint32_t)e->shaderRGBA[2] << 16) |
        ((uint32_t)e->shaderRGBA[3] << 24);
}

// borrowed from CG_RGBForSaberColor in cg_palyer.c
static void vk_rtx_get_saber_lights_color( vec3_t rgb, refEntity_t *e )
{
	// e->customSkin = saber color type sett in cg_player.c
	switch ( (saber_colors_t)e->customSkin )
	{
		case SABER_RED:
			VectorSet( rgb, 1.0f, 0.2f, 0.2f );
			break;
		case SABER_ORANGE:
			VectorSet( rgb, 1.0f, 0.5f, 0.1f );
			break;
		case SABER_YELLOW:
			VectorSet( rgb, 1.0f, 1.0f, 0.2f );
			break;
		case SABER_GREEN:
			VectorSet( rgb, 0.2f, 1.0f, 0.2f );
			break;
		case SABER_BLUE:
			VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			break;
		case SABER_PURPLE:
			VectorSet( rgb, 0.9f, 0.2f, 1.0f );
			break;
		case SABER_BLACK:
			VectorSet( rgb, 1.0f, 1.0f, 1.0f );
			break;
		case SABER_RGB:
			VectorSet( rgb, 
				e->shaderRGBA[0] / 255.0f,
				e->shaderRGBA[1] / 255.0f,
				e->shaderRGBA[2] / 255.0f
			);
			break;
		case SABER_FLAME1:
		case SABER_ELEC1:
		case SABER_FLAME2:
		case SABER_ELEC2:
#if 0
			if ( cnum < MAX_CLIENTS ) {
				int i;

				if ( bnum == 0 )
					VectorCopy( ci->rgb1, rgb );
				else
					VectorCopy( ci->rgb2, rgb );
				for ( i = 0; i < 3; i++ )
					rgb[i] /= 255;
			}
			else
#endif
				VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			break;
		default:
			VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			break;
	}
}


void vk_rtx_build_saber_lights( light_poly_t *light_list, int *num_lights, 
	int max_lights, world_t *worldData, const trRefdef_t *refdef, float adapted_luminance, int *light_entity_ids )
{
	uint32_t i;

	int num_sabers = 0;

	static trRefEntity_t *sabers[MAX_SABER_LIGHTS];

	for ( i = 0; i < refdef->num_entities; i++ )
	{
		if ( num_sabers == MAX_SABER_LIGHTS )
			break;

		if (refdef->entities[i].e.reType == RT_SABER_GLOW)
			sabers[num_sabers++] = refdef->entities + i;
	}

	if ( num_sabers == 0 )
		return;

	for ( i = 0; i < num_sabers; i++ )
	{
		trRefEntity_t *saber = sabers[i];
        refEntity_t *e = &saber->e;

		entity_hash_t hash;
		hash.entity = (sabers[i] - refdef->entities) + 1; //entity ID
		hash.model = RT_SABER_GLOW;

        vec3_t begin, end, color;
        VectorCopy(e->origin, begin);
        VectorMA(e->origin, e->saberLength, e->axis[0], end);

		vk_rtx_get_saber_lights_color( color, e );

        vkpt_build_cylinder_light( light_list, num_lights, max_lights, worldData, begin, end, color, e->radius, hash, light_entity_ids );
	}
}

static void write_sprite_geometry(const float* view_matrix, const trRefdef_t *refdef) 
{
	if (transparency.sprite_num == 0)
		return;

	const vec3_t view_x = { view_matrix[0], view_matrix[4], view_matrix[8] };
	const vec3_t view_y = { view_matrix[1], view_matrix[5], view_matrix[9] };
	const vec3_t world_y = { 0.f, 0.f, 1.f };

	// TODO: remove vkpt_refdef.fd, it's better to calculate it from the view matrix
	const vec3_t view_origin = { refdef->vieworg[0], refdef->vieworg[1], refdef->vieworg[2] };

	const size_t particle_vertex_data_size = transparency.particle_num * 4 * TR_POSITION_SIZE;
	const size_t sprite_vertex_offset = transparency.vertex_position_host_offset + particle_vertex_data_size;

	// TODO: use better alignment?
	vec3_t* vertex_positions = (vec3_t*)(transparency.host_buffer_shadow + sprite_vertex_offset);
	uint32_t* sprite_info = (uint32_t*)(transparency.host_buffer_shadow + transparency.sprite_info_host_offset);

	int sprite_count = 0;
	shader_t *shader;
	rtx_material_t *mat;
	uint32_t i;
	float j, radius;

	for ( i = 0; i < refdef->num_entities; i++ )
	{
		trRefEntity_t *entity = refdef->entities + i;

		if (entity->e.reType != RT_LINE && entity->e.reType != RT_SPRITE && entity->e.reType != RT_SABER_GLOW )
			continue;

		shader = R_GetShaderByHandle( entity->e.customShader );
		mat = vk_rtx_shader_to_material( shader );

		if ( !mat || !mat->active || !mat->uploaded[vk.current_frame_index] ) 
			continue;

		if ( !mat->stage[0].bundle[0].image )
			continue;

		if (entity->e.reType == RT_LINE) 
		{
			vec3_t start, end;
			vec3_t v1, v2;
			vec3_t right;

			VectorCopy(entity->e.origin, start);
			VectorCopy(entity->e.oldorigin, end);

			VectorSubtract(start, refdef->vieworg, v1);
			VectorSubtract(end, refdef->vieworg, v2);

			CrossProduct(v1, v2, right);

			if (VectorNormalize(right) <= 0.0001f)
				continue;

			write_sprite_info(sprite_info, mat, &entity->e);
			do_line( vertex_positions, start, end, right, entity->e.radius );

			vertex_positions += 4;
			sprite_info += TR_SPRITE_INFO_SIZE / sizeof(uint32_t);
		}
		else if (entity->e.reType == RT_SPRITE )
		{
			write_sprite_info(sprite_info, mat, &entity->e);
			do_sprite( vertex_positions, entity->e.origin, entity->e.radius);

			vertex_positions += 4;
			sprite_info += TR_SPRITE_INFO_SIZE / sizeof(uint32_t);
		}
		else if (entity->e.reType == RT_SABER_GLOW)
		{
			vec3_t		end;
			refEntity_t *e;

			e = &entity->e;
			radius = e->radius;

			for ( j = e->saberLength; j > 0; j -= radius * 0.65f)
			{
				VectorMA( e->origin, j, e->axis[0], end );

				write_sprite_info(sprite_info, mat, &entity->e);
				do_sprite( vertex_positions, end, e->radius);

				vertex_positions += 4;
				sprite_info += TR_SPRITE_INFO_SIZE / sizeof(uint32_t);

				radius += 0.017f;

				if (++sprite_count >= TR_SPRITE_MAX_NUM)
					return;
			}

			write_sprite_info(sprite_info, mat, &entity->e);
			do_sprite( vertex_positions, e->origin, 5.5f + Q_flrand(0.0f, 1.0f) * 0.25f);

			vertex_positions += 4;
			sprite_info += TR_SPRITE_INFO_SIZE / sizeof(uint32_t);
		}

		if (++sprite_count >= TR_SPRITE_MAX_NUM)
			return;
	}
}

static void upload_geometry(VkCommandBuffer command_buffer)
{
	transparency.sprite_vertex_device_offset = transparency.particle_num * 4 * TR_POSITION_SIZE;

    const size_t host_buffer_offset = transparency.host_frame_index * transparency.host_frame_size;

	assert(transparency.current_upload_size > 0);
	memcpy(transparency.mapped_host_buffer + host_buffer_offset, transparency.host_buffer_shadow, transparency.current_upload_size);
	transparency.current_upload_size = 0;

	VkBufferCopy vertices;
	Com_Memset( &vertices, 0, sizeof(VkBufferCopy) );
	vertices.srcOffset = host_buffer_offset + transparency.vertex_position_host_offset;
	vertices.dstOffset = 0;
	vertices.size = (transparency.particle_num + transparency.sprite_num) * 4 * TR_POSITION_SIZE;


	VkBufferCopy beam_aabbs;
	Com_Memset( &beam_aabbs, 0, sizeof(VkBufferCopy) );
	beam_aabbs.srcOffset = host_buffer_offset + transparency.beam_aabb_host_offset;
	beam_aabbs.dstOffset = 0;
	beam_aabbs.size = transparency.beam_num * TR_BEAM_AABB_SIZE;


	VkBufferCopy particle_colors;
	Com_Memset( &particle_colors, 0, sizeof(VkBufferCopy) );
	particle_colors.srcOffset = host_buffer_offset + transparency.particle_color_host_offset;
	particle_colors.dstOffset = 0;
	particle_colors.size = transparency.particle_num * TR_COLOR_SIZE;


	VkBufferCopy beam_colors;
	Com_Memset( &beam_colors, 0, sizeof(VkBufferCopy) );
	beam_colors.srcOffset = host_buffer_offset + transparency.beam_color_host_offset;
	beam_colors.dstOffset = 0;
	beam_colors.size = transparency.beam_num * TR_COLOR_SIZE;


	VkBufferCopy sprite_infos;
	Com_Memset( &sprite_infos, 0, sizeof(VkBufferCopy) );
	sprite_infos.srcOffset = host_buffer_offset + transparency.sprite_info_host_offset;
	sprite_infos.dstOffset = 0;
	sprite_infos.size = transparency.sprite_num * TR_SPRITE_INFO_SIZE;


	VkBufferCopy beam_intersect;
	Com_Memset( &beam_intersect, 0, sizeof(VkBufferCopy) );
	beam_intersect.srcOffset = host_buffer_offset + transparency.beam_intersect_host_offset;
	beam_intersect.dstOffset = 0;
	beam_intersect.size = transparency.beam_num * TR_BEAM_INTERSECT_SIZE;

	if (vertices.size)
		qvkCmdCopyBuffer(command_buffer, transparency.host_buffer, transparency.vertex_buffer.buffer,
			1, &vertices);
	
	if (beam_aabbs.size)
		qvkCmdCopyBuffer(command_buffer, transparency.host_buffer, transparency.beam_aabb_buffer.buffer,
			1, &beam_aabbs);

	if (particle_colors.size)
		qvkCmdCopyBuffer(command_buffer, transparency.host_buffer, transparency.particle_color_buffer.buffer,
			1, &particle_colors);

	if (beam_colors.size)
		qvkCmdCopyBuffer(command_buffer, transparency.host_buffer, transparency.beam_color_buffer.buffer,
			1, &beam_colors);

	if (sprite_infos.size)
		qvkCmdCopyBuffer(command_buffer, transparency.host_buffer, transparency.sprite_info_buffer.buffer,
			1, &sprite_infos);

	if (beam_intersect.size)
		qvkCmdCopyBuffer(command_buffer, transparency.host_buffer, transparency.beam_intersect_buffer.buffer,
			1, &beam_intersect);

	for (size_t i = 0; i < ARRAY_LEN(transparency.transfer_barriers); i++)
	{
		transparency.transfer_barriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		transparency.transfer_barriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		transparency.transfer_barriers[i].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		transparency.transfer_barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		transparency.transfer_barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	}

	transparency.transfer_barriers[0].buffer = transparency.vertex_buffer.buffer;
	transparency.transfer_barriers[0].size = vertices.size;
	transparency.transfer_barriers[1].buffer = transparency.particle_color_buffer.buffer;
	transparency.transfer_barriers[1].size = particle_colors.size;
	transparency.transfer_barriers[2].buffer = transparency.beam_color_buffer.buffer;
	transparency.transfer_barriers[2].size = beam_colors.size;
	transparency.transfer_barriers[3].buffer = transparency.sprite_info_buffer.buffer;
	transparency.transfer_barriers[3].size = sprite_infos.size;
	transparency.transfer_barriers[4].buffer = transparency.beam_aabb_buffer.buffer;
	transparency.transfer_barriers[4].size = beam_aabbs.size;
	transparency.transfer_barriers[5].buffer = transparency.beam_intersect_buffer.buffer;
	transparency.transfer_barriers[5].size = beam_intersect.size;
}

static void create_buffers(void)
{
	VkBufferCreateInfo host_buffer_info;
	Com_Memset( &host_buffer_info, 0, sizeof(VkBufferCreateInfo) );
	host_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	host_buffer_info.pNext = NULL;
	host_buffer_info.size = transparency.host_buffered_frame_num * transparency.host_frame_size;
	host_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	qvkCreateBuffer(vk.device, &host_buffer_info, NULL, &transparency.host_buffer);

	vk_rtx_buffer_create(
		&transparency.vertex_buffer, 
		TR_VERTEX_MAX_NUM * sizeof(vec3_t),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk_rtx_buffer_create(
		&transparency.beam_aabb_buffer,
		TR_BEAM_MAX_NUM * sizeof(VkAabbPositionsKHR),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk_rtx_buffer_create(
		&transparency.index_buffer,
		TR_INDEX_MAX_NUM * sizeof(uint16_t),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk_rtx_buffer_create(
		&transparency.particle_color_buffer,
		TR_PARTICLE_MAX_NUM * TR_COLOR_SIZE,
		VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk_rtx_buffer_create(
		&transparency.beam_color_buffer,
		TR_BEAM_MAX_NUM * TR_COLOR_SIZE,
		VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk_rtx_buffer_create(
		&transparency.sprite_info_buffer,
		TR_SPRITE_MAX_NUM * TR_SPRITE_INFO_SIZE,
		VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk_rtx_buffer_create(
		&transparency.beam_intersect_buffer,
		TR_BEAM_MAX_NUM * TR_BEAM_INTERSECT_SIZE,
		VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

static bool allocate_and_bind_memory_to_buffers(void)
{
	VkMemoryRequirements host_buffer_requirements;
	qvkGetBufferMemoryRequirements(vk.device, transparency.host_buffer, &host_buffer_requirements);

	const VkMemoryPropertyFlags host_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	const uint32_t host_memory_type = vk_find_memory_type(host_buffer_requirements.memoryTypeBits, host_flags);

	VkMemoryAllocateInfo host_memory_allocate_info;
	Com_Memset( &host_memory_allocate_info, 0, sizeof(VkMemoryAllocateInfo) );
	host_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	host_memory_allocate_info.pNext = NULL;
	host_memory_allocate_info.allocationSize = host_buffer_requirements.size;
	host_memory_allocate_info.memoryTypeIndex = host_memory_type;

	qvkAllocateMemory(vk.device, &host_memory_allocate_info, NULL, &transparency.host_buffer_memory);

	VkBindBufferMemoryInfo bindings[1];
	Com_Memset( &bindings, 0, sizeof(VkBindBufferMemoryInfo) );

	bindings[0].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
	bindings[0].pNext = NULL;
	bindings[0].buffer = transparency.host_buffer;
	bindings[0].memory = transparency.host_buffer_memory;
	bindings[0].memoryOffset = 0;

	qvkBindBufferMemory2(vk.device, LENGTH(bindings), bindings);

	const size_t host_buffer_size = transparency.host_buffered_frame_num * transparency.host_frame_size;

	qvkMapMemory(vk.device, transparency.host_buffer_memory, 0, host_buffer_size, 0,
		(void**)&transparency.mapped_host_buffer);

	transparency.host_buffer_shadow = (char*)Z_Malloc(transparency.host_frame_size, TAG_GENERAL);
	
	return true;
}

static void create_buffer_views(void)
{
	VkBufferViewCreateInfo particle_color_view_info;
	Com_Memset( &particle_color_view_info, 0, sizeof(VkBufferViewCreateInfo) );
	particle_color_view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	particle_color_view_info.pNext = NULL;
	particle_color_view_info.buffer = transparency.particle_color_buffer.buffer;
	particle_color_view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	particle_color_view_info.range = TR_PARTICLE_MAX_NUM * TR_COLOR_SIZE;

	VkBufferViewCreateInfo beam_color_view_info;
	Com_Memset( &beam_color_view_info, 0, sizeof(VkBufferViewCreateInfo) );
	beam_color_view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	beam_color_view_info.pNext = NULL;
	beam_color_view_info.buffer = transparency.beam_color_buffer.buffer;
	beam_color_view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	beam_color_view_info.range = TR_BEAM_MAX_NUM * TR_COLOR_SIZE;

	VkBufferViewCreateInfo sprite_info_view_info;
	Com_Memset( &sprite_info_view_info, 0, sizeof(VkBufferViewCreateInfo) );
	sprite_info_view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	sprite_info_view_info.pNext = NULL;
	sprite_info_view_info.buffer = transparency.sprite_info_buffer.buffer;
	sprite_info_view_info.format = VK_FORMAT_R32G32_UINT;
	sprite_info_view_info.range = TR_SPRITE_MAX_NUM * TR_SPRITE_INFO_SIZE;

	VkBufferViewCreateInfo beam_intersect_view_info;
	Com_Memset( &beam_intersect_view_info, 0, sizeof(VkBufferViewCreateInfo) );
	beam_intersect_view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	beam_intersect_view_info.pNext = NULL;
	beam_intersect_view_info.buffer = transparency.beam_intersect_buffer.buffer;
	beam_intersect_view_info.format = VK_FORMAT_R32G32B32A32_UINT;
	beam_intersect_view_info.range = TR_BEAM_MAX_NUM * TR_BEAM_INTERSECT_SIZE;

	qvkCreateBufferView(vk.device, &particle_color_view_info, NULL,
		&transparency.particle_color_buffer_view);

	qvkCreateBufferView(vk.device, &beam_color_view_info, NULL,
		&transparency.beam_color_buffer_view);

	qvkCreateBufferView(vk.device, &sprite_info_view_info, NULL,
		&transparency.sprite_info_buffer_view);

	qvkCreateBufferView(vk.device, &beam_intersect_view_info, NULL,
		&transparency.beam_intersect_buffer_view);
}

static void fill_index_buffer(void)
{
	uint16_t* indices = (uint16_t*)transparency.host_buffer_shadow;

	for (size_t i = 0; i < TR_INDEX_MAX_NUM / 6; i++)
	{
		uint16_t* quad = indices + i * 6;

		const uint16_t base_vertex = i * 4;
#if 1
		quad[0] = base_vertex + 0;
		quad[1] = base_vertex + 1;
		quad[2] = base_vertex + 2;
		quad[3] = base_vertex + 2;
		quad[4] = base_vertex + 3;
		quad[5] = base_vertex + 0;
#else
		quad[0] = base_vertex + 0;
		quad[1] = base_vertex + 1;
		quad[2] = base_vertex + 2;
		quad[3] = base_vertex + 2;
		quad[4] = base_vertex + 1;
		quad[5] = base_vertex + 3;
#endif
	}

	memcpy(transparency.mapped_host_buffer, transparency.host_buffer_shadow, sizeof(uint16_t) * TR_INDEX_MAX_NUM);

	VkCommandBuffer cmd_buf = vkpt_begin_command_buffer(&vk.cmd_buffers_transfer);

	VkBufferMemoryBarrier pre_barrier;
	Com_Memset( &pre_barrier, 0, sizeof(VkBufferMemoryBarrier) );
	pre_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	pre_barrier.pNext = NULL;
	pre_barrier.srcAccessMask = 0;
	pre_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	pre_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	pre_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	pre_barrier.buffer = transparency.index_buffer.buffer;
	pre_barrier.size = VK_WHOLE_SIZE;

	qvkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 1, &pre_barrier, 0, NULL);

	VkBufferCopy region;
	Com_Memset( &region, 0, sizeof(VkBufferCopy) );
	region.size = TR_INDEX_MAX_NUM * sizeof(uint16_t);

	qvkCmdCopyBuffer(cmd_buf, transparency.host_buffer, transparency.index_buffer.buffer, 1, &region);

	VkBufferMemoryBarrier post_barrier;
	Com_Memset( &pre_barrier, 0, sizeof(VkBufferMemoryBarrier) );
	post_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	post_barrier.pNext = NULL;
	post_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	post_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	post_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	post_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	post_barrier.buffer = transparency.index_buffer.buffer;
	post_barrier.size = VK_WHOLE_SIZE;

	qvkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 1, &post_barrier, 0, NULL);

	vkpt_submit_command_buffer_simple(cmd_buf, vk.queue_transfer, true);
	vkpt_wait_idle(vk.queue_transfer, &vk.cmd_buffers_transfer);
}
