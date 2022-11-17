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

#include <imgui.h>
#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_tables.cpp>
#include <backends/imgui_impl_vulkan.cpp>
#include <backends/imgui_impl_sdl.cpp>
#include "icons/FontAwesome5/IconsFontAwesome5.h"
#include "icons/FontAwesome5/fa5solid900.cpp"

#include <SDL_video.h>
#include <typeinfo>

SDL_Window			*screen;
VkDescriptorPool	imguiPool;
ImGuiGlobal			imguiGlobal;
ImGuiContext		*ImContext;

static void vk_imgui_execute_cmd( const char *text ){
	ri.Cbuf_ExecuteText( EXEC_APPEND, va( "%s\n", text ) );	
}

static void vk_imgui_create_gui( void ) 
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::Begin("Debug information");

	ImGui::SetWindowSize(ImVec2((float)350, (float)230));
	ImGui::Text(va("Input state: %s", ( imguiGlobal.input_state ? "enabled" : "disabled" ) ) );
	ImGui::Text(va("Input value: %d", in_imgui->integer) );
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::Text("Mouse x:%d y:%d", (int)io.MousePos.x, (int)io.MousePos.y);
    
	ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 15, 8.5 ) );
	
	static char str0[128] = "";
    ImGui::InputText("##console-text", str0, IM_ARRAYSIZE(str0));
	ImGui::PopStyleVar();

	ImGui::SameLine();

	if ( ImGui::Button( ICON_FA_PAPER_PLANE, ImVec2{ 30, 30 } ) )
		vk_imgui_execute_cmd( va("say %s", str0) );
	
	ImGui::SameLine();

	if ( ImGui::Button( ICON_FA_JEDI, ImVec2{ 30, 30 } ) )
		vk_imgui_execute_cmd( "connect 135.125.145.49" );	

	ImGui::End();
}

static SDL_Window *vk_imgui_get_sdl_window( void )
{
	//return SDL_GetWindowFromID( window.id );
	return (SDL_Window*)window.sdl_handle;
}

void *R_GetImGuiContext( void  ) {
	return ImContext;
}

static void vk_imgui_dark_theme( void )
{
	auto& colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

	// Headers
	colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		
	// Buttons
	colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
	colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
	colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
	colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
	colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

	// Font Awesome 5
	// https://github.com/juliettef/IconFontCppHeaders
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();

	// merge in icons from Font Awesome
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
	ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromMemoryCompressedTTF( FA5SOLID900_compressed_data, FA5SOLID900_compressed_size, 12.0f, &icons_config, icons_ranges );	
}

void vk_imgui_initialize( void )
{
	VkSubmitInfo end_info;
	VkCommandBuffer command_buffer;
	VkCommandBufferBeginInfo begin_info;
	ImGui_ImplVulkan_InitInfo init_info;
	VkDescriptorPoolCreateInfo pool_info;
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER,					1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,				1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,		1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,		1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,			1000 }
	};

	screen = vk_imgui_get_sdl_window();
	Com_Memset(&imguiGlobal, 0, sizeof(imguiGlobal));

	pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = ARRAY_LEN(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VK_CHECK( qvkCreateDescriptorPool( vk.device, &pool_info, nullptr, &imguiPool ) );

	ri.Printf( PRINT_ALL, S_COLOR_CYAN "\n\nDear ImGui library is added to extend the excisting GUI.\n" );
	ri.Printf( PRINT_ALL, S_COLOR_CYAN "Github page & License are referenced here https://github.com/ocornut/imgui \n\n" );
	ri.Printf( PRINT_ALL, S_COLOR_CYAN "\n\nCIMGUI is a thin c-api wrapper for ImGui.\n" );
	ri.Printf( PRINT_ALL, S_COLOR_CYAN "Github page & License are referenced here https://github.com/cimgui/cimgui \n\n" );

	ImContext = ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// https://github.com/ocornut/imgui/issues/3492
	//ImGui::StyleColorsDark();
	vk_imgui_dark_theme();

	ImGui_ImplSDL2_InitForVulkan( screen );
	
	init_info = {};
	init_info.Instance = vk.instance;
	init_info.PhysicalDevice = vk.physical_device;
	init_info.Device = vk.device;
	init_info.Queue = vk.queue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init( &init_info, vk.render_pass.gamma );

	{
		command_buffer = vk.cmd->command_buffer;

		Com_Memset( &begin_info, 0, sizeof(begin_info) );
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = NULL;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = NULL;
		VK_CHECK( qvkBeginCommandBuffer( command_buffer, &begin_info ) );

		ImGui_ImplVulkan_CreateFontsTexture( command_buffer );

		VK_CHECK(qvkEndCommandBuffer(command_buffer));

        end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
		VK_CHECK( qvkQueueSubmit( vk.queue, 1, &end_info, VK_NULL_HANDLE ) );

		vk_wait_idle();

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
}

void vk_imgui_shutdown( void )
{
	ImGui_ImplVulkan_DestroyFontUploadObjects();
	qvkDestroyDescriptorPool( vk.device, imguiPool, nullptr );
	ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext( ImContext );

	Com_Memset( &imguiGlobal, 0, sizeof(imguiGlobal) );
}

static void vk_imgui_get_input_state( void )
{
	if ( in_imgui->integer && !imguiGlobal.input_state ){
		imguiGlobal.input_state = qtrue;

		SDL_SetRelativeMouseMode( SDL_FALSE );
	} 

	if ( !in_imgui->integer && imguiGlobal.input_state ){
		imguiGlobal.input_state = qfalse;

		SDL_SetRelativeMouseMode( SDL_TRUE );
	}
}

void vk_imgui_begin_frame( void )
{
	vk_imgui_get_input_state();
	
	if ( imguiGlobal.input_state ){
		SDL_Event e;

		while ( SDL_PollEvent( &e ) )
		{
			ImGui_ImplSDL2_ProcessEvent( &e );

			// disable input state
			if ( e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F1 )
				ri.Cvar_Set( "in_imgui", "0" );
		}
	}

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();

#if defined( __linux__ ) || defined( __APPLE__ )
    // SDL_GetWindowSize returns invalid size causing an ImGui assert
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2( glConfig.vidWidth, glConfig.vidHeight );
#endif

	ImGui::NewFrame();
}

static void vk_imgui_end_frame( void ) {
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), vk.cmd->command_buffer );
}

void vk_imgui_draw( void ) 
{
	vk_imgui_create_gui();

	vk_imgui_end_frame();
}