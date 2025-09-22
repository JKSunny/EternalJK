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

void vk_rtx_update_descriptor( vkdescriptor_t *descriptor ) 
{
	uint32_t i;
	VkWriteDescriptorSet *desc;
		
	desc = (VkWriteDescriptorSet*)calloc(descriptor->size, sizeof(VkWriteDescriptorSet));
	Com_Memset( desc + 0, 0, sizeof(VkWriteDescriptorSet) * descriptor->size );

	for ( i = 0; i < descriptor->size; ++i ) 
	{
		desc[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc[i].pNext = NULL;
		desc[i].dstSet = descriptor->set;
		desc[i].dstBinding = descriptor->bindings[i].binding;

		switch ( descriptor->bindings[i].descriptorType ) 
		{
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				desc[i].descriptorCount = descriptor->data[i].updateSize;
				desc[i].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				desc[i].pImageInfo		= &descriptor->data[i].image[0];
				assert( descriptor->data[i].image[0].imageView != NULL );
				break;

			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				desc[i].descriptorCount = 1;
				desc[i].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				desc[i].pImageInfo		= &descriptor->data[i].image[0];
				assert( descriptor->data[i].image[0].imageView != NULL );
				break;

			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				desc[i].descriptorCount = descriptor->data[i].updateSize;
				desc[i].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				desc[i].pBufferInfo		= &descriptor->data[i].buffer[0];
				assert( descriptor->data[i].buffer[0].buffer != NULL );
				break;

			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				desc[i].descriptorCount = 1;
				desc[i].descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				desc[i].pBufferInfo		= &descriptor->data[i].buffer[0];
				assert( descriptor->data[i].buffer[0].buffer != NULL );
				break;

			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
				desc[i].descriptorCount = 1;
				desc[i].descriptorType	= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				desc[i].pNext			= &descriptor->data[i].as;
				assert( descriptor->data[i].as.pAccelerationStructures != NULL );
				break;
		}
	}

	qvkUpdateDescriptorSets( vk.device, descriptor->size, &desc[0], 0, NULL );
	free( desc );
}

void vk_rtx_destroy_descriptor( vkdescriptor_t *descriptor ) 
{
	uint32_t i;

	if ( descriptor->set != VK_NULL_HANDLE ) {
		qvkFreeDescriptorSets( vk.device, descriptor->pool, 1, &descriptor->set );
		descriptor->set = VK_NULL_HANDLE;
	}

    if ( descriptor->layout != VK_NULL_HANDLE ) {
        qvkDestroyDescriptorSetLayout( vk.device, descriptor->layout, NULL );
        descriptor->layout = VK_NULL_HANDLE;
    }

    if ( descriptor->pool != VK_NULL_HANDLE ) {
        qvkDestroyDescriptorPool( vk.device, descriptor->pool, NULL );
		descriptor->pool = VK_NULL_HANDLE;
    }
    
	for ( i = 0; i < descriptor->size; ++i ) {
		if ( descriptor->bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ) 
		{
			if ( descriptor->data[i].image != NULL )
			{
				free( descriptor->data[i].image );
				descriptor->data[i].image = NULL;
			}
		}
	}

	if ( descriptor->data )
		free( descriptor->data );

	if ( descriptor->bindings )
		free( descriptor->bindings );

	Com_Memset( descriptor, 0, sizeof(vkdescriptor_t) );

	descriptor->data = NULL;
	descriptor->bindings = NULL;
	descriptor->size = 0;
}

static void vk_rtx_create_descriptor_layout( vkdescriptor_t *descriptor ) 
{ 
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT	info;
	VkDescriptorSetLayoutCreateInfo					desc;
	VkDescriptorBindingFlagBits						*flags; 
		
	flags = (VkDescriptorBindingFlagBits*)calloc(descriptor->size, sizeof(VkDescriptorBindingFlagBits));

	//if ( descriptor->size > 0 ) 
	//	flags[descriptor->size-1] = (VkDescriptorBindingFlagBits)(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT);
	
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	info.pNext = VK_NULL_HANDLE;
	info.bindingCount = descriptor->size;
	info.pBindingFlags = (VkDescriptorBindingFlags*)&flags[0];

	Com_Memset( &desc, 0, sizeof(desc));
    desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    desc.pNext = NULL;

	if ( descriptor->lastBindingVariableSizeExt )
		desc.pNext = &info;

    desc.bindingCount	= descriptor->size;
    desc.pBindings		= &descriptor->bindings[0];
  
    VK_CHECK( qvkCreateDescriptorSetLayout( vk.device, &desc, NULL, &descriptor->layout ) );

	free( flags );
}

static void vk_rtx_create_descriptor_pool( vkdescriptor_t *descriptor ) 
{
	uint32_t					i;
    VkDescriptorPoolSize		*pool_size;
	VkDescriptorPoolCreateInfo	desc;

	pool_size = (VkDescriptorPoolSize*)calloc( descriptor->size, sizeof(VkDescriptorPoolSize) );
    
    for ( i = 0; i < descriptor->size; i++ ) 
	{
        pool_size[i].type = descriptor->bindings[i].descriptorType;
		pool_size[i].descriptorCount = descriptor->bindings[i].descriptorCount;
    }

	Com_Memset( &desc, 0, sizeof(VkDescriptorPoolCreateInfo) );
    desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;//VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    desc.maxSets = 1;
    desc.poolSizeCount = descriptor->size;
    desc.pPoolSizes = &pool_size[0];
    VK_CHECK( qvkCreateDescriptorPool( vk.device, &desc, NULL, &descriptor->pool ) );
    
    free( pool_size );
}

static void vk_rtx_finish_descriptor( vkdescriptor_t *descriptor ) 
{
	//VkDescriptorSetVariableDescriptorCountAllocateInfo alloc;
	VkDescriptorSetAllocateInfo desc;
	//uint32_t count; 
		
	/*count = descriptor->bindings[descriptor->size - 1].descriptorCount;

	Com_Memset( &alloc, 0, sizeof(VkDescriptorSetVariableDescriptorCountAllocateInfo));
	alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	alloc.pNext = NULL;
    alloc.descriptorSetCount = 1;
    alloc.pDescriptorCounts = &count;
	*/

	Com_Memset( &desc, 0, sizeof(VkDescriptorSetAllocateInfo));
    desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	desc.pNext = NULL;
    desc.descriptorSetCount = 1;
    desc.descriptorPool = descriptor->pool;
    desc.pSetLayouts = &descriptor->layout;
        
    VK_CHECK( qvkAllocateDescriptorSets( vk.device, &desc, &descriptor->set ) );
}

vkdescriptor_t *vk_rtx_init_descriptor( vkdescriptor_t *descriptor )
{
	Com_Memset( descriptor, 0, sizeof(vkdescriptor_t) );

	// ~sunny, just cus im scared
	descriptor->data = NULL;
	descriptor->bindings = NULL;
	descriptor->size = 0;

	return descriptor;
}

void vk_rtx_create_descriptor( vkdescriptor_t *descriptor ) 
{
	vk_rtx_create_descriptor_layout( descriptor );
	vk_rtx_create_descriptor_pool( descriptor);
	vk_rtx_finish_descriptor( descriptor );
}

static uint32_t vk_rtx_add_descriptor_layout_binding(	vkdescriptor_t *descriptor, uint32_t count, uint32_t binding, 
														VkShaderStageFlagBits stage, VkDescriptorType type ) 
{
	descriptor->size++;
	descriptor->bindings	= (VkDescriptorSetLayoutBinding*)realloc(descriptor->bindings, descriptor->size * sizeof(VkDescriptorSetLayoutBinding));
	descriptor->data		= (vkdescriptorData_t*)realloc(descriptor->data, descriptor->size * sizeof(vkdescriptorData_t));

	uint32_t index = ( descriptor->size - 1 );

	descriptor->bindings[index].binding				= binding;
	descriptor->bindings[index].descriptorType		= type;
	descriptor->bindings[index].descriptorCount		= count;
	descriptor->bindings[index].stageFlags			= stage;
	descriptor->bindings[index].pImmutableSamplers	= NULL;

	return index;
}

static void vk_rtx_set_descriptor_image_info(	vkdescriptor_t *descriptor, uint32_t index, 
												uint32_t count, VkImageLayout layout )
{
	uint32_t i;

	//descriptor->data[index].size = count;
	descriptor->data[index].image = (VkDescriptorImageInfo*)malloc(count * sizeof(VkDescriptorImageInfo));
	
	for ( i = 0; i < count; i++ ) 
	{
		descriptor->data[index].image[i].imageLayout	= layout;
		descriptor->data[index].image[i].imageView		= VK_NULL_HANDLE;
		descriptor->data[index].image[i].sampler		= VK_NULL_HANDLE;
	}
}

void vk_rtx_add_descriptor_sampler( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage,
						 uint32_t count, VkImageLayout layout ) 
{
	uint32_t index;

	index = vk_rtx_add_descriptor_layout_binding( descriptor, count, binding, stage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER );
	
	vk_rtx_set_descriptor_image_info( descriptor, index, count, layout );
}

void vk_rtx_add_descriptor_image( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage ) 
{
	uint32_t index;
	const uint32_t count = 1;

	index = vk_rtx_add_descriptor_layout_binding( descriptor, count, binding, stage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE );
	
	vk_rtx_set_descriptor_image_info( descriptor, index, 1, VK_IMAGE_LAYOUT_GENERAL );
}

uint32_t vk_rtx_add_descriptor_buffer(  vkdescriptor_t *descriptor, uint32_t count, uint32_t binding, 
									VkShaderStageFlagBits stage, VkDescriptorType type ) 
{
	uint32_t i, index;

	index = vk_rtx_add_descriptor_layout_binding( descriptor, count, binding, stage, type );

	descriptor->data[index].buffer = (VkDescriptorBufferInfo*)malloc(count * sizeof(VkDescriptorBufferInfo));

	for ( i = 0; i < count; i++ ) 
	{
		descriptor->data[index].buffer[i].buffer	= VK_NULL_HANDLE;
		descriptor->data[index].buffer[i].offset	= 0;
		descriptor->data[index].buffer[i].range		= 0;
	}

	return index;
}

void vk_rtx_add_descriptor_as( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage ) 
{
	uint32_t index;
	const uint32_t count = 1;

	index = vk_rtx_add_descriptor_layout_binding( descriptor, count, binding, stage, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR );

	descriptor->data[index].as.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptor->data[index].as.pNext = VK_NULL_HANDLE;
	descriptor->data[index].as.accelerationStructureCount = 1;
	descriptor->data[index].as.pAccelerationStructures = VK_NULL_HANDLE;
}

void vk_rtx_bind_descriptor_as( vkdescriptor_t *descriptor, uint32_t binding,
								VkShaderStageFlagBits stage, VkAccelerationStructureKHR *as ) 
{
	uint32_t i;

	for ( i = 0; i < descriptor->size; ++i ) 
	{
		if ( descriptor->bindings[i].binding != binding || descriptor->bindings[i].stageFlags != stage )
			continue;

		descriptor->data[i].as.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptor->data[i].as.pAccelerationStructures = as;
		descriptor->data[i].as.accelerationStructureCount = 1;
		return;
	}
}

void vk_rtx_bind_descriptor_image_sampler(	vkdescriptor_t *descriptor, uint32_t binding,
											VkShaderStageFlagBits stage, VkSampler sampler, 
											VkImageView view, uint32_t index )
{
	uint32_t i;
	assert( view != NULL );

	for ( i = 0; i < descriptor->size; ++i ) 
	{
        if ( descriptor->bindings[i].binding != binding || descriptor->bindings[i].stageFlags != stage )
			continue;

        descriptor->data[i].image[index].sampler = sampler;
        descriptor->data[i].image[index].imageView = view;
        return;
    }
}

void vk_rtx_bind_descriptor_buffer( vkdescriptor_t *descriptor, uint32_t binding, 
									VkShaderStageFlagBits stage, VkBuffer buffer ) 
{
	uint32_t i;

	for ( i = 0; i < descriptor->size; ++i )
	{
		if ( descriptor->bindings[i].binding != binding || descriptor->bindings[i].stageFlags != stage ) 
			continue;

		descriptor->data[i].buffer[0].buffer = buffer;
		descriptor->data[i].buffer[0].offset = 0;
		descriptor->data[i].buffer[0].range = VK_WHOLE_SIZE;
		return;
	}
}

void vk_rtx_set_descriptor_update_size(  vkdescriptor_t *descriptor, uint32_t binding, 
										 VkShaderStageFlagBits stage, uint32_t size ) 
{
	uint32_t i;

	for ( i = 0; i < descriptor->size; ++i ) 
	{
		if ( descriptor->bindings[i].binding != binding || descriptor->bindings[i].stageFlags != stage )
			continue;

		descriptor->data[i].updateSize = size;
		return;
	}
}