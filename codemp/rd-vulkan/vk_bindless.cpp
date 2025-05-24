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

#ifdef VK_BINDLESS


void vk_bind_texture_idx( const uint32_t offset, const uint32_t idx )
{
	vk.cmd->uniform_global.texture_indices[offset] = idx;
}

void vk_update_draw_id( const uint32_t& draw_id )
{
	pushConst *push_constants = vk_get_push_constant();
	push_constants->draw_id = draw_id;
}

void vk_increase_draw_item_id( void )
{
	if ( !vk.bindlessActive )
		return;

	vk.draw_item_id++;
}

/**
 *	borrowed buffer, descriptor methods from raytracing branch
 */

// 
//
// model
//
#ifdef VK_BINDLESS_MODEL_VBO
	static vkbuffer_t null_buffer;
	uint32_t model_index = 0;

	static void vk_rtx_write_model_descriptor( int index, VkDescriptorSet descriptor, VkBuffer buffer, VkDeviceSize size )
	{
		VkDescriptorBufferInfo info;
		info.buffer = buffer;
		info.offset = 0;
		info.range = size;

		VkWriteDescriptorSet desc;
		Com_Memset( &desc, 0, sizeof(VkWriteDescriptorSet) );
		desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc.pNext = NULL;
		desc.dstSet = descriptor;
		desc.dstBinding = 0;
		desc.dstArrayElement = index;
		desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		desc.descriptorCount = 1;
		desc.pBufferInfo = &info;

		qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );
	}


	void vk_rtx_bind_model( int index )
	{
		if ( !vk.model_instance.descriptor.vbos || !vk.model_instance.descriptor.ibos )
			return;

		vk_rtx_write_model_descriptor( index, vk.model_instance.descriptor.vbos, tr.vbos[ index - 1 ]->buffer, tr.vbos[ index - 1 ]->size );
		vk_rtx_write_model_descriptor( index, vk.model_instance.descriptor.ibos, tr.ibos[ index - 1 ]->buffer, tr.ibos[ index - 1 ]->size );
	}

	void vk_rtx_create_model_vbo_ibo_descriptor( void ) 
	{
		VkDescriptorPoolSize pool_size[] = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_VBOS },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_VBOS },
		};

		VkDescriptorPoolCreateInfo pool_info;
		Com_Memset( &pool_info, 0, sizeof(VkDescriptorPoolCreateInfo) );
		pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.pNext         = NULL;
		pool_info.poolSizeCount = ARRAY_LEN( pool_size );
		pool_info.pPoolSizes    = pool_size;
		pool_info.maxSets       = ARRAY_LEN( pool_size );

		// sunny needs destroy method
		VK_CHECK( qvkCreateDescriptorPool( vk.device, &pool_info, NULL, &vk.model_instance.pool ) );

		VkDescriptorSetLayoutBinding bind;
		Com_Memset( &bind, 0, sizeof(VkDescriptorSetLayoutBinding) );
		bind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bind.descriptorCount = MAX_VBOS;
		bind.binding = 0;
		bind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;// VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo desc;
		Com_Memset( &desc, 0, sizeof(VkDescriptorSetLayoutCreateInfo) );
		desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.bindingCount = 1;
		desc.pBindings = &bind;

		VK_CHECK( qvkCreateDescriptorSetLayout( vk.device, &desc, NULL, &vk.model_instance.layout ) );

		VkDescriptorSetAllocateInfo alloc;
		Com_Memset( &alloc, 0, sizeof(VkDescriptorSetAllocateInfo) );
		alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc.descriptorPool = vk.model_instance.pool;
		alloc.descriptorSetCount = 1;
		alloc.pSetLayouts = &vk.model_instance.layout;

		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.model_instance.descriptor.vbos ) );
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.model_instance.descriptor.ibos ) );

		vk_rtx_buffer_create( &null_buffer, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	
		for ( int i = 0; i < MAX_VBOS; i++ )
		{
			vk_rtx_write_model_descriptor( i, vk.model_instance.descriptor.vbos, null_buffer.buffer, null_buffer.size );
			vk_rtx_write_model_descriptor( i, vk.model_instance.descriptor.ibos, null_buffer.buffer, null_buffer.size );
		}
	}
#endif

//
// buffers
//
static VkDeviceAddress vk_get_buffer_device_address(VkBuffer buffer)
{
	VkBufferDeviceAddressInfo address_info;
	address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address_info.pNext = NULL;
	address_info.buffer = buffer;

	return qvkGetBufferDeviceAddress( vk.device, &address_info );
}

