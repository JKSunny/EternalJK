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

static VkSpecializationMapEntry specEntry;
static VkSpecializationInfo		specInfo[2];

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
	shader->shader_stages = (VkPipelineShaderStageCreateInfo*)calloc(shader->size, sizeof(VkPipelineShaderStageCreateInfo));
	shader->shader_groups = NULL;

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
}

#define LIST_PATH_TRACER_SHADERS \
	SHADER_MODULE_DO( SHADER_PATH_TRACER_PRIMARY_RAYS_RGEN,			primary_rays_rgen		) \
	SHADER_MODULE_DO( SHADER_PATH_TRACER_REFLECT_REFRACT_RGEN,		reflect_refract_rgen	) \
	SHADER_MODULE_DO( SHADER_PATH_TRACER_DIRECT_LIGHTING_RGEN,		direct_lighting_rgen	) \
	SHADER_MODULE_DO( SHADER_PATH_TRACER_INDIRECT_LIGHTING_RGEN,	indirect_lighting_rgen	) \
	\
	SHADER_MODULE_DO( SHADER_PATH_TRACER_PATH_TRACER_RMISS,			path_tracer_rmiss		) \
	SHADER_MODULE_DO( SHADER_PATH_TRACER_PATH_TRACER_RCHIT,			path_tracer_rchit		) \

enum {
#define SHADER_MODULE_DO( _index, ... ) _index,
	LIST_PATH_TRACER_SHADERS
#undef SHADER_MODULE_DO
	NUM_PATH_TRACER_SHADER_MODULES
};

void vk_rtx_create_shader_modules( void )
{
	#define SHADER_MODULE_DO( _index, _bytes ) \
	vk.shader_modules[_index] = vk_rtx_create_shader( _bytes, sizeof(_bytes) );
		LIST_PATH_TRACER_SHADERS
	#undef SHADER_MODULE_DO
}

#define SHADER_STAGE( index, _module, _stage ) \
	shader_stages[index].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; \
	shader_stages[index].pNext = NULL; \
	shader_stages[index].stage = _stage; \
	shader_stages[index].module = vk.shader_modules[_module]; \
	shader_stages[index].pName = "main";

