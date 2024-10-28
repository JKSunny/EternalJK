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
#include "ghoul2/g2_local.h"

typedef struct entity_hash_s {
	unsigned int mesh : 8;
	unsigned int model : 9;
	unsigned int entity : 15;
} entity_hash_t;

static int entity_frame_num = 0;
static int model_entity_ids[2][MAX_REFENTITIES];
static int world_entity_ids[2][MAX_REFENTITIES];
static int model_entity_id_count[2];
static int world_entity_id_count[2];

static int			num_model_lights;
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
	if ( vk.imageDescriptor.needsUpdate == qtrue ) 
	{
		vk_rtx_update_descriptor( &vk.imageDescriptor );
		vk.imageDescriptor.needsUpdate = qfalse;
	}
}

void append_blas( vk_geometry_instance_t *instances, int *num_instances, uint32_t type, vk_blas_t* blas, int instance_id, int mask, int flags, int sbt_offset )
{
	vk_geometry_instance_t instance;
	Com_Memset( &instance, 0, sizeof(vk_geometry_instance_t) );

	instance.instance_id = instance_id;
	instance.mask = RAY_FIRST_PERSON_MIRROR_OPAQUE_VISIBLE;
	instance.instance_offset = sbt_offset;
	instance.flags = flags;
	
	VkAccelerationStructureDeviceAddressInfoKHR  info;
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	info.pNext = NULL;
	info.accelerationStructure = blas->accel_khr;
	instance.acceleration_structure = qvkGetAccelerationStructureDeviceAddressKHR(vk.device, &info);

	mat3x4_t transform = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };

	Com_Memcpy( &instance.transform, &transform, sizeof(mat3x4_t) );

	// additional instance data
	// rework this
	{
		blas->data.type = type;

		//Com_Memcpy( &blas->data.modelmat, &transform, sizeof(mat3x4_t) );

		vkbuffer_t *instance_buffer_data	= &vk.buffer_blas_instance_data[vk.swapchain_image_index];
		vk_rtx_upload_buffer_data_offset( instance_buffer_data, *num_instances * sizeof(ASInstanceData), sizeof(ASInstanceData), (const byte*)&blas->data );
	}

	//assert(*num_instances < INSTANCE_MAX_NUM);
	memcpy(instances + *num_instances, &instance, sizeof(instance));
	vk.buffer_uniform_instance.tlas_instance_type[*num_instances] = type;
	++*num_instances;
}

void vkpt_pt_create_all_dynamic( VkCommandBuffer cmd_buf, int idx, const EntityUploadInfo *upload_info )
{
	vk.scratch_buffer_ptr = 0;
	uint64_t offset_vertex_base = offsetof(ModelDynamicVertexBuffer, positions_instanced);
	uint64_t offset_vertex = offset_vertex_base;
	uint64_t offset_index = 0;

	const qboolean instanced = qtrue;

	vk_rtx_create_blas( cmd_buf, 
		&vk.model_instance.buffer_vertex, offset_vertex, NULL, offset_index, 		
		upload_info->dynamic_vertex_num, 0,
		&vk.model_instance.blas.dynamic[idx], 
		qtrue, qtrue, qfalse, instanced );

	MEM_BARRIER_BUILD_ACCEL(cmd_buf);
	vk.scratch_buffer_ptr = 0;
}

static void vkpt_pt_create_toplevel( VkCommandBuffer cmd_buf, uint32_t idx, drawSurf_t *drawSurfs, int numDrawSurfs ) 
{
	vk_geometry_instance_t instances[1000];
	int num_instances = 0;

	//
	// world instances ( static & dynamic, no model/entity )
	//
	append_blas( instances, &num_instances, BAS_WORLD_STATIC,		&vk.blas_static.world, 0, 0, 0, 0 );
	
	append_blas( instances, &num_instances, BAS_WORLD_DYNAMIC_DATA,	&vk.blas_dynamic.data_world, 0, 0, 0, 0 );
	//append_blas( instances, &num_instances, BAS_WORLD_DYNAMIC_AS,	&vk.blas_dynamic.data_world_transparent, 0, 0, 0, 0 );
	
	append_blas( instances, &num_instances, BAS_WORLD_DYNAMIC_AS,	&vk.blas_dynamic.as_world[idx], 0, 0, 0, 0 );
	//append_blas( instances, &num_instances, BAS_WORLD_DYNAMIC_AS,	&vk.blas_dynamic.as_world_transparent[idx], 0, 0, 0, 0 );
	
	//
	// model/entity instances
	//
	append_blas( instances, &num_instances, BAS_ENTITY_DYNAMIC,		&vk.model_instance.blas.dynamic[idx], AS_INSTANCE_FLAG_DYNAMIC, 0, 0, 0 );

	void *instance_data = buffer_map(vk.buffer_blas_instance + idx);
	memcpy(instance_data, &instances, sizeof(vk_geometry_instance_t) * num_instances);

	buffer_unmap(vk.buffer_blas_instance + idx);
	instance_data = NULL;

	vk_rtx_destroy_tlas( &vk.tlas_geometry[idx] );
	vk_rtx_create_tlas( cmd_buf, &vk.tlas_geometry[idx], vk.buffer_blas_instance[idx].address, num_instances);
}

