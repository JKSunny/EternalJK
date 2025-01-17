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
#include "imgui_internal.h"
#include "icons/FontAwesome5/IconsFontAwesome5.h"
#include "icons/FontAwesome5/fa5solid900.cpp"
#include <inspector/vk_imgui.h>

#include <typeinfo>

vk_imgui_inspector_t inspector;
vk_imgui_window_t	windows;

void vk_imgui_clear_inspector( qboolean reset ) 
{
	if ( reset ) {
		Com_Memset( &inspector, 0, sizeof(inspector) );
		return;
	}

	Com_Memset( &inspector.transform, 0, sizeof(inspector.transform) );
	Com_Memset( &inspector.shader, 0, sizeof(inspector.shader) );
	Com_Memset( &inspector.entity, 0, sizeof(inspector.entity) );
	Com_Memset( &inspector.node, 0, sizeof(inspector.node) );
	Com_Memset( &inspector.surface, 0, sizeof(inspector.surface) );
#ifdef USE_RTX
	Com_Memset( &inspector.rtx_light_poly, 0, sizeof(inspector.rtx_light_poly) );
#endif
}

// handler to set/update ptrs and info for the current object selected
static void vk_imgui_selected_object_handler( void ) 
{
	if ( inspector.selected.ptr != nullptr ) 
	{
		if ( inspector.selected.prev == inspector.selected.ptr )
			return;

		// switch to a newly selected object
		vk_imgui_clear_inspector( qfalse );

		switch ( inspector.selected.type ) {
			case OT_CUBEMAP:
				break;
			case OT_FLARE:
			{
				srfFlare_t *flare = (srfFlare_t*)inspector.selected.ptr;

				inspector.transform.origin = &flare->origin;
				inspector.transform.active = qtrue;

				inspector.shader.index = flare->shader->index;
				inspector.shader.active = qtrue;
					
				break;
			}
			case OT_SHADER:
			{
				inspector.shader.index = (int)((shader_t*)inspector.selected.ptr)->index;
				inspector.shader.active = qtrue;
					
				break;
			}
			case OT_ENTITY:
			{
				trRefEntity_t *ent = (trRefEntity_t*)inspector.selected.ptr;

				inspector.transform.origin = &ent->e.origin;
				inspector.transform.radius = &ent->e.radius;
				inspector.transform.rotation = &ent->e.rotation;
				inspector.transform.active = qtrue;

				inspector.entity.active = qtrue;
				inspector.entity.ptr = ent;
				
				break;
			}
			case OT_NODE:
			{
				inspector.node.active = qtrue;
				inspector.node.node = (mnode_t*)inspector.selected.ptr;
				
				break;
			}
			case OT_SURFACE:
			{
				inspector.surface.active = qtrue;
				inspector.surface.surf = inspector.selected.ptr;
				
				break;
			}
#ifdef USE_RTX
			case OT_RTX_LIGHT_POLY:
			{
				inspector.rtx_light_poly.active = qtrue;
				inspector.rtx_light_poly.light = (light_poly_t*)inspector.selected.ptr;
				break;
			}
#endif
		}

		inspector.selected.prev = inspector.selected.ptr;
		return;
	}
}

static void vk_imgui_draw_objects( void ) 
{
	ImGui::Begin( "Objects" );  

	ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
	ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 10.0f, 7.0f ) );
	ImGui::PushStyleColor( ImGuiCol_Border , ImVec4(1.00f, 1.00f, 1.00f, 0.20f) );
	ImGui::InputTextWithHint("##SearchObjects", "Search..", inspector.search_keyword, 32);
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);

	ImGui::Dummy( ImVec2( 0.0f, 15.0f ) );

	ImGui::PushStyleColor( ImGuiCol_FrameBg , ImVec4(0.10f, 0.10f, 0.10f, 1.00f) );
	if( ImGui::BeginListBox( "##Objects", ImGui::GetContentRegionAvail() ) )
	{	
		if ( tr.registered && tr.world ) {
			//vk_imgui_draw_objects_cubemaps();
			//vk_imgui_draw_objects_nodes();
			vk_imgui_draw_objects_entities();
			vk_imgui_draw_objects_flares();
#ifdef USE_RTX
			vk_rtx_imgui_draw_objects_lights();
#endif
		}

		vk_imgui_draw_objects_shaders();
		
		ImGui::EndListBox();
	}
	ImGui::PopStyleColor();

	ImGui::End();
}

