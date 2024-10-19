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
#include <utils/ImGuiColorTextEdit/TextEditor.h>
#include <utils/ImNodeFlow/include/ImNodeFlow.h>
#include <icons/FontAwesome5/IconsFontAwesome5.h>
#include "icons/FontAwesome5/fa5solid900.cpp"

#include "vk_imgui_shader.h"
#include "vk_imgui_shader_nodes.h"
#include "vk_imgui_shader_nodes_stage.h"
#include "vk_imgui_shader_nodes_general.h"

#include <algorithm>
#include <iostream>
#include <regex>
#include <string>
#include <cmath>

// to-do list

// nodes:
// sort %i override
// light %p
// surfacfesprites and params
// pbr gloss, roughness, specular.. paralax.. normalsscale, specularscale
// allow only filtered types to be connected

// other:
// tweak positioning
// tweak styling

using namespace ImFlow;
extern ImNodeFlow node_editor;

static qboolean initialized = qfalse;

static NodeFactory node_factory;	// currently only used for right-click popup menu

// create right-click popup menu on a node or grid
static void create_grid_node_popup_menu( void ) 
{
	node_editor.rightClickPopUpContent([&]( BaseNode* node )
	{
		static ImGuiTextFilter filter;
		const ImVec2 popup_position = ImGui::GetWindowPos();

		// search bar
		ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 10 );
		ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 7 );
		ImGui::Text( ICON_FA_SEARCH );
		ImGui::SameLine();
		ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 10 );

		ImGui::SetCursorPosX( ImGui::GetCursorPosX() + 5 );
		ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.0f );
		ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 10.0f, 7.0f ) );
		ImGui::PushStyleColor( ImGuiCol_Border , ImVec4(1.00f, 1.00f, 1.00f, 0.20f) );
		if ( node_editor.m_rightClickPopUpFocusSearch )
			ImGui::SetKeyboardFocusHere(); 
		filter.Draw( "##node_select", 175.0f );
		ImGui::PopStyleColor();
		ImGui::PopStyleVar( 2 );

		ImGui::Dummy(ImVec2(5.0f, 0.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(10.0f, 0.0f));

		// node items
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );
		ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0.0 );

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.27f, 0.55f, 0.67f, 1.0f } );

		ImVec2 size = ImVec2{ 210.0f, 20.0f };
		for ( const auto &node_item : node_factory.getNodes()[0] )
		{
			if ( !filter.PassFilter( node_item.second.alias.c_str() ) )
				continue;

			// hack to align button text left, cus of broken ImGuiStyleVar_ButtonTextAlign?
			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPosY( pos.y + 2.0f );
			if ( ImGui::Button( (const char*)va( "##%s", node_item.second.alias.c_str() ), size) )
			{
				auto n = node_factory.placeNodeAt( node_item.first, popup_position )->setDrawActive();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			ImGui::SetCursorPosY( pos.y + (size.y - ImGui::GetTextLineHeight()) * 0.5f);
			ImGui::SetCursorPosX( pos.x + 10 );
			ImGui::Text( node_item.second.alias.c_str() );
		}

		ImGui::PopStyleVar( 2 );
		ImGui::PopStyleColor( 2 );
	});
}

static void vk_imgui_shader_node_editor_initialize( void ) 
{
	if ( initialized )
		return;

	// register all node types in factory
	// currently only used for right-click popup menu
#define TYPE_DO( _className, _alias ) \
	node_factory.registerNode<_className> ( #_className, #_alias );
	N_NODE_TYPES
#undef TYPE_DO

	create_grid_node_popup_menu();

	initialized = qtrue;
}

static void parse_text_to_nodes( void ) {
	TextToNodesParser().parse();
}

void vk_imgui_parse_shader_nodes_to_text( void ) {
	NodesToTextParser().parse();
}

void vk_imgui_draw_shader_editor_node( void )
{
	if ( text_editor.hasFlag( SHADER_TEXT_UPDATE_NODES ) || !initialized )
	{
		node_editor.setFlag( SHADER_NODES_PARSE_TEXT );
		text_editor.unsetFlag( SHADER_TEXT_UPDATE_NODES );
	}

	vk_imgui_shader_node_editor_initialize();
	
	if ( node_editor.hasFlag( SHADER_NODES_PARSE_TEXT ) )
		parse_text_to_nodes();
	else
		node_editor.update();
}