static inline uint32_t fill_mdxm_instance( const trRefEntity_t* entity, const mdxmVBOMesh_t* mesh, shader_t *shader,
	const float* transform, int model_instance_index, qboolean is_viewer_weapon, qboolean is_double_sided )
{
	uint32_t material_index, material_flags;

	vk_rtx_shader_to_material( shader, material_index, material_flags );
	uint32_t material_id = (material_flags & ~MATERIAL_INDEX_MASK) | (material_index & MATERIAL_INDEX_MASK);

	ModelInstance *instance = &vk.buffer_uniform_instance.model_instances[model_instance_index];
	memcpy(instance->M, transform, sizeof(float) * 16);

	instance->idx_offset = mesh->indexOffset;
	instance->model_index = mesh->modelIndex;	
#ifdef USE_RTX_GLOBAL_MODEL_VBO
	instance->offset_curr = 0;
	instance->offset_prev = 0;
#else
	//int frame = entity->e.frame;
	//int oldframe = entity->e.oldframe;

	frame = 0;
	instance->offset_curr = mesh->vertexOffset + frame * mesh->numVertexes * (sizeof(model_vertex_t) / sizeof(uint32_t));
	instance->offset_prev = mesh->vertexOffset + frame * mesh->numVertexes * (sizeof(model_vertex_t) / sizeof(uint32_t));
#endif
	instance->backlerp = entity->e.backlerp;
	instance->material = material_id;
	instance->alpha = /*(entity->flags & RF_TRANSLUCENT) ? entity->alpha :*/ 1.0f;

	return material_id;
}

#define MESH_FILTER_TRANSPARENT 1
#define MESH_FILTER_OPAQUE 2
#define MESH_FILTER_ALL 3

// bad sunny, rework this
uint32_t					mdxm_meshes_num = 0;
static mdxmVBOMesh_t		*mdxm_meshes[200];
static shader_t				*mdxm_meshes_shader[200];

void vk_rtx_found_mdxm_vbo_mesh( mdxmVBOMesh_t *mesh, shader_t *shader ) 
{
	mdxm_meshes[mdxm_meshes_num] = mesh;
	mdxm_meshes_shader[mdxm_meshes_num++] = shader;
}

static qboolean vk_rtx_find_mdxm_meshes_for_entity( const uint32_t entityNum, trRefEntity_t *entity )
{
	// bad sunny, rework this
	Com_Memset( &mdxm_meshes, NULL, sizeof(mdxmVBOMesh_t*) * 200 );
	Com_Memset( &mdxm_meshes_shader, NULL, sizeof(shader_t*) * 200 );
	mdxm_meshes_num = 0;

	if ( entity->e.ghoul2 && G2API_HaveWeGhoul2Models( *( (CGhoul2Info_v*)entity->e.ghoul2 ) ) )
		vk_rtx_AddGhoulSurfaces( entity, entityNum );

	return mdxm_meshes_num > 0 ? qtrue : qfalse;
}

