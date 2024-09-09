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
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_tables.cpp>
#include <inspector/vk_imgui.h>

ImVector<ImRect> s_GroupPanelLabelStack;

//
// Globals
//

int vk_imgui_get_shader_editor_index ( void ) 
{
	return inspector.shader.index;
}

shader_t *vk_imgui_get_selected_shader( void ) 
{
	return tr.shaders[inspector.shader.index];
}

static void vk_imgui_execute_cmd( const char *text )
{
	ri.Cbuf_ExecuteText( EXEC_APPEND, va( "%s\n", text ) );	
}

int vk_imgui_get_render_mode( void )
{
	return inspector.render_mode.index;
}

void *vk_imgui_get_selected_surface( void ) 
{
	return inspector.surface.surf;
}

// if enabled, merge shaders with same name and update in bulk
qboolean vk_imgui_merge_shaders( void ) 
{
	return (qboolean)inspector.merge_shaders;
}

qboolean vk_imgui_outline_selected( void ) 
{
	return (qboolean)inspector.outline_selected;
}

void HelpMarker( const char* desc )
{
    ImGui::TextDisabled( "(?)" );

    if ( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayShort ) )
    {
		ImGui::BeginTooltip();
        ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
        ImGui::TextUnformatted( desc );
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

qboolean imgui_draw_vec3_control( const char *label, vec3_t &values, float resetValue, float columnWidth ){
	// inspired by Hazel
	// https://github.com/TheCherno/Hazel/blob/master/Hazelnut/src/Panels/SceneHierarchyPanel.cpp#L109
	qboolean modified = qfalse;
	ImGuiIO& io = ImGui::GetIO();

	ImFont *boldFont = io.Fonts->Fonts[0];

	ImGui::PushID( label );

	ImGui::Columns(2);
	ImGui::SetColumnWidth( 0, columnWidth );
	ImGui::Text( label );
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths( 3, ImGui::CalcItemWidth() );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	{
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f } );
		ImGui::PushFont( boldFont );
		if ( ImGui::Button( "X", buttonSize ) ){
				values[0] = resetValue;
				modified = qtrue;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		if( ImGui::DragFloat( "##X", &values[0], 0.1f, 0.0f, 0.0f, "%.2f" ) )
			modified = qtrue;
		ImGui::PopItemWidth();
		ImGui::SameLine();
	}

	{
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f } );
		ImGui::PushFont( boldFont );
		if ( ImGui::Button( "Y", buttonSize ) ){
				values[1] = resetValue;
				modified = qtrue;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		if( ImGui::DragFloat( "##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.2f" ) )
			modified = qtrue;
		ImGui::PopItemWidth();
		ImGui::SameLine();	
	}

	{
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f } );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f } );
		ImGui::PushFont( boldFont );
		if ( ImGui::Button( "Z", buttonSize ) ){
				values[2] = resetValue;
				modified = qtrue;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor( 3 );

		ImGui::SameLine();
		if( ImGui::DragFloat( "##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.2f" ) )
			modified = qtrue;
		ImGui::PopItemWidth();	
	}

	ImGui::PopStyleVar();
	ImGui::Columns(1);
	ImGui::PopID();

	ImGui::Dummy(ImVec2(0.0f, 3.0f));

	return modified;
}

qboolean imgui_draw_colorpicker3( const char *label, vec3_t values ){
	uint32_t i;
	vec3_t temp;
	qboolean modified = qfalse;
	
	Com_Memcpy( temp, values, sizeof(vec3_t) );
	for( i = 0; i < 3; i++ )
		temp[i] /= 255;

	ImGui::PushID( label );

	if( ImGui::ColorPicker3("##picker", (float*)temp  ) ){
		for( i = 0; i < 3; i++ )
			temp[i] *= 255;

		Com_Memcpy( values, temp, sizeof(vec3_t) );
		modified = qtrue;
	}

	ImGui::PopID();

	return modified;
}

