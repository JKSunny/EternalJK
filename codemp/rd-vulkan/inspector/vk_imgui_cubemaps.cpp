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

//
//	inspector: 
//



//
// object: 
//

void vk_imgui_draw_objects_cubemaps( void ) 
{
	uint32_t i;
	ImGuiTreeNodeFlags flags;
	
	flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	bool parentNode = ImGui::TreeNodeEx( "Cubemaps", flags );

	if ( parentNode ) 
	{
		cubemap_t *cubemap;
		bool opened, selected;

		for ( i = 0; i < tr.numCubemaps; i++ )
		{
			selected = false;
			cubemap = &tr.cubemaps[i];

			if ( inspector.search_keyword != NULL && !strstr( cubemap->name, inspector.search_keyword ) )
				continue;

			flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;

			if ( inspector.selected.ptr == cubemap )
				selected = true;

			if ( selected ) {
				flags |= ImGuiTreeNodeFlags_Selected ;
				ImGui::PushStyleColor( ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f) );
			}

			opened = ImGui::TreeNodeEx( (void*)cubemap, flags, cubemap->name );

			// focus on object
			if ( ImGui::IsItemClicked() ) {
				inspector.selected.type = OT_CUBEMAP;
				inspector.selected.ptr = cubemap;
			}

			if ( opened )
				ImGui::TreePop();

			if ( selected )
				ImGui::PopStyleColor();
		}

		ImGui::TreePop();
	}
}