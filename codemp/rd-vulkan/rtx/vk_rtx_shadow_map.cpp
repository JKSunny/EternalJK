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

VkPipelineLayout        pipeline_layout_smap;
VkRenderPass            render_pass_smap;
VkPipeline              pipeline_smap;
static VkFramebuffer           framebuffer_smap;
static VkFramebuffer           framebuffer_smap2;
static VkImage                 img_smap;
static VkImageView             imv_smap_depth;
static VkImageView             imv_smap_depth2;
static VkImageView             imv_smap_depth_array;
static VkDeviceMemory          mem_smap;

typedef struct
{
	mat4_t model_matrix;
	VkBuffer buffer;
	size_t vertex_offset;
	uint32_t prim_count;
} shadowmap_instance_t;

static uint32_t num_shadowmap_instances;
static shadowmap_instance_t shadowmap_instances[MAX_MODEL_INSTANCES];

void vkpt_shadow_map_reset_instances()
{
	num_shadowmap_instances = 0;
}

void vkpt_shadow_map_add_instance(const float* model_matrix, VkBuffer buffer, size_t vertex_offset, uint32_t prim_count)
{
	if (num_shadowmap_instances < MAX_MODEL_INSTANCES)
	{
		shadowmap_instance_t* instance = shadowmap_instances + num_shadowmap_instances;

		memcpy(instance->model_matrix, model_matrix, sizeof(mat4_t));
		instance->buffer = buffer;
		instance->vertex_offset = vertex_offset;
		instance->prim_count = prim_count;

		++num_shadowmap_instances;
	}
}

static void create_render_pass( void )
{
	//LOG_FUNC();
	VkAttachmentDescription depth_attachment;
	Com_Memset( &depth_attachment, 0, sizeof(VkAttachmentDescription) );
	depth_attachment.format         = VK_FORMAT_D32_SFLOAT;
	depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {
		0, /* index in fragment shader */
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR
	};

	VkSubpassDescription subpass;
	Com_Memset( &subpass, 0, sizeof(VkSubpassDescription) );
	subpass.flags					= 0;
	subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;

	VkSubpassDependency dependencies[1];
	Com_Memset( &dependencies, 0, sizeof(dependencies) );
	dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass    = 0; /* index for own subpass */
	dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0; /* XXX verify */
	dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
			        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info;
	Com_Memset( &render_pass_info, 0, sizeof(VkRenderPassCreateInfo) );
	render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.pNext           = NULL;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments    = &depth_attachment;
	render_pass_info.subpassCount    = 1;
	render_pass_info.pSubpasses      = &subpass;
	render_pass_info.dependencyCount = ARRAY_LEN(dependencies);
	render_pass_info.pDependencies   = dependencies;

	VK_CHECK( qvkCreateRenderPass( vk.device, &render_pass_info, NULL, &render_pass_smap ) );
	VK_SET_OBJECT_NAME( render_pass_smap, "render pass - smap", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);
}

