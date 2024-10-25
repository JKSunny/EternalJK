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

// ========================================================================= //
//
// This is the CPU-side code for the tone mapper, which is based on part of
// Eilertsen, Mantiuk, and Unger's paper *Real-time noise-aware tone mapping*,
// with some additional modifications that we found useful.
//
// This file shows how the tone mapping pipeline is created and updated, as
// well as part of how the tone mapper can be controlled by the application
// and by CVARs. (For a CVAR reference, see `global_ubo.h`, and in particular,
// those CVARS starting with `tm_`, for `tonemapper_`.)
//
// The tone mapper consists of three compute shaders, a utilities file, and
// a CPU-side code file. For an overview of the tone mapper, see
// `tone_mapping_histogram.comp`.
//
// Here are the functions in this file:
//   VkResult vkpt_tone_mapping_initialize() - creates our pipeline layouts
//
//   VkResult vkpt_tone_mapping_destroy() - destroys our pipeline layouts
//
//   void vkpt_tone_mapping_request_reset() - tells the tone mapper to calculate
// the next tone curve without blending with previous frames' tone curves
//
//   VkResult vkpt_tone_mapping_create_pipelines() - creates our pipelines
//
//   VkResult vkpt_tone_mapping_reset(VkCommandBuffer cmd_buf) - adds commands
// to the command buffer to clear the histogram image
//
//   VkResult vkpt_tone_mapping_destroy_pipelines() - destroys our pipelines
//
//   VkResult vkpt_tone_mapping_record_cmd_buffer(VkCommandBuffer cmd_buf,
// float frame_time) - records the commands to apply tone mapping to the
// VKPT_IMG_TAA_OUTPUT image in-place, given the time between this frame and
// the previous frame.
//   
// ========================================================================= //

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
		\
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

static int reset_required = 1; // If 1, recomputes tone curve based only on this frame

// Tells the tone mapper to calculate the next tone curve without blending with
// previous frames' tone curves.
void vkpt_tone_mapping_request_reset( void )
{
	reset_required = 1;
}

// Adds commands to the command buffer to clear the histogram image.
VkResult vkpt_tone_mapping_reset( VkCommandBuffer cmd_buf )
{
	VkClearColorValue clear_histogram;
	Com_Memset( &clear_histogram.uint32 , 0, sizeof(uint32_t) * 4 );

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_NONE_KHR,
		VK_ACCESS_NONE_KHR,
		vk.buffer_tonemap.buffer,
		0,
		VK_WHOLE_SIZE
	);

	qvkCmdFillBuffer( cmd_buf, vk.buffer_tonemap.buffer, 0, VK_WHOLE_SIZE, 0 );

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_NONE_KHR,
		VK_ACCESS_SHADER_WRITE_BIT,
		vk.buffer_tonemap.buffer,
		0,
		VK_WHOLE_SIZE
	);

	return VK_SUCCESS;
}

static void vk_create_tonemap_pipeline( uint32_t pipeline_index, uint32_t shader_index, 
	VkSpecializationInfo *spec, uint32_t push_size ) 
{
	vkpipeline_t *pipeline = &vk.tonemap_pipeline[pipeline_index];

	VkDescriptorSetLayout set_layouts[] = {
		vk.computeDescriptor[0].layout,
		vk.desc_set_layout_textures,
		vk.imageDescriptor.layout,
		vk.desc_set_layout_ubo
	};

	vk_rtx_bind_pipeline_shader( pipeline, vk.tonemap_shader[shader_index] );
	vk_rtx_bind_pipeline_desc_set_layouts( pipeline, set_layouts, ARRAY_LEN(set_layouts) );
	vk_rtx_create_compute_pipeline( pipeline, spec, push_size );
}