static void process_regular_entity( 
	const uint32_t entityNum,
	const trRefdef_t *refdef,
	trRefEntity_t* entity, 
	qboolean is_viewer_weapon, 
	qboolean is_double_sided, 
	int* model_instance_idx, 
	int* instance_idx, 
	int* num_instanced_vert, 
	int mesh_filter, 
	qboolean* contains_transparent )
{
	// find all meshes for this ghoul2 entity
	if ( !vk_rtx_find_mdxm_meshes_for_entity( entityNum, entity ) )
		return;

	vkInstanceRTX_t *uniform_instance_buffer = &vk.buffer_uniform_instance;
	uint32_t *ubo_instance_buf_offset	= (uint32_t*)uniform_instance_buffer->model_instance_buf_offset;
	uint32_t *ubo_instance_buf_size		= (uint32_t*)uniform_instance_buffer->model_instance_buf_size;
	uint32_t *ubo_model_idx_offset		= (uint32_t*)uniform_instance_buffer->model_idx_offset;
	uint32_t *ubo_model_cluster_id		= (uint32_t*)uniform_instance_buffer->model_cluster_id;
	uint32_t i;

	float transform[16];
	create_entity_matrix( transform, (trRefEntity_t*)entity, qfalse );

	int current_model_instance_index = *model_instance_idx;
	int current_instance_index = *instance_idx;
	int current_num_instanced_vert = *num_instanced_vert;

	if ( contains_transparent )
		*contains_transparent = qfalse;

	int render_first_model_only = -1;

	for ( i = 0; i < mdxm_meshes_num; i++ ) 
	{
		mdxmVBOMesh_t *mesh = mdxm_meshes[i];
		shader_t *shader = mdxm_meshes_shader[i];

		render_first_model_only = mesh->modelIndex;

		//if ( render_first_model_only > 0 && render_first_model_only != mesh->modelIndex )
		//	break;

		//if ( i > 0 )
		//	break;

		// do checks here
		if ( current_model_instance_index >= SHADER_MAX_ENTITIES )
			return assert(!"Model entity count overflow");

		if ( current_instance_index >= ( SHADER_MAX_ENTITIES + SHADER_MAX_BSP_ENTITIES ) )
			return assert(!"Total entity count overflow");

		if ( mesh->indexOffset < 0 ) // failed to upload the vertex data - don't instance this mesh
			return;

		uint32_t material_id = fill_mdxm_instance( entity, mesh, shader,  transform, current_model_instance_index, is_viewer_weapon, is_double_sided );
		if (!material_id)
			return;

		entity_hash_t hash;
		hash.entity = entity->e.id;
		hash.model = mesh->modelIndex;
		hash.mesh = i;

		model_entity_ids[entity_frame_num][current_model_instance_index] = *(uint32_t*)&hash;

		uint32_t cluster_id = ~0u;
		if ( tr.world )
			cluster_id = R_FindClusterForPos3( *tr.world, entity->e.origin );
		ubo_model_cluster_id[current_model_instance_index] = cluster_id;

		ubo_model_idx_offset[current_model_instance_index] = mesh->indexOffset;
		
		ubo_instance_buf_offset[current_model_instance_index] = current_num_instanced_vert / 3;
		ubo_instance_buf_size[current_model_instance_index] = mesh->numIndexes / 3;

		((int*)uniform_instance_buffer->model_indices)[current_instance_index] = current_model_instance_index;
	
		current_model_instance_index++;
		current_instance_index++;
		current_num_instanced_vert += mesh->numIndexes;
	}

	*model_instance_idx = current_model_instance_index;
	*instance_idx = current_instance_index;
	*num_instanced_vert = current_num_instanced_vert;
}

static void prepare_entities( EntityUploadInfo *upload_info, const trRefdef_t *refdef ) 
{
	uint32_t		i, j;

	entity_frame_num = !entity_frame_num;

	vkInstanceRTX_t *instance_buffer = &vk.buffer_uniform_instance;

	memcpy( instance_buffer->bsp_mesh_instances_prev, instance_buffer->bsp_mesh_instances,
		sizeof(instance_buffer->bsp_mesh_instances_prev) );
	memcpy( instance_buffer->model_instances_prev, instance_buffer->model_instances,
		sizeof(instance_buffer->model_instances_prev) );

	memcpy( instance_buffer->bsp_cluster_id_prev, instance_buffer->bsp_cluster_id, sizeof(instance_buffer->bsp_cluster_id) );
	memcpy( instance_buffer->model_cluster_id_prev, instance_buffer->model_cluster_id, sizeof(instance_buffer->model_cluster_id) );

	static int transparent_model_indices[MAX_REFENTITIES];
	static int viewer_model_indices[MAX_REFENTITIES];
	static int viewer_weapon_indices[MAX_REFENTITIES];
	static int explosion_indices[MAX_REFENTITIES];
	int transparent_model_num = 0;
	int viewer_model_num = 0;
	int viewer_weapon_num = 0;
	int explosion_num = 0;

	int model_instance_idx = 0;
	int bsp_mesh_idx = 0;
	int num_instanced_vert = 0; /* need to track this here to find lights */
	int instance_idx = 0;

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

					switch ( model->type )
					{
						case MOD_MESH:
						case MOD_BRUSH:
							break;
						case MOD_MDXM:
						case MOD_BAD:
							{
								qboolean contains_transparent = qfalse;
								process_regular_entity( i, refdef, entity, qfalse, qfalse, &model_instance_idx, &instance_idx, &num_instanced_vert, 2, &contains_transparent );
					
								if ( contains_transparent )
									transparent_model_indices[transparent_model_num++] = i;
							}
							break;
						default:
							break;
					}

					break;
				}
			default:
				Com_Error(ERR_DROP, "R_AddEntitySurfaces: Bad reType");
				break;
		}
	}

	upload_info->dynamic_vertex_num = num_instanced_vert;

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
	upload_info->num_vertices  = num_instanced_vert;

	memset(instance_buffer->world_current_to_prev, ~0u, sizeof(instance_buffer->world_current_to_prev));
	memset(instance_buffer->world_prev_to_current, ~0u, sizeof(instance_buffer->world_prev_to_current));
	memset(instance_buffer->model_current_to_prev, ~0u, sizeof(instance_buffer->model_current_to_prev));
	memset(instance_buffer->model_prev_to_current, ~0u, sizeof(instance_buffer->model_prev_to_current));

