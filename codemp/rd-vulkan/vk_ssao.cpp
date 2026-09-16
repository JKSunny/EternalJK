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

#ifdef USE_VK_SSAO

//
// SSAO
//
static void vk_begin_ssao_extract_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.ssao.extract;

    vk.renderWidth = glConfig.vidWidth;
    vk.renderHeight = glConfig.vidHeight;

    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( vk.render_pass.ssao.extract.handle, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}

static void vk_begin_ssao_blur_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.ssao.blur;

    vk.renderWidth = glConfig.vidWidth;
    vk.renderHeight = glConfig.vidHeight;

    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( vk.render_pass.ssao.blur.handle, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}

static qboolean vk_require_ssao_blur(void)
{
	//return (r_ssao->integer == 1) ? qtrue : qfalse;
	return qtrue;
}

static void vk_ssao_extract( uint32_t width, uint32_t height, uint32_t screen_width, uint32_t screen_height )
{
	vk_begin_ssao_extract_render_pass();

	// renderwidth/height migh need to be checked in inspector branch
	// and use vidwidth/height
	vk_set_viewport_scissor( width, height );

	qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.ssao.extract.pipeline );
	qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.ssao.extract.pipeline_layout, 0, 1, &vk.depth.extract.descriptor, 0, NULL );

	{
		vkExtractSSAO_t push = { 0 };

		const float *p = backEnd.viewParms.projectionMatrix;
		push.projection[0] = (p[0] != 0.0f) ? 1.0f / p[0] : 1.0f;
		push.projection[1] = (p[5] != 0.0f) ? 1.0f / p[5] : 1.0f;
		push.projection[2] = p[10];
		push.projection[3] = p[14];

#ifdef USE_VK_IMGUI
		push.texture_scale[0] = (float)width / (float)screen_width;
		push.texture_scale[1] = (float)height / (float)screen_height;
#endif

		push.radius				= r_ssao_radius->value;
		push.bias				= r_ssao_bias->value;
		push.intensity			= r_ssao_intensity->value;
		push.power				= r_ssao_power->value;

		push.samples			= int(r_ssao_samples->integer);
		push.inv_width  = width  > 0 ? 1.0f / (float)width  : 1.0f;
		push.inv_height = height > 0 ? 1.0f / (float)height : 1.0f;
		push.reverse_depth		= 0.0;

		push.depth_falloff		= r_ssao_falloff->value;
		push.mode				= r_ssao->integer;
		push.slices				= 3;
		push.steps				= 6;

		push.frame_noise		= (float)( tr.frameCount & 63 ) * 0.0625f;

		qvkCmdPushConstants( vk.cmd->command_buffer, vk.ssao.extract.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( push ), &push );
	}

	qvkCmdDraw( vk.cmd->command_buffer, 4, 1, 0, 0 );

	vk_end_render_pass();
}

static void vk_ssao_blur( uint32_t width, uint32_t height, uint32_t screen_width, uint32_t screen_height )
{
	vk_begin_ssao_blur_render_pass();
	qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.ssao.blur.pipeline );

	VkDescriptorSet sets[2] = {
		vk.ssao.extract.descriptor,
		vk.depth.extract.descriptor
	};
	qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.ssao.blur.pipeline_layout, 0, 2, sets, 0, NULL );

	{
		vkBlurSSAO_t push = { 0 };
		push.projection[0]		= (backEnd.viewParms.projectionMatrix[0] != 0.0f) ? 1.0f / backEnd.viewParms.projectionMatrix[0] : 1.0f;
		push.projection[1]		= (backEnd.viewParms.projectionMatrix[5] != 0.0f) ? 1.0f / backEnd.viewParms.projectionMatrix[5] : 1.0f;
		push.projection[2]		= backEnd.viewParms.projectionMatrix[10];
		push.projection[3]		= backEnd.viewParms.projectionMatrix[14];

#ifdef USE_VK_IMGUI
		push.texture_scale[0] = (float)width / (float)screen_width;
		push.texture_scale[1] = (float)height / (float)screen_height;
#endif

		push.depth_radius		= 2.0f;
		push.depth_sharpness	= 48.0f;
		push.inv_width  = width  > 0 ? 1.0f / (float)width  : 1.0f;
		push.inv_height = height > 0 ? 1.0f / (float)height : 1.0f;

		qvkCmdPushConstants( vk.cmd->command_buffer, vk.ssao.blur.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( push ), &push );
	}

	qvkCmdDraw( vk.cmd->command_buffer, 4, 1, 0, 0 );

	vk_end_render_pass();
}

void vk_ssao_extract_blur( void )
{
	if ( !vk.ssaoActive /*|| backEnd.doneSSAO*/ )
		return;

	uint32_t width			= (uint32_t)glConfig.vidWidth;
	uint32_t height			= (uint32_t)glConfig.vidHeight;
	uint32_t screen_width	= (uint32_t)gls.captureWidth;
	uint32_t screen_height	= (uint32_t)gls.captureHeight;

	const qboolean do_blur = vk_require_ssao_blur();

	vk_ssao_extract( width, height, screen_width, screen_height );
	
	if ( do_blur )
		vk_ssao_blur( width, height, screen_width, screen_height );
}

void vk_ssao_blend( void )
{
	if ( !vk.ssaoActive /*|| backEnd.doneSSAO*/ )
		return;

	qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.ssao.blend.pipeline );

	const qboolean do_blur = vk_require_ssao_blur();
	qvkCmdBindDescriptorSets( 
		vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_blend, 0, 1, 
		do_blur ? &vk.ssao.blur.descriptor : &vk.ssao.extract.descriptor, 
		0, NULL 
	);

	qvkCmdDraw( vk.cmd->command_buffer, 4, 1, 0, 0 );
	//backEnd.doneSSAO = qtrue;
}
#endif //USE_VK_SSAO