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

static void* R_LocalMalloc( size_t size )
{
	return ri.Hunk_AllocateTempMemory( size );
}

static void* R_LocalReallocSized( void *ptr, size_t old_size, size_t new_size )
{
	void *mem = ri.Hunk_AllocateTempMemory( new_size );
	if  ( ptr )
	{
		memcpy( mem, ptr, old_size );
		ri.Hunk_FreeTempMemory( ptr );
	}
	return mem;
}

static void R_LocalFree( void *ptr )
{
	if ( ptr )
		ri.Hunk_FreeTempMemory( ptr );
}

#define STBI_MALLOC			R_LocalMalloc
#define STBI_REALLOC_SIZED	R_LocalReallocSized
#define STBI_FREE			R_LocalFree

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_GIF
//#define STBI_TEMP_ON_STACK
//#define STBI_ONLY_HDR
#include <utils/stb_image.h>

#define IMG_BYTE 0
#define IMG_FLOAT 1

static void LoadPNG16( const char *filename, byte **pic, int *width, int *height ) 
{
	byte *fbuffer;
	int len, components;

	len = ri.FS_ReadFile( (char*)filename, (void**)&fbuffer );
	if ( !fbuffer )
		return;

	*pic = (byte*)stbi_load_16_from_memory( fbuffer, len, width, height, &components, STBI_rgb_alpha );
	if ( *pic == NULL ) 
	{
		ri.FS_FreeFile( fbuffer );
		return;
	}

	ri.FS_FreeFile( fbuffer );
}

static void R_LoadImage16( const char *name, byte **pic, int *width, int *height ) {
	int		len;

	*pic = NULL;
	*width = 0;
	*height = 0;

	len = (int)strlen( name );
	if ( len < 5 )
		return;

	if ( !Q_stricmp( name + len - 4, ".png" ) )
		LoadPNG16( name, pic, width, height );
}

