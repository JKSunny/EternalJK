/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

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
#include "conversion.h"
#include "ghoul2/g2_local.h"

static int entity_frame_num = 0;
static int model_entity_ids[2][MAX_REFENTITIES];
static int world_entity_ids[2][MAX_REFENTITIES];
static int light_entity_ids[2][MAX_MODEL_LIGHTS];
static int model_entity_id_count[2];
static int world_entity_id_count[2];
static int light_entity_id_count[2];
static ModelInstance model_instances_prev[MAX_REFENTITIES];

static uint32_t g_num_instances = 0;
static vk_geometry_instance_t g_instances[MAX_TLAS_INSTANCES];

static const mat4_t g_identity_transform = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

static int			num_model_lights;
static light_poly_t model_lights[MAX_MODEL_LIGHTS];

static qboolean		temporal_frame_valid = qfalse;

static vec3_t avg_envmap_color = { 0.0, 0.0, 0.0 };

typedef struct reference_mode_s 
{
	qboolean enable_accumulation;
	qboolean enable_denoiser;
	float num_bounce_rays;
	float temporal_blend_factor;
	int reflect_refract;
} reference_mode_t;

#define BARRIER_COMPUTE(cmd_buf, img) \
	do { \
		VkImageSubresourceRange range; \
		VkImageMemoryBarrier barrier; \
		Com_Memset( &barrier, 0, sizeof(VkImageMemoryBarrier) ); \
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; \
		range.baseMipLevel = 0; \
		range.levelCount = 1; \
		range.baseArrayLayer = 0; \
		range.layerCount = 1; \
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; \
		barrier.pNext = NULL; \
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; \
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; \
		barrier.image = img.handle; \
		barrier.subresourceRange = range; \
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; \
		/*barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;*/ \
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT; \
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL; \
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL; \
		qvkCmdPipelineBarrier( cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, \
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, \
				1, &barrier); \
	} while(0)

void temporal_cvar_changed( void )
{
	temporal_frame_valid = qfalse;
}

void VK_BeginRenderClear()
{
	vkpt_reset_command_buffers( &vk.cmd_buffers_graphics );
	vkpt_reset_command_buffers( &vk.cmd_buffers_transfer );

	if ( vk.imageDescriptor.needsUpdate == qtrue ) 
	{
		vk_rtx_update_descriptor( &vk.imageDescriptor );
		vk.imageDescriptor.needsUpdate = qfalse;
	}
}

void vkpt_pt_create_all_dynamic( VkCommandBuffer cmd_buf, int idx, const EntityUploadInfo *upload_info )
{
	vk.scratch_buf_ptr = 0;

	accel_build_batch_t batch;
	Com_Memset( &batch, 0, sizeof(accel_build_batch_t) );

	uint64_t offset_vertex_base = 0;
	uint64_t offset_vertex = offset_vertex_base;
	uint64_t offset_index = 0;

	vk_rtx_create_blas( &batch, 
		&vk.buf_positions_instanced, offset_vertex, 
		NULL, offset_index, 		
		upload_info->opaque_prim_count * 3, 0,
		&vk.model_instance.blas.dynamic[idx], 
		qtrue, qtrue, qfalse, 0, "instanced opaque" );

	if ( batch.numBuilds > 0)
		qvkCmdBuildAccelerationStructuresKHR( cmd_buf, batch.numBuilds, batch.buildInfos, batch.rangeInfoPtrs );

	MEM_BARRIER_BUILD_ACCEL(cmd_buf);
	vk.scratch_buf_ptr = 0;
}

static void append_blas( vk_geometry_instance_t *instances, uint32_t *num_instances, vk_blas_t* blas, int vbo_index, uint32_t prim_offset, int mask, int flags, int sbt_offset )
{
	if (!blas->present)
		return;

	vk_geometry_instance_t instance;
	Com_Memset( &instance, 0, sizeof(vk_geometry_instance_t) );

	mat3x4_t transform = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };
	Com_Memcpy( &instance.transform, &transform, sizeof(mat3x4_t) );

	instance.instance_id		= vbo_index;
	instance.mask				= mask;
	instance.instance_offset	= sbt_offset;
	instance.flags				= flags;
	
	VkAccelerationStructureDeviceAddressInfoKHR  info;
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	info.pNext = NULL;
	info.accelerationStructure = blas->accel;

	instance.acceleration_structure = qvkGetAccelerationStructureDeviceAddressKHR(vk.device, &info);

	//assert(*num_instances < INSTANCE_MAX_NUM);
	memcpy(instances + *num_instances, &instance, sizeof(instance));
	vk.uniform_instance_buffer.tlas_instance_prim_offsets[*num_instances] = prim_offset;
	vk.uniform_instance_buffer.tlas_instance_model_indices[*num_instances] = -1;
	++*num_instances;
}

void vkpt_pt_reset_instances()
{
	g_num_instances = 0;
}

#define MAT4_GET(mat, row, col) (mat)[(col) * 4 + (row)]  // For column-major 4x4

//vkpt_pt_instance_model_blas( 
//	&tr.world->geometry.world_static.geom_opaque, g_identity_transform, VERTEX_BUFFER_WORLD, -1, 0 );

void vkpt_pt_instance_model_blas( const model_geometry_t* geom, const mat4_t transform, uint32_t buffer_idx, int model_instance_index, uint32_t override_instance_mask )
{
	if (!geom->accel)
		return;

	vk_geometry_instance_t gpu_instance;
	Com_Memset( &gpu_instance, 0, sizeof(vk_geometry_instance_t) );

	gpu_instance.transform[0] = MAT4_GET(transform, 0, 0);
	gpu_instance.transform[1] = MAT4_GET(transform, 0, 1);
	gpu_instance.transform[2] = MAT4_GET(transform, 0, 2);
	gpu_instance.transform[3] = MAT4_GET(transform, 0, 3);

	gpu_instance.transform[4] = MAT4_GET(transform, 1, 0);
	gpu_instance.transform[5] = MAT4_GET(transform, 1, 1);
	gpu_instance.transform[6] = MAT4_GET(transform, 1, 2);
	gpu_instance.transform[7] = MAT4_GET(transform, 1, 3);

	gpu_instance.transform[8]  = MAT4_GET(transform, 2, 0);
	gpu_instance.transform[9]  = MAT4_GET(transform, 2, 1);
	gpu_instance.transform[10] = MAT4_GET(transform, 2, 2);
	gpu_instance.transform[11] = MAT4_GET(transform, 2, 3);

	gpu_instance.instance_id			= buffer_idx;
	gpu_instance.mask					= override_instance_mask ? override_instance_mask : geom->instance_mask;
	gpu_instance.instance_offset		= geom->sbt_offset;
	gpu_instance.flags					= geom->instance_flags,
	gpu_instance.acceleration_structure = geom->blas_device_address,

	assert(g_num_instances < MAX_TLAS_INSTANCES);
	memcpy(g_instances + g_num_instances, &gpu_instance, sizeof(gpu_instance));
	vk.uniform_instance_buffer.tlas_instance_prim_offsets[g_num_instances] = geom->prim_offsets[0];
	vk.uniform_instance_buffer.tlas_instance_model_indices[g_num_instances] = model_instance_index;
	++g_num_instances;
}

static void vkpt_pt_create_toplevel( VkCommandBuffer cmd_buf, uint32_t idx, world_t &worldData ) 
{
	//
	// model/entity instances
	//
	append_blas( g_instances, &g_num_instances,		
		&vk.model_instance.blas.dynamic[idx], VERTEX_BUFFER_INSTANCED, 0, 
		AS_FLAG_OPAQUE, VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR, SBTO_OPAQUE );

	uint32_t num_instances_geometry = g_num_instances;

	void *instance_data = buffer_map(vk.buf_instances + idx);
	memcpy(instance_data, &g_instances, sizeof(vk_geometry_instance_t) * g_num_instances);

	buffer_unmap(vk.buf_instances + idx);
	instance_data = NULL;

	accel_build_batch_t batch;
	Com_Memset( &batch, 0, sizeof(accel_build_batch_t) );

	vk_rtx_destroy_tlas( &vk.tlas_geometry[idx] );
	vk_rtx_create_tlas( &batch, &vk.tlas_geometry[idx], vk.buf_instances[idx].address, num_instances_geometry );

	qvkCmdBuildAccelerationStructuresKHR( cmd_buf, batch.numBuilds, batch.buildInfos, batch.rangeInfoPtrs );

	MEM_BARRIER_BUILD_ACCEL(cmd_buf); /* probably not needed here but doesn't matter */
}

static uint32_t compute_mesh_material_flags(const trRefEntity_t* entity, const int model_index, shader_t *shader )
{
	rtx_material_t *material = vk_rtx_shader_to_material( shader );

	uint32_t material_id = material->flags;

	if ( ( entity && entity->e.renderfx & RF_FIRST_PERSON ) )
		material_id |= MATERIAL_FLAG_WEAPON;

	return material_id;
}

static inline void transform_point(const float* p, const float* matrix, float* result)
{
	vec4_t point = { p[0], p[1], p[2], 1.f };
	vec4_t transformed;
	mult_matrix_vector(transformed, matrix, point);
	VectorCopy(transformed, result); // vec4 -> vec3
}

