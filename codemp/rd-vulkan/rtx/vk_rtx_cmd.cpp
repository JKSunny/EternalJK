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

void vk_rtx_create_command_pool( void ) 
{
	VkCommandPoolCreateInfo cmd_pool_create_info;
	Com_Memset( &cmd_pool_create_info, 0, sizeof(VkCommandPoolCreateInfo) );
	cmd_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_create_info.pNext            = NULL;
	cmd_pool_create_info.queueFamilyIndex = vk.queue_idx_graphics;
	cmd_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	/* command pool and buffers */
	VK_CHECK( qvkCreateCommandPool( vk.device, &cmd_pool_create_info, NULL, &vk.cmd_buffers_graphics.command_pool) );
	
	cmd_pool_create_info.queueFamilyIndex = vk.queue_idx_transfer;
	VK_CHECK( qvkCreateCommandPool( vk.device, &cmd_pool_create_info, NULL, &vk.cmd_buffers_transfer.command_pool) );
}

VkCommandBuffer vkpt_begin_command_buffer( cmd_buf_group_t* group )
{
	if (group->used_this_frame == group->count_per_frame)
	{
		uint32_t new_count = MAX(4, group->count_per_frame * 2);
		VkCommandBuffer* new_buffers =  (VkCommandBuffer*)Z_Malloc(new_count * NUM_COMMAND_BUFFERS * sizeof(VkCommandBuffer), TAG_GENERAL, qtrue );

		for ( int frame = 0; frame < NUM_COMMAND_BUFFERS; frame++ )
		{
			if ( group->count_per_frame > 0 )
				memcpy(new_buffers + new_count * frame, group->buffers + group->count_per_frame * frame, group->count_per_frame * sizeof(VkCommandBuffer));

			VkCommandBufferAllocateInfo cmd_buf_alloc_info;
			Com_Memset( &cmd_buf_alloc_info, 0, sizeof(VkCommandBufferAllocateInfo) );
			cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmd_buf_alloc_info.pNext = NULL;
			cmd_buf_alloc_info.commandPool = group->command_pool;
			cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmd_buf_alloc_info.commandBufferCount = new_count - group->count_per_frame;


			VK_CHECK( qvkAllocateCommandBuffers(vk.device, &cmd_buf_alloc_info, new_buffers + new_count * frame + group->count_per_frame));
		}
#if 0
#ifdef _DEBUG
		void** new_addrs = (void**)Z_Malloc(new_count * NUM_COMMAND_BUFFERS * sizeof(void*), TAG_GENERAL, qtrue );

		if (group->count_per_frame > 0)
		{
			for (int frame = 0; frame < NUM_COMMAND_BUFFERS; frame++)
			{
				memcpy(new_addrs + new_count * frame, group->buffer_begin_addrs + group->count_per_frame * frame, group->count_per_frame * sizeof(void*));
			}
		}

		Z_Free(group->buffer_begin_addrs);
		group->buffer_begin_addrs = new_addrs;
#endif
#endif

		Z_Free(group->buffers);
		group->buffers = new_buffers;
		group->count_per_frame = new_count;
	}

	VkCommandBuffer cmd_buf = group->buffers[group->count_per_frame * vk.current_frame_index + group->used_this_frame];

	VkCommandBufferBeginInfo begin_info;
	Com_Memset( &begin_info, 0, sizeof(VkCommandBufferBeginInfo) );
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = NULL;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	begin_info.pInheritanceInfo = NULL;

	VK_CHECK( qvkResetCommandBuffer( cmd_buf, 0 ) );
	VK_CHECK( qvkBeginCommandBuffer( cmd_buf, &begin_info ) );

#if 0
#ifdef _DEBUG
	void** begin_addr = group->buffer_begin_addrs + group->count_per_frame * vk.current_frame_index + group->used_this_frame;

#if (defined __GNUC__)
	*begin_addr = __builtin_return_address(0);
#elif (defined _MSC_VER)
	*begin_addr = _ReturnAddress();
#else
	*begin_addr = NULL;
#endif
#endif
#endif

	group->used_this_frame += 1;

	return cmd_buf;
}