static void vk_rtx_copy_buffer_to_image(vkimage_t* image, uint32_t width, uint32_t height, VkBuffer *buffer, uint32_t mipLevel, uint32_t arrayLayer)
{
	VkCommandBuffer command_buffer;
	command_buffer = vk_begin_command_buffer();

	VkImageMemoryBarrier barrier;
	Com_Memset( &barrier, 0, sizeof(VkImageMemoryBarrier) );
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = image->arrayLayers;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = image->mipLevels;

	// transition to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.image = image->handle;

	qvkCmdPipelineBarrier(command_buffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, NULL, 0, NULL,
		1, &barrier);

	// buffer to image

	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = mipLevel;
	region.imageSubresource.baseArrayLayer = arrayLayer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = 0;
	region.imageOffset.y = 0;
	region.imageOffset.z = 0;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;

	qvkCmdCopyBufferToImage(command_buffer, *buffer, image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.image = image->handle;
	qvkCmdPipelineBarrier(command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, NULL, 0, NULL,
		1, &barrier);

	vk_end_command_buffer(command_buffer);
}

static void vk_rtx_create_image_array( const char *name, vkimage_t *image, uint32_t width, uint32_t height, 
	VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels, uint32_t arrayLayers, VkImageCreateFlags flags,
	VkComponentMapping *component_map = NULL ) 
{
	image->extent.width = width;
	image->extent.height = height;
	image->extent.depth = 1;

	image->mipLevels = mipLevels;
	image->arrayLayers = arrayLayers;

	// create image
	{
		VkImageCreateInfo desc;
		desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = flags;
		desc.imageType = VK_IMAGE_TYPE_2D;
		desc.format = format;
		desc.extent.width = width;
		desc.extent.height = height;
		desc.extent.depth = 1;
		desc.mipLevels = image->mipLevels;
		desc.arrayLayers = image->arrayLayers;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.tiling = VK_IMAGE_TILING_OPTIMAL;
		desc.usage = usage;
		desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		desc.queueFamilyIndexCount = 0;
		desc.pQueueFamilyIndices = NULL;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VK_CHECK(qvkCreateImage(vk.device, &desc, NULL, &image->handle));
		VK_CreateImageMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &image->handle, &image->memory);
	}

	// create image view
	{
		VkImageViewCreateInfo desc;
		desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.image = image->handle;

		if ( image->arrayLayers == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY )
			desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		else
			desc.viewType = image->arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

		desc.format = format;

		if ( component_map != NULL )
			desc.components = *component_map;
		else {
			desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		}

		desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		desc.subresourceRange.baseMipLevel = 0;
		desc.subresourceRange.levelCount = image->mipLevels;//VK_REMAINING_MIP_LEVELS;
		desc.subresourceRange.baseArrayLayer = 0;
		desc.subresourceRange.layerCount = image->arrayLayers;
		VK_CHECK(qvkCreateImageView(vk.device, &desc, NULL, &image->view));
	}

	VK_SET_OBJECT_NAME( &image->handle, name, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
	VK_SET_OBJECT_NAME( &image->view, name, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
}

static void vk_rtx_create_image( const char *name, vkimage_t *image, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels ) 
{
	vk_rtx_create_image_array( name, image, width, height, format, usage, mipLevels, 1, 0 );
}

void vk_rtx_create_cubemap( vkimage_t *image, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels ) 
{
	vk_rtx_create_image_array( "cubemap", image, width, height, format, usage, mipLevels, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT );
}

void vk_rtx_upload_image_data( vkimage_t *image, uint32_t width, uint32_t height, const uint8_t *pixels, 
						 uint32_t bytes_per_pixel, uint32_t mipLevel, uint32_t arrayLayer ) 
{
	VkDeviceSize imageSize = (uint64_t) width * (uint64_t) height * (uint64_t) 1 * (uint64_t)bytes_per_pixel;
	vkbuffer_t staging;

	vk_rtx_buffer_create( &staging, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );	

	// write data to buffer
	uint8_t *p;
	VK_CHECK( qvkMapMemory( vk.device, staging.memory, 0, imageSize, 0, (void**)(&p) ) );
	Com_Memcpy( p, pixels, (size_t)(imageSize) );
	qvkUnmapMemory( vk.device, staging.memory );

	vk_rtx_copy_buffer_to_image( image, width, height, &staging.buffer, mipLevel, arrayLayer );

	qvkDestroyBuffer( vk.device, staging.buffer, NULL );
	qvkFreeMemory( vk.device, staging.memory, NULL );
}

static void vk_rtx_record_image_layout_transition( vkimage_t *image, VkImageLayout oldLayout, VkImageLayout newLayout )
{
	VkCommandBuffer command_buffer;
	command_buffer = vk_begin_command_buffer();

	VkImageMemoryBarrier barrier;
	Com_Memset( &barrier, 0, sizeof(VkImageMemoryBarrier) );
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = image->mipLevels;
	
	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch ( oldLayout )
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			barrier.srcAccessMask = VK_ACCESS_NONE;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source 
			// Make sure any reads from the image have been finished
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch ( newLayout )
	{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (barrier.srcAccessMask == VK_ACCESS_NONE)
			{
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_GENERAL:
			barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
	}

	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.image = image->handle;

	qvkCmdPipelineBarrier( command_buffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 0, NULL, 0, NULL,
		1, &barrier );

	vk_end_command_buffer( command_buffer );
}

#define IMG_FLAGS	VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT  | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
static void vk_rtx_create_images( const char *name, uint32_t index, uint32_t width, uint32_t height, VkFormat format )
{
	vk_rtx_create_image( name, &vk.rtx_images[index], width, height, format, IMG_FLAGS, 1);

	vk_rtx_record_image_layout_transition( &vk.rtx_images[index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );
	VK_SET_OBJECT_NAME( vk.rtx_images[index].handle,	name,	VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
	VK_SET_OBJECT_NAME( vk.rtx_images[index].view,		name,	VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
}
#undef IMG_FLAGS

void vk_rtx_create_images( void ) 
{
	// create image
	#define IMG_DO( _handle, _binding, _format, _glsl_format, _w, _h ) \
		vk_rtx_create_images( #_handle, RTX_IMG_##_handle, _w, _h, VK_FORMAT_##_format );
		LIST_IMAGES
		LIST_IMAGES_A_B
	#undef IMG_DO

	// bind sampler
	#define IMG_DO( _handle, _binding, _format, _glsl_format, _w, _h ) \
		vk.rtx_images[RTX_IMG_##_handle].sampler = vk.tex_sampler_nearest;
		LIST_IMAGES
		LIST_IMAGES_A_B
	#undef IMG_DO

	vk.rtx_images[RTX_IMG_ASVGF_TAA_A].sampler = vk.tex_sampler;
	vk.rtx_images[RTX_IMG_ASVGF_TAA_B].sampler = vk.tex_sampler;
	vk.rtx_images[RTX_IMG_TAA_OUTPUT].sampler = vk.tex_sampler;


	// new method
	
	#define IMG_DO(_name, ...) \
		desc_output_img_info[RTX_IMG_##_name] = { \
			VK_NULL_HANDLE, \
			vk.rtx_images[RTX_IMG_##_name].view, \
			VK_IMAGE_LAYOUT_GENERAL \
		},
	VkDescriptorImageInfo desc_output_img_info[] = {
		LIST_IMAGES
		LIST_IMAGES_A_B
	};
	#undef IMG_DO

	VkDescriptorImageInfo img_info[] = {
	#define IMG_DO(_name, ...) \
		img_info[RTX_IMG_##_name] = { \
			vk.tex_sampler_nearest, \
			vk.rtx_images[RTX_IMG_##_name].view, \
			VK_IMAGE_LAYOUT_GENERAL \
		},
		LIST_IMAGES
		LIST_IMAGES_A_B
	};
	#undef IMG_DO

	VkWriteDescriptorSet output_img_write[NUM_RTX_IMAGES * 2];

	for ( int even_odd = 0; even_odd < 2; even_odd++ )
	{
		/* create information to update descriptor sets */
		#define IMG_DO( _name, _binding, ...) { \
			VkWriteDescriptorSet elem_image; \
			Com_Memset( &elem_image, 0, sizeof(VkWriteDescriptorSet) ); \
			elem_image.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; \
			elem_image.dstSet          = even_odd ? vk.desc_set_textures_odd : vk.desc_set_textures_even; \
			elem_image.dstBinding      = BINDING_OFFSET_IMAGES + _binding; \
			elem_image.dstArrayElement = 0; \
			elem_image.descriptorCount = 1; \
			elem_image.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; \
			elem_image.pImageInfo      = desc_output_img_info + RTX_IMG_##_name; \
			\
			VkWriteDescriptorSet elem_texture; \
			Com_Memset( &elem_texture, 0, sizeof(VkWriteDescriptorSet) ); \
			elem_texture.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; \
			elem_texture.dstSet          = even_odd ? vk.desc_set_textures_odd : vk.desc_set_textures_even; \
			elem_texture.dstBinding      = BINDING_OFFSET_TEXTURES + _binding; \
			elem_texture.dstArrayElement = 0; \
			elem_texture.descriptorCount = 1; \
			elem_texture.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; \
			elem_texture.pImageInfo      = img_info + RTX_IMG_##_name; \
			\
			output_img_write[RTX_IMG_##_name] = elem_image; \
			output_img_write[RTX_IMG_##_name + NUM_VKPT_IMAGES] = elem_texture; \
		}
		LIST_IMAGES;
		if ( even_odd )
		{
			LIST_IMAGES_B_A;
		}
		else
		{
			LIST_IMAGES_A_B;
		}
		#undef IMG_DO

		qvkUpdateDescriptorSets( vk.device, ARRAY_LEN(output_img_write), output_img_write, 0, NULL );
	}
}

void vk_rtx_initialize_images( void ) 
{
	VkSamplerCreateInfo sampler_info;
	Com_Memset( &sampler_info, 0, sizeof(VkSamplerCreateInfo) );
	sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter               = VK_FILTER_LINEAR;
	sampler_info.minFilter               = VK_FILTER_LINEAR;
	sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable        = VK_TRUE;
	sampler_info.maxAnisotropy           = 16;
	sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.minLod                  = 0.0f;
	sampler_info.maxLod                  = 128.0f;
	VK_CHECK( qvkCreateSampler( vk.device, &sampler_info, NULL, &vk.tex_sampler ) );

	VkSamplerCreateInfo sampler_nearest_info;
	Com_Memset( &sampler_nearest_info, 0, sizeof(VkSamplerCreateInfo) );
	sampler_nearest_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_nearest_info.magFilter               = VK_FILTER_NEAREST;
	sampler_nearest_info.minFilter               = VK_FILTER_NEAREST;
	sampler_nearest_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_nearest_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_nearest_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_nearest_info.anisotropyEnable        = VK_FALSE;
	sampler_nearest_info.maxAnisotropy           = 16;
	sampler_nearest_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_nearest_info.unnormalizedCoordinates = VK_FALSE;
	sampler_nearest_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	VK_CHECK( qvkCreateSampler( vk.device, &sampler_nearest_info, NULL, &vk.tex_sampler_nearest ) );

	VkSamplerCreateInfo sampler_nearest_mipmap_aniso_info;
	Com_Memset( &sampler_nearest_mipmap_aniso_info, 0, sizeof(VkSamplerCreateInfo) );
	sampler_nearest_mipmap_aniso_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_nearest_mipmap_aniso_info.magFilter               = VK_FILTER_NEAREST;
	sampler_nearest_mipmap_aniso_info.minFilter               = VK_FILTER_LINEAR;
	sampler_nearest_mipmap_aniso_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_nearest_mipmap_aniso_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_nearest_mipmap_aniso_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_nearest_mipmap_aniso_info.anisotropyEnable        = VK_TRUE;
	sampler_nearest_mipmap_aniso_info.maxAnisotropy           = 16;
	sampler_nearest_mipmap_aniso_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_nearest_mipmap_aniso_info.unnormalizedCoordinates = VK_FALSE;
	sampler_nearest_mipmap_aniso_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_nearest_mipmap_aniso_info.minLod                  = 0.0f;
	sampler_nearest_mipmap_aniso_info.maxLod                  = 128.0f;
	VK_CHECK( qvkCreateSampler( vk.device, &sampler_nearest_mipmap_aniso_info, NULL, &vk.tex_sampler_nearest_mipmap_aniso ) );

	VkSamplerCreateInfo sampler_linear_clamp_info;
	Com_Memset( &sampler_linear_clamp_info, 0, sizeof(VkSamplerCreateInfo) );
	sampler_linear_clamp_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_linear_clamp_info.magFilter               = VK_FILTER_LINEAR;
	sampler_linear_clamp_info.minFilter               = VK_FILTER_LINEAR;
	sampler_linear_clamp_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_linear_clamp_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_linear_clamp_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_linear_clamp_info.anisotropyEnable        = VK_FALSE;
	sampler_linear_clamp_info.maxAnisotropy           = 16;
	sampler_linear_clamp_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_linear_clamp_info.unnormalizedCoordinates = VK_FALSE;
	sampler_linear_clamp_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	VK_CHECK( qvkCreateSampler( vk.device, &sampler_linear_clamp_info, NULL, &vk.tex_sampler_linear_clamp ) );
	
	VkDescriptorSetLayoutBinding layout_bindings[] = {
		{
			0, // reserve for game textures, currently using seperate descriptor set
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			MAX_RIMAGES,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		#define IMG_DO(_name, _binding, ...) \
			{ \
				BINDING_OFFSET_IMAGES + _binding, \
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, \
				1, \
				VK_SHADER_STAGE_ALL, \
				VK_NULL_HANDLE \
			}, 
		LIST_IMAGES
		LIST_IMAGES_A_B
		#undef IMG_DO
		#define IMG_DO(_name, _binding, ...) \
			{ \
				BINDING_OFFSET_TEXTURES + _binding, \
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, \
				1, \
				VK_SHADER_STAGE_ALL, \
				VK_NULL_HANDLE \
			},
		LIST_IMAGES
		LIST_IMAGES_A_B
		#undef IMG_DO
		{
			BINDING_OFFSET_BLUE_NOISE,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_ENVMAP,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_PHYSICAL_SKY,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_PHYSICAL_SKY_IMG,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_SKY_TRANSMITTANCE,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_SKY_SCATTERING,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_SKY_IRRADIANCE,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_SKY_CLOUDS,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_TERRAIN_ALBEDO,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_TERRAIN_NORMALS,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_TERRAIN_DEPTH,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		},
		{
			BINDING_OFFSET_TERRAIN_SHADOWMAP,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_ALL,
			VK_NULL_HANDLE,
		}
	};

	VkDescriptorSetLayoutCreateInfo layout_info;
	Com_Memset( &layout_info, 0, sizeof(VkDescriptorSetLayoutCreateInfo) );
	layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = ARRAY_LEN(layout_bindings);
	layout_info.pBindings    = layout_bindings;
	VK_CHECK( qvkCreateDescriptorSetLayout( vk.device, &layout_info, NULL, &vk.desc_set_layout_textures ) );

	VkDescriptorPoolSize pool_sizes[2];
	Com_Memset( &pool_sizes, 0, sizeof(VkDescriptorPoolSize) * 2 );
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[0].descriptorCount = VK_MAX_SWAPCHAIN_SIZE * (MAX_RIMAGES + 2 * NUM_VKPT_IMAGES) + 128;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pool_sizes[1].descriptorCount = VK_MAX_SWAPCHAIN_SIZE * MAX_RIMAGES;

	VkDescriptorPoolCreateInfo pool_info;
	Com_Memset( &pool_info, 0, sizeof(VkDescriptorPoolCreateInfo) );
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = ARRAY_LEN(pool_sizes);
	pool_info.pPoolSizes    = pool_sizes;
	pool_info.maxSets       = VK_MAX_SWAPCHAIN_SIZE * 2;
	VK_CHECK( qvkCreateDescriptorPool( vk.device, &pool_info, NULL, &vk.desc_pool_textures ) );

	VkDescriptorSetAllocateInfo alloc;
	Com_Memset( &alloc, 0, sizeof(VkDescriptorSetAllocateInfo) );
	alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc.descriptorPool     = vk.desc_pool_textures;
	alloc.descriptorSetCount = 1;
	alloc.pSetLayouts        = &vk.desc_set_layout_textures;

	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.desc_set_textures_even ) );
	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.desc_set_textures_odd ) );

	vk_rtx_create_blue_noise();
}

void vk_rtx_destroy_image( vkimage_t *image )
{
	VK_CHECK( qvkQueueWaitIdle( vk.queue ) );

    if ( image->handle != NULL ) 
	{
        qvkDestroyImage( vk.device, image->handle, NULL );
        image->handle = VK_NULL_HANDLE;
    }

    if ( image->view != NULL ) 
	{
        qvkDestroyImageView( vk.device, image->view, NULL );
        image->view = VK_NULL_HANDLE;
    }

    if ( image->sampler != NULL ) 
	{
        qvkDestroySampler( vk.device, image->sampler, NULL );
        image->sampler = VK_NULL_HANDLE;
    }

	if ( image->memory != NULL ) 
	{
		qvkFreeMemory( vk.device, image->memory, NULL );
		image->memory = VK_NULL_HANDLE;
	}

	memset( image, 0, sizeof(vkimage_t) );
}

void vk_rtx_create_blue_noise( void ) 
{
	uint32_t	i, j, channel;
	int			width, height;
	int			bytes_per_channel = 2;
	byte		*pic;

	const int num_blue_noise_images = NUM_BLUE_NOISE_TEX / 4;
	const int res = BLUE_NOISE_RES;
	const size_t img_size = (size_t)res * (size_t)res;
	const size_t total_size = img_size * sizeof(uint16_t);

	VkComponentMapping component_map;
	component_map.r = VK_COMPONENT_SWIZZLE_R;
	component_map.g = VK_COMPONENT_SWIZZLE_R;
	component_map.b = VK_COMPONENT_SWIZZLE_R;
	component_map.a = VK_COMPONENT_SWIZZLE_R;

	vk_rtx_create_image_array( "blue noise array", &vk.blue_noise, res, res, VK_FORMAT_R16_UNORM, 
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		1, NUM_BLUE_NOISE_TEX, 0, &component_map );
	
	for ( i = 0; i < num_blue_noise_images; i++ ) 
	{
		char buf[1024];
		snprintf(buf, sizeof buf, "blue_noise_textures/%d_%d/HDR_RGBA_%04d.png", res, res, i);
			
		R_LoadImage16( buf, &pic, &width, &height );

		// HDR is RGBA
		for ( channel = 0; channel < 4; channel++ ) 
		{
			uint8_t img[2 * res * res];

			for ( j = 0; j < img_size; j++ ) 
			{
				img[(j * bytes_per_channel) + 0] = *(pic + ((j * 8) + ((channel * bytes_per_channel) + 0)));
				img[(j * bytes_per_channel) + 1] = *(pic + ((j * 8) + ((channel * bytes_per_channel) + 1)));
			}

			vk_rtx_upload_image_data( &vk.blue_noise, width, height, img, bytes_per_channel, 0, (i*4) + channel );
		}

		Z_Free( pic );
	}

	vk.blue_noise.sampler = vk.tex_sampler;

	VkDescriptorImageInfo desc_img_info;
	desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	desc_img_info.imageView   = vk.blue_noise.view;
	desc_img_info.sampler     = vk.blue_noise.sampler;

	VkWriteDescriptorSet s;
	Com_Memset( &s, 0, sizeof(VkWriteDescriptorSet) );
	s.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	s.pNext				= NULL;
	s.dstSet			= vk.desc_set_textures_even;
	s.dstBinding		= BINDING_OFFSET_BLUE_NOISE;
	s.dstArrayElement	= 0;
	s.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	s.descriptorCount	= 1;
	s.pImageInfo		= &desc_img_info;

	qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );

	s.dstSet = vk.desc_set_textures_odd;
	qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );
	
	qvkQueueWaitIdle( vk.queue );
}