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

#define     MAX_MAP_CLUSTERS    65536

static int local_light_counts[MAX_MAP_CLUSTERS];
static int cluster_light_counts[MAX_MAP_CLUSTERS];
static int light_list_tails[MAX_MAP_CLUSTERS];
static int max_model_lights = 0;

void vkpt_light_buffer_reset_counts( void )	// not used yet
{
	max_model_lights = 0;
}

static inline void
copy_light(const light_poly_t* light, float* vblight, const float* sky_radiance)
{
	float style_scale = 1.f;
	float prev_style = 1.f;

#if 0
	if (light->style != 0 && vkpt_refdef.fd->lightstyles)
	{
		style_scale = vkpt_refdef.fd->lightstyles[light->style].white;
		style_scale = MAX(0, MIN(1, style_scale));

		prev_style = vkpt_refdef.prev_lightstyles[light->style].white;
		prev_style = MAX(0, MIN(1, prev_style));
	}
#endif

	//float mat_scale = light->material ? light->material->emissive_scale : 1.f;
	float mat_scale = 10.f;

	VectorCopy(light->positions + 0, vblight + 0);
	VectorCopy(light->positions + 3, vblight + 4);
	VectorCopy(light->positions + 6, vblight + 8);

	if (light->color[0] < 0.f)
	{
		vblight[3] = -sky_radiance[0] * 0.5f;
		vblight[7] = -sky_radiance[1] * 0.5f;
		vblight[11] = -sky_radiance[2] * 0.5f;
	}
	else
	{
		vblight[3] = light->color[0] * mat_scale;
		vblight[7] = light->color[1] * mat_scale;
		vblight[11] = light->color[2] * mat_scale;
	}

	vblight[12] = style_scale;
	vblight[13] = prev_style;
	vblight[14] = light->type;
	vblight[15] = 0.f;
}

VkResult allocate_gpu_memory( VkMemoryRequirements mem_req, VkDeviceMemory *pMemory )
{
	VkMemoryAllocateInfo mem_alloc_info;
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.pNext = NULL;
	mem_alloc_info.allocationSize = mem_req.size;
	mem_alloc_info.memoryTypeIndex = vk_find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

#ifdef VKPT_DEVICE_GROUPS
	VkMemoryAllocateFlagsInfo mem_alloc_flags = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT,
		.deviceMask = (1 << qvk.device_count) - 1
	};

	if (qvk.device_count > 1) {
		mem_alloc_info.pNext = &mem_alloc_flags;
	}
#endif

	return qvkAllocateMemory( vk.device, &mem_alloc_info, NULL, pMemory );
}

static VkDeviceAddress vk_get_buffer_device_address(VkBuffer buffer)
{
	VkBufferDeviceAddressInfo address_info;
	address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address_info.pNext = NULL;
	address_info.buffer = buffer;

	return qvkGetBufferDeviceAddress( vk.device, &address_info );
}