static void fill_model_instance( ModelInstance* instance, const trRefEntity_t* entity, const maliasmesh_t *mesh, shader_t *shader,
	const float* transform, qboolean is_viewer_weapon, qboolean is_double_sided,
	qboolean is_mdxm, uint32_t material_id )
{
	int cluster = -1;
	if ( tr.world )
		cluster = BSP_PointLeaf( tr.world->nodes, (float*)entity->e.origin )->cluster;

#if 0
	int frame = entity->e.frame;
	int oldframe = entity->e.oldframe;
#else
	int frame = 0;
	int oldframe = 0;
#endif
	memcpy(instance->transform, transform, sizeof(float) * 16);
	memcpy(instance->transform_prev, transform, sizeof(float) * 16);
	instance->material = material_id;
	instance->shell = 0U;
	instance->cluster = cluster;
	instance->is_mdxm = is_mdxm ? 1 : 0;
	instance->source_buffer_idx = mesh->modelIndex; // + VERTEX_BUFFER_FIRST_MODEL ;
	instance->prim_count = mesh->numIndexes / 3;

	const int offset_cur = 0;
	instance->prim_offset_curr_pose_curr_frame = offset_cur;
	instance->prim_offset_prev_pose_curr_frame = offset_cur;
	instance->prim_offset_curr_pose_prev_frame = instance->prim_offset_curr_pose_curr_frame;
	instance->prim_offset_prev_pose_prev_frame = instance->prim_offset_prev_pose_curr_frame;

#if 1
	instance->pose_lerp_curr_frame = entity->e.backlerp;
	instance->pose_lerp_prev_frame = instance->pose_lerp_curr_frame;
#else
	instance->iqm_matrix_offset_curr_frame = 0;
	instance->iqm_matrix_offset_prev_frame = instance->iqm_matrix_offset_curr_frame;
#endif

	instance->alpha_and_frame = floatToHalf(1.0f);
	instance->render_buffer_idx = 0; // to be filled later
	instance->render_prim_offset = 0;

	instance->idx_offset = mesh->indexOffset;
}

static void add_dlight_spot(const dlight_t* dlight, light_poly_t* light)
{
	// Copy spot data
	VectorCopy(dlight->spot.direction, light->positions + 6);
	uint32_t spot_type = 0;
	uint32_t spot_data = 0;
	switch(dlight->spot.emission_profile)
	{
	case DLIGHT_SPOT_EMISSION_PROFILE_FALLOFF:
		spot_type = DYNLIGHT_SPOT_EMISSION_PROFILE_FALLOFF;
		spot_data = floatToHalf(dlight->spot.cos_total_width) | (floatToHalf(dlight->spot.cos_falloff_start) << 16);
		break;
	case DLIGHT_SPOT_EMISSION_PROFILE_AXIS_ANGLE_TEXTURE:
		spot_type = DYNLIGHT_SPOT_EMISSION_PROFILE_AXIS_ANGLE_TEXTURE;
		spot_data = floatToHalf(dlight->spot.total_width) | (dlight->spot.texture << 16);
		break;
	}
	light->positions[4] = uintBitsToFloat(spot_type);
	light->positions[5] = uintBitsToFloat(spot_data);
}

static void
add_dlights(const dlight_t* dlights, int num_dlights, light_poly_t* light_list, int* num_lights, int max_lights, world_t *worldData, int* light_entity_ids)
{
	for (int i = 0; i < num_dlights; i++)
	{
		if (*num_lights >= max_lights)
			return;

		const dlight_t* dlight = dlights + i;
		light_poly_t* light = light_list + *num_lights;

		light->cluster = BSP_PointLeaf(worldData->nodes, (float*)dlight->origin)->cluster;

		entity_hash_t hash;
		hash.entity = i + 1; //entity ID
		hash.mesh = 0xAA;

		//if(light->cluster >= 0)
		{
			//Super wasteful but we want to have all lights in the same list.

			VectorCopy(dlight->origin, light->positions + 0);
			VectorScale(dlight->color, dlight->radius / 250.f, light->color);
			light->positions[3] = dlight->radius;
			light->material = NULL;
			light->style = 0;

			switch(dlight->light_type) {
				case DLIGHT_SPHERE:
					light->type = LIGHT_SPHERE;
					hash.model = 0xFE;
					break;
				case DLIGHT_SPOT:
					light->type = LIGHT_SPOT;
					// Copy spot data
					add_dlight_spot(dlight, light);
					hash.model = 0xFD;
					break;
			}

			light_entity_ids[(*num_lights)] = *(uint32_t*)&hash;
			(*num_lights)++;

		}
	}
}


#define MESH_FILTER_TRANSPARENT 1
#define MESH_FILTER_OPAQUE 2
#define MESH_FILTER_ALL 3

// bad sunny, rework this
uint32_t			enitity_num_meshes = 0;
static maliasmesh_t	*enitity_meshes[1024];
static shader_t		*enitity_meshes_shader[1024];

void vk_rtx_found_entity_vbo_mesh( maliasmesh_t *mesh, shader_t *shader ) 
{
	enitity_meshes[enitity_num_meshes] = mesh;
	enitity_meshes_shader[enitity_num_meshes++] = shader;
}

static qboolean vk_rtx_find_entity_vbo_meshes( const model_t* model, const uint32_t entityNum, trRefEntity_t *entity )
{
	Com_Memset( &enitity_meshes, NULL, sizeof(maliasmesh_t*) * 1024 );
	Com_Memset( &enitity_meshes_shader, NULL, sizeof(shader_t*) * 1024 );
	enitity_num_meshes = 0;

	if ( model->type == MOD_MDXM || model->type == MOD_BAD  )
		vk_rtx_AddGhoulSurfaces( entity, entityNum );

	else if ( model->type == MOD_MESH )
		vk_rtx_AddMD3Surfaces( entity, entityNum, model );

	return enitity_num_meshes > 0 ? qtrue : qfalse;
}

static void instance_model_lights(int num_light_polys, const light_poly_t* light_polys, const float* transform)
{
	for (int nlight = 0; nlight < num_light_polys; nlight++)
	{
		if (num_model_lights >= MAX_MODEL_LIGHTS)
		{
			assert(!"Model light count overflow");
			break;
		}

		const light_poly_t* src_light = light_polys + nlight;
		light_poly_t* dst_light = model_lights + num_model_lights;

		// Transform the light's positions and center
		transform_point(src_light->positions + 0, transform, dst_light->positions + 0);
		transform_point(src_light->positions + 3, transform, dst_light->positions + 3);
		transform_point(src_light->positions + 6, transform, dst_light->positions + 6);
		transform_point(src_light->off_center, transform, dst_light->off_center);

		// Find the cluster based on the center. Maybe it's OK to use the model's cluster, need to test.
		dst_light->cluster = BSP_PointLeaf(tr.world->nodes, dst_light->off_center)->cluster;

		// We really need to map these lights to a cluster
		if (dst_light->cluster < 0)
			continue;

		// Copy the other light properties
		VectorCopy(src_light->color, dst_light->color);
		dst_light->material = src_light->material;
		dst_light->style = src_light->style;

		num_model_lights++;
	}
}

static void process_bsp_entity(
	const uint32_t entityNum,
	const trRefdef_t *refdef,
	trRefEntity_t *entity, 
	const model_t *model,
	int* instance_count )
{
	InstanceBuffer* uniform_instance_buffer = &vk.uniform_instance_buffer;

	const int current_instance_idx = *instance_count;
	if (current_instance_idx >= MAX_REFENTITIES)
	{
		assert(!"Entity count overflow");
		return;
	}

	mat4_t transform;
	create_entity_matrix( transform, entity, qfalse );

	bmodel_t* bmodel = model->data.bmodel;

	vec3_t origin;
	transform_point(bmodel->center, transform, origin);
	int cluster = BSP_PointLeaf( tr.world->nodes, origin)->cluster;

	if (cluster < 0)
	{
		// In some cases, a model slides into a wall, like a push button, so that its center 
		// is no longer in any BSP node. We still need to assign a cluster to the model,
		// so try the corners of the model instead, see if any of them has a valid cluster.

		for (int corner = 0; corner < 8; corner++)
		{
			vec3_t corner_pt = {
				(corner & 1) ? bmodel->aabb_max[0] : bmodel->aabb_min[0],
				(corner & 2) ? bmodel->aabb_max[1] : bmodel->aabb_min[1],
				(corner & 4) ? bmodel->aabb_max[2] : bmodel->aabb_min[2]
			};

			vec3_t corner_pt_world;
			transform_point(corner_pt, transform, corner_pt_world);

			cluster = BSP_PointLeaf( tr.world->nodes, corner_pt_world)->cluster;

			if(cluster >= 0)
				break;
		}
	}

	entity_hash_t hash;
	hash.entity = entity->e.id;
	hash.model = ~entity->e.hModel;
	hash.mesh = 0;
	hash.bsp = 1;

	memcpy(&model_entity_ids[entity_frame_num][current_instance_idx], &hash, sizeof(uint32_t));

#if 0
	float model_alpha = (entity->flags & RF_TRANSLUCENT) ? entity->alpha : 1.f;
#else
	float model_alpha = 1.f;
#endif

	ModelInstance* mi = uniform_instance_buffer->model_instances + current_instance_idx;
	memcpy(&mi->transform, transform, sizeof(transform));
	memcpy(&mi->transform_prev, transform, sizeof(transform));
	mi->material = 0;
	mi->cluster = cluster;
	mi->source_buffer_idx = VERTEX_BUFFER_SUB_MODELS;
	mi->prim_count = bmodel->geometry.prim_counts[0];
	mi->prim_offset_curr_pose_curr_frame = 0; // bsp models are not processed by the instancing shader
	mi->prim_offset_prev_pose_curr_frame = 0;
	mi->prim_offset_curr_pose_prev_frame = 0;
	mi->prim_offset_prev_pose_prev_frame = 0;
#if 1
	mi->pose_lerp_curr_frame = 0.f;
	mi->pose_lerp_prev_frame = 0.f;
#else
	mi->iqm_matrix_offset_curr_frame = -1;
	mi->iqm_matrix_offset_prev_frame = -1;
#endif
	mi->alpha_and_frame = (entity->e.frame << 16) | floatToHalf(model_alpha);
	mi->render_buffer_idx = VERTEX_BUFFER_SUB_MODELS;
	mi->render_prim_offset = bmodel->geometry.prim_offsets[0];

	if (bmodel->geometry.accel)
	{
		vkpt_pt_instance_model_blas(&bmodel->geometry, mi->transform, VERTEX_BUFFER_SUB_MODELS, current_instance_idx, (model_alpha < 1.f) ? AS_FLAG_TRANSPARENT : 0);
	}

	if (!bmodel->transparent)
	{
		vkpt_shadow_map_add_instance(transform, tr.world->geometry.world_submodels.buffer->buffer,  tr.world->geometry.world_submodels.vertex_data_offset
			+ mi->render_prim_offset * sizeof(prim_positions_t), mi->prim_count);
	}

	(*instance_count)++;
}