void vk_rtx_create_pipelines( void ) 
{
	Com_Memset( &vk.rt_pipelines, 0, sizeof(VkPipeline) * PIPELINE_COUNT );

	uint32_t numbers[2] = { 0, 1 };

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

	uint32_t num_shader_groups = SBT_ENTRIES_PER_PIPELINE * PIPELINE_COUNT;
	char* shader_handles = (char*)alloca(num_shader_groups * vk.shaderGroupHandleSize);
	memset(shader_handles, 0, num_shader_groups * vk.shaderGroupHandleSize);

	VkPipelineShaderStageCreateInfo shader_stages[SBT_ENTRIES_PER_PIPELINE];
	Com_Memset( &shader_stages, 0, sizeof(VkPipelineShaderStageCreateInfo) * SBT_ENTRIES_PER_PIPELINE );

	// Stages used by all pipelines. Count must match num_base_shader_stages below!
	{
		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].pNext = NULL;
		shader_stages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		shader_stages[0].module = VK_NULL_HANDLE;
		shader_stages[0].pName = "main";
		// Shader module is set below
	}
	SHADER_STAGE( 1, SHADER_PATH_TRACER_PATH_TRACER_RMISS,	VK_SHADER_STAGE_MISS_BIT_KHR		)
	SHADER_STAGE( 2, SHADER_PATH_TRACER_PATH_TRACER_RCHIT,	VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR )

	for ( uint32_t index = 0; index < PIPELINE_COUNT; index++ )
	{
		unsigned int num_shader_stages = ARRAY_LEN(shader_stages);

		switch (index)
		{
		case PIPELINE_PRIMARY_RAYS:
			shader_stages[0].module = vk.shader_modules[SHADER_PATH_TRACER_PRIMARY_RAYS_RGEN];
			shader_stages[0].pSpecializationInfo = NULL;
			break;
		case PIPELINE_REFLECT_REFRACT_1:
			shader_stages[0].module = vk.shader_modules[SHADER_PATH_TRACER_REFLECT_REFRACT_RGEN];
			shader_stages[0].pSpecializationInfo = &specInfo[0];
			break;
		case PIPELINE_REFLECT_REFRACT_2:
			shader_stages[0].module = vk.shader_modules[SHADER_PATH_TRACER_REFLECT_REFRACT_RGEN];
			shader_stages[0].pSpecializationInfo = &specInfo[1];
			break;
		case PIPELINE_DIRECT_LIGHTING:
			shader_stages[0].module = vk.shader_modules[SHADER_PATH_TRACER_DIRECT_LIGHTING_RGEN];
			shader_stages[0].pSpecializationInfo = &specInfo[0];
			break;
		case PIPELINE_DIRECT_LIGHTING_CAUSTICS:
			shader_stages[0].module = vk.shader_modules[SHADER_PATH_TRACER_DIRECT_LIGHTING_RGEN];
			shader_stages[0].pSpecializationInfo = &specInfo[1];
			break;
		case PIPELINE_INDIRECT_LIGHTING_FIRST:
			shader_stages[0].module = vk.shader_modules[SHADER_PATH_TRACER_INDIRECT_LIGHTING_RGEN];
			shader_stages[0].pSpecializationInfo = &specInfo[0];
			break;
		case PIPELINE_INDIRECT_LIGHTING_SECOND:
			shader_stages[0].module = vk.shader_modules[SHADER_PATH_TRACER_INDIRECT_LIGHTING_RGEN];
			shader_stages[0].pSpecializationInfo = &specInfo[1];
			break;
		default:
			assert(!"invalid pipeline index");
			break;
		}

		VkRayTracingShaderGroupCreateInfoKHR rt_shader_group_info[SBT_ENTRIES_PER_PIPELINE];
		VkRayTracingShaderGroupCreateInfoKHR *group;
		Com_Memset( &rt_shader_group_info, 0, sizeof(VkRayTracingShaderGroupCreateInfoKHR) * SBT_ENTRIES_PER_PIPELINE );

		group						= &rt_shader_group_info[SBT_RGEN];
		group->pNext				= NULL;
		group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group->generalShader		= 0;
		group->closestHitShader		= VK_SHADER_UNUSED_KHR;
		group->anyHitShader			= VK_SHADER_UNUSED_KHR;
		group->intersectionShader	= VK_SHADER_UNUSED_KHR;

		group						= &rt_shader_group_info[SBT_RMISS_EMPTY];
		group->pNext				= NULL;
		group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group->generalShader		= 1;
		group->closestHitShader		= VK_SHADER_UNUSED_KHR;
		group->anyHitShader			= VK_SHADER_UNUSED_KHR;
		group->intersectionShader	= VK_SHADER_UNUSED_KHR;

		group						= &rt_shader_group_info[SBT_RCHIT_GEOMETRY];
		group->pNext				= NULL;
		group->sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group->type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		group->generalShader		= VK_SHADER_UNUSED_KHR;
		group->closestHitShader		= 2;
		group->anyHitShader			= VK_SHADER_UNUSED_KHR;
		group->intersectionShader	= VK_SHADER_UNUSED_KHR;

		unsigned int num_shader_groups = ARRAY_LEN(rt_shader_group_info);

		VkPipelineLibraryCreateInfoKHR library_info;
		VkRayTracingPipelineCreateInfoKHR rt_pipeline_info;

		Com_Memset( &library_info, 0, sizeof(VkPipelineLibraryCreateInfoKHR) );
		library_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
		library_info.pNext = NULL;
	
		Com_Memset( &rt_pipeline_info, 0, sizeof(VkRayTracingPipelineCreateInfoKHR) );
		rt_pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rt_pipeline_info.pNext = NULL;
		rt_pipeline_info.flags = 0;
		rt_pipeline_info.stageCount = num_shader_stages;
		rt_pipeline_info.pStages = shader_stages;
		rt_pipeline_info.groupCount = num_shader_groups;
		rt_pipeline_info.pGroups = rt_shader_group_info;
		rt_pipeline_info.maxPipelineRayRecursionDepth = 1;
		rt_pipeline_info.layout = vk.rt_pipeline_layout;
		rt_pipeline_info.pLibraryInfo = &library_info,
		rt_pipeline_info.pLibraryInterface = NULL,
		rt_pipeline_info.pDynamicState = NULL,
		rt_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		rt_pipeline_info.basePipelineIndex = 0;

		VkResult res = qvkCreateRayTracingPipelinesKHR( vk.device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rt_pipeline_info, NULL, &vk.rt_pipelines[index] );
		if ( res != VK_SUCCESS )
		{
			ri.Error( ERR_DROP, "Failed to create ray tracing pipeline #%d, vkCreateRayTracingPipelinesKHR\n", index );
			return;
		}

		VK_CHECK( qvkGetRayTracingShaderGroupHandlesKHR( 
			vk.device, vk.rt_pipelines[index], 0, num_shader_groups,
				/* dataSize = */ num_shader_groups * vk.shaderGroupHandleSize,
				/* pData = */ shader_handles + SBT_ENTRIES_PER_PIPELINE * vk.shaderGroupHandleSize * index ) );
	}

	// create the SBT buffer
	uint32_t shader_binding_table_size = num_shader_groups * vk.shaderGroupBaseAlignment;
	VK_CHECK( vk_rtx_buffer_create( &vk.buf_shader_binding_table, shader_binding_table_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
		
	// copy/unpack the shader handles into the SBT:
	// shaderGroupBaseAlignment is likely greater than shaderGroupHandleSize (64 vs 32 on NV)
	char *shader_binding_table = (char*)buffer_map( &vk.buf_shader_binding_table );
	for ( uint32_t group = 0; group < num_shader_groups; group++ )
	{
		memcpy(	shader_binding_table + group * vk.shaderGroupBaseAlignment,
				shader_handles + group * vk.shaderGroupHandleSize,
				vk.shaderGroupHandleSize );
	}

	buffer_unmap( &vk.buf_shader_binding_table );
	shader_binding_table = NULL;
}