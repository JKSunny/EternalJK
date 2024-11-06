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
static void vk_rtx_create_rt_pipeline_layout( void )
{
	VkDescriptorSetLayout set_layouts[] = {
		vk.rt_descriptor_set[0].layout,
		vk.desc_set_layout_textures,
		vk.imageDescriptor.layout,
		vk.desc_set_layout_ubo,
		vk.desc_set_vertex_buffer[0].layout,
	};

	VkPipelineLayoutCreateInfo desc;
	
	VkPushConstantRange push_constant_range;
	push_constant_range.stageFlags		= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	push_constant_range.offset			= 0;
	push_constant_range.size			= sizeof(pt_push_constants_t);

	Com_Memset( &desc, 0, sizeof(VkPipelineLayoutCreateInfo) );
	desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	desc.pNext = NULL;
	desc.pSetLayouts = set_layouts;
	desc.setLayoutCount = ARRAY_LEN(set_layouts);
	desc.pushConstantRangeCount = 1;
	desc.pPushConstantRanges = &push_constant_range;

	VK_CHECK( qvkCreatePipelineLayout( vk.device, &desc, NULL, &vk.rt_pipeline_layout ) );
}

void vk_rtx_create_rt_pipelines( void ) 
{
	vk_rtx_create_rt_pipeline_layout();
	vk_rtx_create_pipelines(); 

	vk_load_final_blit_shader();
	vk_rtx_create_final_blit_pipeline();
}