static void process_regular_entity( 
	const uint32_t entityNum,
	const trRefdef_t *refdef,
	trRefEntity_t *entity, 
	const model_t *model,
	qboolean is_viewer_weapon, 
	qboolean is_double_sided, 

	int* instance_count, 
	int* animated_count, 
	int* num_instanced_prim, 

	int mesh_filter, 
	qboolean* contains_transparent 
)
{
	qboolean is_mdxm = qfalse;

	if ( model->type == MOD_MDXM || model->type == MOD_BAD )
		is_mdxm = qtrue;

	if ( !vk_rtx_find_entity_vbo_meshes( model, entityNum, entity ) )
		return;

	InstanceBuffer *uniform_instance_buffer = &vk.uniform_instance_buffer;
	uint32_t i;

	mat4_t transform;
	create_entity_matrix( transform, (trRefEntity_t*)entity, qfalse );

	int current_instance_index = *instance_count;
	int current_animated_index = *animated_count;
	int current_num_instanced_prim = *num_instanced_prim;

	if ( contains_transparent )
		*contains_transparent = qfalse;

	bool use_static_blas = false;

	for ( i = 0; i < enitity_num_meshes; i++ ) 
	{
		maliasmesh_t *mesh = enitity_meshes[i];
		shader_t *shader = enitity_meshes_shader[i];

		if ( current_instance_index >= SHADER_MAX_ENTITIES )
			return assert(!"Model entity count overflow");

		if (!use_static_blas && current_animated_index >= SHADER_MAX_ENTITIES)
			return assert(!"Total entity count overflow");

		if ( mesh->indexOffset < 0 ) // failed to upload the vertex data - don't instance this mesh
			return;

		uint32_t material_id = compute_mesh_material_flags( entity, mesh->modelIndex, shader );

		if (!material_id)
			continue;

		entity_hash_t hash;
		hash.entity = entity->e.id;
		hash.model = mesh->modelIndex;
		hash.mesh = i;
		hash.bsp = 0;

		memcpy(&model_entity_ids[entity_frame_num][current_instance_index], &hash, sizeof(uint32_t));
		
		//ModelInstance* mi = uniform_instance_buffer->model_instances + current_instance_index;
		ModelInstance* mi = &uniform_instance_buffer->model_instances[current_instance_index];

		fill_model_instance( mi, entity, mesh, 
							 shader,  transform, is_viewer_weapon, is_double_sided, 
							 is_mdxm, material_id 
		);


#if 0
		// ~sunny, not implemented yet
		if (use_static_blas)
		{
			mi->render_buffer_idx = mi->source_buffer_idx;
			mi->render_prim_offset = mi->prim_offset_curr_pose_curr_frame;

			if (!MAT_IsTransparent(mat_shell.material_id))
			{
				vkpt_shadow_map_add_instance(transform, vbo->buffer.buffer, vbo->vertex_data_offset
					+ mi->render_prim_offset * sizeof(prim_positions_t), mi->prim_count);
			}
		}
		else 
#endif
		{
			uniform_instance_buffer->animated_model_indices[current_animated_index] = current_instance_index;

			mi->render_buffer_idx = VERTEX_BUFFER_INSTANCED;
			mi->render_prim_offset = current_num_instanced_prim;

			current_animated_index++;
			current_num_instanced_prim += mesh->numIndexes / 3;
		}

		current_instance_index++;
	}

#if 0
	// add cylinder lights for wall lamps
	if (model->model_class == MCLASS_STATIC_LIGHT)
	{
		vec4_t begin, end, color;
		vec4_t offset1 = { 0.f, 0.5f, -10.f, 1.f };
		vec4_t offset2 = { 0.f, 0.5f,  10.f, 1.f };

		mult_matrix_vector(begin, transform, offset1);
		mult_matrix_vector(end, transform, offset2);
		VectorSet(color, 0.25f, 0.5f, 0.07f);

		vkpt_build_cylinder_light(model_lights, &num_model_lights, MAX_MODEL_LIGHTS, bsp_world_model, begin, end, color, 1.5f);
	}
#endif

	*instance_count = current_instance_index;
	*animated_count = current_animated_index;
	*num_instanced_prim = current_num_instanced_prim;
}

static void prepare_entities( EntityUploadInfo *upload_info, const trRefdef_t *refdef ) 
{
	uint32_t		i, j;

	entity_frame_num = !entity_frame_num;

	InstanceBuffer *instance_buffer = &vk.uniform_instance_buffer;

	static int transparent_model_indices[MAX_REFENTITIES];
	static int viewer_model_indices[MAX_REFENTITIES];
	static int viewer_weapon_indices[MAX_REFENTITIES];
	static int explosion_indices[MAX_REFENTITIES];
	int transparent_model_num = 0;
	int masked_model_num = 0;
	int viewer_model_num = 0;
	int viewer_weapon_num = 0;
	int explosion_num = 0;

	int model_instance_idx = 0;
	int num_instanced_prim = 0; /* need to track this here to find lights */
	int instance_idx = 0;
	int iqm_matrix_offset = 0;

	for ( i = 0; i < refdef->num_entities; i++ )
	{
		if ( !r_drawentities->integer )
			break;

		trRefEntity_t *entity = refdef->entities + i;

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in
		// mirrors, because the true body position will already be drawn
		//
		//if ( ( entity->e.renderfx & RF_FIRST_PERSON ) && ( tr.viewParms.portalView != PV_NONE ) )
		//	return;

		switch ( entity->e.reType ) 
		{
			case RT_PORTALSURFACE:
				break;		// don't draw anything
			case RT_SPRITE:
			case RT_BEAM:
			case RT_ORIENTED_QUAD:
			case RT_ELECTRICITY:
			case RT_LINE:
			case RT_ORIENTEDLINE:
			case RT_CYLINDER:
			case RT_SABER_GLOW:
			case RT_ENT_CHAIN:
				break;
			case RT_MODEL:
				{
	#if 0
					if ( entity->e.renderfx & RF_FIRST_PERSON ){}
					else if ( entity->e.renderfx & RF_THIRD_PERSON ){}
					else 
	#endif
					model_t *model = R_GetModelByHandle( entity->e.hModel );

					if ( !model )
						continue;
					
					qboolean contains_transparent = qfalse;

					switch ( model->type )
					{
						case MOD_BRUSH:
							{
								process_bsp_entity( i, refdef, entity, model, &model_instance_idx );
							}
							break;
						case MOD_MESH:
						case MOD_MDXM:
						case MOD_BAD:
							{
								process_regular_entity( i, refdef, entity, model, qfalse, qfalse, &model_instance_idx, &instance_idx, &num_instanced_prim, MESH_FILTER_OPAQUE, &contains_transparent );
							
								if (model->num_light_polys > 0)
								{
									mat4_t transform;
									//const bool is_viewer_weapon = (entity->e.renderfx & RF_FIRST_PERSON) != 0;
									create_entity_matrix( transform, entity, qfalse );

									instance_model_lights( model->num_light_polys, model->light_polys, transform );
								}
							}
							break;
						default:
							break;
					}

					if ( contains_transparent )
						transparent_model_indices[transparent_model_num++] = i;

					break;
				}
			default:
				Com_Error(ERR_DROP, "R_AddEntitySurfaces: Bad reType");
				break;
		}
	}

	upload_info->opaque_prim_count = num_instanced_prim;
	//upload_info->transparent_prim_offset = num_instanced_prim;

	// transparent
	{
	}

	// viewer models
	{
	}

	// viewer weapons
	{
	}

	// bind
	upload_info->num_instances = instance_idx;
	upload_info->num_prims  = num_instanced_prim;

	memset(instance_buffer->model_current_to_prev, -1, sizeof(instance_buffer->model_current_to_prev));
	memset(instance_buffer->model_prev_to_current, -1, sizeof(instance_buffer->model_prev_to_current));
	memset(instance_buffer->mlight_prev_to_current, ~0u, sizeof(instance_buffer->mlight_prev_to_current));

	model_entity_id_count[entity_frame_num] = model_instance_idx;
	for(int i = 0; i < model_entity_id_count[entity_frame_num]; i++) {
		for(int j = 0; j < model_entity_id_count[!entity_frame_num]; j++) {
			entity_hash_t hash;
			memcpy(&hash, &model_entity_ids[entity_frame_num][i], sizeof(entity_hash_t));

			if(model_entity_ids[entity_frame_num][i] == model_entity_ids[!entity_frame_num][j] && hash.entity != 0u) {
				instance_buffer->model_current_to_prev[i] = j;
				instance_buffer->model_prev_to_current[j] = i;

				// Copy the "prev" instance paramters from the previous frame's instance buffer
				ModelInstance* mi_curr = instance_buffer->model_instances + i;
				ModelInstance* mi_prev = model_instances_prev + j;

				memcpy(mi_curr->transform_prev, mi_prev->transform, sizeof(mi_curr->transform_prev));
				mi_curr->prim_offset_curr_pose_prev_frame = mi_prev->prim_offset_curr_pose_curr_frame;
				mi_curr->prim_offset_prev_pose_prev_frame = mi_prev->prim_offset_prev_pose_curr_frame;
#if 1
				mi_curr->pose_lerp_prev_frame = mi_prev->pose_lerp_curr_frame;
#else
				mi_curr->iqm_matrix_offset_prev_frame = mi_prev->iqm_matrix_offset_curr_frame;
#endif
			}
		}
	}

	// Save the current model instances for the next frame
	memcpy(model_instances_prev, instance_buffer->model_instances, sizeof(ModelInstance) * model_entity_id_count[entity_frame_num]);
}