VkResult vk_rtx_buffer_create( vkbuffer_t *buf, VkDeviceSize size, 
	VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_properties )
{
	assert(size > 0);
	assert(buf);
	VkResult result = VK_SUCCESS;

	VkBufferCreateInfo buf_create_info;
	buf_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_create_info.pNext = NULL;
	buf_create_info.flags = 0;
	buf_create_info.size  = size;
	buf_create_info.usage = usage;
	buf_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_create_info.queueFamilyIndexCount = 0;
	buf_create_info.pQueueFamilyIndices = NULL;

	buf->size = size;
	buf->is_mapped = 0;

	result = qvkCreateBuffer( vk.device, &buf_create_info, NULL, &buf->buffer );
	if ( result != VK_SUCCESS ) {
		goto fail_buffer;
	}
	assert(buf->buffer != VK_NULL_HANDLE);

	VkMemoryRequirements mem_reqs;
	qvkGetBufferMemoryRequirements( vk.device, buf->buffer, &mem_reqs );

	VkMemoryAllocateInfo mem_alloc_info;
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.pNext = NULL;
	mem_alloc_info.allocationSize = mem_reqs.size;
	mem_alloc_info.memoryTypeIndex = vk_find_memory_type( mem_reqs.memoryTypeBits, mem_properties );


	VkMemoryAllocateFlagsInfo mem_alloc_flags;
	mem_alloc_flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	mem_alloc_flags.pNext = NULL;
	mem_alloc_flags.flags = ( usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT ) ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0;
	mem_alloc_flags.deviceMask = 0;

#ifdef VKPT_DEVICE_GROUPS
	if ( qvk.device_count > 1 && !( mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) ) {
		mem_alloc_flags.flags |= VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
		mem_alloc_flags.deviceMask = (1 << qvk.device_count) - 1;
	}
#endif

	mem_alloc_info.pNext = &mem_alloc_flags;

	result = qvkAllocateMemory( vk.device, &mem_alloc_info, NULL, &buf->memory );
	if ( result != VK_SUCCESS ) {
		goto fail_mem_alloc;
	}

	assert( buf->memory != VK_NULL_HANDLE );

	result = qvkBindBufferMemory( vk.device, buf->buffer, buf->memory, 0 );
	if ( result != VK_SUCCESS ) {
		goto fail_bind_buf_memory;
	}

	if ( usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT )
	{
		buf->address = vk_get_buffer_device_address( buf->buffer );
		assert(buf->address);
	}
	else
	{
		buf->address = 0;
	}

	return VK_SUCCESS;

fail_bind_buf_memory:
	qvkFreeMemory( vk.device, buf->memory, NULL );
fail_mem_alloc:
	qvkDestroyBuffer( vk.device, buf->buffer, NULL );
fail_buffer:
	buf->buffer = VK_NULL_HANDLE;
	buf->memory = VK_NULL_HANDLE;
	buf->size   = 0;
	return result;
}

VkResult vk_rtx_buffer_destroy( vkbuffer_t *buf )
{
	assert(!buf->is_mapped);

	if ( buf->memory != VK_NULL_HANDLE )
		qvkFreeMemory( vk.device, buf->memory, NULL );

	if ( buf->buffer != VK_NULL_HANDLE )
		qvkDestroyBuffer( vk.device, buf->buffer, NULL );

	buf->buffer = VK_NULL_HANDLE;
	buf->memory = VK_NULL_HANDLE;
	buf->size   = 0;
	buf->address = 0;

	return VK_SUCCESS;
}

void VK_DestroyBuffer( vkbuffer_t *buffer )
{
	if ( buffer->p )
		qvkUnmapMemory( vk.device, buffer->memory );

	if ( buffer->buffer != NULL )
		qvkDestroyBuffer( vk.device, buffer->buffer, NULL );

	if ( buffer->memory != NULL )
		qvkFreeMemory( vk.device, buffer->memory, NULL );

	memset( buffer, 0, sizeof(vkbuffer_t) );
}

void *buffer_map( vkbuffer_t *buf )
{
	assert( !buf->is_mapped );

	buf->is_mapped = 1;
	void *ret = NULL;

	assert(buf->memory != VK_NULL_HANDLE);
	assert(buf->size > 0);

	VK_CHECK( qvkMapMemory( vk.device, buf->memory, 0 /*offset*/, buf->size, 0 /*flags*/, &ret ) );
	return ret;
}

void buffer_unmap( vkbuffer_t *buf )
{
	assert( buf->is_mapped );

	buf->is_mapped = 0;
	qvkUnmapMemory( vk.device, buf->memory );
}

