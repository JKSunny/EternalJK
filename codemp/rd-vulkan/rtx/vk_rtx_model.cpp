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

static vkbuffer_t null_buffer;

model_vbo_t model_vertex_data[MAX_VBOS];
uint32_t model_index = 0;

static VkPipeline       pipeline_instance_geometry;
static VkPipeline       pipeline_animate_materials;
static VkPipelineLayout pipeline_layout_instance_geometry;

void vk_rtx_write_model_descriptor( int index, VkDescriptorSet descriptor, VkBuffer buffer, VkDeviceSize size )
{
	VkDescriptorBufferInfo info;
	info.buffer = buffer;
	info.offset = 0;
	info.range = size;

	VkWriteDescriptorSet desc;
	Com_Memset( &desc, 0, sizeof(VkWriteDescriptorSet) );
	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.pNext = NULL;
	desc.dstSet = descriptor;
	desc.dstBinding = 0;
	desc.dstArrayElement = index;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc.descriptorCount = 1;
	desc.pBufferInfo = &info;

	qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );
}

#ifdef USE_RTX_GLOBAL_MODEL_VBO
void vk_rtx_bind_model( int index )
{
	if ( !vk.model_instance.descriptor.vbos || !vk.model_instance.descriptor.ibos )
		return;

	vk_rtx_write_model_descriptor( index, vk.model_instance.descriptor.vbos, tr.vbos[ index - 1 ]->buffer, tr.vbos[ index - 1 ]->size );
	vk_rtx_write_model_descriptor( index, vk.model_instance.descriptor.ibos, tr.ibos[ index - 1 ]->buffer, tr.ibos[ index - 1 ]->size );
}
#endif

void vk_rtx_create_model_vbo_ibo_descriptor( void ) 
{
	VkDescriptorPoolSize pool_size[] = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_VBOS },
#ifdef USE_RTX_GLOBAL_MODEL_VBO
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_VBOS },
#endif
	};

	VkDescriptorPoolCreateInfo pool_info;
	Com_Memset( &pool_info, 0, sizeof(VkDescriptorPoolCreateInfo) );
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.pNext         = NULL;
	pool_info.poolSizeCount = ARRAY_LEN( pool_size );
	pool_info.pPoolSizes    = pool_size;
	pool_info.maxSets       = ARRAY_LEN( pool_size );

	// sunny needs destroy method
	VK_CHECK( qvkCreateDescriptorPool( vk.device, &pool_info, NULL, &vk.model_instance.pool ) );

	VkDescriptorSetLayoutBinding bind;
	Com_Memset( &bind, 0, sizeof(VkDescriptorSetLayoutBinding) );
	bind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bind.descriptorCount = MAX_VBOS;
	bind.binding = 0;
	bind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo desc;
	Com_Memset( &desc, 0, sizeof(VkDescriptorSetLayoutCreateInfo) );
	desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.bindingCount = 1;
	desc.pBindings = &bind;

	VK_CHECK( qvkCreateDescriptorSetLayout( vk.device, &desc, NULL, &vk.model_instance.layout ) );

	VkDescriptorSetAllocateInfo alloc;
	Com_Memset( &alloc, 0, sizeof(VkDescriptorSetAllocateInfo) );
	alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc.descriptorPool = vk.model_instance.pool;
	alloc.descriptorSetCount = 1;
	alloc.pSetLayouts = &vk.model_instance.layout;

	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.model_instance.descriptor.vbos ) );
#ifdef USE_RTX_GLOBAL_MODEL_VBO
	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.model_instance.descriptor.ibos ) );
#endif

	vk_rtx_buffer_create( &null_buffer, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	
	for ( int i = 0; i < MAX_VBOS; i++ )
	{
		vk_rtx_write_model_descriptor( i, vk.model_instance.descriptor.vbos, null_buffer.buffer, null_buffer.size );
#ifdef USE_RTX_GLOBAL_MODEL_VBO
		vk_rtx_write_model_descriptor( i, vk.model_instance.descriptor.ibos, null_buffer.buffer, null_buffer.size );
#endif
	}
}