static void vk_rtx_process_render_feedback( ref_feedback_t *feedback, mnode_t *viewleaf, 
	qboolean *sun_visible, float *adapted_luminance )
{
	if (viewleaf)
		feedback->viewcluster = viewleaf->cluster;
	else
		feedback->viewcluster = -1;

	{
		static char const * unknown = "<unknown>";
		char const * view_material = unknown;
		char const * view_material_override = unknown;
		ReadbackBuffer readback;
		vk_rtx_readback( &readback );

		if ( readback.material != ~0u )
		{
#if 0
			int material_id = readback.material & MATERIAL_INDEX_MASK;
			pbr_material_t const * material = MAT_GetPBRMaterial(material_id);
			if (material)
			{
				image_t const * image = material->image_diffuse;
				if (image)
				{
					view_material = image->name;
					view_material_override = image->filepath;
				}
			}
#endif
		}
		strcpy(feedback->view_material, view_material);
		strcpy(feedback->view_material_override, view_material_override);

		feedback->lookatcluster = readback.cluster;
		feedback->num_light_polys = 0;

		if ( tr.world && feedback->lookatcluster >= 0 && feedback->lookatcluster < tr.world->numClusters )
		{
			int* light_offsets = tr.world->cluster_light_offsets + feedback->lookatcluster;
			feedback->num_light_polys = light_offsets[1] - light_offsets[0];
		}

		VectorCopy( readback.hdr_color, feedback->hdr_color );

		*sun_visible = readback.sun_luminance > 0.f ? qtrue : qfalse;
		*adapted_luminance = readback.adapted_luminance;
	}
}

static void evaluate_reference_mode( reference_mode_t *ref_mode )
{
#if 0
	if (is_accumulation_rendering_active())
	{
		num_accumulated_frames++;

		const int num_warmup_frames = 5;
		const int num_frames_to_accumulate = get_accumulation_rendering_framenum();

		ref_mode->enable_accumulation = qtrue;
		ref_mode->enable_denoiser = qfalse;
		ref_mode->num_bounce_rays = 2;
		ref_mode->temporal_blend_factor = 1.f / min(max(1, num_accumulated_frames - num_warmup_frames), num_frames_to_accumulate);
		ref_mode->reflect_refract = max(4, cvar_pt_reflect_refract->integer);

		switch (cvar_pt_accumulation_rendering->integer)
		{
		case 1: {
			char text[MAX_QPATH];
			float percentage = powf(max(0.f, (num_accumulated_frames - num_warmup_frames) / (float)num_frames_to_accumulate), 0.5f);
			Q_snprintf(text, sizeof(text), "Photo mode: accumulating samples... %d%%", (int)(min(1.f, percentage) * 100.f));

			int frames_after_accumulation_finished = num_accumulated_frames - num_warmup_frames - num_frames_to_accumulate;
			float hud_alpha = max(0.f, min(1.f, (50 - frames_after_accumulation_finished) * 0.02f)); // fade out for 50 frames after accumulation finishes

			int x = r_config.width / 4;
			int y = 30;
			R_SetScale(0.5f);
			R_SetAlphaScale(hud_alpha);
			draw_shadowed_string(x, y, UI_CENTER, MAX_QPATH, text);

			if (cvar_pt_dof->integer)
			{
				x = 5;
				y = r_config.height / 2 - 55;
				Q_snprintf(text, sizeof(text), "Focal Distance: %.1f", cvar_pt_focus->value);
				draw_shadowed_string(x, y, UI_LEFT, MAX_QPATH, text);

				y += 10;
				Q_snprintf(text, sizeof(text), "Aperture: %.2f", cvar_pt_aperture->value);
				draw_shadowed_string(x, y, UI_LEFT, MAX_QPATH, text);

				y += 10;
				draw_shadowed_string(x, y, UI_LEFT, MAX_QPATH, "Use Mouse Wheel, Shift, Ctrl to adjust");
			}

			R_SetAlphaScale(1.f);

			SCR_SetHudAlpha(hud_alpha);
			break;
		}
		case 2:
			SCR_SetHudAlpha(0.f);
			break;
		}
	}
	else
#endif
	{
		//num_accumulated_frames = 0;

		ref_mode->enable_accumulation = qfalse;
		ref_mode->enable_denoiser = (qboolean)sun_flt_enable->integer;

		if ( sun_pt_num_bounce_rays->value == 0.5f )
			ref_mode->num_bounce_rays = 0.5f;
		else
			ref_mode->num_bounce_rays = MAX( 0, MIN( 2, round( sun_pt_num_bounce_rays->value ) ) );

		ref_mode->temporal_blend_factor = 0.f;
		ref_mode->reflect_refract = MAX( 0, sun_pt_reflect_refract->integer );
	}

	ref_mode->reflect_refract = MIN(10, ref_mode->reflect_refract);
}

static void evaluate_taa_settings( const reference_mode_t* ref_mode )
{
	vk.effective_aa_mode = AA_MODE_OFF;
	vk.extent_taa_output = vk.extent_render;

	if ( !ref_mode->enable_denoiser )
		return;

	if ( sun_flt_taa->integer == AA_MODE_TAA ) // sun_flt_taa
	{
		vk.effective_aa_mode = AA_MODE_TAA;
	}
	else if ( sun_flt_taa->integer == AA_MODE_UPSCALE ) // sun_flt_taa
	{
		if (vk.extent_render.width > vk.extent_unscaled.width || vk.extent_render.height > vk.extent_unscaled.height)
		{
			vk.effective_aa_mode = AA_MODE_TAA;
		}
		else
		{
			vk.effective_aa_mode = AA_MODE_UPSCALE;
			vk.extent_taa_output = vk.extent_unscaled;
		}
	}
}

