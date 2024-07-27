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
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; \
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL; \
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL; \
		qvkCmdPipelineBarrier( cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, \
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, \
				1, &barrier); \
	} while(0)

static void vk_create_asvgf_pipeline( uint32_t pipeline_index, uint32_t shader_index, 
	VkSpecializationInfo *spec, uint32_t push_size ) 
{
	vkpipeline_t *pipeline = &vk.asvgf_pipeline[pipeline_index];

	VkDescriptorSetLayout set_layouts[3] = {
		vk.computeDescriptor[0].layout,
		vk.imageDescriptor.layout,
		vk.desc_set_layout_ubo
	};

	vk_rtx_bind_pipeline_shader( pipeline, vk.asvgf_shader[shader_index] );
	vk_rtx_bind_pipeline_desc_set_layouts( pipeline, set_layouts, ARRAY_LEN(set_layouts) );
	vk_rtx_create_compute_pipeline( pipeline, spec, push_size );
}

void vk_create_asvgf_pipelines( void )
{	
	vk_load_asvgf_shaders();

	uint32_t i;
	VkSpecializationMapEntry spec_entries;
	VkSpecializationInfo spec_info[4];
	uint32_t spec_data[4] = { 0, 1, 2, 3 };

    spec_entries.constantID = 0;
    spec_entries.offset = 0;
    spec_entries.size = sizeof(uint32_t);

	for ( i = 0; i < 4; i++ ) 
	{
		spec_info[i].mapEntryCount = 1; 
		spec_info[i].pMapEntries = &spec_entries; 
		spec_info[i].dataSize = sizeof(uint32_t); 
		spec_info[i].pData = &spec_data[i];
	}

	vk_create_asvgf_pipeline( GRADIENT_IMAGE,		SHADER_ASVGF_GRADIENT_IMG_COMP,			NULL,			0 );
	vk_create_asvgf_pipeline( GRADIENT_ATROUS,		SHADER_ASVGF_GRADIENT_ATROUS_COMP,		NULL,			sizeof(uint32_t) );
	vk_create_asvgf_pipeline( GRADIENT_REPROJECT,	SHADER_ASVGF_GRADIENT_REPROJECT_COMP,	NULL,			0 );
	vk_create_asvgf_pipeline( TEMPORAL,				SHADER_ASVGF_TEMPORAL_COMP,				NULL,			0 );
	vk_create_asvgf_pipeline( ATROUS_LF,			SHADER_ASVGF_LF_COMP,					NULL,			sizeof(uint32_t) );
	vk_create_asvgf_pipeline( ATROUS_ITER_0,		SHADER_ASVGF_ATROUS_COMP,				&spec_info[0],	0 );
	vk_create_asvgf_pipeline( ATROUS_ITER_1,		SHADER_ASVGF_ATROUS_COMP,				&spec_info[1],	0 );
	vk_create_asvgf_pipeline( ATROUS_ITER_2,		SHADER_ASVGF_ATROUS_COMP,				&spec_info[2],	0 );
	vk_create_asvgf_pipeline( ATROUS_ITER_3,		SHADER_ASVGF_ATROUS_COMP,				&spec_info[3],	0 );
	vk_create_asvgf_pipeline( CHECKERBOARD_INTERLEAVE,	SHADER_CHECKERBOARD_INTERLEAVE_COMP, NULL,			0 );
	vk_create_asvgf_pipeline( TAAU,					SHADER_ASVGF_TAAU_COMP,					NULL,			0 );
	vk_create_asvgf_pipeline( COMPOSITING,			SHADER_COMPOSITING_COMP,				NULL,			0 );
}

void vk_destroy_asvgf_pipelines( void ) 
{
	 uint32_t i;

	 for ( i = 0; i < ASVGF_NUM_PIPELINES; i++ )
		 vk_rtx_destroy_pipeline( &vk.asvgf_pipeline[i] );
}

VkResult vkpt_asvgf_gradient_reproject( VkCommandBuffer cmd_buf )
{
	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	VkDescriptorSet desc_sets[] = {
		vk.computeDescriptor[idx].set,
		vk.imageDescriptor.set,
		vk.desc_set_ubo
	};

	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.asvgf_pipeline[GRADIENT_REPROJECT].handle );

	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
		vk.asvgf_pipeline[GRADIENT_REPROJECT].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

	uint32_t group_size_pixels = 24; // matches GROUP_SIZE_PIXELS in asvgf_gradient_reproject.comp
	qvkCmdDispatch( cmd_buf,
		(vk.gpu_slice_width + group_size_pixels - 1) / group_size_pixels,
		(glConfig.vidHeight + group_size_pixels - 1) / group_size_pixels,
		1 );

	const int offset = ( idx * RTX_IMG_NUM_A_B );

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_RNG_SEED_A + offset] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_GRAD_SMPL_POS_A + offset] ); 

	return VK_SUCCESS;
}

