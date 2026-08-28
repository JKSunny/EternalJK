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
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include "vk_imgui.h"

static uint32_t vk_get_readback_index( void )
{
    return (vk.cmd_index + 1) % NUM_COMMAND_BUFFERS;
}

static vkReadback_t *vk_get_readback_cmd( uint32_t cmd_index ) 
{
	return (vkReadback_t*)vk.readback[cmd_index].buffer_ptr;
}

// called in vk_begin_frame()
void vk_reset_readback_cmd( uint32_t cmd_index ) 
{
    vkReadback_t *readback = vk_get_readback_cmd( cmd_index );
    readback->count = 0U;
}

vkReadbackEntry_t *vk_get_readback_front( void )
{
	const uint32_t readback_index = vk_get_readback_index();

    vkReadback_t *readback = vk_get_readback_cmd( readback_index );
    vkReadbackEntry_t *front = NULL;

    //for ( uint32_t i = 0; i < readback->count; i++ )
	for ( int32_t i = (int32_t)readback->count - 1; i >= 0; i-- )
    {
        vkReadbackEntry_t *entry = &readback->entry[i];
        if ( !front || entry->depth < front->depth )
            front = entry;
    }

    return front;
}

void vk_imgui_process_readback( void )
{
	vkReadbackEntry_t *front_readback = vk_get_readback_front();
	uint32_t shader_index = front_readback ? front_readback->shaderIndex : 0U;
	shader_t *hovered_shader = NULL;
	ImGuiIO& io = ImGui::GetIO();

	if ( shader_index && shader_index < ARRAY_LEN(tr.shaders) ) 
		hovered_shader = tr.shaders[shader_index];

	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	if ( hovered_shader && ctrl && imguiGlobal.input_state ) 
	{
		inspector.shader.hovered_index = shader_index;

		auto click = ImGui::IsMouseClicked(0);
		//auto doubleClick = ImGui::IsMouseDoubleClicked(0);
		if ( click && hovered_shader->shaderText ) {
			inspector.shader.index = shader_index;
			inspector.shader.active = qtrue;
		}
	}
	else
		inspector.shader.hovered_index = 0U;
}

void vk_init_readback_descriptor( void ) 
{
	VkDescriptorSetAllocateInfo alloc;
	VkDescriptorBufferInfo info;
	VkWriteDescriptorSet desc;
	uint32_t i;

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) 
	{
		alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc.pNext = NULL;
		alloc.descriptorPool = vk.descriptor_pool;
		alloc.descriptorSetCount = 1;
		alloc.pSetLayouts = &vk.set_layout_storage_static;
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.readback[i].descriptor ) );

		info.buffer = vk.readback[i].buffer;
		info.offset = 0;
		info.range = sizeof(vkReadback_t);

		desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc.dstSet = vk.readback[i].descriptor;
		desc.dstBinding = 0;
		desc.dstArrayElement = 0;
		desc.descriptorCount = 1;
		desc.pNext = NULL;
		desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		desc.pImageInfo = NULL;
		desc.pBufferInfo = &info;
		desc.pTexelBufferView = NULL;

		qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );
	}
}

void vk_init_readback_storage_buffer (void )
{
	uint32_t i;

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) 
	{
		vk_create_storage_buffer( &vk.readback[i], sizeof(vkReadback_t), va("readback ssbo %d", i) );
	}
}

void vk_destroy_readback_storage_buffer( void ) 
{
	uint32_t i;

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) 
	{
		VK_DESTROY_BUFFER(vk.device, vk.readback[i].buffer);
		VK_FREE_MEMORY(vk.device, vk.readback[i].memory);
	}
}

void vk_imgui_draw_readback( void )
{ 
	uint32_t i, j;
	const shader_t *sh = NULL;
	const shaderStage_t		*pStage;

	if( !windows.readback.p_open )
		return;

	ImGui::Begin( "Readback", &windows.readback.p_open );
	ImGui::GetWindowDrawList()->ChannelsSplit( 3 );
	ImGui::GetWindowDrawList()->ChannelsSetCurrent( 1 );

	vkReadback_t *readback = vk_get_readback_cmd( (vk.cmd_index + 1) % 2 );
	for ( i = 0; i < MAX(readback->count, 0); i++ ) 
	{
		vkReadbackEntry_t *entry = &readback->entry[i];
		uint32_t shader_index = entry ? entry->shaderIndex : 0U;
		sh = NULL;

		if ( shader_index && shader_index < ARRAY_LEN(tr.shaders) ) 
			sh = tr.shaders[shader_index];

		if ( !sh )
			continue;

		// sky
		if ( sh->sky )
		{
			ImGui::BeginGroup();
			// outer box
			if ( sh->sky->outerbox[0] ) {
				ImGui::Text( "Outerbox" );

				for ( j = 0; j < 6; j++ ) {
					vk_imgui_draw_inspector_shader_visualize_texture( sh->sky->outerbox[j], va( "Outerbox: %d", j ) );
					ImGui::SameLine();
				}
			}

			ImGui::NewLine();

			// inner box
			if ( sh->sky->innerbox[0] ) {
				ImGui::Text( "Innerbox" );

				for ( j = 0; j < 6; j++ ) {
					vk_imgui_draw_inspector_shader_visualize_texture( sh->sky->innerbox[j], va( "Innerbox: %d", j ) );
					ImGui::SameLine();
				}
			}
			ImGui::EndGroup();
		}

		// surface
		else{
			pStage = sh->stages[0];
			if ( pStage && pStage->active && pStage->bundle[0].image[0] )
			{
				vk_imgui_draw_inspector_shader_visualize_texture(  pStage->bundle[0].image[0], va( "##Image_0%d", shader_index ) );
			}
			else
				vk_imgui_draw_inspector_shader_visualize_texture( tr.defaultShader->stages[0]->bundle[0].image[0] , va("##Image_0%d", shader_index));
			
			ImGui::SameLine();
		}

		ImGui::BeginGroup();
		if ( sh )
		{
			ImGui::Text( "Shader: %s ", sh->name );

			if ( !sh->shaderText ) {
				ImGui::SameLine();
				ImGui::TextColored( ImVec4( 1.0f, 0.65f, 0.25f, 1.0f ), " <no entry>" );
			}
		}
		else
			ImGui::Text( "Shader: %s", "undefined" );

		ImGui::Text( "Depth: %.6f", entry->depth );
		ImGui::EndGroup();

		if ( sh->sky )
			ImGui::Separator();
	}

	ImGui::GetWindowDrawList()->ChannelsMerge();
	ImGui::End();
}