static void vk_rtx_prepare_ubo( trRefdef_t *refdef, world_t *world, mnode_t *viewleaf, reference_mode_t *ref_mode, const vec3_t sky_matrix[3], qboolean render_world ) 
{
	float			P[16];
	float			V[16];
	float			raw_proj[16];
	float			viewport_proj[16];

	viewParms_t		*vkpt_refdef = &backEnd.viewParms;
	
	vkUniformRTX_t *ubo = &vk.uniform_buffer;
	memcpy( ubo->V_prev, ubo->V, sizeof(float) * 16 );
	memcpy( ubo->P_prev, ubo->P, sizeof(float) * 16 );
	memcpy( ubo->invP_prev, ubo->invP, sizeof(float) * 16 );
	ubo->cylindrical_hfov_prev = ubo->cylindrical_hfov;
	ubo->prev_taa_output_width = ubo->taa_output_width;
	ubo->prev_taa_output_height = ubo->taa_output_height;

	// matrices
	{
		create_projection_matrix( raw_proj, vkpt_refdef->zNear, vkpt_refdef->zFar, refdef->fov_x, refdef->fov_y );

		// In some cases (ex.: player setup), 'fd' will describe a viewport that is not full screen.
		// Simulate that with a projection matrix adjustment to avoid modifying the rendering code.

		Com_Memset( viewport_proj, 0, sizeof(float) * 16);

		viewport_proj[0] = (float)refdef->width / (float)vk.extent_unscaled.width;
		viewport_proj[12] = (float)(refdef->x * 2 + refdef->width - (int)vk.extent_unscaled.width) / (float)vk.extent_unscaled.width;
		viewport_proj[5] = (float)refdef->height / (float)vk.extent_unscaled.height;
		viewport_proj[13] = -(float)(refdef->y * 2 + refdef->height - (int)vk.extent_unscaled.height) / (float)vk.extent_unscaled.height;
		viewport_proj[10] = 1.f;
		viewport_proj[15] = 1.f;

		mult_matrix_matrix( P, viewport_proj, raw_proj );
	}
	create_view_matrix( V, refdef );
	memcpy( ubo->V, V, sizeof(float) * 16 );
	memcpy( ubo->P, P, sizeof(float) * 16 );
	inverse( V, ubo->invV );
	inverse( P, ubo->invP );

	float vfov = refdef->fov_y * (float)M_PI / 180.f;
	float unscaled_aspect = (float)vk.extent_unscaled.width / (float)vk.extent_unscaled.height;
	float rad_per_pixel;
	float fov_scale[2] = { 0.f, 0.f };

	switch (pt_projection->integer)
	{
	case PROJECTION_PANINI:
		fov_scale[1] = tanf(vfov / 2.f);
		fov_scale[0] = fov_scale[1] * unscaled_aspect;
		break;
	case PROJECTION_STEREOGRAPHIC:
		fov_scale[1] = tanf(vfov / 2.f * STEREOGRAPHIC_ANGLE);
		fov_scale[0] = fov_scale[1] * unscaled_aspect;
		break;
	case PROJECTION_CYLINDRICAL:
		rad_per_pixel = atanf(tanf(refdef->fov_y * (float)M_PI / 360.f) / ((float)vk.extent_unscaled.height * 0.5f));
		ubo->cylindrical_hfov = rad_per_pixel * (float)vk.extent_unscaled.width;
		break;
	case PROJECTION_EQUIRECTANGULAR:
		fov_scale[1] = vfov / 2.f;
		fov_scale[0] = fov_scale[1] * unscaled_aspect;
		break;
	case PROJECTION_MERCATOR:
		fov_scale[1] = logf(tanf((float)M_PI * 0.25f + (vfov / 2.f) * 0.5f));
		fov_scale[0] = fov_scale[1] * unscaled_aspect;
		break;
	}

	ubo->projection_fov_scale_prev[0] = ubo->projection_fov_scale[0];
	ubo->projection_fov_scale_prev[1] = ubo->projection_fov_scale[1];
	ubo->projection_fov_scale[0] = fov_scale[0];
	ubo->projection_fov_scale[1] = fov_scale[1];
	ubo->pt_projection = render_world ? pt_projection->integer : 0; // always use rectilinear projection when rendering the player setup view
	ubo->current_frame_idx = vk.frame_counter;	// tr.frameCount, vk.swapchain_image_index;
#ifdef USE_VK_IMGUI
	ubo->render_mode = vk_imgui_get_render_mode();
#else
	ubo->render_mode = 0;
#endif
	ubo->width = vk.extent_render.width;
	ubo->height = vk.extent_render.height;
	ubo->prev_width = vk.extent_render_prev.width;
	ubo->prev_height = vk.extent_render_prev.height;
	ubo->inv_width = 1.0f / (float)vk.extent_render.width;
	ubo->inv_height = 1.0f / (float)vk.extent_render.height;
	ubo->unscaled_width = vk.extent_unscaled.width;
	ubo->unscaled_height = vk.extent_unscaled.height;
	ubo->taa_image_width = vk.extent_taa_images.width;
	ubo->taa_image_height = vk.extent_taa_images.height;
	ubo->taa_output_width = vk.extent_taa_output.width;
	ubo->taa_output_height = vk.extent_taa_output.height;
	ubo->current_gpu_slice_width = vk.gpu_slice_width;
	ubo->prev_gpu_slice_width = vk.gpu_slice_width_prev;
	ubo->screen_image_width = vk.extent_screen_images.width;
	ubo->screen_image_height = vk.extent_screen_images.height;
	//ubo->water_normal_texture = water_normal_texture - r_images;
	ubo->pt_swap_checkerboard = 0;
	vk.extent_render_prev = vk.extent_render;
	vk.gpu_slice_width_prev = vk.gpu_slice_width;

	int camera_cluster_contents = viewleaf ? viewleaf->contents : 0;

	if ( camera_cluster_contents & CONTENTS_WATER )
		ubo->medium = MEDIUM_WATER;
	else if ( camera_cluster_contents & CONTENTS_SLIME )
		ubo->medium = MEDIUM_SLIME;
	else if ( camera_cluster_contents & CONTENTS_LAVA )
		ubo->medium = MEDIUM_LAVA;
	else
		ubo->medium = MEDIUM_NONE;

	ubo->time = refdef->time;
	ubo->num_static_lights = render_world ? world->num_light_polys : 0;

#define UBO_CVAR_DO( handle, default_value ) ubo->handle = sun_##handle->value;
	UBO_CVAR_LIST
#undef UBO_CVAR_DO
	
	if ( !ref_mode->enable_denoiser ) 
	{
		// disable fake specular because it is not supported without denoiser, and the result
		// looks too dark with it missing
		ubo->pt_fake_roughness_threshold = 1.f;

		// swap the checkerboard fields every frame in reference or noisy mode to accumulate 
		// both reflection and refraction in every pixel
		ubo->pt_swap_checkerboard = (vk.frame_counter & 1);

		if (ref_mode->enable_accumulation)
		{
			//ubo->pt_texture_lod_bias = -log2(sqrt(get_accumulation_rendering_framenum()));

			// disable the other stabilization hacks
			ubo->pt_specular_anti_flicker = 0.f;
			ubo->pt_sun_bounce_range = 10000.f;
			ubo->pt_ndf_trim = 1.f;
		}
	} 
	else if ( vk.effective_aa_mode == AA_MODE_UPSCALE )
	{
		// adjust texture LOD bias to the resolution scale, i.e. use negative bias if scale is < 100
		//float resolution_scale = (drs_effective_scale != 0) ? (float)drs_effective_scale : (float)scr_viewsize->integer;
		//resolution_scale *= 0.01f;
		//resolution_scale = clamp(resolution_scale, 0.1f, 1.f);
		//ubo->pt_texture_lod_bias = cvar_pt_texture_lod_bias->value + log2f(resolution_scale);
	}

	{
		// figure out if DoF should be enabled in the current rendering mode
		qboolean enable_dof = qtrue;

		switch ( pt_dof->integer) 
		{
			case 0: enable_dof = qfalse; break;
			case 1: enable_dof = ref_mode->enable_accumulation; break;
			case 2: enable_dof = ref_mode->enable_denoiser ? qfalse : qtrue; break;
			default: enable_dof = qtrue; break;
		}
		
		// DoF does not make physical sense with the cylindrical projection
		if ( pt_projection->integer != 0) 
			enable_dof = qfalse;
		
		// if DoF should not be enabled, make the aperture size zero
		if (!enable_dof)
			ubo->pt_aperture = 0.f;
	}

	// number of polygon vertices must be an integer
	ubo->pt_aperture_type = roundf(ubo->pt_aperture_type);

	ubo->temporal_blend_factor = ref_mode->temporal_blend_factor;	
	ubo->flt_enable = ref_mode->enable_denoiser;	
	ubo->flt_taa = vk.effective_aa_mode;
	ubo->pt_num_bounce_rays = ref_mode->num_bounce_rays;
	ubo->pt_reflect_refract = ref_mode->reflect_refract;

	if ( ref_mode->num_bounce_rays < 1.f )
		ubo->pt_specular_mis = 0; // disable MIS if there are no specular rays

	ubo->pt_min_log_sky_luminance = exp2f(ubo->pt_min_log_sky_luminance);
	ubo->pt_max_log_sky_luminance = exp2f(ubo->pt_max_log_sky_luminance);

	//memcpy(ubo->cam_pos, backEnd.viewParms.ori.origin, sizeof(float) * 3);
	memcpy(ubo->cam_pos, refdef->vieworg, sizeof(float) * 3);
	ubo->cluster_debug_index = vk.cluster_debug_index;

	if ( !temporal_frame_valid )
	{
		ubo->flt_temporal_lf = 0;
		ubo->flt_temporal_hf = 0;
		ubo->flt_temporal_spec = 0;
		ubo->flt_taa = 0;
	}

	if ( vk.effective_aa_mode == AA_MODE_UPSCALE )
	{
		int taa_index = (int)(vk.frame_counter % NUM_TAA_SAMPLES);
		ubo->sub_pixel_jitter[0] = vk.taa_samples[taa_index][0];
		ubo->sub_pixel_jitter[1] = vk.taa_samples[taa_index][1];
	}
	else
	{
		ubo->sub_pixel_jitter[0] = 0.f;
		ubo->sub_pixel_jitter[1] = 0.f;
	}

	memset( ubo->environment_rotation_matrix, 0, sizeof(ubo->environment_rotation_matrix) );
	VectorCopy( sky_matrix[0], ubo->environment_rotation_matrix + 0 );
	VectorCopy( sky_matrix[1], ubo->environment_rotation_matrix + 4 );
	VectorCopy( sky_matrix[2], ubo->environment_rotation_matrix + 8 );

	//add_dlights( refdef->dlights, refdef->num_dlights, ubo );
	//add_dlights( backEnd.viewParms.dlights, backEnd.viewParms.num_dlights, ubo );

	ubo->pt_cameras = 0;
	//ubo->num_cameras = 0;
}

static void
update_mlight_prev_to_current(void)
{
	light_entity_id_count[entity_frame_num] = num_model_lights;
	for(int i = 0; i < light_entity_id_count[entity_frame_num]; i++) {
		entity_hash_t hash = *(entity_hash_t*)&light_entity_ids[entity_frame_num][i];
		if(hash.entity == 0u) continue;
		for(int j = 0; j < light_entity_id_count[!entity_frame_num]; j++) {
			if(light_entity_ids[entity_frame_num][i] == light_entity_ids[!entity_frame_num][j]) {
				vk.uniform_instance_buffer.mlight_prev_to_current[j] = i;
				break;
			}
		}
	}
}

static void vk_rtx_setup_rt_pipeline( VkCommandBuffer cmd_buf, VkPipelineBindPoint bind_point, uint32_t index )
{
	// https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8038
	qvkCmdBindPipeline( cmd_buf, bind_point, vk.rt_pipelines[index] );

	VkDescriptorSet desc_sets[] = {
		vk.rt_descriptor_set[vk.current_frame_index].set,
        vk_rtx_get_current_desc_set_textures(),
		vk.imageDescriptor.set,
        vk.desc_set_ubo,
		vk.desc_set_vertex_buffer[vk.current_frame_index].set,
	};

	qvkCmdBindDescriptorSets( cmd_buf, bind_point,
			vk.rt_pipeline_layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0);
}