VkResult vkpt_asvgf_filter( VkCommandBuffer cmd_buf, qboolean enable_lf )
{
	uint32_t i, idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	VkDescriptorSet desc_sets[] = {
		vk.computeDescriptor[idx].set,
		vk.imageDescriptor.set,
		vk.desc_set_ubo
	};

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_LF_SH] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_LF_COCG] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_HF] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_SPEC] );

	BEGIN_PERF_MARKER( cmd_buf, PROFILER_ASVGF_RECONSTRUCT_GRADIENT );

	/* create gradient image */
	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.asvgf_pipeline[GRADIENT_IMAGE].handle );

	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
		vk.asvgf_pipeline[GRADIENT_IMAGE].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

	qvkCmdDispatch( cmd_buf,
			(vk.gpu_slice_width / GRAD_DWN + 15) / 16,
			(vk.extent_render.height / GRAD_DWN + 15) / 16,
			1 );

	// XXX BARRIERS!!!
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_GRAD_LF_PING] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_GRAD_HF_SPEC_PING] );

	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.asvgf_pipeline[GRADIENT_ATROUS].handle );

	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
		vk.asvgf_pipeline[GRADIENT_ATROUS].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

	/* reconstruct gradient image */
	const int num_atrous_iterations_gradient = 7;
	for ( i = 0; i < num_atrous_iterations_gradient; i++ ) 
	{
		uint32_t push_constants[1] = {
			i
		};

		qvkCmdPushConstants( cmd_buf, vk.asvgf_pipeline[GRADIENT_ATROUS].layout,
		VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants), push_constants );

		qvkCmdDispatch( cmd_buf,
				(vk.gpu_slice_width / GRAD_DWN + 15) / 16,
				(vk.extent_render.height / GRAD_DWN + 15) / 16,
				1 );

		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_GRAD_LF_PING + !(i & 1)] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_GRAD_HF_SPEC_PING + !(i & 1)] );
	}

	END_PERF_MARKER( cmd_buf, PROFILER_ASVGF_RECONSTRUCT_GRADIENT );
	BEGIN_PERF_MARKER( cmd_buf, PROFILER_ASVGF_TEMPORAL );

	/* temporal accumulation / filtering */
	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.asvgf_pipeline[TEMPORAL].handle );
	
	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
		vk.asvgf_pipeline[TEMPORAL].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

	qvkCmdDispatch( cmd_buf,
			(vk.gpu_slice_width + 14) / 15,
			(vk.extent_render.height + 14) / 15,
			1 );

	const int offset = ( idx * RTX_IMG_NUM_A_B );

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_LF_SH] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_LF_COCG] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_HF] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_MOMENTS] );

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_FILTERED_SPEC_A + offset] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_HIST_MOMENTS_HF_A] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_HIST_MOMENTS_HF_B] ); // B aka prev

	END_PERF_MARKER( cmd_buf, PROFILER_ASVGF_TEMPORAL );
	BEGIN_PERF_MARKER( cmd_buf, PROFILER_ASVGF_ATROUS );

	/* spatial reconstruction filtering */
	const int num_atrous_iterations = 4;
	for ( i = 0; i < num_atrous_iterations; i++ ) 
	{
		if ( enable_lf )
		{
			uint32_t push_constants[1] = {
				i
			};

			qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.asvgf_pipeline[ATROUS_LF].handle );
			
			qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
				vk.asvgf_pipeline[ATROUS_LF].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

			qvkCmdPushConstants( cmd_buf, vk.asvgf_pipeline[ATROUS_LF].layout, 
				VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants), push_constants );

			qvkCmdDispatch( cmd_buf,
				(vk.gpu_slice_width / GRAD_DWN + 15) / 16,
				(vk.extent_render.height / GRAD_DWN + 15) / 16,
				1 );

			if ( i == num_atrous_iterations - 1 )
			{
				BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_LF_SH] );
				BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_LF_COCG] );
			}
		}

		int specialization = ATROUS_ITER_0 + i;

		qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.asvgf_pipeline[specialization].handle );

		qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
				vk.asvgf_pipeline[specialization].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

		qvkCmdDispatch( cmd_buf,
				(vk.gpu_slice_width + 15) / 16,
				(vk.extent_render.height + 15) / 16,
				1 );

		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_LF_SH] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_LF_COCG] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_HF] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_MOMENTS] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PONG_LF_SH] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PONG_LF_COCG] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PONG_HF] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PONG_MOMENTS] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_HIST_COLOR_HF] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_LF_SH + !(i & 1)] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_ATROUS_PING_LF_COCG + !(i & 1)] );
		BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_COLOR] );
	}

	END_PERF_MARKER( cmd_buf, PROFILER_ASVGF_ATROUS );

	return VK_SUCCESS;
}

