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

static const uint32_t THREAD_GROUP_SIZE = 16;
static const uint32_t FILTER_THREAD_GROUP_SIZE = 16;

static void create_image_views( void );
static void create_pipeline_layout( void );
static void create_pipelines( void );
static void create_descriptor_set( void );
static void update_descriptor_set( void );

struct
{
	VkPipeline pipelines[2];
	VkPipelineLayout pipeline_layout;
	VkDescriptorSetLayout descriptor_set_layout;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_set;

	VkImageView shadow_image_view;
	VkSampler shadow_sampler;

	cvar_t* intensity;
	cvar_t* eccentricity;
	cvar_t* enable;
} god_rays;


void vk_rtx_get_god_rays_shadowmap( VkImageView &view, VkSampler &sampler )
{
	// used for ImGui
	view = god_rays.shadow_image_view;
	sampler = god_rays.shadow_sampler;
}

VkResult vk_rtx_god_rays_initialize( void )
{
	memset( &god_rays, 0, sizeof(god_rays) );

	VkPhysicalDeviceProperties properties;
	qvkGetPhysicalDeviceProperties( vk.physical_device, &properties );

	god_rays.intensity		= ri.Cvar_Get( "gr_intensity",		"2.0",	0, "" );
	god_rays.eccentricity	= ri.Cvar_Get( "gr_eccentricity",	"0.75",	0, "" );
	god_rays.enable			= ri.Cvar_Get( "gr_enable",			"1",	0, "" );

	return VK_SUCCESS;
}

VkResult vk_rtx_god_rays_destroy( void )
{
	qvkDestroySampler( vk.device, god_rays.shadow_sampler, NULL );
	qvkDestroyDescriptorPool( vk.device, god_rays.descriptor_pool, NULL );

	return VK_SUCCESS;
}

VkResult vk_rtx_god_rays_create_pipelines( void )
{
	create_pipeline_layout();
	create_pipelines();
	create_descriptor_set();
	
	// this is a noop outside a shader reload
	update_descriptor_set();

	return VK_SUCCESS;
}

VkResult vk_rtx_god_rays_destroy_pipelines( void )
{
	for ( size_t i = 0; i < ARRAY_LEN(god_rays.pipelines); i++ ) {
		if ( god_rays.pipelines[i] ) {
			qvkDestroyPipeline( vk.device, god_rays.pipelines[i], NULL );
			god_rays.pipelines[i] = NULL;
		}
	}

	if ( god_rays.pipeline_layout ) {
		qvkDestroyPipelineLayout( vk.device, god_rays.pipeline_layout, NULL );
		god_rays.pipeline_layout = NULL;
	}
	
	if ( god_rays.descriptor_set_layout ) {
		qvkDestroyDescriptorSetLayout( vk.device, god_rays.descriptor_set_layout, NULL );
		god_rays.descriptor_set_layout = NULL;
	}

	return VK_SUCCESS;
}

VkResult
vk_rtx_god_rays_update_images( void )
{
	create_image_views();
	update_descriptor_set();
	return VK_SUCCESS;
}

VkResult
vk_rtx_god_rays_noop( void )
{
	return VK_SUCCESS;
}

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
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT; \
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL; \
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL; \
		qvkCmdPipelineBarrier( cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, \
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, \
				1, &barrier); \
	} while(0)

void vk_rtx_record_god_rays_trace_command_buffer( VkCommandBuffer command_buffer, int pass )
{
	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	BARRIER_COMPUTE( command_buffer, vk.rtx_images[RTX_IMG_PT_GODRAYS_THROUGHPUT_DIST] );
	BARRIER_COMPUTE( command_buffer, vk.rtx_images[RTX_IMG_ASVGF_COLOR] );

	qvkCmdBindPipeline( command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, god_rays.pipelines[0] );

	VkDescriptorSet desc_sets[4];
	desc_sets[0] = vk.computeDescriptor[idx].set;
	desc_sets[1] = vk.imageDescriptor.set;
	desc_sets[2] = vk.desc_set_ubo;
	desc_sets[3] = god_rays.descriptor_set;

	qvkCmdBindDescriptorSets( command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, god_rays.pipeline_layout, 0, ARRAY_LEN(desc_sets),
		desc_sets, 0, NULL );

	qvkCmdPushConstants( command_buffer, god_rays.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &pass );

	uint32_t group_size = THREAD_GROUP_SIZE * 2;
	uint32_t group_num_x = (vk.extent_render.width / vk.device_count + (group_size - 1)) / group_size;
	uint32_t group_num_y = (vk.extent_render.height + (group_size - 1)) / group_size;

	qvkCmdDispatch( command_buffer, group_num_x, group_num_y, 1 );

	BARRIER_COMPUTE( command_buffer, vk.rtx_images[RTX_IMG_PT_GODRAYS_THROUGHPUT_DIST] );
	BARRIER_COMPUTE( command_buffer, vk.rtx_images[RTX_IMG_ASVGF_COLOR] );
}