static void dispatch_rays( VkCommandBuffer cmd_buf, uint32_t pipeline_index, pt_push_constants_t push, uint32_t height ) 
{
	vk_rtx_setup_rt_pipeline( cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_index );

	assert( vk.buf_shader_binding_table.address );

	qvkCmdPushConstants( cmd_buf, vk.rt_pipeline_layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(push), &push );	

	uint32_t sbt_offset = SBT_ENTRIES_PER_PIPELINE * pipeline_index * vk.shaderGroupBaseAlignment;

	VkStridedDeviceAddressRegionKHR raygen;
	raygen.deviceAddress = vk.buf_shader_binding_table.address + sbt_offset;
	raygen.stride = vk.shaderGroupBaseAlignment;
	raygen.size = vk.shaderGroupBaseAlignment;

	VkStridedDeviceAddressRegionKHR miss_and_hit;
	miss_and_hit.deviceAddress = vk.buf_shader_binding_table.address + sbt_offset;
	miss_and_hit.stride = vk.shaderGroupBaseAlignment;
	miss_and_hit.size = (VkDeviceSize)vk.shaderGroupBaseAlignment * SBT_ENTRIES_PER_PIPELINE;

	VkStridedDeviceAddressRegionKHR callable;
	callable.deviceAddress = NULL;
	callable.stride = 0;
	callable.size = 0;

	qvkCmdTraceRaysKHR( cmd_buf,
		&raygen,
		&miss_and_hit,
		&miss_and_hit,
		&callable,
		vk.extent_render.width / 2, height, vk.device_count == 1 ? 2 : 1);
}

static void vk_rtx_trace_primary_rays( VkCommandBuffer cmd_buf )
{
	int frame_idx = vk.frame_counter & 1;

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_SHADER_WRITE_BIT,
		vk.buf_readback.buffer,
		0,
		VK_WHOLE_SIZE
	);

	BEGIN_PERF_MARKER( cmd_buf, PROFILER_PRIMARY_RAYS );

	pt_push_constants_t push;
	push.gpu_index = -1; // vk.device_count == 1 ? -1 : i;
	push.bounce = 0;	
		
	dispatch_rays( cmd_buf, PIPELINE_PRIMARY_RAYS, push, vk.extent_render.height );

	END_PERF_MARKER( cmd_buf, PROFILER_PRIMARY_RAYS );

	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_VISBUF_PRIM_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_VISBUF_BARY_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_TRANSPARENT] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_MOTION] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_SHADING_POSITION] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_VIEW_DIRECTION] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_THROUGHPUT] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_BOUNCE_THROUGHPUT] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_GODRAYS_THROUGHPUT_DIST] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_BASE_COLOR_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_METALLIC_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_CLUSTER_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_VIEW_DEPTH_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_NORMAL_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_ASVGF_RNG_SEED_A + frame_idx] );
#ifdef USE_RTX_INSPECT_TANGENTS
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_TANGENT_A + frame_idx] );
#endif
}

static void vk_rtx_trace_reflections( VkCommandBuffer cmd_buf, int bounce ) 
{
	int frame_idx = vk.frame_counter & 1;

	uint32_t pipeline_index = (bounce == 0) ? PIPELINE_REFLECT_REFRACT_1 : PIPELINE_REFLECT_REFRACT_2;

	pt_push_constants_t push;
	push.gpu_index = -1; // vk.device_count == 1 ? -1 : i;
	push.bounce = bounce;

	dispatch_rays( cmd_buf, pipeline_index, push, vk.extent_render.height );

	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_TRANSPARENT] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_MOTION] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_SHADING_POSITION] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_VIEW_DIRECTION] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_THROUGHPUT] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_GODRAYS_THROUGHPUT_DIST] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_BASE_COLOR_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_METALLIC_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_CLUSTER_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_VIEW_DEPTH_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_NORMAL_A + frame_idx] );
#ifdef USE_RTX_INSPECT_TANGENTS
	//BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_TANGENT_A + frame_idx] );
#endif
}

static void vk_rxt_trace_lighting( VkCommandBuffer cmd_buf, float num_bounce_rays )
{
	// direct lighting
	BEGIN_PERF_MARKER(cmd_buf, PROFILER_DIRECT_LIGHTING);

	for ( int i = 0; i < vk.device_count; i++ )
	{
		uint32_t pipeline_index = (pt_caustics->value != 0) ? PIPELINE_DIRECT_LIGHTING_CAUSTICS : PIPELINE_DIRECT_LIGHTING;

		pt_push_constants_t push;
		push.gpu_index = vk.device_count == 1 ? -1 : i;
		push.bounce = 0;

		dispatch_rays( cmd_buf, pipeline_index, push, vk.extent_render.height );
	}

	END_PERF_MARKER(cmd_buf, PROFILER_DIRECT_LIGHTING);

	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_COLOR_LF_SH] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_COLOR_LF_COCG] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_COLOR_HF] );
	BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_COLOR_SPEC] );

	if ( pt_restir->value != 0 ) 
	{
		int frame_idx = vk.frame_counter & 1;
		BARRIER_COMPUTE(cmd_buf, vk.img_rtx[RTX_IMG_PT_RESTIR_A + frame_idx]);
	}

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		vk.buf_readback.buffer,
		0,
		VK_WHOLE_SIZE
	);

	// indirect lighting
	BEGIN_PERF_MARKER( cmd_buf, PROFILER_INDIRECT_LIGHTING );

	if ( num_bounce_rays > 0 ) 
	{
		for (int i = 0; i < vk.device_count; i++)
		{
			int height;
			if (num_bounce_rays == 0.5f)
				height = vk.extent_render.height / 2;
			else
				height = vk.extent_render.height;	

			for (int bounce_ray = 0; bounce_ray < (int)ceilf(num_bounce_rays); bounce_ray++)
			{
				uint32_t pipeline_index = (bounce_ray == 0) ? PIPELINE_INDIRECT_LIGHTING_FIRST : PIPELINE_INDIRECT_LIGHTING_SECOND;

				pt_push_constants_t push;
				push.gpu_index = vk.device_count == 1 ? -1 : i;
				push.bounce = 0;
		
				dispatch_rays( cmd_buf, pipeline_index, push, height );

				BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_COLOR_LF_SH] );
				BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_COLOR_LF_COCG] );
				BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_COLOR_HF] );
				BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_COLOR_SPEC] );
				BARRIER_COMPUTE( cmd_buf, vk.img_rtx[RTX_IMG_PT_BOUNCE_THROUGHPUT] );
			}
		}
	}

	END_PERF_MARKER( cmd_buf, PROFILER_INDIRECT_LIGHTING );
}