static void vk_imgui_draw_inspector_transform( void )
{
	if ( !inspector.transform.active )
		return;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
	bool opened = ImGui::TreeNodeEx((void*)typeid(inspector.transform).hash_code(), flags, ICON_FA_CUBE " Transform");

	if ( !opened )
		return;

	if ( inspector.transform.origin )
		imgui_draw_vec3_control( "Origin", *inspector.transform.origin, 0.0f, 100.0f );
	
	if ( inspector.transform.radius )
		imgui_draw_text_column( "Radius",  va("%d", *inspector.transform.radius), 100.0f );

	if ( inspector.transform.rotation )
		imgui_draw_text_column( "Rotation",  va("%d", *inspector.transform.rotation), 100.0f );

	ImGui::TreePop();
}

static void vk_imgui_draw_inspector( void )
{
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2{ 4.0f, 10.0f } );
	ImGui::Begin( "Inspector##Object" );

	if ( tr.registered && tr.world && inspector.selected.ptr != nullptr ) {
		vk_imgui_draw_inspector_transform();
		vk_imgui_draw_inspector_entity();
		vk_imgui_draw_inspector_world_node();
		vk_imgui_draw_inspector_world_node_surface();
#ifdef USE_RTX
		vk_imgui_draw_inspector_rtx_light_poly();
#endif
	}

	vk_imgui_draw_inspector_shader();

	ImGui::End();
	ImGui::PopStyleVar();
}