VkResult vk_rtx_model_vbo_create_pipelines( void )
{
	assert(!pipeline_instance_geometry);
	assert(!pipeline_animate_materials);
	assert(!pipeline_layout_instance_geometry);

	VkDescriptorSetLayout set_layouts[] = {
		vk.computeDescriptor[0].layout,
		vk.desc_set_layout_textures,
		vk.model_instance.layout,	// vbo
		vk.desc_set_layout_ubo,
		vk.model_instance.layout,	// ibo ( USE_RTX_GLOBAL_MODEL_VBO )
	};

	VkPipelineLayoutCreateInfo layout_create_info;
	Com_Memset( &layout_create_info, 0, sizeof(VkPipelineLayoutCreateInfo) );
	layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_create_info.pNext = NULL;
	layout_create_info.pSetLayouts = set_layouts;
	layout_create_info.setLayoutCount = ARRAY_LEN(set_layouts);
	layout_create_info.pushConstantRangeCount = 0;
	layout_create_info.pPushConstantRanges = NULL;

	VK_CHECK( qvkCreatePipelineLayout( vk.device, &layout_create_info, NULL,
		&pipeline_layout_instance_geometry ) );

	VkPipelineShaderStageCreateInfo shader_geometry;
	Com_Memset( &shader_geometry, 0, sizeof(VkPipelineShaderStageCreateInfo) );
	shader_geometry.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_geometry.pNext = NULL;
	shader_geometry.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shader_geometry.module = vk.asvgf_shader[SHADER_INSTANCE_GEOMETRY_COMP]->modules[0];
	shader_geometry.pName = "main";

	VkComputePipelineCreateInfo create_info[2];
	Com_Memset( &create_info, 0, sizeof(VkComputePipelineCreateInfo) * 2 );

	{
		create_info[0].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		create_info[0].pNext = NULL;
		create_info[0].stage = shader_geometry;
		create_info[0].layout = pipeline_layout_instance_geometry;
	}

	{
		create_info[1].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		create_info[1].pNext = NULL;
		create_info[1].stage = shader_geometry;
		create_info[1].layout = pipeline_layout_instance_geometry;
	}

	VkPipeline pipelines[2];
	VK_CHECK( qvkCreateComputePipelines( vk.device, 0, ARRAY_LEN(create_info), create_info, 0, pipelines ) );

	pipeline_instance_geometry = pipelines[0];
	pipeline_animate_materials = pipelines[1];

	return VK_SUCCESS;
}

void vkpt_instance_geometry( VkCommandBuffer cmd_buf, uint32_t num_instances, qboolean update_world_animations ) 
{
	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	VkDescriptorSet desc_sets[] = {
		vk.computeDescriptor[idx].set,
		vk_rtx_get_current_desc_set_textures(),
		vk.model_instance.descriptor.vbos,
		vk.desc_set_ubo,
		vk.model_instance.descriptor.ibos
	};

	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_instance_geometry );
	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_instance_geometry, 0, ARRAY_LEN(desc_sets),
		desc_sets, 0, NULL );
	qvkCmdDispatch( cmd_buf, num_instances, 1, 1 );

	if ( update_world_animations )
	{
		// ..
	}

	VkBufferMemoryBarrier barrier;
	Com_Memset( &barrier, 0, sizeof(VkBufferMemoryBarrier) );
	barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.pNext               = NULL;
	barrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
	barrier.buffer              = vk.model_instance.buffer_vertex.buffer;
	barrier.size                = vk.model_instance.buffer_vertex.size;
	barrier.srcQueueFamilyIndex = vk.queue_family_index;
	barrier.dstQueueFamilyIndex = vk.queue_family_index;

	qvkCmdPipelineBarrier(
		cmd_buf,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		0, NULL,
		1, &barrier,
		0, NULL );
}