static VkResult vkpt_final_blit_simple( VkCommandBuffer cmd_buf )
{
	VkImageSubresourceRange subresource_range;
	subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel = 0;
	subresource_range.levelCount = 1;
	subresource_range.baseArrayLayer = 0;
	subresource_range.layerCount = 1;

	IMAGE_BARRIER(cmd_buf,
		vk.color_image,
		subresource_range,
		0,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

#ifdef USE_VK_IMGUI
	VkImage output_img = vk_imgui_get_rtx_render_mode();
#else
	VkImage output_img = vk.img_rtx[RTX_IMG_TAA_OUTPUT].handle;
#endif

	IMAGE_BARRIER( cmd_buf,
		output_img,
		subresource_range,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	);

	VkOffset3D blit_size;
	blit_size.x = vk.extent_taa_output.width;
	blit_size.y = vk.extent_taa_output.height;
	blit_size.z = 1;

	VkOffset3D blit_size_unscaled;
	blit_size_unscaled.x = vk.extent_unscaled.width;
	blit_size_unscaled.y = vk.extent_unscaled.height;
	blit_size_unscaled.z = 1;

	VkImageBlit img_blit;
	Com_Memset( &img_blit, 0, sizeof(VkImageBlit) );
	img_blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	img_blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	img_blit.srcOffsets[1] = blit_size;
	img_blit.dstOffsets[1] = blit_size_unscaled;

	qvkCmdBlitImage( cmd_buf,
		output_img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		vk.color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &img_blit, VK_FILTER_NEAREST );

	IMAGE_BARRIER( cmd_buf,
		vk.color_image,
		subresource_range,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		0,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	IMAGE_BARRIER( cmd_buf,
		output_img,
		subresource_range,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL
	);

	return VK_SUCCESS;
}

VkResult
vkpt_light_buffer_upload_staging( VkCommandBuffer cmd_buf )
{
	vkbuffer_t *staging = vk.buf_light_staging + vk.current_frame_index;

	assert( !staging->is_mapped );

	VkBufferCopy copyRegion = { 0, 0, sizeof(LightBuffer) };

	qvkCmdCopyBuffer( cmd_buf, staging->buffer, vk.buf_light.buffer, 1, &copyRegion );

	int buffer_idx = vk.frame_counter % 3;
	if ( vk.buf_light_stats[buffer_idx].buffer )
	{
		qvkCmdFillBuffer(cmd_buf, vk.buf_light_stats[buffer_idx].buffer, 0, vk.buf_light_stats[buffer_idx].size, 0);
	}

	return VK_SUCCESS;
}

static void vk_rtx_end_command_buffer( 
	VkCommandBuffer cmd_buf, 
	int wait_semaphore_count, VkSemaphore *wait_semaphores, VkPipelineStageFlags *wait_stages,
	int signal_semaphore_count, VkSemaphore *signal_semaphores ) 
{
	VkSubmitInfo submit_info;

	VK_CHECK( qvkEndCommandBuffer( cmd_buf ) );

	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf;

	if ( !ri.VK_IsMinimized() ) 
	{			
		submit_info.waitSemaphoreCount		= wait_semaphore_count;
		submit_info.pWaitSemaphores			= wait_semaphores;
		submit_info.pWaitDstStageMask		= wait_stages;
		submit_info.signalSemaphoreCount	= signal_semaphore_count;
		submit_info.pSignalSemaphores		= signal_semaphores;
	}
	else 
	{
		submit_info.waitSemaphoreCount		= 0;
		submit_info.pWaitSemaphores			= NULL;
		submit_info.pWaitDstStageMask		= NULL;
		submit_info.signalSemaphoreCount	= 0;
		submit_info.pSignalSemaphores		= NULL;
	}

	VK_CHECK( qvkQueueSubmit( vk.queue, 1, &submit_info, VK_NULL_HANDLE ) );
};

static VkResult vkpt_pt_update_descripter_set_bindings( int idx )
{
	vk_rtx_bind_descriptor_as( &vk.rt_descriptor_set[idx], RAY_GEN_DESCRIPTOR_SET_IDX, VK_SHADER_STAGE_RAYGEN_BIT_KHR, &vk.tlas_geometry[idx].accel );
	vk_rtx_update_descriptor( &vk.rt_descriptor_set[idx] );

	return VK_SUCCESS;
}

static void vk_begin_trace_rays( world_t &worldData, trRefdef_t *refdef, reference_mode_t *ref_mode, 
	vkUniformRTX_t *ubo, drawSurf_t *drawSurfs, int numDrawSurfs, 
	float *shadowmap_view_proj, qboolean god_rays_enabled, qboolean render_world, 
	const EntityUploadInfo *upload_info ) 
{
	VkSemaphore					transfer_semaphores;
	VkSemaphore					trace_semaphores;
	VkSemaphore					prev_trace_semaphores;
	VkPipelineStageFlags		wait_stages;
	//VkCommandBuffer				transfer_cmd_buf;
	//VkCommandBufferBeginInfo	begin_info;
	uint32_t device_indices[2];
	uint32_t all_device_mask = (1 << vk.device_count) - 1;
	bool *prev_trace_signaled = &vk.tess[(vk.current_frame_index - 1) % NUM_COMMAND_BUFFERS].semaphores.trace_signaled;
	bool *curr_trace_signaled = &vk.tess[vk.current_frame_index].semaphores.trace_signaled;

	{
		// Transfer the light buffer from staging into device memory.
		// Previous frame's tracing still uses device memory, so only do the copy after that is finished.
			
		VkCommandBuffer transfer_cmd_buf = vkpt_begin_command_buffer(&vk.cmd_buffers_transfer);

		vkpt_light_buffer_upload_staging( transfer_cmd_buf );

		for ( int gpu = 0; gpu < vk.device_count; gpu++ )	// multi-gpu not implemented
		{
			device_indices[gpu]		= gpu;
			transfer_semaphores		= vk.tess[vk.current_frame_index].semaphores.transfer_finished;
			trace_semaphores		= vk.tess[vk.current_frame_index].semaphores.trace_finished;
			prev_trace_semaphores	= vk.tess[(vk.current_frame_index - 1) % NUM_COMMAND_BUFFERS].semaphores.trace_finished;
			wait_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}

		vkpt_submit_command_buffer(
			transfer_cmd_buf, 
			vk.queue_transfer, 
			all_device_mask, 
			(*prev_trace_signaled) ? vk.device_count : 0, &prev_trace_semaphores, &wait_stages, device_indices, 
			vk.device_count, &transfer_semaphores, device_indices, 
			VK_NULL_HANDLE);

		*prev_trace_signaled = false;
	}

	{
		VkCommandBuffer trace_cmd_buf = vkpt_begin_command_buffer(&vk.cmd_buffers_graphics);

		// Copy the UBO contents from the staging buffer.
		// Actual contents are uploaded to the staging UBO below, right before executing the command buffer.
		vkpt_uniform_buffer_copy_from_staging( trace_cmd_buf );
		//VK_CHECK( vkpt_uniform_buffer_update( trace_cmd_buf ) );

		BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_UPDATE_ENVIRONMENT );
		if ( render_world )
		{
			vkpt_physical_sky_record_cmd_buffer( trace_cmd_buf );
		}
		END_PERF_MARKER( trace_cmd_buf, PROFILER_UPDATE_ENVIRONMENT );

		BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_INSTANCE_GEOMETRY );
		vkpt_instance_geometry( trace_cmd_buf, upload_info->num_instances, qfalse );
		END_PERF_MARKER( trace_cmd_buf, PROFILER_INSTANCE_GEOMETRY );

		BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_BVH_UPDATE );
		vkpt_pt_create_all_dynamic( trace_cmd_buf, vk.current_frame_index, upload_info );
		vkpt_pt_create_toplevel( trace_cmd_buf, vk.current_frame_index, worldData );
		vkpt_pt_update_descripter_set_bindings( vk.current_frame_index );
		END_PERF_MARKER( trace_cmd_buf, PROFILER_BVH_UPDATE );

		BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_SHADOW_MAP );
		if ( god_rays_enabled )
		{
			vk_rtx_shadow_map_render( trace_cmd_buf, worldData, shadowmap_view_proj, 
				tr.world->geometry.world_static.geom_opaque.prim_offsets[0] * 3,
				tr.world->geometry.world_static.geom_opaque.prim_counts[0] * 3,
				0,
				upload_info->opaque_prim_count * 3,
				tr.world->geometry.world_static.geom_transparent.prim_offsets[0] * 3,
				tr.world->geometry.world_static.geom_transparent.prim_counts[0] * 3
			);
		}
		END_PERF_MARKER( trace_cmd_buf, PROFILER_SHADOW_MAP );

		vk_rtx_trace_primary_rays( trace_cmd_buf );

		// The host-side image of the uniform buffer is only ready after the `vkpt_pt_create_toplevel` call above
		VK_CHECK( vkpt_uniform_buffer_upload_to_staging() );

		vkpt_submit_command_buffer(
			trace_cmd_buf,
			vk.queue_graphics,
			all_device_mask,
			vk.device_count, &transfer_semaphores, &wait_stages, device_indices,
			0, 0, 0,
			VK_NULL_HANDLE);
	}


	{
		VkCommandBuffer trace_cmd_buf = vkpt_begin_command_buffer( &vk.cmd_buffers_graphics );

		if ( god_rays_enabled )
		{
			BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_GOD_RAYS );
			vk_rtx_record_god_rays_trace_command_buffer( trace_cmd_buf, 0 );
			END_PERF_MARKER( trace_cmd_buf, PROFILER_GOD_RAYS );
		}

		if ( ref_mode->reflect_refract > 0 )
		{
			BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_REFLECT_REFRACT_1 );
			vk_rtx_trace_reflections( trace_cmd_buf, 0 );
			END_PERF_MARKER( trace_cmd_buf, PROFILER_REFLECT_REFRACT_1 );
		}

		if ( god_rays_enabled )
		{
			if ( ref_mode->reflect_refract > 0 )
			{
				BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_GOD_RAYS_REFLECT_REFRACT );
				vk_rtx_record_god_rays_trace_command_buffer( trace_cmd_buf, 1 );
				END_PERF_MARKER(trace_cmd_buf, PROFILER_GOD_RAYS_REFLECT_REFRACT );
			}

			BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_GOD_RAYS_FILTER );
			vk_rtx_record_god_rays_filter_command_buffer( trace_cmd_buf );
			END_PERF_MARKER( trace_cmd_buf, PROFILER_GOD_RAYS_FILTER );
		}

		if ( ref_mode->reflect_refract > 1 )
		{
			BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_REFLECT_REFRACT_2 );
			for (int pass = 0; pass < ref_mode->reflect_refract - 1; pass++)
			{
				vk_rtx_trace_reflections( trace_cmd_buf, pass );
			}
			END_PERF_MARKER( trace_cmd_buf, PROFILER_REFLECT_REFRACT_2 );
		}

		if ( ref_mode->enable_denoiser ) 
		{
			BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_ASVGF_GRADIENT_REPROJECT );
			vkpt_asvgf_gradient_reproject( trace_cmd_buf );
			END_PERF_MARKER( trace_cmd_buf, PROFILER_ASVGF_GRADIENT_REPROJECT );
		}
		vk_rxt_trace_lighting( trace_cmd_buf, ref_mode->num_bounce_rays );

		vkpt_submit_command_buffer(
			trace_cmd_buf,
			vk.queue_graphics,
			all_device_mask,
			0, 0, 0, 0,
			vk.device_count, &trace_semaphores, device_indices,
			VK_NULL_HANDLE);

		*curr_trace_signaled = true;
	}

	{
		VkCommandBuffer post_cmd_buf = vkpt_begin_command_buffer(&vk.cmd_buffers_graphics);

		BEGIN_PERF_MARKER( post_cmd_buf, PROFILER_ASVGF_FULL );
		if ( ref_mode->enable_denoiser ) 
			vkpt_asvgf_filter( post_cmd_buf, sun_pt_num_bounce_rays->value >= 0.5f ? qtrue : qfalse );
		else
			vkpt_compositing( post_cmd_buf );
		END_PERF_MARKER( post_cmd_buf, PROFILER_ASVGF_FULL );

		vkpt_interleave( post_cmd_buf );

		vkpt_taa( post_cmd_buf );

#if 0
		BEGIN_PERF_MARKER(post_cmd_buf, PROFILER_BLOOM);
		if ( cvar_bloom_enable->integer != 0 || menu_mode )
		{
			vkpt_bloom_record_cmd_buffer(post_cmd_buf);
		}
		END_PERF_MARKER(post_cmd_buf, PROFILER_BLOOM);
#endif

		BEGIN_PERF_MARKER( post_cmd_buf, PROFILER_TONE_MAPPING );
		if ( sun_tm_enable->integer != 0 )
		{
			float frame_time = MIN( 1.f, MAX(0.f, refdef->frametime) );

			static unsigned previous_wallclock_time = 0;
			unsigned current_wallclock_time = ri.Milliseconds();
			float frame_wallclock_time = (previous_wallclock_time != 0) ? (float)(current_wallclock_time - previous_wallclock_time) * 1e-3f : 0.f;
			previous_wallclock_time = current_wallclock_time;

			vkpt_tone_mapping_record_cmd_buffer( post_cmd_buf, frame_time <= 0.f ? frame_wallclock_time : frame_time );
		}
		END_PERF_MARKER( post_cmd_buf, PROFILER_TONE_MAPPING );

		{
			VkBufferCopy copyRegion = { 0, 0, sizeof(ReadbackBuffer) };
			qvkCmdCopyBuffer( post_cmd_buf, vk.buf_readback.buffer, vk.buf_readback_staging[vk.current_frame_index].buffer, 1, &copyRegion);
		}

		vkpt_submit_command_buffer_simple( post_cmd_buf, vk.queue_graphics, true );
	}
}