#if 0
	world_entity_id_count[entity_frame_num] = bsp_mesh_idx;
	for(int i = 0; i < world_entity_id_count[entity_frame_num]; i++) {
		for(int j = 0; j < world_entity_id_count[!entity_frame_num]; j++) {
			if(world_entity_ids[entity_frame_num][i] == world_entity_ids[!entity_frame_num][j]) {
				instance_buffer->world_current_to_prev[i] = j;
				instance_buffer->world_prev_to_current[j] = i;
			}
		}
	}
#endif

	model_entity_id_count[entity_frame_num] = model_instance_idx;
	for ( i = 0; i < model_entity_id_count[entity_frame_num]; i++ ) 
	{
		for ( j = 0; j < model_entity_id_count[!entity_frame_num]; j++ ) 
		{
			entity_hash_t hash = *(entity_hash_t*)&model_entity_ids[entity_frame_num][i];

			if ( model_entity_ids[entity_frame_num][i] == model_entity_ids[!entity_frame_num][j] && hash.entity != 0 ) 
			{
				instance_buffer->model_current_to_prev[i] = j;
				instance_buffer->model_prev_to_current[j] = i;
			}
		}
	}
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

		if ( tr.world && feedback->lookatcluster >= 0 && feedback->lookatcluster < vk.numClusters )
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

static void vk_rtx_prepare_ubo( trRefdef_t *refdef, world_t *world, vkUniformRTX_t *ubo, mnode_t *viewleaf, reference_mode_t *ref_mode, const vec3_t sky_matrix[3], qboolean render_world ) 
{
	float			P[16];
	float			V[16];
	float			raw_proj[16];
	float			viewport_proj[16];

	viewParms_t		*vkpt_refdef = &backEnd.viewParms;
	
	memcpy( ubo->V_prev, ubo->V, sizeof(float) * 16 );
	memcpy( ubo->P_prev, ubo->P, sizeof(float) * 16 );
	memcpy( ubo->invP_prev, ubo->invP, sizeof(float) * 16 );
	//ubo->cylindrical_hfov_prev = ubo->cylindrical_hfov;
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
	
		create_view_matrix( V, refdef );
		memcpy( ubo->V, V, sizeof(float) * 16 );
		memcpy( ubo->P, P, sizeof(float) * 16 );
		inverse( V, ubo->invV );
		inverse( P, ubo->invP );
	}

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

	ubo->pt_cameras = 0;
	//ubo->num_cameras = 0;
}

static void vk_rtx_trace_rays( VkCommandBuffer cmd_buf, vkpipeline_t *pipeline, uint32_t height, int group_index ) 
{
	assert( vk.rt_shader_binding_table.address );

	VkStridedDeviceAddressRegionKHR raygen;
	raygen.deviceAddress = vk.rt_shader_binding_table.address + group_index * vk.shaderGroupBaseAlignment;
	raygen.stride = vk.shaderGroupBaseAlignment;
	raygen.size = vk.shaderGroupBaseAlignment;

	VkStridedDeviceAddressRegionKHR miss_and_hit;
	miss_and_hit.deviceAddress = vk.rt_shader_binding_table.address;
	miss_and_hit.stride = vk.shaderGroupBaseAlignment;
	miss_and_hit.size = vk.shaderGroupBaseAlignment;

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

static void vk_rtx_trace_primary_rays( VkCommandBuffer cmd_buf, uint32_t idx )
{
	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_SHADER_WRITE_BIT,
		vk.buffer_readback.buffer,
		0,
		VK_WHOLE_SIZE
	);

	VkDescriptorSet desc_sets[] = {
		vk.rtxDescriptor[idx].set,
        vk_rtx_get_current_desc_set_textures(),
		vk.imageDescriptor.set,
        vk.desc_set_ubo
	};

	BEGIN_PERF_MARKER( cmd_buf, PROFILER_PRIMARY_RAYS );

	// https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8038
	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vk.rt_pipeline.handle );

	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			vk.rt_pipeline.layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0);

	pt_push_constants_t push;
	push.gpu_index = -1; // vk.device_count == 1 ? -1 : i;
	push.bounce = 0;
	qvkCmdPushConstants( cmd_buf, vk.rt_pipeline.layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(push), &push );		
		
	vk_rtx_trace_rays( cmd_buf, &vk.rt_pipeline, vk.extent_render.height, SBT_RGEN_PRIMARY_RAYS );

	END_PERF_MARKER( cmd_buf, PROFILER_PRIMARY_RAYS );

	int frame_idx = idx;

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_VISBUF_PRIM_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_VISBUF_BARY_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_TRANSPARENT] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_MOTION] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_SHADING_POSITION] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_VIEW_DIRECTION] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_THROUGHPUT] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_BOUNCE_THROUGHPUT] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_GODRAYS_THROUGHPUT_DIST] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_BASE_COLOR_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_METALLIC_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_CLUSTER_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_VIEW_DEPTH_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_NORMAL_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_RNG_SEED_A + frame_idx] );
#ifdef USE_RTX_INSPECT_TANGENTS
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_TANGENT_A + frame_idx] );
#endif
}