static void vk_imgui_draw_render_mode( const char **modes, uint32_t num_render_modes )
{
	static const char *current_render_mode;
	
	current_render_mode = modes[ inspector.render_mode.index ];

	ImGui::SetNextItemWidth( 200 );

	ImGui::PushStyleColor( ImGuiCol_FrameBg , ImVec4(0.00f, 0.00f, 0.00f, 1.00f) );
	if ( ImGui::BeginCombo( "##RenderModeInspector", current_render_mode, ImGuiComboFlags_HeightLarge ) )
	{
		for ( uint32_t n = 0; n < num_render_modes; n++ )
		{
			bool is_selected = ( current_render_mode == modes[n] );

			if ( ImGui::Selectable( modes[n], is_selected ) )
				inspector.render_mode.index = n;

			if ( is_selected )
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::PopStyleColor();
}

static void vk_imgui_draw_play_button( float height ) 
{
	if ( !com_sv_running->integer || !tr.world )
		return;

	qboolean modified = qfalse;
	const ImVec2 region = ImGui::GetContentRegionAvail();
	const ImVec2 pos = ImGui::GetCursorScreenPos(); 
	ImGui::SetCursorScreenPos( ImVec2( ( gls.windowWidth / 2 ) - height, pos.y ) );

	if ( !cl_paused->integer ) ImGui::BeginDisabled();
	if ( ImGui::Button( ICON_FA_PLAY, ImVec2{ height, height } ) )
		modified = qtrue;

	if ( !cl_paused->integer ) ImGui::EndDisabled();
	else ImGui::BeginDisabled();
			
	if ( ImGui::Button( ICON_FA_PAUSE, ImVec2{ height, height } ) )
		modified = qtrue;

	if ( cl_paused->integer ) ImGui::EndDisabled();

	if ( modified ) {
		ri.Cvar_Set( "cl_paused", cl_paused->integer ? "0" : "1" );
	
		// clear entity selection
		if ( !cl_paused->integer ) {
			inspector.selected.ptr = NULL;
			inspector.entity.ptr = NULL;
			inspector.entity.active = qfalse;
		}
	}
}

static void vk_imgui_draw_bar_filemenu( ImGuiIO &io, ImGuiViewportP *viewport )
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = ImGui::GetFrameHeight();
    
	if ( ImGui::BeginViewportSideBar( "##MenuBar", viewport, ImGuiDir_Up, height, window_flags ) ) {
		if ( ImGui::BeginMenuBar() ) {
			if ( ImGui::BeginMenu( "File" ) ) {
				if ( ImGui::MenuItem( " Quit" ) ) 
					ri.Cmd_ExecuteString( "quit" );

				ImGui::EndMenu();
			}

			if ( ImGui::BeginMenu( "Window" ) ) {
				if ( ImGui::MenuItem( " Shader editor" ) ) 
					windows.shader.p_open = true;

				if ( ImGui::MenuItem( " Profiler" ) ) 
					windows.profiler.p_open = true;

				if ( ImGui::MenuItem( " Viewport" ) ) 
					windows.viewport.p_open = true;

				ImGui::EndMenu();
			}

			vk_imgui_draw_play_button( height );

			ImGui::SameLine( gls.windowWidth - 200.0f );
			ImGui::Text( "%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate );

			ImGui::EndMenuBar();
		}
		ImGui::End();
	}
}

static void vk_imgui_draw_bar_tools( ImGuiIO &io, ImGuiViewportP *viewport )
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = ImGui::GetFrameHeight();

	if ( ImGui::BeginViewportSideBar( "##ToolBar", viewport, ImGuiDir_Up, height, window_flags ) ) {
		if ( ImGui::BeginMenuBar() ) 
		{	
			ImGui::EndMenuBar();
		}
		ImGui::End();
	}
}

static void vk_imgui_draw_bar_status( ImGuiIO &io, ImGuiViewportP *viewport )
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = ImGui::GetFrameHeight();

	if ( ImGui::BeginViewportSideBar( "##StatusBar", viewport, ImGuiDir_Down, height, window_flags ) ) {
		if (ImGui::BeginMenuBar()) {
			ImGui::Text( va("[F1] Input %s", ( imguiGlobal.input_state ? "enabled" : "disabled" ) ) );
			ImGui::SameLine( 225 );
			ImGui::Text( "Mouse x:%d y:%d", (int)io.MousePos.x, (int)io.MousePos.y );
			ImGui::EndMenuBar();
		}
		ImGui::End();
	}
}

static inline void vk_imgui_draw_bars( void ) 
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiViewportP *viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();

	vk_imgui_draw_bar_filemenu( io, viewport );
	//vk_imgui_draw_bar_tools( io, viewport );
	vk_imgui_draw_bar_status( io, viewport );
}

