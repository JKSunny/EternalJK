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
#include "shaders/spirv/shader_data_rtx.c"

static vkshader_t				*global_rtx_shader;
static VkSpecializationMapEntry specEntry;
static VkSpecializationInfo		specInfo[2];

uint32_t numbers[2] = { 0, 1 };

#define SHADER_STAGE( index, handle, flag ) \
	shader->flags[index] = flag; \
	shader->modules[index] = vk_rtx_create_shader( handle, sizeof(handle) ); \
	\
	shader->shader_stages[index].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; \
	shader->shader_stages[index].pNext = NULL; \
	shader->shader_stages[index].stage = shader->flags[index]; \
	shader->shader_stages[index].module = shader->modules[index]; \
	shader->shader_stages[index].pName = "main";

#define SHADER_STAGE_SPEC( index, handle, flag, spec ) \
	shader->flags[index] = flag; \
	shader->modules[index] = vk_rtx_create_shader( handle, sizeof(handle) ); \
	\
	shader->shader_stages[index].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; \
	shader->shader_stages[index].pNext = NULL; \
	shader->shader_stages[index].stage = shader->flags[index]; \
	shader->shader_stages[index].module = shader->modules[index]; \
	shader->shader_stages[index].pName = "main"; \
	shader->shader_stages[index].pSpecializationInfo = spec;

static VkShaderModule vk_rtx_create_shader( const uint8_t *bytes, const int count )
{
    VkShaderModuleCreateInfo desc;
    VkShaderModule module;

    if ( count % 4 != 0 )
        ri.Error(ERR_FATAL, "Vulkan: SPIR-V binary buffer size is not a multiple of 4");

	Com_Memset( &desc, 0, sizeof(VkShaderModuleCreateInfo) );
    desc.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.codeSize = count;
    desc.pCode = (const uint32_t*)bytes;

    VK_CHECK(qvkCreateShaderModule(vk.device, &desc, NULL, &module));

    return module;
}

static void vk_rtx_load_shader( vkshader_t *shader, const uint8_t *bytes, const uint32_t size, VkShaderStageFlagBits flags ) 
{
	shader->size = 1;
	shader->modules = (VkShaderModule*)malloc(shader->size * sizeof(VkShaderModule));
	shader->flags = (VkShaderStageFlagBits*)malloc(shader->size * sizeof(VkShaderStageFlagBits));
	shader->shader_stages = (VkPipelineShaderStageCreateInfo*)calloc(shader->size, sizeof(VkPipelineShaderStageCreateInfo));
	shader->shader_groups = NULL;

	shader->flags[0] = flags;

	shader->modules[0] = vk_rtx_create_shader( bytes, size );

	shader->shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader->shader_stages[0].stage = flags;
	shader->shader_stages[0].module = shader->modules[0];
	shader->shader_stages[0].pName = "main";
}

static void vk_rtx_destroy_shader( vkshader_t *shader ) 
{
	uint32_t i;

	for ( i = 0; i < shader->size; i++ ) 
		qvkDestroyShaderModule( vk.device, shader->modules[i], NULL );

	free( shader->modules );
	free( shader->flags );
	free( shader->shader_stages );

	if ( shader->shader_groups ) 
		free( shader->shader_groups) ;

	Com_Memset( shader, 0, sizeof(vkshader_t) );
}