static void vk_rtx_trace_reflections( VkCommandBuffer cmd_buf, uint32_t idx, int bounce ) 
{

	VkDescriptorSet desc_sets[] = {
		vk.rtxDescriptor[idx].set,
        vk_rtx_get_current_desc_set_textures(),
		vk.imageDescriptor.set,
        vk.desc_set_ubo
	};

	// reflections/refractions
	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vk.rt_pipeline.handle );

	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			vk.rt_pipeline.layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0);

	pt_push_constants_t push;
	push.gpu_index = -1; // vk.device_count == 1 ? -1 : i;
	push.bounce = bounce;
	qvkCmdPushConstants( cmd_buf, vk.rt_pipeline.layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(push), &push );	

	int shader = (bounce == 0) ? SBT_RGEN_REFLECT_REFRACT1 : SBT_RGEN_REFLECT_REFRACT2;

	vk_rtx_trace_rays( cmd_buf, &vk.rt_pipeline, vk.extent_render.height, shader );

	int frame_idx = idx;

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_TRANSPARENT] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_MOTION] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_SHADING_POSITION] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_VIEW_DIRECTION] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_THROUGHPUT] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_GODRAYS_THROUGHPUT_DIST] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_BASE_COLOR_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_METALLIC_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_CLUSTER_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_VIEW_DEPTH_A + frame_idx] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_NORMAL_A + frame_idx] );
#ifdef USE_RTX_INSPECT_TANGENTS
	//BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_TANGENT_A + frame_idx] );
#endif
}

static void vk_rxt_trace_lighting( VkCommandBuffer cmd_buf, uint32_t idx, float num_bounce_rays )
{
	VkDescriptorSet desc_sets[] = {
		vk.rtxDescriptor[idx].set,
        vk_rtx_get_current_desc_set_textures(),
		vk.imageDescriptor.set,
        vk.desc_set_ubo
	};

	// direct lighting
	BEGIN_PERF_MARKER(cmd_buf, PROFILER_DIRECT_LIGHTING);

	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vk.rt_pipeline.handle );
	
	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			vk.rt_pipeline.layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0);

	pt_push_constants_t push;
	push.gpu_index = -1; // vk.device_count == 1 ? -1 : i;
	push.bounce = 0;
	qvkCmdPushConstants( cmd_buf, vk.rt_pipeline.layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(push), &push );

	int rgen_index = SBT_RGEN_DIRECT_LIGHTING;
	if ( pt_caustics->value != 0 )
		rgen_index = SBT_RGEN_DIRECT_LIGHTING_CAUSTICS;

	vk_rtx_trace_rays( cmd_buf, &vk.rt_pipeline, vk.extent_render.height, rgen_index );

	END_PERF_MARKER(cmd_buf, PROFILER_DIRECT_LIGHTING);

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_LF_SH] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_LF_COCG] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_HF] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_SPEC] );

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		vk.buffer_readback.buffer,
		0,
		VK_WHOLE_SIZE
	);

	// indirect lighting
	BEGIN_PERF_MARKER( cmd_buf, PROFILER_INDIRECT_LIGHTING );

	if ( num_bounce_rays > 0 ) 
	{
		qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vk.rt_pipeline.handle );

		qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			vk.rt_pipeline.layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0);

		pt_push_constants_t push;
		push.gpu_index = -1; // vk.device_count == 1 ? -1 : i;
		push.bounce = 0;
		qvkCmdPushConstants( cmd_buf, vk.rt_pipeline.layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(push), &push );

		for (int bounce_ray = 0; bounce_ray < (int)ceilf(num_bounce_rays); bounce_ray++)
		{
			uint32_t height;
			if (num_bounce_rays == 0.5f)
				height = vk.extent_render.height / 2;
			else
				height = vk.extent_render.height;	

			int rgen_index = (bounce_ray == 0) 
				? SBT_RGEN_INDIRECT_LIGHTING_FIRST 
				: SBT_RGEN_INDIRECT_LIGHTING_SECOND;

			vk_rtx_trace_rays( cmd_buf, &vk.rt_pipeline, height, rgen_index );
		}

		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_LF_SH] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_LF_COCG] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_HF] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_SPEC] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_BOUNCE_THROUGHPUT] );
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

	int output_img = RTX_IMG_TAA_OUTPUT;

	IMAGE_BARRIER( cmd_buf,
		vk.rtx_images[output_img].handle,
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
		vk.rtx_images[output_img].handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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
		vk.rtx_images[output_img].handle,
		subresource_range,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL
	);

	return VK_SUCCESS;
}

