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

#ifdef VK_COMPUTE_NORMALMAP

/*
`This file implements runtime normal map generation using compute shaders.

Workflow:
1. Albedo textures without an associated normal map are added to a batching queue
   using `vk_add_compute_normalmap()`. This function creates a target storage image
   for the normal map, allocates a descriptor set, and binds the albedo and target
   images to it.

2. In `vk_begin_frame()` the queued items from the previous frame are processed 
   by `vk_dispatch_compute_normalmaps()`:
   - Transitions image layouts as needed.
   - Binds the compute pipeline and descriptor sets.
   - Dispatches compute shader workgroups to generate normal maps.

3. when required, `vk_compute_normalmap_mips()` will generate mipmaps using Vulkan image blits.

The compute pipeline and descriptor set layout are initialized once during startup
via `vk_create_compute_normalmap_pipelines()`.

This system allows automatic normal map generation from albedo textures during runtime
without needing manually authored normal maps.

Enabled via: `VK_COMPUTE_NORMALMAP`
*/

void vk_create_compute_normalmap_pipelines()
{
    if ( !r_genNormalMaps->integer )
        return;

    // descriptor layout
    VkDescriptorSetLayoutBinding bind[2];

    bind[0].binding = 0;
    bind[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bind[0].descriptorCount = 1;
    bind[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bind[0].pImmutableSamplers = NULL;

    bind[1].binding = 1;
    bind[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bind[1].descriptorCount = 1;
    bind[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bind[1].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo desc;
    Com_Memset( &desc, 0, sizeof(VkDescriptorSetLayoutCreateInfo) );
    desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    desc.pNext  = NULL,
    desc.bindingCount = 2,
    desc.pBindings = bind;

    qvkCreateDescriptorSetLayout( vk.device, &desc, NULL, &vk.set_layout_compute_normalmap );

    // pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout;
    Com_Memset(&pipeline_layout, 0, sizeof(pipeline_layout));
    pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout.setLayoutCount = 1;
    pipeline_layout.pSetLayouts = &vk.set_layout_compute_normalmap;
    pipeline_layout.pushConstantRangeCount = 0;
    pipeline_layout.pPushConstantRanges = NULL;

    VkResult res = qvkCreatePipelineLayout( vk.device, &pipeline_layout,
        NULL,  &vk.pipeline_layout_compute_normalmap );

    if ( res != VK_SUCCESS )
        Com_Error(ERR_FATAL, "Failed to create pipeline layout for normal map gen");

    // pipeline
    VkPipelineShaderStageCreateInfo stage;
    Com_Memset( &stage, 0, sizeof(VkPipelineShaderStageCreateInfo) );
    stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.pNext  = NULL;
    stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = vk.shaders.normalmap;
    stage.pName  = "main";
    stage.pSpecializationInfo = NULL;

    VkComputePipelineCreateInfo info;
    Com_Memset( &info, 0, sizeof(VkComputePipelineCreateInfo) );
    info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.pNext  = NULL;
    info.stage  = stage;
    info.layout = vk.pipeline_layout_compute_normalmap;

    qvkCreateComputePipelines( vk.device, VK_NULL_HANDLE, 1, &info, NULL, &vk.compute_normalmap_pipeline );
}

void vk_add_compute_normalmap( shaderStage_t *stage, image_t *albedo, imgFlags_t flags ) 
{
    if ( !r_genNormalMaps->integer )
        return;


	if ( stage->normalMap || !albedo )
		return;

	if ( !(albedo->flags & IMGFLAG_PICMIP) || !(albedo->flags & IMGFLAG_MIPMAP) )
		return;

	if ( albedo->width < 64 && albedo->height < 64  )
		return;

	if ( tr.compute_normalmaps_batch_num == MAX_BATCH_COMPUTE_NORMALMAPS ) {
		ri.Error( ERR_DROP, "%s: MAX_BATCH_COMPUTE_NORMALMAPS hit", __func__ );
	}

	Com_Printf("%s \n", albedo->imgName );

	VkWriteDescriptorSet writes[2] = {};
	Vk_Sampler_Def sampler_def;
	VkDescriptorImageInfo image_info, normal_info;
	char imageName[MAX_QPATH];

	Com_Memset( &writes, 0, sizeof(VkWriteDescriptorSet) * 2 );
	Com_Memset( &sampler_def, 0, sizeof(sampler_def) );

	sampler_def.address_mode = albedo->wrapClampMode;

	sampler_def.gl_mag_filter = GL_LINEAR;
	sampler_def.gl_min_filter = GL_LINEAR;
	// no anisotropy without mipmaps
	sampler_def.noAnisotropy = qtrue;

	image_info.sampler = vk_find_sampler(&sampler_def);
	image_info.imageView = albedo->view;
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	COM_StripExtension( albedo->imgName, imageName, MAX_QPATH );
	Q_strcat( imageName, MAX_QPATH, "_gen_n" );
				
	
	image_t *normal = R_CreateImage( imageName, NULL, 
		albedo->width, albedo->height, flags | IMGFLAG_STORAGE, 0, 0 );

	normal_info.sampler = VK_NULL_HANDLE;
	normal_info.imageView = normal->view;
	normal_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	// descriptor set
	VkDescriptorSetAllocateInfo alloc;
	alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc.pNext = NULL;
	alloc.descriptorPool = vk.descriptor_pool;
	alloc.descriptorSetCount = 1;
	alloc.pSetLayouts = &vk.set_layout_compute_normalmap;
	//VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.storage.descriptor ) );

	VkDescriptorSet descriptor_set;
	VkResult res = qvkAllocateDescriptorSets( vk.device, &alloc, &descriptor_set );
	if ( res != VK_SUCCESS ) {
		Com_Error( ERR_FATAL, "Failed to allocate descriptor set for normal map generation" );
	}

	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = NULL;
	writes[0].dstSet = descriptor_set;
	writes[0].dstBinding = 0;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[0].pImageInfo = &image_info;

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].pNext = NULL;
	writes[1].dstSet = descriptor_set;
	writes[1].dstBinding = 1;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writes[1].pImageInfo = &normal_info;

	qvkUpdateDescriptorSets( vk.device, 2, writes, 0, NULL );

	stage->normalMap = normal;
	stage->vk_pbr_flags |= PBR_HAS_NORMALMAP;
	VectorSet4( stage->normalScale, r_baseNormalX->value, r_baseNormalY->value, 1.0f, r_baseParallax->value );

	tr.compute_normalmaps[tr.compute_normalmaps_batch_num].normal = normal;
	tr.compute_normalmaps[tr.compute_normalmaps_batch_num].descriptor_set = descriptor_set;
	tr.compute_normalmaps_batch_num++;
}

static void vk_compute_normalmap_mips( VkCommandBuffer cmd, image_t *image, 
    uint32_t width, uint32_t height, uint32_t mipLevels ) 
{
    VkImageMemoryBarrier barrier;
    uint32_t i, mipWidth, mipHeight;

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = NULL;
    barrier.image = image->handle;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    mipWidth = width;
    mipHeight = height;

    for ( i = 1; i < mipLevels; i++ )
	{
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        qvkCmdPipelineBarrier( cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, NULL, 0, NULL, 1, &barrier);

		// could create mips from base level, to batch blit operations.
		// but this results in a few artifacts during progressive filtering.
        VkImageBlit region;
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = (i - 1);
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcOffsets[0].x = 0;
		region.srcOffsets[0].y = 0;
		region.srcOffsets[0].z = 0;
		region.srcOffsets[1].x = mipWidth;
		region.srcOffsets[1].y = mipHeight;
		region.srcOffsets[1].z = 1;
        region.dstSubresource = region.srcSubresource;
        region.dstSubresource.mipLevel = i;
        region.dstOffsets[0] = { 0, 0, 0 };
        region.dstOffsets[1] = { 
            (int32_t)(mipWidth > 1 ? mipWidth / 2 : 1), 
            (int32_t)(mipHeight > 1 ? mipHeight / 2 : 1),  
            1  
        };

        qvkCmdBlitImage( cmd,
                       image->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &region, VK_FILTER_LINEAR );

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        qvkCmdPipelineBarrier( cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, NULL, 0, NULL, 1, &barrier );

		mipWidth >>= 1;
		if (mipWidth < 1) mipWidth = 1;

		mipHeight >>= 1;
		if (mipHeight < 1) mipHeight = 1;
    }

    // Final transition of last mip level
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    qvkCmdPipelineBarrier( cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, NULL, 0, NULL, 1, &barrier );
}

void vk_dispatch_compute_normalmaps( void )
{
	if (tr.compute_normalmaps_batch_num > 0)
	{
		VkCommandBuffer command_buffer = vk_begin_command_buffer();
		for (uint32_t i = 0; i < tr.compute_normalmaps_batch_num; i++) {
			comp_normalmap_item_t* item = &tr.compute_normalmaps[i];

			//Com_Printf("%s \n", item->normal->imgName);

			vk_record_image_layout_transition(
				command_buffer,
				item->normal->handle,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL,
				0, 0);

			qvkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk.compute_normalmap_pipeline);
			qvkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
				vk.pipeline_layout_compute_normalmap, 0, 1, &item->descriptor_set, 0, NULL);

			uint32_t groupsX = (item->normal->width + 7) / 8;
			uint32_t groupsY = (item->normal->height + 7) / 8;
			qvkCmdDispatch(command_buffer, groupsX, groupsY, 1);

			vk_record_image_layout_transition(
				command_buffer,
				item->normal->handle,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				item->normal->mipLevels > 1 ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				0, 0);

			if (item->normal->mipLevels > 1)
				vk_compute_normalmap_mips(command_buffer, item->normal, item->normal->uploadWidth, item->normal->uploadHeight, item->normal->mipLevels);
		}

		vk_end_command_buffer(command_buffer, __func__);

		tr.compute_normalmaps_batch_num = 0;
    }
}
#endif // VK_COMPUTE_NORMALMAP