static qboolean frame_ready;

void vk_rtx_begin_scene( trRefdef_t *refdef, drawSurf_t *drawSurfs, int numDrawSurfs ) 
{
#if 0
	// reset offsets
	vk.scratch_buf_ptr = 0;
#endif
	reference_mode_t	ref_mode;
	vkUniformRTX_t		*ubo;
	sun_light_t			sun_light;
	vec3_t				sky_matrix[3];
	qboolean			render_world = qfalse;
	mnode_t				*viewleaf;

	if ( tr.world )
		render_world = qtrue;

	//bool render_world = (fd->rdflags & RDF_NOWORLDMODEL) == 0;
#if 0
	if ( !temporal_frame_valid )
	{
		if ( vkpt_refdef.fd && vkpt_refdef.fd->lightstyles )
			memcpy( vkpt_refdef.prev_lightstyles, vkpt_refdef.fd->lightstyles, sizeof(vkpt_refdef.prev_lightstyles) );
		else
			memset( vkpt_refdef.prev_lightstyles, 0, sizeof(vkpt_refdef.prev_lightstyles) );
	}
#endif

	viewleaf = tr.world ? BSP_PointLeaf( tr.world->nodes, refdef->vieworg ) : NULL;

	qboolean sun_visible_prev = qfalse;
	static float prev_adapted_luminance = 0.f;
	float adapted_luminance = 0.f;
	vk_rtx_process_render_feedback( &refdef->feedback, viewleaf, &sun_visible_prev, &adapted_luminance );

	// Sometimes, the readback returns 1.0 luminance instead of the real value.
	// Ignore these mysterious spikes.
	if ( adapted_luminance != 1.0f ) 
		prev_adapted_luminance = adapted_luminance;
	
	if ( prev_adapted_luminance <= 0.f )
		prev_adapted_luminance = 0.005f;

	if ( !tr.world && !render_world )
		return;

	// environment	
	prepare_sky_matrix( refdef->time, sky_matrix );
	
	Com_Memset( &sun_light, 0, sizeof(sun_light_t) );
	if ( render_world )
	{
		vk_rtx_evaluate_sun_light( &sun_light, sky_matrix, refdef->time );

		if ( !vkpt_physical_sky_needs_update() )
			sun_light.visible = (qboolean)(sun_light.visible && sun_visible_prev);
	}
	
	evaluate_reference_mode( &ref_mode );
	evaluate_taa_settings( &ref_mode );

	num_model_lights = 0;
	EntityUploadInfo upload_info;
	Com_Memset( &upload_info, 0, sizeof(EntityUploadInfo) );
	vkpt_pt_reset_instances();
	vkpt_shadow_map_reset_instances();
	prepare_entities( &upload_info, refdef );

	if ( tr.world && render_world )
	{
		vkpt_pt_instance_model_blas( &tr.world->geometry.world_static.geom_opaque,					g_identity_transform, VERTEX_BUFFER_WORLD, -1, 0 );
		vkpt_pt_instance_model_blas( &tr.world->geometry.world_static.geom_transparent,				g_identity_transform, VERTEX_BUFFER_WORLD, -1, 0 );
		vkpt_pt_instance_model_blas( &tr.world->geometry.world_dynamic_material.geom_opaque,		g_identity_transform, VERTEX_BUFFER_WORLD_D_MATERIAL, -1, 0 );
		//vkpt_pt_instance_model_blas( &tr.world->geometry.world_dynamic_material.geom_transparent,	g_identity_transform, VERTEX_BUFFER_WORLD_D_MATERIAL, -1, 0 );
		vkpt_pt_instance_model_blas( &tr.world->geometry.world_dynamic_geometry.geom_opaque,		g_identity_transform, VERTEX_BUFFER_WORLD_D_GEOMETRY, -1, 0 );
		//vkpt_pt_instance_model_blas( &tr.world->geometry.world_dynamic_geometry.geom_transparent,	g_identity_transform, VERTEX_BUFFER_WORLD_D_GEOMETRY, -1, 0 );
		vkpt_pt_instance_model_blas( &tr.world->geometry.sky_static.geom_opaque,					g_identity_transform, VERTEX_BUFFER_SKY, -1, 0 );

#ifdef DEBUG_POLY_LIGHTS
		if ( pt_debug_poly_lights->integer )
			vkpt_pt_instance_model_blas( &tr.world->geometry.debug_light_polys.geom_opaque,			g_identity_transform, VERTEX_BUFFER_DEBUG_LIGHT_POLYS, -1, 0 );
#endif

#if 0
		vkpt_build_beam_lights(model_lights, &num_model_lights, MAX_MODEL_LIGHTS, bsp_world_model, fd->entities, fd->num_entities, prev_adapted_luminance, light_entity_ids[entity_frame_num], &num_model_lights);
#endif
		add_dlights(refdef->dlights, refdef->num_dlights, model_lights, &num_model_lights, MAX_MODEL_LIGHTS, tr.world, light_entity_ids[entity_frame_num]);
	}

	update_mlight_prev_to_current();

#if 1
	vkpt_vertex_buffer_ensure_primbuf_size(upload_info.num_prims);
#endif

	ubo = &vk.uniform_buffer;
	vk_rtx_prepare_ubo( refdef, tr.world, viewleaf, &ref_mode, sky_matrix, render_world );
	ubo->prev_adapted_luminance = prev_adapted_luminance;

#if 0
	if ( tm_blend_enable->integer )
		Vector4Copy( fd->blend, ubo->fs_blend_color );
	else
#endif
		Vector4Set( ubo->fs_blend_color, 0.f, 0.f, 0.f, 0.f );

#if 0
	if (vkpt_refdef.fd->rdflags & RDF_IRGOGGLES)
		Vector4Set(ubo->fs_colorize, 1.f, 0.f, 0.f, 0.8f);
	else
#endif
		Vector4Set( ubo->fs_colorize, 0.f, 0.f, 0.f, 0.f );

	vk_rtx_physical_sky_update_ubo( ubo, &sun_light, render_world );
#if 0
	vkpt_bloom_update(ubo, frame_time, ubo->medium != MEDIUM_NONE, menu_mode);
#endif

	vec3_t sky_radiance;
	VectorScale( avg_envmap_color, ubo->pt_env_scale, sky_radiance );

	vkpt_light_buffer_upload_to_staging( render_world, tr.world, num_model_lights, model_lights, sky_radiance );

	float shadowmap_view_proj[16];
	float shadowmap_depth_scale;
	const float unused = 0.0f;
	vk_rtx_shadow_map_setup(
		&sun_light,
		tr.world->world_aabb.mins,
		tr.world->world_aabb.maxs,
		shadowmap_view_proj,
		&shadowmap_depth_scale,
		qfalse);

	vk_rtx_god_rays_prepare_ubo(
		ubo,
		&tr.world->world_aabb,
		ubo->P,
		ubo->V,
		shadowmap_view_proj,
		shadowmap_depth_scale);

	qboolean god_rays_enabled = ( vk_rtx_god_rays_enabled(&sun_light) && render_world) ? qtrue : qfalse;

	vk_begin_trace_rays( *tr.world, refdef, &ref_mode, ubo, 
		drawSurfs, numDrawSurfs, shadowmap_view_proj, 
		god_rays_enabled, render_world, &upload_info );

	temporal_frame_valid = ref_mode.enable_denoiser;

	frame_ready = qtrue;

#if 0
	if ( vkpt_refdef.fd && vkpt_refdef.fd->lightstyles ) {
		memcpy(vkpt_refdef.prev_lightstyles, vkpt_refdef.fd->lightstyles, sizeof(vkpt_refdef.prev_lightstyles));
	}
#endif

	VK_BeginRenderClear();
}

static inline qboolean extents_equal( VkExtent2D a, VkExtent2D b )
{
	return (qboolean)( a.width == b.width && a.height == b.height );
}

void vk_rtx_begin_blit( void ) 
{
	if ( !frame_ready )
		return;

	vk_end_render_pass(); // end main

	if ( vk.effective_aa_mode == AA_MODE_UPSCALE )
	{
		vkpt_final_blit_simple( vk.cmd->command_buffer );
	}
	else
	{
		VkExtent2D extent_unscaled_half;
		extent_unscaled_half.width = vk.extent_unscaled.width / 2;
		extent_unscaled_half.height = vk.extent_unscaled.height / 2;

		if ( extents_equal( vk.extent_render, vk.extent_unscaled ) ||
			extents_equal( vk.extent_render, extent_unscaled_half )/* && drs_effective_scale == 0*/ ) // don't do nearest filter 2x upscale with DRS enabled
			vkpt_final_blit_simple( vk.cmd->command_buffer );
		else {
			vkpt_final_blit_filtered( vk.cmd->command_buffer );
		}
	}

	frame_ready = qfalse;

	vk_begin_post_blend_render_pass( vk.render_pass.rtx_final_blit.blend, qfalse );
}