// Creates our pipeline layouts.
void vk_create_tonemap_pipelines( void )
{
	vk_load_tonemap_shaders();

	VkSpecializationMapEntry spec_entries;
	VkSpecializationInfo specInfo_SDR, specInfo_HDR;

    spec_entries.constantID = 0;
    spec_entries.offset = 0;
    spec_entries.size = sizeof(uint32_t);

	// "HDR tone mapping" flag
	uint32_t spec_data[2] = { 
		0, 
		1 
	};

	specInfo_SDR.mapEntryCount = 1;
	specInfo_SDR.pMapEntries = &spec_entries; 
	specInfo_SDR.dataSize = sizeof(uint32_t);
	specInfo_SDR.pData = &spec_data[0];

	specInfo_HDR = specInfo_SDR;
	specInfo_HDR.pData = &spec_data[1];

	vk_create_tonemap_pipeline( TONE_MAPPING_HISTOGRAM,		SHADER_TONE_MAPPING_HISTOGRAM_COMP,	NULL, 0 );
	vk_create_tonemap_pipeline( TONE_MAPPING_CURVE,			SHADER_TONE_MAPPING_CURVE_COMP,		NULL, sizeof(float) * 16 );
	vk_create_tonemap_pipeline( TONE_MAPPING_APPLY_SDR,		SHADER_TONE_MAPPING_APPLY_COMP,		&specInfo_SDR, sizeof(float) * 3 );
	vk_create_tonemap_pipeline( TONE_MAPPING_APPLY_HDR,		SHADER_TONE_MAPPING_APPLY_COMP,		&specInfo_HDR, sizeof(float) * 3 );

	reset_required = 1;
}

void vk_destroy_tonemap_pipelines( void ) 
{
	 uint32_t i;

	 for ( i = 0; i < TM_NUM_PIPELINES; i++ )
		 vk_rtx_destroy_pipeline( &vk.tonemap_pipeline[i] );
}