#ifndef USE_RTX_GLOBAL_MODEL_VBO
void vk_rtx_build_mdxm_vbo( model_t *mod, mdxmHeader_t *mdxm )
{
	if ( !vk.vboGhoul2Active )
		return;

	if ( !vk.rtxActive )
		return;


	mdxmVBOModel_t		*vboModel;
	mdxmSurface_t		*surf;
	mdxmLOD_t			*lod;
	uint32_t			i, n, k, w;

	lod = (mdxmLOD_t *)( (byte *)mdxm + mdxm->ofsLODs );
	mod->data.glm->vboModels = (mdxmVBOModel_t *)ri.Hunk_Alloc( sizeof (mdxmVBOModel_t) * mdxm->numLODs, h_low );

	for ( i = 0; i < mdxm->numLODs; i++ )
	{
		vboModel = &mod->data.glm->vboModels[i];
		vboModel->model_index = model_index++;
		mdxmVBOMesh_t *vboMeshes;

		vboModel->numVBOMeshes = mdxm->numSurfaces;
		vboModel->vboMeshes = (mdxmVBOMesh_t *)ri.Hunk_Alloc( sizeof (mdxmVBOMesh_t) * (mdxm->numSurfaces ), h_low );
		vboMeshes = vboModel->vboMeshes;

		model_vbo_t* vbo = &vboModel->vbo_rtx;

		surf = (mdxmSurface_t *)( (byte *)lod + sizeof (mdxmLOD_t) + ( mdxm->numSurfaces * sizeof (mdxmLODSurfOffset_t) ) );
		int model_vertices = 0;
		int model_indices = 0;
		for ( n = 0; n < mdxm->numSurfaces; n++ )
		{
			model_vertices += surf->numVerts;
			model_indices += surf->numTriangles * 3;

			surf = (mdxmSurface_t *)((byte *)surf + surf->ofsEnd);
		}

		size_t vbo_size = model_vertices * sizeof(model_vertex_t) + model_indices * sizeof(uint32_t);

		vk_rtx_buffer_create( &vbo->buffer, vbo_size, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		vk_rtx_buffer_create( &vbo->staging_buffer, vbo_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

		uint32_t* staging_data = (uint32_t*)buffer_map( &vbo->staging_buffer );
		int write_ptr = 0;

		surf = (mdxmSurface_t *)((byte *)lod + sizeof(mdxmLOD_t) + (mdxm->numSurfaces * sizeof(mdxmLODSurfOffset_t)));

		for ( n = 0; n < mdxm->numSurfaces; n++ )
		{
			vboMeshes[n].meshIndex = n;
			vboMeshes[n].modelIndex = vboModel->model_index;
			vboMeshes[n].vbo_rtx = vbo;
			vboMeshes[n].vertexOffset = write_ptr;
			int num_verts = surf->numVerts;
			vboMeshes[n].numVertexes = surf->numVerts;
			vboMeshes[n].numIndexes = surf->numTriangles * 3;

			mdxmVertex_t *v = (mdxmVertex_t *)((byte *)surf + surf->ofsVerts);
			mdxmVertexTexCoord_t *tc = (mdxmVertexTexCoord_t *)(v + surf->numVerts);
			for ( k = 0; k < num_verts; k++ )
			{
				model_vertex_t* vtx = (model_vertex_t*)(staging_data + write_ptr) + k;
				memcpy( vtx->position,	v[k].vertCoords,	sizeof(vec3_t) );
				memcpy( vtx->normal,	v[k].normal,		sizeof(vec3_t) );
				memcpy( vtx->texcoord,	tc[k].texCoords,	sizeof(vec2_t) );
			}
				
			write_ptr += num_verts * (sizeof(model_vertex_t) / sizeof(uint32_t));

			vboMeshes[n].indexOffset = write_ptr;
			
			mdxmTriangle_t *t = (mdxmTriangle_t *)((byte *)surf + surf->ofsTriangles);
			memcpy( staging_data + write_ptr, t + 0, sizeof(uint32_t) * surf->numTriangles * 3 );

			write_ptr += surf->numTriangles * 3;

			surf = (mdxmSurface_t *)((byte *)surf + surf->ofsEnd);
		}

		buffer_unmap(&vbo->staging_buffer);

		VkCommandBuffer cmd_buf = vk_begin_command_buffer();
		if (vbo->staging_buffer.buffer)
		{
			VkBufferCopy copyRegion;
			Com_Memset( &copyRegion, 0, sizeof(VkBufferCopy) );
			copyRegion.size = vbo->staging_buffer.size;

			qvkCmdCopyBuffer(cmd_buf, vbo->staging_buffer.buffer, vbo->buffer.buffer, 1, &copyRegion);
		}
		vk_end_command_buffer(cmd_buf);
		vk_wait_idle();

		if (vbo->staging_buffer.buffer)
		{
			VK_DestroyBuffer( &vbo->staging_buffer );

			vk_rtx_write_model_descriptor( vboModel->model_index, vk.model_instance.descriptor.vbos, vbo->buffer.buffer, vbo->buffer.size );
		}
	}
}
#endif