VkResult vk_rtx_buffer_create( vkbuffer_t *buf, VkDeviceSize size, 
	VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_properties )
{
	assert(size > 0);
	assert(buf);
	VkResult result = VK_SUCCESS;

	VkBufferCreateInfo buf_create_info;
	buf_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_create_info.pNext = NULL;
	buf_create_info.flags = 0;
	buf_create_info.size  = size;
	buf_create_info.usage = usage;
	buf_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_create_info.queueFamilyIndexCount = 0;
	buf_create_info.pQueueFamilyIndices = NULL;

	buf->size = size;
	buf->is_mapped = 0;

	result = qvkCreateBuffer( vk.device, &buf_create_info, NULL, &buf->buffer );
	if ( result != VK_SUCCESS ) {
		goto fail_buffer;
	}
	assert(buf->buffer != VK_NULL_HANDLE);

	VkMemoryRequirements mem_reqs;
	qvkGetBufferMemoryRequirements( vk.device, buf->buffer, &mem_reqs );

	VkMemoryAllocateInfo mem_alloc_info;
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.pNext = NULL;
	mem_alloc_info.allocationSize = mem_reqs.size;
	mem_alloc_info.memoryTypeIndex = vk_find_memory_type( mem_reqs.memoryTypeBits, mem_properties );


	VkMemoryAllocateFlagsInfo mem_alloc_flags;
	mem_alloc_flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	mem_alloc_flags.pNext = NULL;
	mem_alloc_flags.flags = ( usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT ) ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT : 0;
	mem_alloc_flags.deviceMask = 0;

#ifdef VKPT_DEVICE_GROUPS
	if ( qvk.device_count > 1 && !( mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) ) {
		mem_alloc_flags.flags |= VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
		mem_alloc_flags.deviceMask = (1 << qvk.device_count) - 1;
	}
#endif

	mem_alloc_info.pNext = &mem_alloc_flags;

	result = qvkAllocateMemory( vk.device, &mem_alloc_info, NULL, &buf->memory );
	if ( result != VK_SUCCESS ) {
		goto fail_mem_alloc;
	}

	assert( buf->memory != VK_NULL_HANDLE );

	result = qvkBindBufferMemory( vk.device, buf->buffer, buf->memory, 0 );
	if ( result != VK_SUCCESS ) {
		goto fail_bind_buf_memory;
	}

	if ( usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT )
	{
		buf->address = vk_get_buffer_device_address( buf->buffer );
		assert(buf->address);
	}
	else
	{
		buf->address = 0;
	}

	return VK_SUCCESS;

fail_bind_buf_memory:
	qvkFreeMemory( vk.device, buf->memory, NULL );
fail_mem_alloc:
	qvkDestroyBuffer( vk.device, buf->buffer, NULL );
fail_buffer:
	buf->buffer = VK_NULL_HANDLE;
	buf->memory = VK_NULL_HANDLE;
	buf->size   = 0;
	return result;
}

VkResult vk_rtx_buffer_destroy( vkbuffer_t *buf )
{
	assert(!buf->is_mapped);

	if ( buf->memory != VK_NULL_HANDLE )
		qvkFreeMemory( vk.device, buf->memory, NULL );

	if ( buf->buffer != VK_NULL_HANDLE )
		qvkDestroyBuffer( vk.device, buf->buffer, NULL );

	buf->buffer = VK_NULL_HANDLE;
	buf->memory = VK_NULL_HANDLE;
	buf->size   = 0;
	buf->address = 0;

	return VK_SUCCESS;
}

void VK_DestroyBuffer( vkbuffer_t *buffer )
{
	if ( buffer->p ) {
		qvkUnmapMemory( vk.device, buffer->memory );
		buffer->p = NULL;
	}

	if ( buffer->buffer != NULL )
		qvkDestroyBuffer( vk.device, buffer->buffer, NULL );

	if ( buffer->memory != NULL )
		qvkFreeMemory( vk.device, buffer->memory, NULL );

	memset(buffer, 0, sizeof(vkbuffer_t));
}

void vk_rtx_destroy_buffers( void ) 
{
	uint32_t i = 0;

	/*
	VK_DestroyBuffer( &vk.buf_readback );
	VK_DestroyBuffer( &vk.buf_tonemap );
	VK_DestroyBuffer( &vk.buf_light );
	VK_DestroyBuffer( &vk.buf_sun_color );

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		VK_DestroyBuffer( vk.buf_readback_staging + i );
		VK_DestroyBuffer( vk.buf_light_staging + i );
	}*/
}

void *buffer_map( vkbuffer_t *buf )
{
	assert( !buf->is_mapped );

	buf->is_mapped = 1;
	void *ret = NULL;

	assert(buf->memory != VK_NULL_HANDLE);
	assert(buf->size > 0);

	VK_CHECK( qvkMapMemory( vk.device, buf->memory, 0 /*offset*/, buf->size, 0 /*flags*/, &ret ) );
	return ret;
}