static uint32_t VK_DeviceLocalMemoryIndex()	// hmm
{
	uint32_t deviceLocalMemIndex = -1;
	VkPhysicalDeviceMemoryProperties physDevMemProps = { 0 };

	qvkGetPhysicalDeviceMemoryProperties(vk.physical_device, &physDevMemProps);
	for (uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
		const VkMemoryType* memType = physDevMemProps.memoryTypes;
		// Just pick the first device local memtype.
		if (memType[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
			deviceLocalMemIndex = i;
			break;
		}
	}

	return deviceLocalMemIndex;

}

void VK_CreateImageMemory(VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *bufferMemory) {
	VkMemoryRequirements memory_requirements = { 0 };
	qvkGetImageMemoryRequirements(vk.device, *image, &memory_requirements);

	int32_t memoryTypeIndex = vk_find_memory_type( memory_requirements.memoryTypeBits, properties );

	VkMemoryAllocateInfo allocInfo;
	Com_Memset( &allocInfo, 0, sizeof(VkMemoryAllocateInfo) );

	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memory_requirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex != -1 ? memoryTypeIndex : VK_DeviceLocalMemoryIndex();

	VK_CHECK(qvkAllocateMemory(vk.device, &allocInfo, NULL, bufferMemory));
	VK_CHECK(qvkBindImageMemory(vk.device, *image, *bufferMemory, 0));
}

void VK_CreateAttributeBuffer(vkbuffer_t* buffer, VkDeviceSize size, VkBufferUsageFlagBits usage) {
	VkDeviceSize nCAS = vk.props.limits.nonCoherentAtomSize;
	buffer->size = ((size + (nCAS - 1)) / nCAS) * nCAS;

	vk_rtx_buffer_create( buffer, buffer->size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	VK_CHECK(qvkMapMemory(vk.device, buffer->memory, 0, buffer->size, 0, (void**)&buffer->p));
}

void vk_rtx_upload_buffer_data_offset( vkbuffer_t *buffer, VkDeviceSize offset, VkDeviceSize size, const byte *data ) 
{
    if (offset + size > buffer->size) {
        ri.Error(ERR_FATAL, "Vulkan: Buffer to small!");
    }
	if (buffer->p == NULL) {
		ri.Error(ERR_FATAL, "Vulkan: Buffer not mapped!");
	}

    byte*p = buffer->p + offset;
    Com_Memcpy(p, data, (size_t)(size));
}

void vk_rtx_upload_buffer_data( vkbuffer_t *buffer, const byte *data ) 
{
	vk_rtx_upload_buffer_data_offset(buffer, 0, buffer->size, data);
}

static void copy_bsp_lights( world_t *world, LightBuffer *lbo )
{
	memcpy( lbo->light_list_lights, world->cluster_lights, world->cluster_light_offsets[world->numClusters] * sizeof(uint32_t) );
	memcpy( lbo->light_list_offsets, world->cluster_light_offsets, (world->numClusters + 1) * sizeof(uint32_t)  );
		
	// Store the light counts in the light counts history entry for the current frame
	uint32_t history_index = vk.frame_counter % LIGHT_COUNT_HISTORY;
	uint32_t *sample_light_counts = (uint32_t *)buffer_map( vk.buf_light_counts_history + history_index );
		
	for ( int c = 0; c < world->numClusters; c++ )
	{
		sample_light_counts[c] = world->cluster_light_offsets[c + 1] - world->cluster_light_offsets[c];
	}

	buffer_unmap( vk.buf_light_counts_history + history_index );
}

static void inject_model_lights( world_t* world, int num_model_lights, light_poly_t* transformed_model_lights, int model_light_offset, LightBuffer *lbo)
{
	uint32_t *dst_list_offsets = lbo->light_list_offsets;
	uint32_t *dst_lists = lbo->light_list_lights;

	memset(local_light_counts, 0, world->numClusters * sizeof(int));
	memset(cluster_light_counts, 0, world->numClusters * sizeof(int));

	// Count the number of model lights per cluster

	for (int nlight = 0; nlight < num_model_lights; nlight++)
	{
		local_light_counts[transformed_model_lights[nlight].cluster]++;
	}

	// Count the number of model lights visible from each cluster, using the PVS

	for (int c = 0; c < world->numClusters; c++)
	{
		if (local_light_counts[c])
		{
			const byte* mask = BSP_GetPvs(world, c);

			for (int j = 0; j < world->clusterBytes; j++) {
				if (mask[j]) {
					for (int k = 0; k < 8; ++k) {
						if (mask[j] & (1 << k))
							cluster_light_counts[j * 8 + k] += local_light_counts[c];
					}
				}
			}
		}
	}

	// Count the total required list size

	int required_size = world->cluster_light_offsets[world->numClusters];
	for (int c = 0; c < world->numClusters; c++)
	{
		required_size += cluster_light_counts[c];
	}

	// See if we have enough room in the interaction buffer
	
	if (required_size > MAX_LIGHT_LIST_NODES)
	{
		Com_Printf("Insufficient light interaction buffer size (%d needed). Increase MAX_LIGHT_LIST_NODES.\n", required_size);

		// Copy the BSP light lists verbatim
		copy_bsp_lights( world, lbo );

		return;
	}
	
	// Store the light counts in the light counts history entry for the current frame
	uint32_t history_index = vk.frame_counter % LIGHT_COUNT_HISTORY;
	uint32_t *sample_light_counts = (uint32_t *)buffer_map(vk.buf_light_counts_history + history_index);

	// Copy the static light lists, and make room in these lists to inject the model lights

	int tail = 0;
	for (int c = 0; c < world->numClusters; c++)
	{
		int original_size = world->cluster_light_offsets[c + 1] - world->cluster_light_offsets[c];

		dst_list_offsets[c] = tail;
		memcpy(dst_lists + tail, world->cluster_lights + world->cluster_light_offsets[c], sizeof(uint32_t) * original_size);
		tail += original_size;
		
		assert(tail + cluster_light_counts[c] < MAX_LIGHT_LIST_NODES);
		
		light_list_tails[c] = tail;
		tail += cluster_light_counts[c];

		sample_light_counts[c] = original_size + cluster_light_counts[c];
	}
	dst_list_offsets[world->numClusters] = tail;

	buffer_unmap(vk.buf_light_counts_history + history_index);

	// Write the model light indices into the light lists

	for (int nlight = 0; nlight < num_model_lights; nlight++)
	{
		const byte* mask = BSP_GetPvs(world, transformed_model_lights[nlight].cluster);

		if (!mask) continue;

		for (int j = 0; j < world->clusterBytes; j++) {
			if (mask[j]) {
				for (int k = 0; k < 8; ++k) {
					if (mask[j] & (1 << k))
					{
						int other_cluster = j * 8 + k;
						int list_index = light_list_tails[other_cluster]++;
						// assert we're not writing into the space reserved for following cluster
						assert(list_index < dst_list_offsets[other_cluster + 1]);
						dst_lists[list_index] = model_light_offset + nlight;
					}
				}
			}
		}
	}

#if defined(_DEBUG)
	// Verify tight packing
	for (int c = 0; c < world->numClusters; c++)
	{
		int list_start = dst_list_offsets[c];
		int list_end = dst_list_offsets[c + 1];
		int original_size = world->cluster_light_offsets[c + 1] - world->cluster_light_offsets[c];
		assert(list_end - list_start == original_size + cluster_light_counts[c]);
	}
#endif
}

VkResult vkpt_light_buffer_upload_to_staging( qboolean render_world, 
	world_t *world, int num_model_lights, light_poly_t *transformed_model_lights, const float* sky_radiance )
{
	vkbuffer_t *staging = vk.buf_light_staging + vk.current_frame_index;

	LightBuffer *lbo = (LightBuffer *)buffer_map(staging);
	assert(lbo);

	if ( render_world )
	{
		assert( world->num_light_polys + num_model_lights < MAX_LIGHT_POLYS);

		int model_light_offset = world->num_light_polys;
		max_model_lights = MAX(max_model_lights, num_model_lights);

		if ( max_model_lights > 0 )
		{
			// If any of the BSP models contain lights, inject these lights right into the visibility lists.
			// The shader doesn't know that these lights are dynamic.

			inject_model_lights( world, num_model_lights, transformed_model_lights, model_light_offset, lbo );
		}
		else
		{
			copy_bsp_lights( world, lbo );
		}

		for ( int nlight = 0; nlight < world->num_light_polys; nlight++ )
		{
			light_poly_t* light = world->light_polys + nlight;
			float* vblight = *(lbo->light_polys + nlight * LIGHT_POLY_VEC4S);
			copy_light(light, vblight, sky_radiance);
		}

		for (int nlight = 0; nlight < num_model_lights && nlight + model_light_offset < MAX_LIGHT_POLYS; nlight++)
		{
			light_poly_t* light = transformed_model_lights + nlight;
			float* vblight = *(lbo->light_polys + (nlight + model_light_offset) * LIGHT_POLY_VEC4S);
			copy_light(light, vblight, sky_radiance);
		}
	}
	else
	{
		lbo->light_list_offsets[0] = 0;
		lbo->light_list_offsets[1] = 0;
	}

	// add light styles here

	{
		// moved to vk_rtx_material.cpp
		vk_rtx_upload_materials( lbo );
	}

	memcpy( lbo->cluster_debug_mask, vk.cluster_debug_mask, MAX_LIGHT_LISTS / 8 );
	memcpy( lbo->sky_visibility, tr.world->sky_visibility, MAX_LIGHT_LISTS / 8);

	buffer_unmap( staging );
	lbo = NULL;

	return VK_SUCCESS;
}

static void vk_rtx_add_stage_color( const int stage, VertexBuffer *vbo ) 
{
	for ( uint32_t i = 0; i < tess.numVertexes; i++ )
	{
		vbo[i].color[stage] = tess.svars.colors[0][i][0] | 
						  tess.svars.colors[0][i][1] << 8 | 
			              tess.svars.colors[0][i][2] << 16 | 
			              tess.svars.colors[0][i][3] << 24;
	}	
}

static void vk_rtx_add_stage_tex_coords( const int stage, VertexBuffer *vbo ) 
{
	// no need for iteration,  simple memcpy?
	for ( uint32_t i = 0; i < tess.numVertexes; i++ )
	{
		vbo[i].uv[stage][1] = tess.svars.texcoordPtr[0][i][1];
		vbo[i].uv[stage][0] = tess.svars.texcoordPtr[0][i][0];
	}	
}

void vk_rtx_bind_indicies( uint32_t* cluster, uint32_t base_vertex )
{
	uint32_t i;

	// ~sunny, rename xyz_count in geom strucs to base_vertex, base_index?

	for ( i = 0; i < tess.numIndexes; i++ )
	{
		cluster[i] = (uint32_t)(tess.indexes[i] + base_vertex );
	}
}

void vk_rtx_bind_cluster( uint32_t *cluster, uint32_t cluster_count, int cluster_id )
{
	uint32_t i;

	for ( i = 0; i < cluster_count; i++ )
	{
		cluster[i] = cluster_id;
	}
}

void vk_rtx_bind_vertices( VertexBuffer *vbo, int cluster ) 

{
	const shaderStage_t *pStage;
	uint32_t			i, material_index, material_flags;
	vk_rtx_shader_to_material( tess.shader, material_index, material_flags );

	// encode two tex idx and its needColor and blend flags into one 4 byte uint
	//uint32_t tex0 = (RB_GetNextTexEncoded(0)) | (RB_GetNextTexEncoded(1) << TEX_SHIFT_BITS);
	//uint32_t tex1 = (RB_GetNextTexEncoded(2)) | (RB_GetNextTexEncoded(3) << TEX_SHIFT_BITS);

	//uint32_t tex0 = (0U) | (TEX0_IDX_MASK << TEX_SHIFT_BITS);
	//uint32_t tex0 = (RB_GetNextTexEncoded(0));// | (TEX0_IDX_MASK << TEX_SHIFT_BITS);
	uint32_t tex0 = material_index;// | (TEX0_IDX_MASK << TEX_SHIFT_BITS);
	uint32_t tex1 = (TEX0_IDX_MASK) | (TEX0_IDX_MASK << TEX_SHIFT_BITS);

	for ( i = 0; i < tess.numVertexes; i++ ) 
	{
		vbo[i].material = (material_flags & ~MATERIAL_INDEX_MASK) | (material_index & MATERIAL_INDEX_MASK);
		vbo[i].texIdx0 = tex0;
		vbo[i].texIdx1 = tex1;
		//int c = R_FindClusterForPos(tess.xyz[i]);
		vbo[i].cluster = cluster;// c != -1 ? c : cluster;

		memcpy( vbo[i].pos, tess.xyz + i, sizeof(vec3_t) );

		vbo[i].normal[0] = tess.normal[i][0];
		vbo[i].normal[1] = tess.normal[i][1];
		vbo[i].normal[2] = tess.normal[i][2];
		vbo[i].normal[3] = 0;

		vbo[i].qtangent[0] = tess.qtangent[i][0];
		vbo[i].qtangent[1] = tess.qtangent[i][1];
		vbo[i].qtangent[2] = tess.qtangent[i][2];
		vbo[i].qtangent[3] = 0;
	}

	for ( i = 0; i < MAX_RTX_STAGES; i++ ) 
	{
		pStage = tess.shader->stages[i];

		if ( !pStage || !pStage->active )
			break;

		//
		// only compute bundle 0 for now
		//
		if ( pStage->tessFlags & TESS_RGBA0 )
		{
			ComputeColors( 0, tess.svars.colors[0], pStage, 0 );
			vk_rtx_add_stage_color( i, vbo );
		}

		if ( pStage->tessFlags & TESS_ST0 )
		{
			ComputeTexCoords( 0, &pStage->bundle[0] );
			vk_rtx_add_stage_tex_coords( i, vbo );
		}
	}
#if 0
	// if there are multiple stages we need to upload them all
	if ( tess.shader->stages[0] != NULL && tess.shader->stages[0]->active ) 
	{
		ComputeTexCoords( 0, &tess.shader->stages[0]->bundle[0] );
		ComputeColors(0, tess.svars.colors[0], tess.shader->stages[0], 0);

#if 0
		if ( tr.world != NULL ) {
			if (strstr(tess.shader->name, "fog")) {
			int x = 2;
			//if (tess.fogNum && tess.shader->fogPass) {
				fog_t* fog;
				fog = tr.world->fogs + 4;// tess.fogNum;
				for (int i = 0; i < tess.numVertexes; i++) {
					*(int*)&tess.svars.colors[i] = fog->colorInt;
				}
				//RB_CalcFogTexCoords((float*)tess.svars.texcoords[0]);
			}
		}
#endif
		for ( j = 0; j < tess.numVertexes; j++ ) {
			vData[j].color0 = tess.svars.colors[0][j][0] | tess.svars.colors[0][j][1] << 8 | tess.svars.colors[0][j][2] << 16 | tess.svars.colors[0][j][3] << 24;
			
			vData[j].uv0[1] = tess.svars.texcoordPtr[0][j][1];
			vData[j].uv0[0] = tess.svars.texcoordPtr[0][j][0];

		}
	}

	if ( tess.shader->stages[1] != NULL && tess.shader->stages[1]->active) 
	{
		ComputeTexCoords( 1, &tess.shader->stages[1]->bundle[0] );
		ComputeColors( 1, tess.svars.colors[0], tess.shader->stages[1], 0);

		for ( j = 0; j < tess.numVertexes; j++ ) {
			vData[j].color1 = tess.svars.colors[0][j][0] | tess.svars.colors[0][j][1] << 8 | tess.svars.colors[0][j][2] << 16 | tess.svars.colors[0][j][3] << 24;
			vData[j].uv1[0] = tess.svars.texcoords[0][j][0];
			vData[j].uv1[1] = tess.svars.texcoords[0][j][1];
		}
	}

	if ( tess.shader->stages[2] != NULL && tess.shader->stages[2]->active ) 
	{
		ComputeTexCoords( 2, &tess.shader->stages[2]->bundle[0] );
		ComputeColors( 2, tess.svars.colors[0], tess.shader->stages[2], 0);

		for ( j = 0; j < tess.numVertexes; j++ ) {
			vData[j].color2 = tess.svars.colors[0][j][0] | tess.svars.colors[0][j][1] << 8 | tess.svars.colors[0][j][2] << 16 | tess.svars.colors[0][j][3] << 24;
			vData[j].uv2[0] = tess.svars.texcoords[0][j][0];
			vData[j].uv2[1] = tess.svars.texcoords[0][j][1];
		}
	}

	if (tess.shader->stages[3] != NULL && tess.shader->stages[3]->active ) 
	{
		ComputeTexCoords( 3, &tess.shader->stages[3]->bundle[0] );
		ComputeColors( 3, tess.svars.colors[0], tess.shader->stages[3], 0);

		for ( j = 0; j < tess.numVertexes; j++ ) {
			vData[j].color3 = tess.svars.colors[0][j][0] | tess.svars.colors[0][j][1] << 8 | tess.svars.colors[0][j][2] << 16 | tess.svars.colors[0][j][3] << 24;
			vData[j].uv3[0] = tess.svars.texcoords[0][j][0];
			vData[j].uv3[1] = tess.svars.texcoords[0][j][1];
		}
	}
#endif

}

VkResult vk_rtx_readback( ReadbackBuffer *dst )
{
	vkbuffer_t *buffer = &vk.buf_readback_staging[vk.current_frame_index];
	void *mapped = buffer_map(buffer);

	if  ( mapped == NULL )
		return VK_ERROR_MEMORY_MAP_FAILED;

	memcpy( dst, mapped, sizeof(ReadbackBuffer) );

	buffer_unmap( buffer );

	return VK_SUCCESS;
}

VkResult vkpt_light_buffers_create( world_t &worldData )
{
	vkpt_light_buffers_destroy();

	// Light statistics: 2 uints (shadowed, unshadowed) per light per surface orientation (6) per cluster.
	uint32_t num_stats = worldData.numClusters * worldData.num_light_polys * 6 * 2;

    // Handle rare cases when the map has zero lights
    if ( num_stats == 0 )
        num_stats = 1;

	for ( int frame = 0; frame < NUM_LIGHT_STATS_BUFFERS; frame++ )
	{
		vk_rtx_buffer_create( vk.buf_light_stats + frame, sizeof(uint32_t) * num_stats,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	}

	for ( int h = 0; h < LIGHT_COUNT_HISTORY; h++ )
	{
		vk_rtx_buffer_create( vk.buf_light_counts_history + h, sizeof(uint32_t) * worldData.numClusters,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	}

	assert(NUM_LIGHT_STATS_BUFFERS == 3);

	return VK_SUCCESS;
}

VkResult vkpt_light_buffers_destroy( void )
{
	for ( int frame = 0; frame < NUM_LIGHT_STATS_BUFFERS; frame++ )
		VK_DestroyBuffer( vk.buf_light_stats + frame );

	for ( int h = 0; h < LIGHT_COUNT_HISTORY; h++ )
		VK_DestroyBuffer( vk.buf_light_counts_history + h );

	return VK_SUCCESS;
}

void vk_rtx_create_buffers( void ) 
{
	uint32_t i;

	// scratch buffer
	vk_rtx_buffer_create( &vk.buf_accel_scratch, VK_MAX_DYNAMIC_BOTTOM_AS_INSTANCES * VK_AS_MEMORY_ALLIGNMENT_SIZE * sizeof(byte), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	vk.scratch_buf_ptr = 0;

	vkpt_uniform_buffer_create();

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		// blas instances in tlas
		vk_rtx_buffer_create(		&vk.buf_instances[i],		VK_MAX_BOTTOM_AS_INSTANCES * sizeof(vk_geometry_instance_t), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );	
	}

	// readback
	vk_rtx_buffer_create( &vk.buf_readback, sizeof(ReadbackBuffer),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		vk_rtx_buffer_create( vk.buf_readback_staging + i, sizeof(ReadbackBuffer),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );	
	}

	vk_rtx_buffer_create( &vk.model_instance.buffer_vertex, sizeof(ModelDynamicVertexBuffer),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// light & material buffer
	vk_rtx_buffer_create( &vk.buf_light, sizeof(LightBuffer),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		vk_rtx_buffer_create( vk.buf_light_staging + i, sizeof(LightBuffer),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );	
	}

	// tonemap
	vk_rtx_buffer_create( &vk.buf_tonemap, sizeof(ToneMappingBuffer),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// sky
	vk_rtx_buffer_create( &vk.buf_sun_color, sizeof(SunColorBuffer),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void vk_rtx_destroy_buffers( void ) 
{
	uint32_t i = 0;

	VK_DestroyBuffer( &vk.buf_accel_scratch );
	VK_DestroyBuffer( &vk.model_instance.buffer_vertex );

	VK_DestroyBuffer( &vk.buf_readback );
	VK_DestroyBuffer( &vk.buf_tonemap );
	VK_DestroyBuffer( &vk.buf_light );
	VK_DestroyBuffer( &vk.buf_sun_color );

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		VK_DestroyBuffer( &vk.buf_instances[i] );
		VK_DestroyBuffer( vk.buf_readback_staging + i );
		VK_DestroyBuffer( vk.buf_light_staging + i );
	}


	vk_rtx_destroy_accel_all();
	vkpt_light_buffers_destroy();
}