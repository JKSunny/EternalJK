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

static void vk_transition_depth_for_graphics(VkCommandBuffer cmd, VkImage image, qboolean toShader)
{
	VkImageMemoryBarrier barrier;
	Com_Memset(&barrier, 0, sizeof(barrier));

	VkImageAspectFlags aspects = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (glConfig.stencilBits > 0)
		aspects |= VK_IMAGE_ASPECT_STENCIL_BIT;

	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.aspectMask = aspects;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (toShader)
	{
		// Depth attachment -> fragment shader sampling.
		barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		qvkCmdPipelineBarrier(
			cmd,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, NULL,
			0, NULL,
			1, &barrier);
	}
	else
	{
		// Fragment shader sampling -> depth attachment.
		barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		qvkCmdPipelineBarrier(
			cmd,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			0,
			0, NULL,
			0, NULL,
			1, &barrier);
	}
}

static void vk_begin_depth_extract_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.depth.extract.framebuffer;

#ifdef USE_VK_IMGUI
    vk.renderWidth = glConfig.vidWidth;
    vk.renderHeight = glConfig.vidHeight;
#else
    vk.renderWidth = gls.captureWidth;
    vk.renderHeight = gls.captureHeight;
#endif

    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( vk.depth.extract.render_pass.handle, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}

void vk_depth_extract( uint32_t width, uint32_t height )
{
    vk_transition_depth_for_graphics( vk.cmd->command_buffer, vk.depth.image, qtrue );
    vk_begin_depth_extract_render_pass();

    qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.depth.extract.pipeline );
    qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.depth.extract.pipeline_layout, 0, 1, &vk.depth.sampler_descriptor, 0, NULL );

	{
		vkExtractDepth_t push = { 0 };

		const float *p = backEnd.viewParms.projectionMatrix;
		push.projection[0] = (p[0] != 0.0f) ? 1.0f / p[0] : 1.0f;
		push.projection[1] = (p[5] != 0.0f) ? 1.0f / p[5] : 1.0f;
		push.projection[2] = p[10];
		push.projection[3] = p[14];

		push.inv_size[0] = width > 0 ? 1.0f / width : 1.0f;
		push.inv_size[1] = height > 0 ? 1.0f / height : 1.0f;

		qvkCmdPushConstants( vk.cmd->command_buffer, vk.depth.extract.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( vkExtractDepth_t ), &push );
	}

    qvkCmdDraw( vk.cmd->command_buffer, 4, 1, 0, 0 );

    vk_end_render_pass();
    vk_transition_depth_for_graphics( vk.cmd->command_buffer, vk.depth.image,  qfalse );
}