VkResult vk_rtx_shadow_map_initialize( void )
{
	create_render_pass();

	VkPushConstantRange push_constant_range = {
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(float) * 16,
	};

	// create pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info;
	Com_Memset( &pipeline_layout_info, 0, sizeof(VkPipelineLayoutCreateInfo) );
	pipeline_layout_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.pNext  = 0;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;
	VK_CHECK( qvkCreatePipelineLayout( vk.device, &pipeline_layout_info, NULL, &pipeline_layout_smap) );

	VkImageCreateInfo img_info;
	Com_Memset( &img_info, 0, sizeof(VkImageCreateInfo) );
	img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img_info.pNext = NULL;
	img_info.extent = {
		SHADOWMAP_SIZE,
		SHADOWMAP_SIZE,
		1		
	};
	img_info.imageType = VK_IMAGE_TYPE_2D;
	img_info.format = VK_FORMAT_D32_SFLOAT;
	img_info.mipLevels = 1;
	img_info.arrayLayers = 2;
	img_info.samples = VK_SAMPLE_COUNT_1_BIT;
	img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	img_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
		    | VK_IMAGE_USAGE_SAMPLED_BIT,
	img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img_info.queueFamilyIndexCount = vk.queue_idx_graphics;
	img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CHECK( qvkCreateImage( vk.device, &img_info, NULL, &img_smap ) );

	VkMemoryRequirements mem_req;
	qvkGetImageMemoryRequirements( vk.device, img_smap, &mem_req );

	VK_CHECK( allocate_gpu_memory( mem_req, &mem_smap ) );

	VK_CHECK( qvkBindImageMemory( vk.device, img_smap, mem_smap, 0 ) );

	VkImageViewCreateInfo img_view_info;
	Com_Memset( &img_view_info, 0, sizeof(VkImageViewCreateInfo) );
	img_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	img_view_info.pNext = NULL;
	img_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	img_view_info.format = VK_FORMAT_D32_SFLOAT;
	img_view_info.image = img_smap;
	img_view_info.subresourceRange = {
		VK_IMAGE_ASPECT_DEPTH_BIT,
		0,
		1,
		0,
		1,
	};
	img_view_info.components = {
		VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A,
	};

	VK_CHECK( qvkCreateImageView( vk.device, &img_view_info, NULL, &imv_smap_depth ) );

	img_view_info.subresourceRange.baseArrayLayer = 1;
	VK_CHECK( qvkCreateImageView( vk.device, &img_view_info, NULL, &imv_smap_depth2 ) );

	img_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	img_view_info.subresourceRange.baseArrayLayer = 0;
	img_view_info.subresourceRange.layerCount = 2;
	VK_CHECK( qvkCreateImageView( vk.device, &img_view_info, NULL, &imv_smap_depth_array ) );

	VkCommandBuffer cmd_buf = vkpt_begin_command_buffer( &vk.cmd_buffers_graphics );

	VkImageSubresourceRange range;
	Com_Memset( &range, 0, sizeof(VkImageSubresourceRange) ); 
	range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	range.levelCount = 1;
	range.layerCount = 2;

	IMAGE_BARRIER( cmd_buf,
		img_smap,
		range,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	vkpt_submit_command_buffer_simple( cmd_buf, vk.queue_graphics, true );

	return VK_SUCCESS;
}

VkResult vk_rtx_shadow_map_destroy( void )
{
	qvkDestroyImageView( vk.device, imv_smap_depth, NULL );
	imv_smap_depth = VK_NULL_HANDLE;

	qvkDestroyImageView( vk.device, imv_smap_depth2, NULL );
	imv_smap_depth2 = VK_NULL_HANDLE;

	qvkDestroyImageView( vk.device, imv_smap_depth_array, NULL );
	imv_smap_depth_array = VK_NULL_HANDLE;

	qvkDestroyImage( vk.device, img_smap, NULL );
	img_smap = VK_NULL_HANDLE;

	qvkFreeMemory( vk.device, mem_smap, NULL );
	mem_smap = VK_NULL_HANDLE;

	qvkDestroyRenderPass( vk.device, render_pass_smap, NULL );
	render_pass_smap = VK_NULL_HANDLE;

	qvkDestroyPipelineLayout( vk.device, pipeline_layout_smap, NULL );
	pipeline_layout_smap = VK_NULL_HANDLE;

	return VK_SUCCESS;
}

VkImageView vk_rtx_shadow_map_get_view( void )
{
	return imv_smap_depth_array;

}

VkResult vk_rtx_shadow_map_create_pipelines( void ) 
{
	VkPipelineShaderStageCreateInfo shader_info[1];
	Com_Memset( &shader_info, 0, sizeof(shader_info) );
	shader_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_info[0].pNext = NULL;
	shader_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_info[0].module = vk.asvgf_shader[SHADER_SHADOW_MAP_VERT]->modules[0];
	shader_info[0].pName = "main";

	VkVertexInputBindingDescription vertex_binding_desc;
	vertex_binding_desc.binding = 0;
	vertex_binding_desc.stride = sizeof(float) * 3;
	vertex_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertex_attribute_desc;
	vertex_attribute_desc.location = 0;
	vertex_attribute_desc.binding = 0;
	vertex_attribute_desc.format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_attribute_desc.offset = 0;

	VkPipelineVertexInputStateCreateInfo vertex_input_info;
	Com_Memset( &vertex_input_info, 0, sizeof(VkPipelineVertexInputStateCreateInfo) );
	vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.pNext                           = NULL;
	vertex_input_info.vertexBindingDescriptionCount   = 1;
	vertex_input_info.pVertexBindingDescriptions      = &vertex_binding_desc;
	vertex_input_info.vertexAttributeDescriptionCount = 1;
	vertex_input_info.pVertexAttributeDescriptions    = &vertex_attribute_desc;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
	Com_Memset( &input_assembly_info, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo) );
	input_assembly_info.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.pNext    = NULL;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	
	VkViewport viewport;
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = (float)SHADOWMAP_SIZE;
	viewport.height   = (float)SHADOWMAP_SIZE;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = { SHADOWMAP_SIZE, SHADOWMAP_SIZE };

	VkPipelineViewportStateCreateInfo viewport_state;
	viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext         = NULL;
	viewport_state.flags         = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports    = &viewport;
	viewport_state.scissorCount  = 1;
	viewport_state.pScissors     = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer_state;
	Com_Memset( &rasterizer_state, 0, sizeof(VkPipelineRasterizationStateCreateInfo) );
	rasterizer_state.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_state.pNext                   = NULL;
	rasterizer_state.polygonMode             = VK_POLYGON_MODE_FILL;
	rasterizer_state.lineWidth               = 1.0f;
	rasterizer_state.cullMode                = VK_CULL_MODE_BACK_BIT;	// VK_CULL_MODE_FRONT_BIT (q2rtx reversed)
	rasterizer_state.frontFace               = VK_FRONT_FACE_CLOCKWISE;

	VkPipelineMultisampleStateCreateInfo multisample_state;
	Com_Memset( &multisample_state, 0, sizeof(VkPipelineMultisampleStateCreateInfo) );
	multisample_state.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.pNext                 = NULL;
	multisample_state.sampleShadingEnable   = VK_FALSE;
	multisample_state.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisample_state.minSampleShading      = 1.0f;
	multisample_state.pSampleMask           = NULL;
	multisample_state.alphaToCoverageEnable = VK_FALSE;
	multisample_state.alphaToOneEnable      = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
	Com_Memset( &depth_stencil_state, 0, sizeof(VkPipelineDepthStencilStateCreateInfo) );
	depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state.pNext = NULL;
	depth_stencil_state.depthTestEnable = VK_TRUE;
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;

	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_info;
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.pNext = NULL;
	dynamic_state_info.flags = 0;
	dynamic_state_info.dynamicStateCount = ARRAY_LEN( dynamic_states );
	dynamic_state_info.pDynamicStates = dynamic_states;

	VkGraphicsPipelineCreateInfo pipeline_info;
	Com_Memset( &pipeline_info, 0, sizeof(VkGraphicsPipelineCreateInfo) );
	pipeline_info.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.pNext      = NULL;
	pipeline_info.stageCount = ARRAY_LEN( shader_info );
	pipeline_info.pStages    = shader_info;

	pipeline_info.pVertexInputState   = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pViewportState      = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer_state;
	pipeline_info.pMultisampleState   = &multisample_state;
	pipeline_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_info.pColorBlendState    = NULL;
	pipeline_info.pDynamicState       = &dynamic_state_info;
		
	pipeline_info.layout              = pipeline_layout_smap;
	pipeline_info.renderPass          = render_pass_smap;
	pipeline_info.subpass             = 0;

	pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex   = -1;

	VK_CHECK( qvkCreateGraphicsPipelines( vk.device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline_smap ) );

	VkImageView attachments[] = {
		imv_smap_depth
	};

	VkFramebufferCreateInfo fb_create_info;
	Com_Memset( &fb_create_info, 0, sizeof(VkFramebufferCreateInfo) );
	fb_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_create_info.pNext           = NULL;
	fb_create_info.renderPass      = render_pass_smap;
	fb_create_info.attachmentCount = 1;
	fb_create_info.pAttachments    = attachments;
	fb_create_info.width           = SHADOWMAP_SIZE;
	fb_create_info.height          = SHADOWMAP_SIZE;
	fb_create_info.layers          = 1;

	VK_CHECK( qvkCreateFramebuffer( vk.device, &fb_create_info, NULL, &framebuffer_smap ) );

	attachments[0] = imv_smap_depth2;

	VK_CHECK( qvkCreateFramebuffer( vk.device, &fb_create_info, NULL, &framebuffer_smap2 ) );

	return VK_SUCCESS;
}