VkResult
vkpt_light_buffer_upload_staging( VkCommandBuffer cmd_buf, uint32_t idx )
{
	vkbuffer_t *staging = vk.buffer_light_staging + idx;

	assert( !staging->is_mapped );

	VkBufferCopy copyRegion = { 0, 0, sizeof(LightBuffer) };

	qvkCmdCopyBuffer( cmd_buf, staging->buffer, vk.buffer_light.buffer, 1, &copyRegion );

	// sunny this is questionable, were not using frame_counter for frame idx
	// see vk_rtx_get_descriptor_index()
	int buffer_idx = vk.frame_counter % 3;
	if ( vk.buffer_light_stats[buffer_idx].buffer )
	{
		qvkCmdFillBuffer(cmd_buf, vk.buffer_light_stats[buffer_idx].buffer, 0, vk.buffer_light_stats[buffer_idx].allocSize, 0);
	}


	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_NONE_KHR,
		VK_ACCESS_SHADER_WRITE_BIT,
		vk.buffer_light.buffer,
		0,
		VK_WHOLE_SIZE
	);

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

//#define VK_RTX_INVESTIGATE_SLOW_CMD_BUF
#define VK_RTX_SHADOWMAP

static void vk_begin_trace_rays( trRefdef_t *refdef, reference_mode_t *ref_mode, 
	vkUniformRTX_t *ubo, drawSurf_t *drawSurfs, int numDrawSurfs, 
	float *shadowmap_view_proj, qboolean god_rays_enabled, qboolean render_world, 
	const EntityUploadInfo *upload_info ) 
{
	VkSemaphore					transfer_semaphores;
	VkSemaphore					trace_semaphores;
	VkSemaphore					prev_trace_semaphores;
	VkPipelineStageFlags		wait_stages;
	VkCommandBuffer				transfer_cmd_buf;
	VkCommandBufferBeginInfo	begin_info;
	uint32_t					idx, prev_idx;

	uint32_t command_index = (uint32_t)vk.cmd_index;
	uint32_t prev_command_index = ( command_index - 1 ) % NUM_COMMAND_BUFFERS;
	qboolean *prev_trace_signaled = &vk.tess[prev_command_index].semaphores.trace_signaled;
	qboolean *curr_trace_signaled = &vk.tess[command_index].semaphores.trace_signaled;
	
	vk_rtx_get_descriptor_index( idx, prev_idx );

	{
		// Transfer the light buffer from staging into device memory.
		// Previous frame's tracing still uses device memory, so only do the copy after that is finished.
			
		transfer_cmd_buf = vk.cmd->command_buffer_transfer;

		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = VK_NULL_HANDLE;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = VK_NULL_HANDLE;

		VK_CHECK( qvkBeginCommandBuffer( transfer_cmd_buf, &begin_info ) );

		vkpt_light_buffer_upload_staging( transfer_cmd_buf, idx );

		for ( int gpu = 0; gpu < vk.device_count; gpu++ )	// multi-gpu not implemented
		{
			transfer_semaphores		= vk.tess[command_index].semaphores.transfer_finished;
			trace_semaphores		= vk.tess[command_index].semaphores.trace_finished;
			prev_trace_semaphores	= vk.tess[prev_command_index].semaphores.trace_finished;
			wait_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}

		vk_rtx_end_command_buffer( transfer_cmd_buf, 
			(*prev_trace_signaled) ? 1 : 0, &prev_trace_semaphores, &wait_stages,
			1, &transfer_semaphores 
		);

		*prev_trace_signaled = qfalse;
	}

	{
		VkCommandBuffer trace_cmd_buf = vk.cmd->command_buffer_trace2;	// temp
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = VK_NULL_HANDLE;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = VK_NULL_HANDLE;

		VK_CHECK( qvkBeginCommandBuffer( trace_cmd_buf, &begin_info ) );

		VK_CHECK( vkpt_uniform_buffer_update( trace_cmd_buf, idx ) );

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
		vkpt_pt_create_all_dynamic( trace_cmd_buf, idx, upload_info );
		vkpt_pt_create_toplevel( trace_cmd_buf, idx, drawSurfs, numDrawSurfs );
		vk_rtx_bind_descriptor_as( &vk.rtxDescriptor[idx], BINDING_OFFSET_AS,VK_SHADER_STAGE_RAYGEN_BIT_KHR, &vk.tlas_geometry[idx].accel_khr );
		vk_rtx_update_descriptor( &vk.rtxDescriptor[idx] );
		END_PERF_MARKER( trace_cmd_buf, PROFILER_BVH_UPDATE );

		BEGIN_PERF_MARKER( trace_cmd_buf, PROFILER_SHADOW_MAP );
		if ( god_rays_enabled )
		{
#ifdef VK_RTX_SHADOWMAP
			vk_rtx_shadow_map_render( trace_cmd_buf, shadowmap_view_proj,
				vk.geometry.idx_world_static_offset, 0, 0, 0 );
#endif
		}
		END_PERF_MARKER( trace_cmd_buf, PROFILER_SHADOW_MAP );

		vk_rtx_trace_primary_rays( trace_cmd_buf, idx );

		vk_rtx_end_command_buffer( trace_cmd_buf, 
			1, &transfer_semaphores, &wait_stages,
			0, 0 
		);
	}

	VkCommandBuffer cmd_buf_trace_post;	//  split into two causing fps drops?

	{
		cmd_buf_trace_post = vk.cmd->command_buffer_trace;

		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = VK_NULL_HANDLE;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = VK_NULL_HANDLE;

		VK_CHECK( qvkBeginCommandBuffer( cmd_buf_trace_post, &begin_info ) );

		if ( god_rays_enabled )
		{
			BEGIN_PERF_MARKER( cmd_buf_trace_post, PROFILER_GOD_RAYS );
			vk_rtx_record_god_rays_trace_command_buffer( cmd_buf_trace_post, 0 );
			END_PERF_MARKER( cmd_buf_trace_post, PROFILER_GOD_RAYS );
		}

		if ( ref_mode->reflect_refract > 0 )
		{
			BEGIN_PERF_MARKER( cmd_buf_trace_post, PROFILER_REFLECT_REFRACT_1 );
			vk_rtx_trace_reflections( cmd_buf_trace_post, idx, 0 );
			END_PERF_MARKER( cmd_buf_trace_post, PROFILER_REFLECT_REFRACT_1 );
		}

		if ( god_rays_enabled )
		{
			if ( ref_mode->reflect_refract > 0 )
			{
				BEGIN_PERF_MARKER( cmd_buf_trace_post, PROFILER_GOD_RAYS_REFLECT_REFRACT );
				vk_rtx_record_god_rays_trace_command_buffer( cmd_buf_trace_post, 1 );
				END_PERF_MARKER(cmd_buf_trace_post, PROFILER_GOD_RAYS_REFLECT_REFRACT );
			}

			BEGIN_PERF_MARKER( cmd_buf_trace_post, PROFILER_GOD_RAYS_FILTER );
			vk_rtx_record_god_rays_filter_command_buffer( cmd_buf_trace_post );
			END_PERF_MARKER( cmd_buf_trace_post, PROFILER_GOD_RAYS_FILTER );
		}

		if ( ref_mode->reflect_refract > 1 )
		{
			BEGIN_PERF_MARKER( cmd_buf_trace_post, PROFILER_REFLECT_REFRACT_2 );
			for (int pass = 0; pass < ref_mode->reflect_refract - 1; pass++)
			{
				vk_rtx_trace_reflections( cmd_buf_trace_post, idx, pass );
			}
			END_PERF_MARKER( cmd_buf_trace_post, PROFILER_REFLECT_REFRACT_2 );
		}

		if ( ref_mode->enable_denoiser ) 
		{
			BEGIN_PERF_MARKER( cmd_buf_trace_post, PROFILER_ASVGF_GRADIENT_REPROJECT );
			vkpt_asvgf_gradient_reproject( cmd_buf_trace_post );
			END_PERF_MARKER( cmd_buf_trace_post, PROFILER_ASVGF_GRADIENT_REPROJECT );
		}
		vk_rxt_trace_lighting( cmd_buf_trace_post, idx, ref_mode->num_bounce_rays );

#ifdef VK_RTX_INVESTIGATE_SLOW_CMD_BUF
		vk_rtx_end_command_buffer( cmd_buf_trace_post, 
			0, 0, 0,
			1, &trace_semaphores 
		);

		*curr_trace_signaled = qtrue;
#endif
	}

	{
#ifdef VK_RTX_INVESTIGATE_SLOW_CMD_BUF
		cmd_buf_trace_post = vk_begin_command_buffer();
#endif
		BEGIN_PERF_MARKER( cmd_buf_trace_post, PROFILER_ASVGF_FULL );
		if ( ref_mode->enable_denoiser ) 
			vkpt_asvgf_filter( cmd_buf_trace_post, sun_pt_num_bounce_rays->value >= 0.5f ? qtrue : qfalse );
		else
			vkpt_compositing( cmd_buf_trace_post );
		END_PERF_MARKER( cmd_buf_trace_post, PROFILER_ASVGF_FULL );

		vkpt_interleave( cmd_buf_trace_post );

		vkpt_taa( cmd_buf_trace_post );

#if 0
		BEGIN_PERF_MARKER(cmd_buf_trace_post, PROFILER_BLOOM);
		if ( cvar_bloom_enable->integer != 0 || menu_mode )
		{
			vkpt_bloom_record_cmd_buffer(cmd_buf_trace_post);
		}
		END_PERF_MARKER(cmd_buf_trace_post, PROFILER_BLOOM);
#endif

		BEGIN_PERF_MARKER( cmd_buf_trace_post, PROFILER_TONE_MAPPING );
		if ( sun_tm_enable->integer != 0 )
		{
			float frame_time = MIN( 1.f, MAX(0.f, refdef->frametime) );

			static unsigned previous_wallclock_time = 0;
			unsigned current_wallclock_time = ri.Milliseconds();
			float frame_wallclock_time = (previous_wallclock_time != 0) ? (float)(current_wallclock_time - previous_wallclock_time) * 1e-3f : 0.f;
			previous_wallclock_time = current_wallclock_time;

			vkpt_tone_mapping_record_cmd_buffer( cmd_buf_trace_post, frame_time <= 0.f ? frame_wallclock_time : frame_time );
		}
		END_PERF_MARKER( cmd_buf_trace_post, PROFILER_TONE_MAPPING );

		{
			VkBufferCopy copyRegion = { 0, 0, sizeof(ReadbackBuffer) };
			qvkCmdCopyBuffer( cmd_buf_trace_post, vk.buffer_readback.buffer, vk.buffer_readback_staging[idx].buffer, 1, &copyRegion);
		}

#ifdef VK_RTX_INVESTIGATE_SLOW_CMD_BUF
		vk_end_command_buffer( cmd_buf_trace_post );
#endif
	}

#ifndef VK_RTX_INVESTIGATE_SLOW_CMD_BUF
	vk_rtx_end_command_buffer( cmd_buf_trace_post, 
		0, 0, 0,
		1, &trace_semaphores 
	);

	*curr_trace_signaled = qtrue;
#endif
}