VkResult vkpt_compositing( VkCommandBuffer cmd_buf )
{
	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	VkDescriptorSet desc_sets[] = {
		vk.computeDescriptor[idx].set,
		vk.imageDescriptor.set,
		vk.desc_set_ubo
	};

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_LF_SH] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_LF_COCG] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_HF] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_PT_COLOR_SPEC] );

	BEGIN_PERF_MARKER( cmd_buf, PROFILER_COMPOSITING );

	/* create gradient image */
	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.asvgf_pipeline[COMPOSITING].handle );

	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
			vk.asvgf_pipeline[COMPOSITING].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

	qvkCmdDispatch( cmd_buf,
		(vk.gpu_slice_width + 15) / 16,
		(vk.extent_render.height + 15) / 16,
		1 );

	END_PERF_MARKER( cmd_buf, PROFILER_COMPOSITING );

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_COLOR] );

	return VK_SUCCESS;
}

VkResult vkpt_interleave( VkCommandBuffer cmd_buf )
{
	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	VkDescriptorSet desc_sets[] = {
		vk.computeDescriptor[idx].set,
		vk.imageDescriptor.set,
		vk.desc_set_ubo
	};

#ifdef VKPT_DEVICE_GROUPS
	if (qvk.device_count > 1) {
		BEGIN_PERF_MARKER(cmd_buf, PROFILER_MGPU_TRANSFERS);

		// create full interleaved motion and color buffers on GPU 0
		VkOffset2D offset_left = { 0, 0 };
		VkOffset2D offset_right = { qvk.extent_render.width / 2, 0 };
		VkExtent2D extent = { qvk.extent_render.width / 2, qvk.extent_render.height };

		vkpt_mgpu_image_copy(cmd_buf,
							VKPT_IMG_PT_MOTION,
							VKPT_IMG_PT_MOTION,
							1,
							0,
							offset_left,
							offset_right,
							extent);
		
		vkpt_mgpu_image_copy(cmd_buf,
							VKPT_IMG_ASVGF_COLOR,
							VKPT_IMG_ASVGF_COLOR,
							1,
							0,
							offset_left,
							offset_right,
							extent);

		vkpt_mgpu_global_barrier(cmd_buf);
		
		END_PERF_MARKER(cmd_buf, PROFILER_MGPU_TRANSFERS);
	}
#endif
	
	BEGIN_PERF_MARKER( cmd_buf, PROFILER_INTERLEAVE );

	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.asvgf_pipeline[CHECKERBOARD_INTERLEAVE].handle );

	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
			vk.asvgf_pipeline[CHECKERBOARD_INTERLEAVE].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

	//set_current_gpu(cmd_buf, 0);

	// dispatch using the image dimensions, not render dimensions - to clear the unused area with black color
	qvkCmdDispatch( cmd_buf,
		(vk.extent_screen_images.width + 15) / 16,
		(vk.extent_screen_images.height + 15) / 16,
		1 );

	END_PERF_MARKER( cmd_buf, PROFILER_INTERLEAVE );

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_FLAT_COLOR] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_FLAT_MOTION] );

	return VK_SUCCESS;
}

VkResult vkpt_taa( VkCommandBuffer cmd_buf )
{
	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	VkDescriptorSet desc_sets[] = {
		vk.computeDescriptor[idx].set,
		vk.imageDescriptor.set,
		vk.desc_set_ubo
	};

	BEGIN_PERF_MARKER( cmd_buf, PROFILER_ASVGF_TAA );

	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.asvgf_pipeline[TAAU].handle );
	
	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
			vk.asvgf_pipeline[TAAU].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );


	VkExtent2D dispatch_size = vk.extent_taa_output;

	if ( dispatch_size.width < vk.extent_taa_images.width )
		dispatch_size.width += 8;

	if ( dispatch_size.height < vk.extent_taa_images.height )
		dispatch_size.height += 8;

	qvkCmdDispatch( cmd_buf,
			(dispatch_size.width + 15) / 16,
			(dispatch_size.height + 15) / 16,
			1 );

	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_TAA_A] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_TAA_B] );
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_ASVGF_TAA_B] );

	END_PERF_MARKER( cmd_buf, PROFILER_ASVGF_TAA );

	return VK_SUCCESS;
}