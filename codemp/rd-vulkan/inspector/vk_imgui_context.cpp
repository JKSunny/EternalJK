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
#include <backends/imgui_impl_vulkan.cpp>
#include <backends/imgui_impl_sdl.cpp>
#include "vk_imgui.h"

#include <icons/FontAwesome5/IconsFontAwesome5.h>
#include "icons/FontAwesome5/fa5solid900.cpp"

SDL_Window			*screen;
VkDescriptorPool	imguiPool;
ImGuiGlobal			imguiGlobal;
ImGuiContext		*ImContext;

static SDL_Window *vk_imgui_get_sdl_window( void )
{
	//return SDL_GetWindowFromID( window.id );
	return (SDL_Window*)window.sdl_handle;
}

void R_GetImGuiContext( void **context  )
{
	*context = ImContext;
}

uint64_t R_GetImGuiTexture( qhandle_t hShader )
{
	shader_t *shader = R_GetShaderByHandle( hShader );
	image_t *image = shader->stages[0]->bundle[0].image[0];

	if ( image->descriptor_set != VK_NULL_HANDLE )
		return (uint64_t)image->descriptor_set;

	return NULL;
}

void vk_imgui_bind_game_color_image( void ) 
{
	Vk_Sampler_Def sd;
	VkSampler	sampler;

	Com_Memset(&sd, 0, sizeof(sd));
	sd.gl_mag_filter = sd.gl_min_filter = vk.blitFilter;
	sd.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sd.max_lod_1_0 = qtrue;
	sd.noAnisotropy = qtrue;

	sampler = vk_find_sampler(&sd);

	inspector.render_mode.image = ImGui_ImplVulkan_AddTexture( sampler, vk.gamma_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
}

#ifdef USE_RTX
void vk_imgui_rtx_add_unbound_texture( VkDescriptorSet *image, VkImageView view, VkSampler sampler )
{
	if ( *image != NULL )
		return;

	vk_rtx_get_god_rays_shadowmap( view, sampler );
	*image = ImGui_ImplVulkan_AddTexture( sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
}

void vk_imgui_bind_rtx_draw_image( void )
{
	if ( !vk.rtxActive ) 
		return;

	uint32_t i, j;
	Vk_Sampler_Def sd;
	VkSampler	sampler;

	Com_Memset(&sd, 0, sizeof(sd));
	sd.gl_mag_filter = sd.gl_min_filter = vk.blitFilter;
	sd.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sd.max_lod_1_0 = qtrue;
	sd.noAnisotropy = qtrue;
	
	sampler = vk_find_sampler( &sd );

	inspector.render_mode.rtx_image.bound[0] = inspector.render_mode.image;	// blitted to color_image
	inspector.render_mode.rtx_image.bound[1] = ImGui_ImplVulkan_AddTexture( sampler, vk.img_rtx[RTX_IMG_FLAT_COLOR].view, VK_IMAGE_LAYOUT_GENERAL );
}
#endif

static void vk_imgui_dark_theme( void )
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
	colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_Button]                 = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
	colors[ImGuiCol_HeaderActive]			= ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
	colors[ImGuiCol_Separator]              = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.00f, 0.00f, 0.00f, 1.29f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TabActive]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_TabUnfocused]           = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
	colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_DockingEmptyBg]         = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderLight]       = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	//style.WindowPadding                     = ImVec2(0.00f, 0.00f);
	//style.FramePadding                      = ImVec2(8.00f, 8.00f);
	//style.CellPadding                       = ImVec2(6.00f, 6.00f);
	//style.ItemSpacing                       = ImVec2(6.00f, 6.00f);
	//style.ItemInnerSpacing                  = ImVec2(6.00f, 6.00f);
	style.TouchExtraPadding                 = ImVec2(0.00f, 0.00f);
	style.IndentSpacing                     = 25;
	style.ScrollbarSize                     = 15;
	style.GrabMinSize                       = 10;
	style.WindowBorderSize                  = 0;
	style.ChildBorderSize                   = 0;
	style.PopupBorderSize                   = 0;
	style.FrameBorderSize                   = 0;
	style.TabBorderSize                     = 0;
	style.WindowRounding                    = 7;
	style.ChildRounding                     = 4;
	style.FrameRounding                     = 3;
	style.PopupRounding                     = 4;
	style.ScrollbarRounding                 = 9;
	style.GrabRounding                      = 3;
	style.LogSliderDeadzone                 = 4;
	style.TabRounding                       = 7;

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
	init_info.MinImageCount = 2;
	init_info.ImageCount = vk.swapchain_image_count;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init( &init_info, vk.render_pass.inspector );

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

	ImGui_ImplVulkan_RemoveTexture( inspector.render_mode.image );

#ifdef USE_RTX
	uint32_t i;

	ImGui_ImplVulkan_RemoveTexture( inspector.render_mode.rtx_image.bound[0] );
	ImGui_ImplVulkan_RemoveTexture( inspector.render_mode.rtx_image.bound[1] );

	for ( i = 0; i < NUM_UNBOUND_RENDER_MODES_RTX; i++ ) 
	{
		ImGui_ImplVulkan_RemoveTexture( inspector.render_mode.rtx_image.unbound[i] );
	}	
#endif
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

static void vk_imgui_end_frame( void )
{

	if ( !ImContext->WithinFrameScope )
		return;

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), vk.cmd->command_buffer );
}

void vk_imgui_draw( void ) 
{
	vk_imgui_create_gui();

	vk_imgui_end_frame();
}