void buffer_unmap( vkbuffer_t *buf )
{
	assert( buf->is_mapped );

	buf->is_mapped = 0;
	qvkUnmapMemory( vk.device, buf->memory );
}

void vk_rtx_upload_buffer_data_offset( vkbuffer_t *buffer, VkDeviceSize offset, VkDeviceSize size, const byte *data ) 
{
    if (offset + size > buffer->size) {
        ri.Error(ERR_FATAL, "Vulkan: Buffer to small!");
    }
	if (buffer->p == NULL) {
		ri.Error(ERR_FATAL, "Vulkan: Buffer not mapped!");
	}

    byte*p = buffer->p + offset;
    Com_Memcpy(p, data, (size_t)(size));
}

void vk_rtx_upload_buffer_data( vkbuffer_t *buffer, const byte *data ) 
{
	vk_rtx_upload_buffer_data_offset(buffer, 0, buffer->size, data);
}


//
// descriptors
//
///
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

			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				desc[i].descriptorCount = descriptor->data[i].updateSize;
				desc[i].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				desc[i].pBufferInfo		= &descriptor->data[i].buffer[0];
				assert( descriptor->data[i].buffer[0].buffer != NULL );
				break;
		}
	}

	qvkUpdateDescriptorSets( vk.device, descriptor->size, &desc[0], 0, NULL );
	free( desc );
}

static void vk_rtx_create_descriptor_layout( vkdescriptor_t *descriptor ) 
{ 
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT	info;
	VkDescriptorSetLayoutCreateInfo					desc;
	VkDescriptorBindingFlagBits						*flags; 
		
	flags = (VkDescriptorBindingFlagBits*)calloc(descriptor->size, sizeof(VkDescriptorBindingFlagBits));

	if ( descriptor->size > 0 ) 
		flags[descriptor->size-1] = (VkDescriptorBindingFlagBits)(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT);
	

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
	VkDescriptorSetVariableDescriptorCountAllocateInfo alloc;
	VkDescriptorSetAllocateInfo desc;
	uint32_t count; 
		
	count = descriptor->bindings[descriptor->size - 1].descriptorCount;

	Com_Memset( &alloc, 0, sizeof(VkDescriptorSetVariableDescriptorCountAllocateInfo));
	alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	alloc.pNext = NULL;
    alloc.descriptorSetCount = 1;
    alloc.pDescriptorCounts = &count;

	Com_Memset( &desc, 0, sizeof(VkDescriptorSetAllocateInfo));
    desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	desc.pNext = &alloc;
    desc.descriptorSetCount = 1;
    desc.descriptorPool = descriptor->pool;
    desc.pSetLayouts = &descriptor->layout;
        
    VK_CHECK( qvkAllocateDescriptorSets( vk.device, &desc, &descriptor->set ) );
}

void vk_rtx_create_descriptor( vkdescriptor_t *descriptor ) 
{
	vk_rtx_create_descriptor_layout( descriptor );
	vk_rtx_create_descriptor_pool( descriptor);
	vk_rtx_finish_descriptor( descriptor );
}


//
// image
// 
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