// tonemap
void vk_load_tonemap_shaders( void ) 
{
	#define SHADER_MODULE_DO( _index, _spirv_handle ) \
	{ \
		if ( vk.tonemap_shader[##_index] == NULL ) {\
			vk.tonemap_shader[##_index] = (vkshader_t*)malloc(sizeof(vkshader_t)); \
			vk_rtx_load_shader( vk.tonemap_shader[##_index], ##_spirv_handle, sizeof(##_spirv_handle), VK_SHADER_STAGE_COMPUTE_BIT ); \
		} \
	}
	LIST_TONEMAP_SHADERS
	#undef SHADER_MODULE_DO
}

static void vk_destroy_tonemap_shaders( void ) 
{
	#define SHADER_MODULE_DO( _index, ... ) \
		vk_rtx_destroy_shader( vk.tonemap_shader[##_index] ); \
		free( vk.tonemap_shader[##_index] ); \
		vk.tonemap_shader[##_index] = NULL; 
	LIST_TONEMAP_SHADERS
	#undef SHADER_MODULE_DO
}

// asvgf
void vk_load_asvgf_shaders( void ) 
{
	#define SHADER_MODULE_DO( _index, _spirv_handle ) \
	{ \
		if ( vk.asvgf_shader[##_index] == NULL ) {\
			vk.asvgf_shader[##_index] = (vkshader_t*)malloc(sizeof(vkshader_t)); \
			vk_rtx_load_shader( vk.asvgf_shader[##_index], ##_spirv_handle, sizeof(##_spirv_handle), VK_SHADER_STAGE_COMPUTE_BIT ); \
		} \
	}
	LIST_ASVGF_SHADERS
	#undef SHADER_MODULE_DO
}

static void vk_destroy_asvgf_shaders( void ) 
{
	#define SHADER_MODULE_DO( _index, ... ) \
		vk_rtx_destroy_shader( vk.asvgf_shader[##_index] ); \
		free( vk.asvgf_shader[##_index] ); \
		vk.asvgf_shader[##_index] = NULL; 
	LIST_ASVGF_SHADERS
	#undef SHADER_MODULE_DO
}

// physical sky
void vk_load_physical_sky_shaders( void )
{
	#define SHADER_MODULE_DO( _index, _spirv_handle ) \
	{ \
		if ( vk.physical_sky_shader[##_index] == NULL ) {\
			vk.physical_sky_shader[##_index] = (vkshader_t*)malloc(sizeof(vkshader_t)); \
			vk_rtx_load_shader( vk.physical_sky_shader[##_index], ##_spirv_handle, sizeof(##_spirv_handle), VK_SHADER_STAGE_COMPUTE_BIT ); \
		} \
	}
	LIST_PHYSICAL_SKY_SHADERS
	#undef SHADER_MODULE_DO
}

static void vk_destroy_physical_sky_shaders( void ) 
{
	#define SHADER_MODULE_DO( _index, ... ) \
		vk_rtx_destroy_shader( vk.physical_sky_shader[##_index] ); \
		free( vk.physical_sky_shader[##_index] ); \
		vk.physical_sky_shader[##_index] = NULL; 
	LIST_PHYSICAL_SKY_SHADERS
	#undef SHADER_MODULE_DO
}

// final blit
void vk_load_final_blit_shader( void )
{
	if ( vk.final_blit_shader_frag != NULL )
		return;

	vk.final_blit_shader_frag = (vkshader_t*)malloc(sizeof(vkshader_t));
	vk_rtx_load_shader( vk.final_blit_shader_frag, final_blit_lanczos_frag, sizeof(final_blit_lanczos_frag), VK_SHADER_STAGE_FRAGMENT_BIT );
}

void vk_rtx_destroy_shaders( void ) 
{
	if ( !vk.rtxActive )
		return;

	vk_destroy_asvgf_shaders();
	vk_destroy_tonemap_shaders();

	vk_rtx_destroy_shader( global_rtx_shader );
	free( global_rtx_shader );
	global_rtx_shader = NULL;
}

static void vk_rtx_prepare_global_shader( vkshader_t *shader ) 
{
	shader->size = 10;
	shader->modules = (VkShaderModule*)calloc(shader->size, sizeof(VkShaderModule));
	shader->flags = (VkShaderStageFlagBits*)calloc(shader->size, sizeof(VkShaderStageFlagBits));
	shader->shader_stages = (VkPipelineShaderStageCreateInfo*)calloc(shader->size, sizeof(VkPipelineShaderStageCreateInfo));

	specEntry.constantID = 0;
	specEntry.offset = 0;
	specEntry.size = sizeof(uint32_t);

	specInfo[0].mapEntryCount = 1;
	specInfo[0].pMapEntries = &specEntry;
	specInfo[0].dataSize = sizeof(uint32_t);
	specInfo[0].pData = &numbers[0];

	specInfo[1].mapEntryCount = 1;
	specInfo[1].pMapEntries = &specEntry;
	specInfo[1].dataSize = sizeof(uint32_t);
	specInfo[1].pData = &numbers[1];

	Com_Memset( shader->shader_stages, 0, sizeof(VkPipelineShaderStageCreateInfo) * shader->size );

	{
		SHADER_STAGE(		0, primary_rays_rgen,				VK_SHADER_STAGE_RAYGEN_BIT_KHR )
		SHADER_STAGE_SPEC(	1, reflect_refract_rgen,			VK_SHADER_STAGE_RAYGEN_BIT_KHR, &specInfo[0] )
		SHADER_STAGE_SPEC(	2, reflect_refract_rgen,			VK_SHADER_STAGE_RAYGEN_BIT_KHR, &specInfo[1] )
		SHADER_STAGE_SPEC(	3, direct_lighting_rgen,			VK_SHADER_STAGE_RAYGEN_BIT_KHR, &specInfo[0] )
		SHADER_STAGE_SPEC(	4, direct_lighting_rgen,			VK_SHADER_STAGE_RAYGEN_BIT_KHR, &specInfo[1] )
		SHADER_STAGE_SPEC(	5, indirect_lighting_rgen,			VK_SHADER_STAGE_RAYGEN_BIT_KHR, &specInfo[0] )
		SHADER_STAGE_SPEC(	6, indirect_lighting_rgen,			VK_SHADER_STAGE_RAYGEN_BIT_KHR, &specInfo[1] )
		SHADER_STAGE(		7, path_tracer_rmiss,				VK_SHADER_STAGE_MISS_BIT_KHR )
		SHADER_STAGE(		8, path_tracer_shadow_rmiss,		VK_SHADER_STAGE_MISS_BIT_KHR )
		SHADER_STAGE(		9, path_tracer_rchit,				VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR )
	}

	VkRayTracingShaderGroupCreateInfoKHR groups[11];
	VkRayTracingShaderGroupCreateInfoKHR *group;
	Com_Memset( &groups, 0, sizeof(VkRayTracingShaderGroupCreateInfoKHR) * 11 );

	group						= &groups[SBT_RGEN_PRIMARY_RAYS];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group->generalShader		= 0;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RGEN_REFLECT_REFRACT1];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group->generalShader		= 1;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RGEN_REFLECT_REFRACT2];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group->generalShader		= 2;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RGEN_DIRECT_LIGHTING];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group->generalShader		= 3;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RGEN_DIRECT_LIGHTING_CAUSTICS];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					 = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group->generalShader		= 4;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RGEN_INDIRECT_LIGHTING_FIRST];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group->generalShader		= 5;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RGEN_INDIRECT_LIGHTING_SECOND];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group->generalShader		= 6;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RMISS_PATH_TRACER];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group->generalShader		= 7;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RMISS_SHADOW];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group->generalShader		= 8;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RCHIT_OPAQUE];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	group->generalShader		= VK_SHADER_UNUSED_KHR;
	group->closestHitShader		= 9;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	group						= &groups[SBT_RCHIT_EMPTY];
	group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	group->generalShader		= VK_SHADER_UNUSED_KHR;
	group->closestHitShader		= VK_SHADER_UNUSED_KHR;
	group->anyHitShader			= VK_SHADER_UNUSED_KHR;
	group->intersectionShader	= VK_SHADER_UNUSED_KHR;

	shader->group_count = sizeof(groups) / sizeof(VkRayTracingShaderGroupCreateInfoKHR);
	shader->shader_groups = (VkRayTracingShaderGroupCreateInfoKHR*)calloc(shader->group_count, sizeof(VkRayTracingShaderGroupCreateInfoKHR));
	memcpy(shader->shader_groups, &groups[0], sizeof(groups));
}

vkshader_t *vk_rtx_create_global_shader( void ) 
{
	if ( !global_rtx_shader )
	{
		global_rtx_shader = (vkshader_t*)malloc(sizeof(vkshader_t));
		vk_rtx_prepare_global_shader( global_rtx_shader );
	}

	return global_rtx_shader;
}