void vk_rtx_record_god_rays_filter_command_buffer( VkCommandBuffer command_buffer )
{
	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	BARRIER_COMPUTE( command_buffer, vk.rtx_images[RTX_IMG_PT_TRANSPARENT] );

	VkImageSubresourceRange subresource_range;
	Com_Memset( &subresource_range, 0, sizeof(VkImageSubresourceRange) );
	subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.levelCount = 1;
	subresource_range.layerCount = 1;

	qvkCmdBindPipeline( command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, god_rays.pipelines[1] );

	VkDescriptorSet desc_sets[4];
	desc_sets[0] = vk.computeDescriptor[idx].set;
	desc_sets[1] = vk.imageDescriptor.set;
	desc_sets[2] = vk.desc_set_ubo;
	desc_sets[3] = god_rays.descriptor_set;

	qvkCmdBindDescriptorSets( command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, god_rays.pipeline_layout, 0, ARRAY_LEN(desc_sets),
		desc_sets, 0, NULL );

	int pass = 0;
	qvkCmdPushConstants( command_buffer, god_rays.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &pass );

	uint32_t group_size = FILTER_THREAD_GROUP_SIZE;
	uint32_t group_num_x = ( vk.extent_render.width / vk.device_count + (group_size - 1) ) / group_size;
	uint32_t group_num_y = ( vk.extent_render.height + (group_size - 1) ) / group_size;

	qvkCmdDispatch( command_buffer, group_num_x, group_num_y, 1 );

	BARRIER_COMPUTE( command_buffer, vk.rtx_images[RTX_IMG_PT_TRANSPARENT] );
}


void vk_rtx_god_rays_prepare_ubo(
	vkUniformRTX_t * ubo,
	/*const aabb_t* world_aabb,*/
	const float* proj,
	const float* view, 
	const float* shadowmap_viewproj,
	float shadowmap_depth_scale)
{
	//VectorAdd(world_aabb->mins, world_aabb->maxs, ubo->world_center);
	VectorAdd( tr.viewParms.visBounds[0], tr.viewParms.visBounds[1], ubo->world_center );
	//Vector4Set( ubo->world_center, 300, 300, 300, 0.0 );

	VectorScale(ubo->world_center, 0.5f, ubo->world_center);
	//VectorSubtract(world_aabb->maxs, world_aabb->mins, ubo->world_size);
	VectorSubtract( tr.viewParms.visBounds[1], tr.viewParms.visBounds[0], ubo->world_size);
	//Vector4Set( ubo->world_size, 1200, 1200, 500, 0.0 );

	VectorScale(ubo->world_size, 0.5f, ubo->world_half_size_inv);
	ubo->world_half_size_inv[0] = 1.f / ubo->world_half_size_inv[0];
	ubo->world_half_size_inv[1] = 1.f / ubo->world_half_size_inv[1];
	ubo->world_half_size_inv[2] = 1.f / ubo->world_half_size_inv[2];
	ubo->shadow_map_depth_scale = shadowmap_depth_scale;

	ubo->god_rays_intensity = MAX(0.f, god_rays.intensity->value);
	ubo->god_rays_eccentricity = god_rays.eccentricity->value;

	// Shadow parameters
	memcpy(ubo->shadow_map_VP, shadowmap_viewproj, 16 * sizeof(float));
}


static void create_image_views( void )
{
	god_rays.shadow_image_view = vk_rtx_shadow_map_get_view();

	VkSamplerReductionModeCreateInfo redutcion_create_info;
	Com_Memset( &redutcion_create_info, 0, sizeof(VkSamplerReductionModeCreateInfo) );
	redutcion_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
	redutcion_create_info.pNext = NULL;
	redutcion_create_info.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;

	VkSamplerCreateInfo sampler_create_info;
	Com_Memset( &sampler_create_info, 0, sizeof(VkSamplerCreateInfo) );
	sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.pNext = NULL;
	sampler_create_info.pNext = &redutcion_create_info;
	sampler_create_info.magFilter = VK_FILTER_LINEAR;
	sampler_create_info.minFilter = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	VK_CHECK( qvkCreateSampler( vk.device, &sampler_create_info, NULL, &god_rays.shadow_sampler ) );
}

