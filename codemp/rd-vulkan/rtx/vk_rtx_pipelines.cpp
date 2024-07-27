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

//
// global
//
void vk_rtx_bind_pipeline_shader( vkpipeline_t *pipeline, vkshader_t *shader )
{
	pipeline->shader = shader;
}

void vk_rtx_bind_pipeline_desc_set_layouts( vkpipeline_t *pipeline, VkDescriptorSetLayout *set_layouts,
											uint32_t count )
{
	const size_t size = ( sizeof(VkDescriptorSetLayout) * count );

	pipeline->set_layouts = (VkDescriptorSetLayout*)Hunk_Alloc( size, h_low );
	pipeline->set_layouts_count = count;

	Com_Memcpy( pipeline->set_layouts + 0, set_layouts, size );
}

static void vk_rtx_bind_specialization_info( vkshader_t *shader, VkSpecializationInfo *spec )
{
	if ( spec == NULL )
		return;

	shader->shader_stages->pSpecializationInfo = spec;
}

static void vk_rtx_create_pipeline_cache( vkpipeline_t *pipeline )
{
	VkPipelineCacheCreateInfo ci;

	Com_Memset( &ci, 0, sizeof(VkPipelineCacheCreateInfo) );
	ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ci.pNext = NULL;

	VK_CHECK( qvkCreatePipelineCache( vk.device, &ci, NULL, &pipeline->cache ) );
}

static void vk_rtx_create_pipeline_layout( vkpipeline_t *pipeline )
{
	VkPipelineLayoutCreateInfo desc;
	
	Com_Memset( &desc, 0, sizeof(VkPipelineLayoutCreateInfo) );
	desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	desc.pNext = NULL;
	desc.pSetLayouts = pipeline->set_layouts;
	desc.setLayoutCount = pipeline->set_layouts_count;
	desc.pushConstantRangeCount = pipeline->pushConstantRange.size;
	desc.pPushConstantRanges = pipeline->pushConstantRange.p;

	VK_CHECK( qvkCreatePipelineLayout( vk.device, &desc, NULL, &pipeline->layout ) );
}

void vk_rtx_destroy_pipeline( vkpipeline_t *pipeline ) 
{
	free( pipeline->pushConstantRange.p );
	Z_Free( pipeline->set_layouts );

	if ( pipeline->layout ) {
		qvkDestroyPipelineLayout( vk.device, pipeline->layout, NULL );
		pipeline->layout = VK_NULL_HANDLE;
	}

	if ( pipeline->handle ) {
		qvkDestroyPipeline( vk.device, pipeline->handle, NULL );
		pipeline->handle = VK_NULL_HANDLE;
	}

	if ( pipeline->cache ) {
		qvkDestroyPipelineCache( vk.device, pipeline->cache, NULL );
		pipeline->cache = VK_NULL_HANDLE;
	}

	memset( pipeline, 0, sizeof(vkpipeline_t) );
}

//
// compute
//
static void vk_rtx_finish_compute_pipeline( vkpipeline_t *pipeline )
{
	assert( pipeline->shader->size == 1 );

	VkComputePipelineCreateInfo desc;
	Com_Memset( &desc, 0, sizeof(VkComputePipelineCreateInfo) );

	desc.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	desc.pNext = NULL;
	desc.stage = pipeline->shader->shader_stages[0];
	desc.layout = pipeline->layout;

	VK_CHECK( qvkCreateComputePipelines( vk.device, pipeline->cache, 1, &desc, NULL, &pipeline->handle ) );
	VK_SET_OBJECT_NAME( pipeline->handle, "compute pipline", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT );
}

static void vk_rtx_bind_compute_push_constant( vkpipeline_t *pipeline, VkShaderStageFlags stage,
	uint32_t offset, uint32_t size ) 
{
	if ( !size )
		return;

	pipeline->pushConstantRange.size++;
	pipeline->pushConstantRange.p = (VkPushConstantRange*)realloc(pipeline->pushConstantRange.p, pipeline->pushConstantRange.size * sizeof(VkPushConstantRange));
	pipeline->pushConstantRange.p[pipeline->pushConstantRange.size - 1].stageFlags = stage;
	pipeline->pushConstantRange.p[pipeline->pushConstantRange.size - 1].offset = offset;
	pipeline->pushConstantRange.p[pipeline->pushConstantRange.size - 1].size = size;
}