VkResult vk_rtx_shadow_map_destroy_pipelines( void )
{
	qvkDestroyFramebuffer( vk.device, framebuffer_smap, NULL );
	framebuffer_smap = VK_NULL_HANDLE;

	qvkDestroyFramebuffer( vk.device, framebuffer_smap2, NULL );
	framebuffer_smap2 = VK_NULL_HANDLE;

	qvkDestroyPipeline( vk.device, pipeline_smap, NULL );
	pipeline_smap = VK_NULL_HANDLE;

	return VK_SUCCESS;
}

VkResult vk_rtx_shadow_map_render( VkCommandBuffer cmd_buf, world_t &worldData, float *view_projection_matrix, 
	uint32_t static_offset, uint32_t num_static_verts, 
	uint32_t dynamic_offset, uint32_t num_dynamic_verts,
	uint32_t transparent_offset, uint32_t num_transparent_verts )
{
	VkImageSubresourceRange range;
	Com_Memset( &range, 0, sizeof(VkImageSubresourceRange) ); 
	range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	range.levelCount = 1;
	range.layerCount = 2;

	IMAGE_BARRIER( cmd_buf,
		img_smap,
		range,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	);

	VkClearValue clear_depth;
	Com_Memset( &clear_depth, 0, sizeof(VkClearValue) );
	clear_depth.depthStencil.depth = 1.f;

	VkRenderPassBeginInfo render_pass_info;
	render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.pNext             = NULL;
	render_pass_info.renderPass        = render_pass_smap;
	render_pass_info.framebuffer       = framebuffer_smap;
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent = { SHADOWMAP_SIZE, SHADOWMAP_SIZE };
	render_pass_info.clearValueCount   = 1;
	render_pass_info.pClearValues      = &clear_depth;

	qvkCmdBeginRenderPass( cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE );
	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_smap );

	VkViewport viewport;
	Com_Memset( &viewport, 0, sizeof(VkViewport) );
	viewport.width = (float)SHADOWMAP_SIZE;
	viewport.height = (float)SHADOWMAP_SIZE;
	viewport.minDepth = 0;
	viewport.maxDepth = 1.0f;

	qvkCmdSetViewport( cmd_buf, 0, 1, &viewport );

	VkRect2D scissor;
	Com_Memset( &scissor, 0, sizeof(VkRect2D) );
	scissor.extent.width = SHADOWMAP_SIZE;
	scissor.extent.height = SHADOWMAP_SIZE;
	scissor.offset.x = 0;
	scissor.offset.y = 0;

	qvkCmdSetScissor( cmd_buf, 0, 1, &scissor);

	qvkCmdPushConstants( cmd_buf, pipeline_layout_smap, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4_t), view_projection_matrix );

	VkDeviceSize vertex_offset = tr.world->geometry.world_static.vertex_data_offset; // render using prim_positions_t offset	
	qvkCmdBindVertexBuffers( cmd_buf, 0, 1, &tr.world->geometry.world_static.buffer[0].buffer, &vertex_offset );
	qvkCmdDraw( cmd_buf, num_static_verts, 1, static_offset, 0 );

	vertex_offset = 0;
	qvkCmdBindVertexBuffers(cmd_buf, 0, 1, &vk.buf_positions_instanced.buffer, &vertex_offset);
	qvkCmdDraw(cmd_buf, num_dynamic_verts, 1, dynamic_offset, 0);

	mat4_t mvp_matrix;

	for (uint32_t instance_idx = 0; instance_idx < num_shadowmap_instances; instance_idx++)
	{
		const shadowmap_instance_t* mi = shadowmap_instances + instance_idx;

		mult_matrix_matrix(mvp_matrix, view_projection_matrix, mi->model_matrix);

		qvkCmdPushConstants(cmd_buf, pipeline_layout_smap, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4_t), mvp_matrix);

		qvkCmdBindVertexBuffers(cmd_buf, 0, 1, &mi->buffer, &mi->vertex_offset);

		qvkCmdDraw(cmd_buf, mi->prim_count * 3, 1, 0, 0);
	}

	qvkCmdEndRenderPass( cmd_buf );

	render_pass_info.framebuffer = framebuffer_smap2;
	qvkCmdBeginRenderPass( cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE );

	qvkCmdPushConstants(cmd_buf, pipeline_layout_smap, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4_t), view_projection_matrix);

	vertex_offset = tr.world->geometry.world_static.vertex_data_offset;
	qvkCmdBindVertexBuffers(cmd_buf, 0, 1, &tr.world->geometry.world_static.buffer[0].buffer, &vertex_offset);

	qvkCmdDraw(cmd_buf, num_transparent_verts, 1, transparent_offset, 0);

	qvkCmdEndRenderPass( cmd_buf );

	IMAGE_BARRIER( cmd_buf,
		img_smap,
		range,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	return VK_SUCCESS;
}