void imgui_draw_text_column( const char *label, const char *content, float columnWidth ) 
{
	if ( (int)columnWidth ) {
		ImGui::PushID( label );
		ImGui::Columns(2);
		ImGui::SetColumnWidth( 0, columnWidth );
		ImGui::Text( label );
		ImGui::NextColumn();
		ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );
	}

	ImGui::Text( content );

	if ( (int)columnWidth ) {
		ImGui::PopStyleVar();
		ImGui::Columns(1);
		ImGui::PopID();
	}
}

qboolean imgui_draw_text_with_button( const char *label, const char *value, const char *button, float columnWidth ) 
{
	qboolean modified = qfalse;

	if ( (int)columnWidth ) {
		ImGui::PushID( label );
		ImGui::Columns(2);
		ImGui::SetColumnWidth( 0, columnWidth );
		ImGui::Text( label );
		ImGui::NextColumn();
		ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList(); 
	const ImVec2 region = ImGui::GetContentRegionAvail();
	const ImVec2 pos = ImGui::GetCursorScreenPos(); 
	float height = (GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f) + 5.0f;

	drawList->AddRect(
		ImVec2(pos.x, pos.y), 
		ImVec2(pos.x + region.x, pos.y + height), 
		RGBA_LE(0x333436ffu), 2.0f,  ImDrawCornerFlags_Left, 1.0); 

	drawList->AddText( ImVec2( pos.x + 4.0f, pos.y + 5.0f ), RGBA_LE(0x44454effu), value );

	ImGui::SetCursorScreenPos( ImVec2( ((pos.x  + region.x)-height), pos.y ) );
		
	if( ImGui::Button( button, ImVec2{ height, height } ) )
		modified = qtrue;
	
	if ( (int)columnWidth ) {
		ImGui::PopStyleVar();
		ImGui::Columns(1);
		ImGui::PopID();
	}

	ImGui::Dummy( ImVec2( 0.0f, 4.0f ) );

	return modified;
}

// https://github.com/ocornut/imgui/issues/1496#issuecomment-569465473
void ImGuiBeginGroupPanel(const char* name, const ImVec2& size)
{
    ImGui::BeginGroup();

    auto cursorPos = ImGui::GetCursorScreenPos();
    auto itemSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();
    ImGui::BeginGroup();

    ImVec2 effectiveSize = size;
    if (size.x < 0.0f)
        effectiveSize.x = ImGui::GetContentRegionAvail().x;
    else
        effectiveSize.x = size.x;
    ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(name);
    auto labelMin = ImGui::GetItemRectMin();
    auto labelMax = ImGui::GetItemRectMax();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
    ImGui::BeginGroup();

    //ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x          -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x         -= frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x                   -= frameHeight;

    auto itemWidth = ImGui::CalcItemWidth();
    ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));

    s_GroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
}

void ImGuiEndGroupPanel( ImU32 border_color )
{
    ImGui::PopItemWidth();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();

    ImGui::EndGroup();

    //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 255, 0, 64), 4.0f);

    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

    ImGui::EndGroup();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();
    //ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

    auto labelRect = s_GroupPanelLabelStack.back();
    s_GroupPanelLabelStack.pop_back();

    ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
    ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));
    labelRect.Min.x -= itemSpacing.x;
    labelRect.Max.x += itemSpacing.x;
    for (int i = 0; i < 4; ++i)
    {
        switch (i)
        {
            // left half-plane
            case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true); break;
            // right half-plane
            case 1: ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true); break;
            // top
            case 2: ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true); break;
            // bottom
            case 3: ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true); break;
        }

        ImGui::GetWindowDrawList()->AddRect(
            frameRect.Min, frameRect.Max,
            border_color,
            halfFrame.x);

        ImGui::PopClipRect();
    }

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x          += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x         += frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x                   += frameHeight;

    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    ImGui::EndGroup();
}