static qboolean frame_ready;

void vk_rtx_begin_scene( trRefdef_t *refdef, drawSurf_t *drawSurfs, int numDrawSurfs ) 
{
	// reset offsets
	vk.scratch_buffer_ptr = 0;

	reference_mode_t	ref_mode;
	vkUniformRTX_t		*ubo;
	sun_light_t			sun_light;
	vec3_t				sky_matrix[3];
	qboolean			render_world = qfalse;
	uint32_t			idx, prev_idx;
	mnode_t				*viewleaf;

	vk_rtx_get_descriptor_index( idx, prev_idx );

	if ( tr.world )
		render_world = qtrue;

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
	prepare_entities( &upload_info, refdef );
#if 0
	if ( tr.world )
		vkpt_build_beam_lights(model_lights, &num_model_lights, MAX_MODEL_LIGHTS, bsp_world_model, fd->entities, fd->num_entities, prev_adapted_luminance);
#endif

	ubo = &vk.buffer_uniform;
	vk_rtx_prepare_ubo( refdef, tr.world, ubo, viewleaf, &ref_mode, sky_matrix, render_world );
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

	vkpt_light_buffer_upload_to_staging( idx, render_world, tr.world, num_model_lights, sky_radiance );

	float shadowmap_view_proj[16];
	float shadowmap_depth_scale;
	const float unused = 0.0f;
	vk_rtx_shadow_map_setup(
		&sun_light,
		&unused,
		&unused,
		shadowmap_view_proj,
		&shadowmap_depth_scale,
		qfalse);

	vk_rtx_god_rays_prepare_ubo(
		ubo,
		/*&vkpt_refdef.bsp_mesh_world.world_aabb,*/
		ubo->P,
		ubo->V,
		shadowmap_view_proj,
		shadowmap_depth_scale);

	qboolean god_rays_enabled = ( vk_rtx_god_rays_enabled(&sun_light) && render_world) ? qtrue : qfalse;

	vk_begin_trace_rays( refdef, &ref_mode, ubo, 
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