static void sample_disk( float *u, float *v )
{
	float a = frand();
	float b = frand();

	float theta = 2.0 * M_PI * a;
	float r = sqrt(b);

	*u = r * cos(theta);
	*v = r * sin(theta);
}

void vk_rtx_shadow_map_setup( const sun_light_t *light, const float *bbox_min, const float *bbox_max, float *VP, float *depth_scale, qboolean random_sampling )
{
	vec3_t up_dir = { 0.0f, 0.0f, 1.0f };
	if (light->direction[2] >= 0.99f)
		VectorSet(up_dir, 1.f, 0.f, 0.f);

	vec3_t look_dir;
	VectorScale(light->direction, -1.f, look_dir);
	VectorNormalize(look_dir);
	vec3_t left_dir;
	CrossProduct(up_dir, look_dir, left_dir);
	VectorNormalize(left_dir);
	CrossProduct(look_dir, left_dir, up_dir);
	VectorNormalize(up_dir);

	if (random_sampling)
	{
		float u, v;
		sample_disk(&u, &v);
		
		VectorMA(look_dir, u * light->angular_size_rad * 0.5f, left_dir, look_dir);
		VectorMA(look_dir, v * light->angular_size_rad * 0.5f, up_dir, look_dir);

		VectorNormalize(look_dir);

		CrossProduct(up_dir, look_dir, left_dir);
		VectorNormalize(left_dir);
		CrossProduct(look_dir, left_dir, up_dir);
		VectorNormalize(up_dir);
	}

	float view_matrix[16] = {
		left_dir[0], up_dir[0], look_dir[0], 0.f,
		left_dir[1], up_dir[1], look_dir[1], 0.f,
		left_dir[2], up_dir[2], look_dir[2], 0.f,
		0.f, 0.f, 0.f, 1.f
	};

	vec3_t view_aabb_min = { FLT_MAX, FLT_MAX, FLT_MAX };
	vec3_t view_aabb_max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (int i = 0; i < 8; i++)
	{
		float corner[4];
#if 1
		corner[0] = (i & 1) ? bbox_max[0] : bbox_min[0];
		corner[1] = (i & 2) ? bbox_max[1] : bbox_min[1];
		corner[2] = (i & 4) ? bbox_max[2] : bbox_min[2];
#else
		corner[0] = tr.viewParms.visBounds[(i >> 0) & 1][0];
		corner[1] = tr.viewParms.visBounds[(i >> 1) & 1][1];
		corner[2] = tr.viewParms.visBounds[(i >> 2) & 1][2];
#endif
		corner[3] = 1.f;

		float view_corner[4];
		mult_matrix_vector(view_corner, view_matrix, corner);

		view_aabb_min[0] = MIN(view_aabb_min[0], view_corner[0]);
		view_aabb_min[1] = MIN(view_aabb_min[1], view_corner[1]);
		view_aabb_min[2] = MIN(view_aabb_min[2], view_corner[2]);
		view_aabb_max[0] = MAX(view_aabb_max[0], view_corner[0]);
		view_aabb_max[1] = MAX(view_aabb_max[1], view_corner[1]);
		view_aabb_max[2] = MAX(view_aabb_max[2], view_corner[2]);
	}

	vec3_t diagonal;
	VectorSubtract(view_aabb_max, view_aabb_min, diagonal);

	float maxXY = MAX(diagonal[0], diagonal[1]);
	vec3_t diff;
	diff[0] = (maxXY - diagonal[0]) * 0.5f;
	diff[1] = (maxXY - diagonal[1]) * 0.5f;
	diff[2] = 0.f;
	VectorSubtract(view_aabb_min, diff, view_aabb_min);
	VectorAdd(view_aabb_max, diff, view_aabb_max);

	float projection_matrix[16];
	create_orthographic_matrix(projection_matrix, view_aabb_min[0], view_aabb_max[0], view_aabb_min[1], view_aabb_max[1], view_aabb_min[2], view_aabb_max[2]);
	mult_matrix_matrix(VP, projection_matrix, view_matrix);

	*depth_scale = view_aabb_max[2] - view_aabb_min[2];
}