void vk_bind_storage_buffer( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkBuffer buffer )
{
	const uint32_t count = 1;

	vk_rtx_add_descriptor_buffer( descriptor, count, binding, stage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
	vk_rtx_set_descriptor_update_size( descriptor, binding, stage, count );
	vk_rtx_bind_descriptor_buffer( descriptor, binding, stage, buffer );
}

void vk_add_bindless_image( image_t *image )
{
	if ( image->layers == 6 ) {
		// revisit this, separate indexes?
		// bind whiteimage so descriptos is not VK_NULL_HANDLE
		vk_rtx_bind_descriptor_image_sampler( &vk.imageDescriptor, VK_BINDLESS_BINDING_TEXTURES, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, tr.whiteImage->sampler, tr.whiteImage->view, image->index );
		vk_rtx_bind_descriptor_image_sampler( &vk.imageDescriptor, VK_BINDLESS_BINDING_CUBEMAPS, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, image->sampler, image->view, image->cubemapIndex );
		vk_rtx_set_descriptor_update_size( &vk.imageDescriptor, VK_BINDLESS_BINDING_CUBEMAPS, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, image->cubemapIndex + 1 );
	}
	else {
		vk_rtx_bind_descriptor_image_sampler( &vk.imageDescriptor, VK_BINDLESS_BINDING_TEXTURES, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, image->sampler, image->view, image->index );
	}

	vk_rtx_set_descriptor_update_size( &vk.imageDescriptor, VK_BINDLESS_BINDING_TEXTURES, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, image->index + 1 );
	vk.imageDescriptor.needsUpdate = qtrue;
}

static void vk_bindless_create_buffers( void ) 
{
	uint32_t i;
	VkResult result;

	// flags & usage needs to be tweaked and optimized

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ )
	{
		result = vk_rtx_buffer_create(&vk.camera[i], sizeof(vkUniformCamera_t),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		assert(result == VK_SUCCESS);
	
		result = vk_rtx_buffer_create(&vk.light[i], sizeof(vkUniformLight_t),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		assert(result == VK_SUCCESS);

		result = vk_rtx_buffer_create(&vk.entity[i], sizeof(vkUniformEntity_t) * (REFENTITYNUM_WORLD + 1),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		assert(result == VK_SUCCESS);

		// use 30 as modelcount, whats safe?
		const int model_count = 1;
		result = vk_rtx_buffer_create(&vk.bones[i], sizeof(vkUniformBones_t) * ((REFENTITYNUM_WORLD + 1) * model_count),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		assert(result == VK_SUCCESS);

		// whats good amount of predicted drawcalls?
		const int draw_calls = 4096;
		result = vk_rtx_buffer_create(&vk.global[i], sizeof(vkUniformGlobal_t) * draw_calls,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK(qvkMapMemory(vk.device, vk.global[i].memory, 0, vk.global[i].size, 0, (void**)&vk.global[i].p));
		assert(result == VK_SUCCESS);

		#ifdef VK_BINDLESS_MODEL_VBO
			result = vk_rtx_buffer_create(&vk.indirect[i], sizeof(vkUniformIndirectDraw_t) * NUM_BINDLESS_INIDIRECT_DRAWS,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK(qvkMapMemory(vk.device, vk.indirect[i].memory, 0, vk.indirect[i].size, 0, (void**)&vk.indirect[i].p));
			assert(result == VK_SUCCESS);
		#endif VK_BINDLESS_MODEL_VBO
	}
}

void vk_init_bindless( void )
{
	if ( !vk.bindlessActive )
		return;

	uint32_t i;

	// frame SSBO
	{
		vk_bindless_create_buffers();
		#ifdef VK_BINDLESS_MODEL_VBO
		vk_rtx_create_model_vbo_ibo_descriptor();
		#endif

		VkShaderStageFlagBits flags = (VkShaderStageFlagBits)( VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT );
		Com_Memset( &vk.ssboDescriptor, 0, sizeof(vkdescriptor_t) * NUM_COMMAND_BUFFERS );

		for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ )
		{
			vk_bind_storage_buffer( &vk.ssboDescriptor[i], 0, flags, vk.entity[i].buffer );
			vk_bind_storage_buffer( &vk.ssboDescriptor[i], 1, flags, vk.bones[i].buffer );
			vk_bind_storage_buffer( &vk.ssboDescriptor[i], 2, flags, vk.global[i].buffer );
			vk_bind_storage_buffer( &vk.ssboDescriptor[i], 3, flags, vk.camera[i].buffer );
			vk_bind_storage_buffer( &vk.ssboDescriptor[i], 4, flags, vk.light[i].buffer );
#ifdef VK_BINDLESS_MODEL_VBO
			vk_bind_storage_buffer( &vk.ssboDescriptor[i], 5, flags, vk.indirect[i].buffer );
#endif
			vk_rtx_create_descriptor( &vk.ssboDescriptor[i] );
			vk_rtx_update_descriptor( &vk.ssboDescriptor[i] );
		}
	}

	// game textures
	{
		Com_Memset( &vk.imageDescriptor, 0, sizeof(vkdescriptor_t) );
		vk.imageDescriptor.lastBindingVariableSizeExt = qtrue;

		vk_rtx_add_descriptor_sampler( &vk.imageDescriptor, VK_BINDLESS_BINDING_TEXTURES, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, MAX_DRAWIMAGES, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

		vk_rtx_add_descriptor_sampler( &vk.imageDescriptor, VK_BINDLESS_BINDING_CUBEMAPS, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, MAX_DRAWIMAGES, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ); 
		vk_rtx_set_descriptor_update_size( &vk.imageDescriptor, VK_BINDLESS_BINDING_CUBEMAPS, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, 1 );
	
		vk_rtx_add_descriptor_sampler( &vk.imageDescriptor, VK_BINDLESS_BINDING_BRDFLUT, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ); 
		vk_rtx_set_descriptor_update_size( &vk.imageDescriptor, VK_BINDLESS_BINDING_BRDFLUT, (VkShaderStageFlagBits)VK_SHADER_STAGE_FRAGMENT_BIT, 1 );
	
		vk_rtx_create_descriptor( &vk.imageDescriptor );
		VK_SET_OBJECT_NAME( vk.imageDescriptor.set, "texture descriptor", VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT );
	}
}
#endif