static void vk_imgui_draw_viewport( void ) 
{
	if ( !windows.viewport.p_open )
		return;

	ImGui::SetNextWindowSizeConstraints( { (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, { (float)gls.windowWidth, (float)gls.windowHeight } );
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin( "Viewport", &windows.viewport.p_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
	ImGui::PopStyleVar();

	const ImVec2& size = ImGui::GetCurrentWindow()->Size;
	const ImVec2 region = ImGui::GetContentRegionAvail();
	const ImVec2 pos = ImGui::GetCursorScreenPos(); 

	// resize handler
	if ( windows.viewport.size.x != size.x || windows.viewport.size.y != size.y )
	{
		// minimize
		if ( size.x < 100.0f || size.y < 100.0f ) {
			ImGui::End();
			return;
		}

		glConfig.vidWidth = (int)size.x;
		glConfig.vidHeight = (int)size.y - 46.0f;

		windows.viewport.size = size;

		//gls.captureWidth = glConfig.vidWidth;
		//gls.captureHeight = glConfig.vidHeight;

		ri.CL_ResChanged();
	}

	ImGui::SetCursorScreenPos( ImVec2( pos.x + 5.0f, pos.y + 6.0f ) );
	
	ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 1.0f, 1.0f ) );
	ImGui::Checkbox("selection wireframe", &inspector.outline_selected );
	ImGui::PopStyleVar();

	ImGui::SetCursorScreenPos( ImVec2( pos.x + region.x - 205.0f, pos.y + 4.0f ) );

#ifdef USE_RTX
	if ( vk.rtxActive )  
	{
		uint32_t index;

		vk_imgui_draw_render_mode( rtx_render_modes, NUM_TOTAL_RENDER_MODES_RTX );

		if ( inspector.render_mode.index < NUM_BOUND_RENDER_MODES_RTX )
		{
			if ( inspector.render_mode.index > 0 )
				index = 1; // FLAT_COLOR;
			else 
				index = 0; // TAA_OUTPUT + TONEMAPPING;

			index = 0; // debug with tonemapping applied.

			ImGui::Image( (ImU64)inspector.render_mode.rtx_image.bound[index], 
				{ (float)gls.windowWidth, (float)gls.windowHeight } );
		}

		// unbound images in the rtx or compute descriptors will have to be drawn directly
		else
		{
			VkDescriptorSet *image = &inspector.render_mode.rtx_image.unbound[SHADOW_MAP_RENDER_MODE_RTX];
			if ( *image == NULL )
			{
				VkImageView shadow_image_view;
				VkSampler shadow_sampler;
				vk_rtx_get_god_rays_shadowmap( shadow_image_view, shadow_sampler );

				vk_imgui_rtx_add_unbound_texture( image, shadow_image_view, shadow_sampler );
			}

			index = ( inspector.render_mode.index - NUM_BOUND_RENDER_MODES_RTX);

			ImGui::Image( (ImU64)inspector.render_mode.rtx_image.unbound[index], 
				{ (float)gls.windowWidth, (float)gls.windowHeight } );
		}
	}
	else
#endif
	{
		vk_imgui_draw_render_mode( render_modes, IM_ARRAYSIZE( render_modes ) );

		ImGui::Image( (ImU64)inspector.render_mode.image, { (float)gls.windowWidth, (float)gls.windowHeight } );
	}

	ImGui::End();
}

//
// bind image descriptors
//
void vk_imgui_swapchain_restarted( void )
{
	vk_imgui_bind_game_color_image();
#ifdef USE_RTX
	vk_imgui_bind_rtx_draw_image();
#endif

	glConfig.vidWidth = (int)windows.viewport.size.x;
    glConfig.vidHeight = (int)windows.viewport.size.y - 46.0f;
}

void vk_imgui_create_gui( void ) 
{
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoWindowMenuButton |ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Inspector", (bool*)true, window_flags);
	ImGui::PopStyleVar();
	ImGui::PopStyleVar(2);

	ImGuiIO& io = ImGui::GetIO();
    if ( io.ConfigFlags & ImGuiConfigFlags_DockingEnable )
    {
        ImGuiID dockspace_id = ImGui::GetID( "dockingNode" );
        ImGui::DockSpace( dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags );
    }

	if ( !inspector.init ) {
		vk_imgui_clear_inspector( qtrue );
		
		inspector.merge_shaders = true;
		inspector.outline_selected = true;
		
		// windows
		Com_Memset( &windows, 0, sizeof(windows) );
		windows.viewport.p_open = true;
		windows.shader.text_mode = true;

		// render image
		vk_imgui_bind_game_color_image();
	#ifdef USE_RTX
		vk_imgui_bind_rtx_draw_image();
		vk_imgui_bind_rtx_cvars();
	#endif

		inspector.init = qtrue;
	}

	vk_imgui_draw_bars();
	vk_imgui_draw_objects();

	vk_imgui_selected_object_handler();

	vk_imgui_draw_inspector();
	vk_imgui_draw_shader_editor();
	vk_imgui_draw_profiler();
	vk_imgui_draw_viewport();
#ifdef USE_RTX
	vk_imgui_draw_rtx_settings();
	vk_imgui_draw_rtx_feedback();
#endif

	ImGui::End();
}