// Records the commands to apply tone mapping to the VKPT_IMG_TAA_OUTPUT image
// in-place, given the time between this frame and the previous frame.
VkResult vkpt_tone_mapping_record_cmd_buffer( VkCommandBuffer cmd_buf, float frame_time )
{
	uint32_t i, idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	VkDescriptorSet desc_sets[] = {
		vk.computeDescriptor[idx].set,
		vk_rtx_get_current_desc_set_textures(),
		vk.imageDescriptor.set,
		vk.desc_set_ubo
	};

	if ( reset_required ) // Clear the histogram image.
		vkpt_tone_mapping_reset( cmd_buf );

	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.tonemap_pipeline[TONE_MAPPING_HISTOGRAM].handle );

	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
		vk.tonemap_pipeline[TONE_MAPPING_HISTOGRAM].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

	qvkCmdDispatch(cmd_buf,
		(vk.extent_taa_output.width + 15) / 16,
		(vk.extent_taa_output.height + 15) / 16,
		1);

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		vk.buffer_tonemap.buffer,
		0,
		VK_WHOLE_SIZE
	);

	// Record instructions to run the compute shader that computes the tone
	// curve and autoexposure constants from the histogram.
	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.tonemap_pipeline[TONE_MAPPING_CURVE].handle );

	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
		vk.tonemap_pipeline[TONE_MAPPING_CURVE].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );

	// Compute the push constants for the tone curve generation shader:
	// whether to ignore previous tone curve results, how much to blend this
	// frame's tone curve with the previous frame's tone curve, and the kernel
	// used to filter the tone curve's slopes.
	// This is one of the things we added to Eilertsen's tone mapper, and helps
	// prevent artifacts that occur when the tone curve's slopes become flat or
	// change too suddenly (much like when you specify a curve in an image 
	// processing application that is too intense). This especially helps on
	// shadow edges in some scenes.
	// In addition, we assume the kernel is symmetric; this allows us to only
	// specify half of it in our push constant buffer.
	
	// Note that the second argument of Cvar_Get only specifies the default
	// value in code if none is set; the value of tm_slope_blur_sigma specified
	// in global_ubo.h will override this.
	float slope_blur_sigma = ri.Cvar_Get("tm_slope_blur_sigma", "6.0", 0, "")->value; // bad
	float push_constants_tm2_curve[16] = {
		 reset_required ? 1.0f : 0.0f, // 1 means reset the histogram
		 frame_time, // Frame time
		 0.0, 0.0, 0.0, 0.0, // Slope kernel filter
		 0.0, 0.0, 0.0, 0.0,
		 0.0, 0.0, 0.0, 0.0,
		 0.0, 0.0
	};

	// Compute Gaussian curve and sum, taking symmetry into account.
	float gaussian_sum = 0.0;

	for ( int j = 0; j < 14; ++j )
	{
		float kernel_value = exp(-(j * j) / (2.0 * slope_blur_sigma * slope_blur_sigma));
		gaussian_sum += kernel_value * (j == 0 ? 1 : 2);
		push_constants_tm2_curve[j + 2] = kernel_value;
	}

	// Normalize the result (since even with an analytic normalization factor,
	// the results may not sum to one).
	for ( i = 0; i < 14; ++i )
		push_constants_tm2_curve[i + 2] /= gaussian_sum;

	qvkCmdPushConstants( cmd_buf, vk.tonemap_pipeline[TONE_MAPPING_CURVE].layout,
		VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants_tm2_curve), push_constants_tm2_curve );

	qvkCmdDispatch( cmd_buf, 1, 1, 1 );

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		vk.buffer_tonemap.buffer,
		0,
		VK_WHOLE_SIZE
	);


	// apply
	// 
	// Record instructions to apply our tone curve to the final image, apply
	// the autoexposure tone mapper to the final image, and blend the results
	// of the two techniques.
	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, 
		vk.tonemap_pipeline[vk.rtx_surf_is_hdr ? TONE_MAPPING_APPLY_HDR : TONE_MAPPING_APPLY_SDR].handle );

	// can use either _SDR or _HDR, since the layout is the same
	qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
		vk.tonemap_pipeline[TONE_MAPPING_APPLY_SDR].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );


	// At the end of the hue-preserving tone mapper, the luminance of every
	// pixel is mapped to the range [0,1]. However, because this tone
	// mapper adjusts luminance while preserving hue and saturation, the values
	// of some RGB channels may lie outside [0,1]. To finish off the tone
	// mapping pipeline and since we want the brightest colors in the scene to
	// be desaturated a bit for display, we apply a subtle user-configurable
	// Reinhard tone mapping curve to the brighest values in the image at this
	// point, preserving pixels with luminance below tm_knee_start.
	//
	// If we wanted to support an arbitrary SDR color grading pipeline here or
	// implement an additional filmic tone mapping pass, for instance, this is
	// roughly around where it would be applied. For applications that need to
	// output both SDR and HDR images but for which creating custom grades
	// for each format is impractical, one common approach is to
	// (roughly) use the HDR->SDR transformation to map an SDR color grading
	// function back to an HDR color grading function.

	// Defines the white point and where we switch from an identity transform
	// to a Reinhard transform in the additional tone mapper we apply at the
	// end of the previous tone mapping pipeline.
	// Must be between 0 and 1; pixels with luminances above this value have
	// their RGB values slowly clamped to 1, up to tm_white_point.
	float knee_start = ri.Cvar_Get("tm_knee_start", "0.9", 0, "")->value;	// bad
	// Should be greater than 1; defines those RGB values that get mapped to 1.
	float knee_white_point = ri.Cvar_Get("tm_white_point", "10.0", 0, "")->value;	// bad

	// We modify Reinhard to smoothly blend with the identity transform up to tm_knee_start.
	// We need to find w, a, and b such that in y(x) = (wx+a)/(x+b),
	// * y(knee_start) = tm_knee_start
	// * dy/dx(knee_start) = 1
	// * y(knee_white_point) = tm_white_point.
	// The solution is as follows:
	float knee_w = (knee_start*(knee_start - 2.0) + knee_white_point) / (knee_white_point - 1.0);
	float knee_a = -knee_start * knee_start;
	float knee_b = knee_w - 2.0*knee_start;

	float push_constants_tm2_apply[3] = {
		knee_w, // knee_w in piecewise knee adjustment
		knee_a, // knee_a in piecewise knee adjustment
		knee_b, // knee_b in piecewise knee adjustment
	};

	// can use either _SDR or _HDR, since the layout is the same
	qvkCmdPushConstants( cmd_buf, vk.tonemap_pipeline[TONE_MAPPING_APPLY_SDR].layout,
		VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants_tm2_apply), push_constants_tm2_apply );

	qvkCmdDispatch(cmd_buf,
		(vk.extent_taa_output.width + 15) / 16,
		(vk.extent_taa_output.height + 15) / 16,
		1);

	// Because VKPT_IMG_TAA_OUTPUT changed, we make sure to wait for the image
	// to be written before continuing. This could be ensured in several
	// other ways as well.
	BARRIER_COMPUTE( cmd_buf, vk.rtx_images[RTX_IMG_TAA_OUTPUT] );

	reset_required = 0;

	return VK_SUCCESS;
}