/*
========================
Z_Freep
========================
*/
static void Z_Freep( void **ptr )
{
    if (*ptr) {
        Z_Free(*ptr);
        *ptr = NULL;
    }
}

void vkpt_free_command_buffers( cmd_buf_group_t* group )
{
	if (group->count_per_frame == 0)
		return;

	qvkFreeCommandBuffers( vk.device, group->command_pool, group->count_per_frame * NUM_COMMAND_BUFFERS, group->buffers );

	Z_Freep((void**)&group->buffers);

#ifdef USE_DEBUG
	Z_Freep((void**)&group->buffer_begin_addrs);
#endif

	group->count_per_frame = 0;
	group->used_this_frame = 0;
}

void vkpt_reset_command_buffers( cmd_buf_group_t* group )
{
	group->used_this_frame = 0;

#if 0 // defined(USE_DEBUG)
	for (int i = 0; i < group->count_per_frame; i++)
	{
		void* addr = group->buffer_begin_addrs[group->count_per_frame * qvk.current_frame_index + i];
		//seth: this seems unrelated to the raytracing changes, but skip it until raytracing is working
		//assert(addr == 0);
	}
#endif
}

void vkpt_wait_idle( VkQueue queue, cmd_buf_group_t* group )
{
	qvkQueueWaitIdle(queue);
	vkpt_reset_command_buffers(group);
}

void vkpt_submit_command_buffer(
	VkCommandBuffer cmd_buf,
	VkQueue queue,
	uint32_t execute_device_mask,
	int wait_semaphore_count,
	VkSemaphore* wait_semaphores,
	VkPipelineStageFlags* wait_stages,
	uint32_t* wait_device_indices,
	int signal_semaphore_count,
	VkSemaphore* signal_semaphores,
	uint32_t* signal_device_indices,
	VkFence fence)
{
	VK_CHECK( qvkEndCommandBuffer( cmd_buf )) ;

	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.waitSemaphoreCount = wait_semaphore_count;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.signalSemaphoreCount = signal_semaphore_count;
	submit_info.pSignalSemaphores = signal_semaphores;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf;

#ifdef VKPT_DEVICE_GROUPS
	VkDeviceGroupSubmitInfo device_group_submit_info;
	device_group_submit_info.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO;
	device_group_submit_info.pNext = NULL;
	device_group_submit_info.waitSemaphoreCount = wait_semaphore_count;
	device_group_submit_info.pWaitSemaphoreDeviceIndices = wait_device_indices;
	device_group_submit_info.commandBufferCount = 1;
	device_group_submit_info.pCommandBufferDeviceMasks = &execute_device_mask;
	device_group_submit_info.signalSemaphoreCount = signal_semaphore_count;
	device_group_submit_info.pSignalSemaphoreDeviceIndices = signal_device_indices;

	if (vk.device_count > 1) {
		submit_info.pNext = &device_group_submit_info;
	}
#endif

	VK_CHECK( qvkQueueSubmit( queue, 1, &submit_info, fence ) );

#if 0
#ifdef _DEBUG
	cmd_buf_group_t* groups[] = { &vk.cmd_buffers_graphics, &vk.cmd_buffers_transfer };
	for (int ngroup = 0; ngroup < ARRAY_LEN(groups); ngroup++)
	{
		cmd_buf_group_t* group = groups[ngroup];
		for (int i = 0; i < NUM_COMMAND_BUFFERS * group->count_per_frame; i++)
		{
			if (group->buffers[i] == cmd_buf)
			{
				group->buffer_begin_addrs[i] = NULL;
				return;
			}
		}
	}
#endif
#endif
}

void vkpt_submit_command_buffer_simple( VkCommandBuffer cmd_buf, VkQueue queue, bool all_gpus )
{
	vkpt_submit_command_buffer( cmd_buf, queue, all_gpus ? (1 << vk.device_count) - 1 : 1, 0, NULL, NULL, NULL, 0, NULL, NULL, 0 );
}