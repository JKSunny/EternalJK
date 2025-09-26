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

static vkbuffer_t host_uniform_buffers[VK_MAX_SWAPCHAIN_SIZE];
static vkbuffer_t device_uniform_buffer;
static VkDescriptorPool desc_pool_ubo;
static size_t ubo_alignment = 0;

static inline size_t align(size_t x, size_t alignment)
{
	return (x + (alignment - 1)) & ~(alignment - 1);
}

VkResult vkpt_uniform_buffer_create( void )
{
	VkDescriptorPoolSize pool_sizes[2] = { };
	VkDescriptorSetLayoutBinding ubo_layout_bindings[2] = { 0 };

	ubo_layout_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_bindings[0].descriptorCount  = 1;
	ubo_layout_bindings[0].binding  = GLOBAL_UBO_BINDING_IDX;
	ubo_layout_bindings[0].stageFlags  = VK_SHADER_STAGE_ALL;

	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = 1;

	ubo_layout_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ubo_layout_bindings[1].descriptorCount  = 1;
	ubo_layout_bindings[1].binding  = GLOBAL_INSTANCE_BUFFER_BINDING_IDX;
	ubo_layout_bindings[1].stageFlags  = VK_SHADER_STAGE_ALL;

	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[1].descriptorCount = 1;

	VkDescriptorSetLayoutCreateInfo layout_info;
	Com_Memset( &layout_info, 0, sizeof(VkDescriptorSetLayoutCreateInfo) );
	layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.pNext        = NULL;
	layout_info.bindingCount = ARRAY_LEN(ubo_layout_bindings);
	layout_info.pBindings    = ubo_layout_bindings;

	VK_CHECK( qvkCreateDescriptorSetLayout( vk.device, &layout_info, NULL, &vk.desc_set_layout_ubo ) );

	const VkMemoryPropertyFlags host_memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	const VkMemoryPropertyFlags device_memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkPhysicalDeviceProperties properties;
	qvkGetPhysicalDeviceProperties( vk.physical_device, &properties );
	ubo_alignment = properties.limits.minUniformBufferOffsetAlignment;

	const size_t buffer_size = align(sizeof(vkUniformRTX_t), ubo_alignment) + sizeof(InstanceBuffer);
	for ( int i = 0; i < VK_MAX_SWAPCHAIN_SIZE; i++ )
		vk_rtx_buffer_create( host_uniform_buffers + i, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, host_memory_flags );
	
	vk_rtx_buffer_create( &device_uniform_buffer, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		device_memory_flags );

	VkDescriptorPoolCreateInfo pool_info;
	Com_Memset( &pool_info, 0, sizeof(VkDescriptorPoolCreateInfo) );
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.pNext         = NULL;
	pool_info.poolSizeCount = ARRAY_LEN(pool_sizes);
	pool_info.pPoolSizes    = pool_sizes;
	pool_info.maxSets       = 1;

	VK_CHECK( qvkCreateDescriptorPool( vk.device, &pool_info, NULL, &desc_pool_ubo ) );

	VkDescriptorSetAllocateInfo descriptor_set_alloc_info;
	Com_Memset( &descriptor_set_alloc_info, 0, sizeof(VkDescriptorSetAllocateInfo) );
	descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_alloc_info.pNext = NULL;
	descriptor_set_alloc_info.descriptorPool = desc_pool_ubo;
	descriptor_set_alloc_info.descriptorSetCount = 1;
	descriptor_set_alloc_info.pSetLayouts = &vk.desc_set_layout_ubo;

	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &descriptor_set_alloc_info, &vk.desc_set_ubo ) );
	
	VkDescriptorBufferInfo buf_info;
	buf_info.buffer = device_uniform_buffer.buffer;
	buf_info.offset = 0;
	buf_info.range  = sizeof(vkUniformRTX_t);

	VkDescriptorBufferInfo buf1_info;
	buf1_info.buffer = device_uniform_buffer.buffer;
	buf1_info.offset = align(sizeof(vkUniformRTX_t), ubo_alignment);
	buf1_info.range  = sizeof(InstanceBuffer);

	VkWriteDescriptorSet writes[2];
	Com_Memset( writes + 0, 0, sizeof(VkWriteDescriptorSet) * 2 );

	writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet          = vk.desc_set_ubo;
	writes[0].dstBinding      = GLOBAL_UBO_BINDING_IDX;
	writes[0].dstArrayElement = 0;
	writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].descriptorCount = 1;
	writes[0].pBufferInfo     = &buf_info;

	writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet          = vk.desc_set_ubo;
	writes[1].dstBinding      = GLOBAL_INSTANCE_BUFFER_BINDING_IDX;
	writes[1].dstArrayElement = 0;
	writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writes[1].descriptorCount = 1;
	writes[1].pBufferInfo     = &buf1_info;

	qvkUpdateDescriptorSets( vk.device, ARRAY_LEN(writes), writes, 0, NULL );

	return VK_SUCCESS;
}

VkResult vkpt_uniform_buffer_destroy( void )
{
	qvkDestroyDescriptorPool( vk.device, desc_pool_ubo, NULL );
	qvkDestroyDescriptorSetLayout( vk.device, vk.desc_set_layout_ubo, NULL );
	desc_pool_ubo = VK_NULL_HANDLE;
	vk.desc_set_layout_ubo = VK_NULL_HANDLE;

	for ( int i = 0; i < VK_MAX_SWAPCHAIN_SIZE; i++ )
		vk_rtx_buffer_destroy( host_uniform_buffers + i );

	vk_rtx_buffer_destroy( &device_uniform_buffer );

	return VK_SUCCESS;
}

VkResult vkpt_uniform_buffer_update( VkCommandBuffer command_buffer )
{
	vkbuffer_t *ubo = host_uniform_buffers + vk.current_frame_index;

	assert(ubo);
	assert(ubo->memory != VK_NULL_HANDLE);
	assert(ubo->buffer != VK_NULL_HANDLE);
	assert(vk.current_frame_index < VK_MAX_SWAPCHAIN_SIZE);

	vkUniformRTX_t *mapped_ubo = (vkUniformRTX_t*)buffer_map( ubo );
	assert(mapped_ubo);
	memcpy( mapped_ubo, &vk.uniform_buffer, sizeof(vkUniformRTX_t) );

	const size_t offset = align( sizeof(vkUniformRTX_t), ubo_alignment );
	memcpy((uint8_t*)mapped_ubo + offset, &vk.uniform_instance_buffer, sizeof(InstanceBuffer));

	buffer_unmap( ubo );
	mapped_ubo = NULL;

	VkBufferCopy copy = { 0 };
	copy.size = align(sizeof(vkUniformRTX_t), ubo_alignment) + sizeof(InstanceBuffer);
	qvkCmdCopyBuffer( command_buffer, ubo->buffer, device_uniform_buffer.buffer, 1, &copy );

	VkBufferMemoryBarrier barrier;
	Com_Memset( &barrier, 0, sizeof(VkBufferMemoryBarrier) );
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = device_uniform_buffer.buffer;
	barrier.size = copy.size;

	qvkCmdPipelineBarrier( command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0, 0, NULL, 1, &barrier, 0, NULL );

	return VK_SUCCESS;
}