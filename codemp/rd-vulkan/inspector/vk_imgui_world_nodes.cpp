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
#include "vk_imgui.h"
#include <icons/FontAwesome5/IconsFontAwesome5.h>
#include "icons/FontAwesome5/fa5solid900.cpp"

#include <typeinfo>

//
//	inspector: 
//

void vk_imgui_draw_inspector_world_node_surface( void ) {
	if ( !inspector.surface.active )
		return;

	msurface_t *surf = (msurface_t*)inspector.surface.surf;

	if( !surf )
		return;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
	bool opened = ImGui::TreeNodeEx((void*)typeid(inspector.surface).hash_code(), flags, ICON_FA_MOUNTAIN " Surface");

	if ( !opened )
		return;

	ImGui::Text( vk_surfacetype_string[*surf->data] );

	shader_t *sh = surf->shader;

	if ( imgui_draw_text_with_button( "Shader", sh->name, ICON_FA_EDIT, 50.0f ) ) {
		if ( !windows.shader.p_open )
			windows.shader.p_open = true;

		windows.shader.index = sh->index;
	}

	if ( *surf->data == SF_GRID ) {
		srfGridMesh_t *cv = (srfGridMesh_t*)surf->data;

		imgui_draw_text_column( "Size", va("%d - %d ", cv->width,cv->height ), 100.0f );
	}

	if ( *surf->data == SF_TRIANGLES ) {
		srfTriangles_t *cv = (srfTriangles_t*)surf->data;

		imgui_draw_text_column( "Verts", va("%d", cv->numVerts ), 100.0f );
	}

	if ( *surf->data == SF_FACE ) {
		srfSurfaceFace_t *cv = (srfSurfaceFace_t*)surf->data;

		imgui_draw_text_column( "Points", va("%d", cv->numPoints ), 100.0f );
	}

	ImGui::TreePop();
}

void vk_imgui_draw_inspector_world_node( void ) {
	if ( !inspector.node.active )
		return;

	mnode_t *node = inspector.node.node;

	if( !node )
		return;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
	bool opened = ImGui::TreeNodeEx((void*)typeid(inspector.node).hash_code(), flags, ICON_FA_FILL_DRIP " Node");

	if ( !opened )
		return;

	imgui_draw_text_column( "Contents", va("%d", node->contents), 100.0f );
	imgui_draw_text_column( "Vis frame", va("%d", node->contents), 100.0f );
	
	imgui_draw_vec3_control( "Mins", node->mins, 0.0f, 100.0f );
	imgui_draw_vec3_control( "Maxs", node->maxs, 0.0f, 100.0f );

	imgui_draw_text_column( "nummarksurfaces", va("%d", node->nummarksurfaces), 100.0f );

	ImGui::TreePop();
}


//
// object: 
//


static inline void vk_imgui_draw_objects_world_node_surface( msurface_t *surf )
{
	const char *type;
	bool opened, selected;
	ImGuiTreeNodeFlags flags;

	if ( surf->viewCount != tr.viewCount ) {
		return;
	}

	selected = false;
	flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
	
	type = vk_surfacetype_string[*surf->data];

	if ( inspector.selected.ptr == surf )
		selected = true;

	if ( selected ) {
		flags |= ImGuiTreeNodeFlags_Selected ;
		ImGui::PushStyleColor( ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f) );
	}

	opened = ImGui::TreeNodeEx( (void*)surf, flags, type );

	// focus on object
	if ( ImGui::IsItemClicked() ) {
		inspector.selected.type = OT_SURFACE;
		inspector.selected.ptr = (void*)surf;
	}

	if ( selected )
		ImGui::PopStyleColor();

	if ( opened )
		ImGui::TreePop();
}

//recursively called for each node to go through the surfaces on that
//node and generate the wireframe map. -rww
static inline void vk_imgui_draw_objects_world_nodes_recursive( mnode_t *node )
{
	int c;
	msurface_t *surf, **mark;
	ImGuiTreeNodeFlags flags;

	if ( !node )
		return;

	while (1)
	{
		bool opened, selected;

		selected = false;

		//if ( inspector.search_keyword != NULL && !strstr( va("node%d", i), inspector.search_keyword ) )
		//	continue;

		flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;

		if ( inspector.selected.ptr == node )
			selected = true;

		if ( selected ) {
			flags |= ImGuiTreeNodeFlags_Selected ;
			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f) );
		}

		opened = ImGui::TreeNodeEx( (void*)node, flags, va("node") );

		// focus on object
		if ( ImGui::IsItemClicked() ) {
			inspector.selected.type = OT_NODE;
			inspector.selected.ptr = node;
		}

		if ( selected )
			ImGui::PopStyleColor();

		if ( !node || node->visframe != tr.visCount ) {
			if ( opened )
				ImGui::TreePop();

			return;
		}

		if ( node->contents != -1 ) {
			if ( opened )
				ImGui::TreePop();

			break;
		}

		if ( opened ) {
			vk_imgui_draw_objects_world_nodes_recursive( node->children[0] );

			ImGui::TreePop();
		}
		
		node = node->children[1];
	}

	// add the individual surfaces
	mark = node->firstmarksurface;
	c = node->nummarksurfaces;
	while ( c-- ) {
		// the surface may have already been added if it
		// spans multiple leafs
		surf = *mark;

		vk_imgui_draw_objects_world_node_surface( surf );

		mark++;
	}
}

void vk_imgui_draw_objects_world_nodes( void ) 
{
	ImGuiTreeNodeFlags flags;

	flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	bool parentNode = ImGui::TreeNodeEx( "Nodes", flags );

	if ( parentNode ) 
	{
		vk_imgui_draw_objects_world_nodes_recursive( tr.world->nodes );

		ImGui::TreePop();
	}
}