static void create_pipeline_layout( void )
{
	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );


	/*
	vkpipeline_t *pipeline = &vk.physical_sky_pipeline[pipeline_index];

	VkDescriptorSetLayout set_layouts[2] = {
		vk.computeDescriptor[0].layout,
		vk.imageDescriptor.layout
	};

	vk_rtx_bind_pipeline_shader( pipeline, vk.physical_sky_shader[shader_index] );
	vk_rtx_bind_pipeline_desc_set_layouts( pipeline, set_layouts, ARRAY_LEN(set_layouts) );
	vk_rtx_create_compute_pipeline( pipeline, spec, push_size );
	*/




	VkDescriptorSetLayoutBinding bindings[1] = { 0 };
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	
	VkDescriptorSetLayoutCreateInfo set_layout_create_info;
	Com_Memset( &set_layout_create_info, 0, sizeof(VkDescriptorSetLayoutCreateInfo) );
	set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_layout_create_info.pNext = NULL;
	set_layout_create_info.bindingCount = ARRAY_LEN(bindings);
	set_layout_create_info.pBindings = bindings;

	VK_CHECK( qvkCreateDescriptorSetLayout( vk.device, &set_layout_create_info, NULL,
		&god_rays.descriptor_set_layout) );

	VkDescriptorSetLayout desc_set_layouts[4];
	desc_set_layouts[0] = vk.computeDescriptor[0].layout;
	desc_set_layouts[1] = vk.imageDescriptor.layout;
	desc_set_layouts[2] = vk.desc_set_layout_ubo;
	desc_set_layouts[3] = god_rays.descriptor_set_layout;

	VkPushConstantRange push_constant_range;
	push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(int);

	VkPipelineLayoutCreateInfo layout_create_info;
	Com_Memset( &layout_create_info, 0, sizeof(VkPipelineLayoutCreateInfo) );
	layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_create_info.pNext = NULL;
	layout_create_info.pSetLayouts = desc_set_layouts;
	layout_create_info.setLayoutCount = ARRAY_LEN(desc_set_layouts);
	layout_create_info.pushConstantRangeCount = 1;
	layout_create_info.pPushConstantRanges = &push_constant_range;

	VK_CHECK( qvkCreatePipelineLayout( vk.device, &layout_create_info, NULL,
		&god_rays.pipeline_layout ) );
}

static void create_pipelines( void )
{
	VkPipelineShaderStageCreateInfo shader;
	Com_Memset( &shader, 0, sizeof(VkPipelineShaderStageCreateInfo) );
	shader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader.pNext = NULL;
	shader.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shader.module = vk.asvgf_shader[SHADER_GOD_RAYS_COMP]->modules[0];
	shader.pName = "main";

	VkPipelineShaderStageCreateInfo filter_shader;
	Com_Memset( &filter_shader, 0, sizeof(VkPipelineShaderStageCreateInfo) );
	filter_shader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	filter_shader.pNext = NULL;
	filter_shader.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	filter_shader.module = vk.asvgf_shader[SHADER_GOD_RAYS_FILTER_COMP]->modules[0];
	filter_shader.pName = "main";

	VkComputePipelineCreateInfo pipeline_create_infos[2];
	Com_Memset( &pipeline_create_infos, 0, sizeof(VkComputePipelineCreateInfo) * 2 );

	{
		pipeline_create_infos[0].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_create_infos[0].pNext = NULL;
		pipeline_create_infos[0].stage = shader;
		pipeline_create_infos[0].layout = god_rays.pipeline_layout;
	}
	{
		pipeline_create_infos[1].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_create_infos[1].pNext = NULL;
		pipeline_create_infos[1].stage = filter_shader;
		pipeline_create_infos[1].layout = god_rays.pipeline_layout;
	}

	VK_CHECK( qvkCreateComputePipelines( vk.device, VK_NULL_HANDLE, ARRAY_LEN(pipeline_create_infos), pipeline_create_infos,
		NULL, god_rays.pipelines ) );
}

static void create_descriptor_set( void )
{
	const VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};

	VkDescriptorPoolCreateInfo pool_create_info;
	Com_Memset( &pool_create_info, 0, sizeof(VkDescriptorPoolCreateInfo) );
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.pNext = NULL;
	pool_create_info.maxSets = 1;
	pool_create_info.poolSizeCount = ARRAY_LEN(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;

	VK_CHECK( qvkCreateDescriptorPool( vk.device, &pool_create_info, NULL, &god_rays.descriptor_pool ) );

	VkDescriptorSetAllocateInfo allocate_info;
	Com_Memset( &allocate_info, 0, sizeof(VkDescriptorSetAllocateInfo) );
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.descriptorPool = god_rays.descriptor_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = &god_rays.descriptor_set_layout;

	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &allocate_info, &god_rays.descriptor_set ) );
}

static void update_descriptor_set( void )
{
	// if we end up here during init before we've called create_image_views(), punt --- we will be called again later
	if ( god_rays.shadow_image_view == NULL )
		return;

	VkDescriptorImageInfo image_info;
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView = god_rays.shadow_image_view;
	image_info.sampler = god_rays.shadow_sampler;
	
	VkWriteDescriptorSet writes[1];
	Com_Memset( &writes, 0, sizeof(VkWriteDescriptorSet) * 1 );
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = NULL;
	writes[0].dstSet = god_rays.descriptor_set;
	writes[0].descriptorCount = 1;
	writes[0].dstBinding = 0;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[0].pImageInfo = &image_info;

	qvkUpdateDescriptorSets( vk.device, ARRAY_LEN(writes), writes, 0, NULL );
}

qboolean vk_rtx_god_rays_enabled( const sun_light_t* sun_light )
{
	return ( god_rays.enable->integer
		&& god_rays.intensity->value > 0.f
		&& sun_light->visible
		&& !physical_sky_space->integer ) ? qtrue : qfalse;  // god rays look weird in space because they also appear outside of the station
}