void vk_rtx_create_compute_pipeline( vkpipeline_t *pipeline, VkSpecializationInfo *spec, uint32_t push_size ) 
{
	vk_rtx_bind_specialization_info( pipeline->shader, spec );
	vk_rtx_bind_compute_push_constant( pipeline, VK_SHADER_STAGE_COMPUTE_BIT, 0, push_size );

	vk_rtx_create_pipeline_cache( pipeline );
	vk_rtx_create_pipeline_layout( pipeline );
	vk_rtx_finish_compute_pipeline( pipeline );
}

void vk_rtx_create_compute_pipelines( void ) 
{
	vk_create_asvgf_pipelines();
	vk_create_tonemap_pipelines();
	vk_create_physical_sky_pipelines();

	vk_rtx_shadow_map_initialize();
	vk_rtx_shadow_map_create_pipelines();

	vk_rtx_god_rays_initialize();
	vk_rtx_god_rays_create_pipelines();
	vk_rtx_god_rays_update_images();

	vk_rtx_model_vbo_create_pipelines();
}

//
// raytracing
//
static void vk_rtx_finish_raytracing_pipeline( vkpipeline_t *pipeline )
{
	VkPipelineLibraryCreateInfoKHR library_info;
	VkRayTracingPipelineCreateInfoKHR desc;

	Com_Memset( &library_info, 0, sizeof(VkPipelineLibraryCreateInfoKHR) );
	library_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
	library_info.pNext = NULL;

	
	Com_Memset( &desc, 0, sizeof(VkRayTracingPipelineCreateInfoKHR) );
	desc.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.stageCount = pipeline->shader->size;
	desc.pStages = pipeline->shader->shader_stages;
	desc.groupCount = pipeline->shader->group_count;
	desc.pGroups = pipeline->shader->shader_groups;
	desc.maxPipelineRayRecursionDepth = 1;
	desc.layout = pipeline->layout;
	desc.pLibraryInfo = &library_info,
	desc.pLibraryInterface = NULL,
	desc.pDynamicState = NULL,
	desc.basePipelineHandle = pipeline->handle;
	desc.basePipelineIndex = 0;

	VK_CHECK( qvkCreateRayTracingPipelinesKHR( vk.device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &desc, NULL, &pipeline->handle ) );
}

static void vk_rtx_create_raytracing_shader_binding_table( vkpipeline_t *pipeline ) 
{
	uint32_t	num_shader_groups = 0;
	char		*shader_handles = NULL;

	num_shader_groups = pipeline->shader->group_count;

	const uint32_t shader_handle_array_size = num_shader_groups * vk.shaderGroupHandleSize;
	shader_handles = (char*)alloca(shader_handle_array_size);

	VK_CHECK( qvkGetRayTracingShaderGroupHandlesKHR( vk.device, pipeline->handle, 0, num_shader_groups,
		shader_handle_array_size, shader_handles ) );

	// create the SBT buffer
	uint32_t shader_binding_table_size = vk.shaderGroupBaseAlignment * num_shader_groups;
	vk_rtx_buffer_create( &vk.rt_shader_binding_table, shader_binding_table_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

	// copy/unpack the shader handles into the SBT:
	// shaderGroupBaseAlignment is likely greater than shaderGroupHandleSize (64 vs 32 on NV)
	char *shader_binding_table = (char*)buffer_map( &vk.rt_shader_binding_table );
	for ( uint32_t group = 0; group < num_shader_groups; group++ )
	{
		memcpy(	shader_binding_table + group * vk.shaderGroupBaseAlignment,
				shader_handles + group * vk.shaderGroupHandleSize,
				vk.shaderGroupHandleSize );
	}

	buffer_unmap( &vk.rt_shader_binding_table );
	shader_binding_table = NULL;
}

void vk_rtx_create_raytracing_pipelines( void ) 
{
	vkpipeline_t *pipeline = &vk.rt_pipeline;

	VkDescriptorSetLayout set_layouts[3] = {
		vk.rtxDescriptor[0].layout,
		vk.imageDescriptor.layout,
		vk.desc_set_layout_ubo
	};

	vk_rtx_bind_pipeline_shader( pipeline, vk_rtx_create_global_shader() );
	vk_rtx_bind_pipeline_desc_set_layouts( pipeline, set_layouts, ARRAY_LEN(set_layouts) );
	vk_rtx_bind_compute_push_constant( pipeline, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(pt_push_constants_t) );

	vk_rtx_create_pipeline_layout( pipeline );
	vk_rtx_finish_raytracing_pipeline( pipeline );
	vk_rtx_create_raytracing_shader_binding_table( pipeline );

	vk_load_final_blit_shader();
	vk_rtx_create_final_blit_pipeline();
}
