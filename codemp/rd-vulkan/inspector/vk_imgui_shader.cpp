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

#include <algorithm>
#include <iostream>
#include <regex>
#include <string>
#include <cmath>

shader_t *hashTable[FILE_HASH_SIZE];
shader_t *merge_shader_list[MAX_SHADERS];

//
//	shader editor
//
using namespace ImFlow;
TextEditor text_editor;
ImNodeFlow node_editor;

void vk_imgui_reload_shader_editor( qboolean close ) 
{
	windows.shader.prev = NULL;	// force reload
	windows.shader.p_open = (bool)close;
}

static void vk_imgui_draw_shader_editor_switch_mode( void ) 
{
	// switch to text editor
	if ( windows.shader.text_mode && node_editor.hasFlag( SHADER_NODES_UPDATE_TEXT ) )
	{
#ifdef _DEBUG
		Com_Printf( S_COLOR_YELLOW "Reload text editor\n" );
#endif
		vk_imgui_parse_shader_nodes_to_text();	
		text_editor.setTextChanged( false );
		node_editor.unsetFlag( SHADER_NODES_UPDATE_TEXT );
	}

	// switch to node editor
	else if ( text_editor.hasFlag( SHADER_TEXT_UPDATE_NODES ) ) 
	{
#ifdef _DEBUG
		Com_Printf( S_COLOR_YELLOW "Reload node editor\n" );
#endif
		node_editor.setFlag( SHADER_NODES_PARSE_TEXT );
		text_editor.unsetFlag( SHADER_TEXT_UPDATE_NODES );
	}
}

static void vk_imgui_draw_shader_editor_apply( shader_t *sh )
{
	if ( node_editor.hasFlag( SHADER_NODES_MODIFIED ) && 
		 node_editor.hasFlag( SHADER_NODES_UPDATE_TEXT ) )
	{
#ifdef _DEBUG
		Com_Printf( S_COLOR_YELLOW "Reload text editor\n" );
#endif
		vk_imgui_parse_shader_nodes_to_text();
		text_editor.setTextChanged( false );
		node_editor.unsetFlag();
	}
		
	R_UpdateShader( sh->index, text_editor.GetText().c_str(), (qboolean)inspector.merge_shaders );
	text_editor.unsetFlag( SHADER_TEXT_MODIFIED );
	node_editor.unsetFlag();
}

static void vk_imgui_draw_shader_editor_reset( shader_t *sh,  shader_t *sh_remap )
{
	char *revert = NULL;

	// use remapped shader
	if ( sh_remap ) {
		if ( !sh_remap->shaderText ) // remove remap
			tr.shaders[ windows.shader.index ]->remappedShader = NULL;
		else
			revert = sh_remap->shaderText;
	}

	// use original shader
	if ( !revert )
		revert = sh->shaderText;

	R_UpdateShader( sh->index, revert, (qboolean)inspector.merge_shaders );

	windows.shader.prev = NULL;
}

static void vk_imgui_draw_shader_editor_toolbar( shader_t *sh, shader_t *sh_remap, shader_t *sh_updated )
{
	if ( imgui_draw_toggle_button( "##shaderEditorMode", 
		&windows.shader.text_mode, ICON_FA_SITEMAP, ICON_FA_CODE, RGBA_LE(0x368b94ffu) ) )
	{
		vk_imgui_draw_shader_editor_switch_mode();
	}

	ImGui::SameLine();

	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0, 1.0) );

	qboolean modified = (qboolean)( text_editor.hasFlag( SHADER_TEXT_MODIFIED ) 
		|| node_editor.hasFlag( SHADER_NODES_MODIFIED ) );

	ImGui::PushStyleColor( ImGuiCol_Border, RGBA_LE(0x25c076ffu) );
	ImGui::PushStyleColor( ImGuiCol_Text, RGBA_LE(0x25c076ffu) );

	if ( !modified )
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}

	if ( ImGui::Button( ICON_FA_SYNC_ALT " Apply", ImVec2{ 80, 30 } ) )
		vk_imgui_draw_shader_editor_apply( sh);

	if ( !modified )
	{
		ImGui::PopItemFlag();
        ImGui::PopStyleVar();
	}

	ImGui::SameLine();

	ImGui::PopStyleColor(2);
	ImGui::PushStyleColor( ImGuiCol_Border, RGBA_LE(0xfe6a41ffu) );
	ImGui::PushStyleColor( ImGuiCol_Text, RGBA_LE(0xfe6a41ffu) );

	ImDrawList* drawList = ImGui::GetWindowDrawList(); 
	const ImVec2 region = ImGui::GetContentRegionAvail();
	const ImVec2 pos = ImGui::GetCursorScreenPos(); 
	float height = (GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f) + 15.0f;

	if ( region.x > 350.0f )
	{
		drawList->AddRect(
			ImVec2(pos.x, pos.y), 
			ImVec2((pos.x + region.x) - (sh_updated ? 80.0f : 0), pos.y + height), 
			RGBA_LE(0x333436ffu), 2.0f,  ImDrawFlags_RoundCornersLeft, 1.0); 

		drawList->AddText( ImVec2( pos.x + 8.0f, pos.y + 8.0f ), RGBA_LE(0x8f909cffu), sh->name );
	}

	if ( sh_updated ) 
	{
		const ImVec2 pos = ImGui::GetCursorScreenPos(); 
		ImGui::SetCursorScreenPos( ImVec2( (pos.x + region.x) - 70.0f, pos.y ) );

		if ( ImGui::Button( ICON_FA_UNDO " Reset", ImVec2{ 70, 30 } ) )
			vk_imgui_draw_shader_editor_reset( sh, sh_remap );
	} 
	else
		ImGui::NewLine();

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
}

void vk_imgui_draw_shader_editor( void ) 
{
	if ( !windows.shader.p_open || !windows.shader.index )
		return;
	
	// always initililze text editor, formats are shared with node editor
	vk_imgui_shader_text_editor_initialize();

	static char shaderText[4069];
	shader_t *sh = tr.shaders[ windows.shader.index ];

	if ( !sh )
		return;

	shader_t *sh_remap = sh->remappedShader;
	shader_t *sh_updated = sh->updatedShader;

	if ( windows.shader.index != windows.shader.prev ) 
	{	
		Com_Memset( &shaderText, 0, sizeof(shaderText) );
		
		// decide which shader text to display
		if ( sh_updated && sh_updated->shaderText )
			Q_strncpyz( shaderText, sh_updated->shaderText, strlen(sh_updated->shaderText) * sizeof(char*) );
		
		else if ( sh_remap && sh_remap->shaderText )
			Q_strncpyz( shaderText, sh_remap->shaderText, strlen(sh_remap->shaderText) * sizeof(char*) );
		
		else if ( sh->shaderText != NULL )
			Q_strncpyz( shaderText, sh->shaderText, strlen(sh->shaderText) * sizeof(char*) );

		else {
			windows.shader.p_open = false;
			windows.shader.index = 0;
			return;
		}

		// clear editor states
		node_editor.unsetFlag();
		text_editor.unsetFlag();

		vk_imgui_shader_text_editor_set_text( shaderText );	
		text_editor.setTextChanged( false );

		// trigger text/node update
		if ( windows.shader.text_mode )
		{
			node_editor.setFlag( SHADER_NODES_PARSE_TEXT );
			node_editor.setFlag( SHADER_NODES_UPDATE_TEXT );
		}
		else {
			text_editor.setFlag( SHADER_TEXT_UPDATE_NODES );
		}
			
		windows.shader.prev = windows.shader.index;
	}

	ImGui::Begin( "Shader", &windows.shader.p_open );

	vk_imgui_draw_shader_editor_toolbar( sh, sh_remap, sh_updated );

	if ( windows.shader.text_mode )
		vk_imgui_draw_shader_editor_text();
	else
		vk_imgui_draw_shader_editor_node();

	ImGui::End();
}

//
//	inspector
//
static void vk_imgui_draw_inspector_shader_visualize_texture( const image_t *image, const char *type ) 
{
	if ( image->descriptor_set != VK_NULL_HANDLE ) 
	{		
		ImGui::Image( (uint64_t)image->descriptor_set, ImVec2( 40.0, 40.0 ) );

		ImVec2 padding = ImVec2{0.0f, 0.0f};
		ImVec2 p0 = ImGui::GetItemRectMin();
		ImVec2 p1 = ImGui::GetItemRectMax();

		if ( ImGui::IsItemHovered() ) 
		{
			ImVec2 m = ImGui::GetCursorScreenPos();
			ImGui::SetNextWindowPos( ImVec2( m.x - 10, m.y ) );
			ImGui::SetNextWindowSize( ImVec2( 250.0f + ImGui::CalcTextSize( image->imgName ).x, 120.0f ) );
			ImGui::Begin( "##image_inspector", NULL, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize );
			
			ImGui::SameLine(0.0f, 5.0f);
			ImGui::Image( (uint64_t)image->descriptor_set, ImVec2( 100.0, 100.0 ) );

			ImGui::SameLine(110.0f, 0.0f);
			ImGui::BeginGroup();
			imgui_draw_text_column( "Type", type, 100.0f );
			imgui_draw_text_column( "Name", va("%s", image->imgName), 100.0f );
			imgui_draw_text_column( "Dimensions", va("%dx%d", image->uploadWidth,image->uploadHeight), 100.0f );
			imgui_draw_text_column( "Layers", va("%d", image->layers), 100.0f );
			imgui_draw_text_column( "Mips", va("%d", image->mipLevels), 100.0f );
			ImGui::EndGroup();

			ImGui::End();
		}

		ImGui::GetWindowDrawList()->ChannelsSetCurrent(2);
		ImGui::GetWindowDrawList()->AddRect(p0, p1, RGBA_LE(0x0f0f0fffu), 5.0f, ImDrawFlags_None, 5.0);
		ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
	}
}

static void vk_imgui_draw_inspector_shader_visualize_sky( const shader_t *sh ) {
	uint32_t i;

	if ( !sh || !sh->sky )
		return;

	ImGui::GetWindowDrawList()->ChannelsSplit( 3 );
	ImGui::GetWindowDrawList()->ChannelsSetCurrent( 1 );

	if ( sh->sun ) {
		ImGui::Text( "Sun color" );
		ImGui::SameLine();
		ImGui::ColorButton("##sunColor", 
			ImVec4{ sh->sunColor[0], sh->sunColor[1], sh->sunColor[2], 1.0f}, 
			ImGuiColorEditFlags_NoBorder, ImVec2(20, 20));
	}

	// outer box
	if ( sh->sky->outerbox[0] ) {
		ImGui::Text( "Outerbox" );

		for ( i = 0; i < 6; i++ ) {
			vk_imgui_draw_inspector_shader_visualize_texture( sh->sky->outerbox[i], va( "Outerbox: %d", i ) );
			ImGui::SameLine();
		}
	}

	ImGui::NewLine();

	// inner box
	if ( sh->sky->innerbox[0] ) {
		ImGui::Text( "Innerbox" );

		for ( i = 0; i < 6; i++ ) {
			vk_imgui_draw_inspector_shader_visualize_texture( sh->sky->innerbox[i], va( "Innerbox: %d", i ) );
			ImGui::SameLine();
		}
	}

	ImGui::NewLine();

	ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
	ImGui::GetWindowDrawList()->ChannelsMerge();
}

static void vk_imgui_draw_inspector_shader_visualize_surface_params( const char *label, const shader_t *sh, float columnWidth ) 
{
	if ( !sh || !sh->numSurfaceParams )
		return;
	
	ImDrawList	*drawList = ImGui::GetWindowDrawList(); 
	float textHeight, textWidth;
	ImVec2 region, pos, tag_pos;
	uint32_t i;

	if ( (int)columnWidth ) {
		ImGui::PushID( label );
		ImGui::Columns(2);
		ImGui::SetColumnWidth( 0, columnWidth );
		ImGui::Dummy( ImVec2(0.0, 7.0f));
		ImGui::Text( label );
		ImGui::NextColumn();
		ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );
	}

	textHeight = (GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f) + 2.0f;
	pos = tag_pos = ImGui::GetCursorScreenPos(); 
	region = ImGui::GetContentRegionAvail();

	pos.y = tag_pos.y = pos.y + 8.0f;

	for ( i = 0; i < sh->numSurfaceParams; i++ ) 
	{
		textWidth = ImGui::CalcTextSize( sh->surfaceParams[i] ).x + 8.0f;

		// new line
		if ( tag_pos.x + textWidth > ((pos.x + region.x) - 10.0f) ) {
			tag_pos.x = pos.x;
			tag_pos.y += (textHeight + 5.0f );
		}

		drawList->AddRect(
			ImVec2(tag_pos.x, tag_pos.y), 
			ImVec2(tag_pos.x + textWidth, tag_pos.y + textHeight), 
			color_palette[ i % MAX_SHADER_STAGES ], 10.0f,  ImDrawFlags_RoundCornersAll, 1.0); 

		drawList->AddText( ImVec2( tag_pos.x + 4.0f, tag_pos.y + 3.0f ), color_palette[ i % MAX_SHADER_STAGES ], sh->surfaceParams[i] );

		tag_pos.x += (textWidth + 5.0f);
	}

	tag_pos.x = pos.x;
	tag_pos.y += textHeight;
	ImGui::SetCursorScreenPos( tag_pos );
	
	if ( (int)columnWidth ) {
		ImGui::PopStyleVar();
		ImGui::Columns(1);
		ImGui::PopID();
	}

	ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x, 0.0f ) );	
}

static void vk_imgui_draw_inspector_shader_visualize_deforms( const char *label, const shader_t *sh, float columnWidth ) 
{
	if ( !sh || !sh->numDeforms )
		return;

	ImDrawList	*drawList = ImGui::GetWindowDrawList(); 
	float textHeight, textWidth;
	ImVec2 region, pos, tag_pos;
	uint32_t i;

	if ( (int)columnWidth ) {
		ImGui::PushID( label );
		ImGui::Columns(2);
		ImGui::SetColumnWidth( 0, columnWidth );
		ImGui::Dummy( ImVec2(0.0, 7.0f));
		ImGui::Text( label );
		ImGui::NextColumn();
		ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );
	}

	textHeight = (GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f) + 2.0f;
	pos = tag_pos = ImGui::GetCursorScreenPos(); 
	region = ImGui::GetContentRegionAvail();

	pos.y = tag_pos.y = pos.y + 8.0f;

	for ( i = 0; i < sh->numDeforms; i++ ) 
	{
		textWidth = ImGui::CalcTextSize( vk_deform_string[ (int)sh->deforms[i]->deformation ] ).x + 8.0f;

		// new line
		if ( tag_pos.x + textWidth > ((pos.x + region.x) - 10.0f) ) {
			tag_pos.x = pos.x;
			tag_pos.y += (textHeight + 5.0f );
		}

		drawList->AddRect(
			ImVec2(tag_pos.x, tag_pos.y), 
			ImVec2(tag_pos.x + textWidth, tag_pos.y + textHeight), 
			color_palette[ i % MAX_SHADER_STAGES ], 10.0f,  ImDrawFlags_RoundCornersAll, 1.0); 

		drawList->AddText( ImVec2( tag_pos.x + 4.0f, tag_pos.y + 3.0f ), color_palette[ i % MAX_SHADER_STAGES ], vk_deform_string[ (int)sh->deforms[i]->deformation ] );

		tag_pos.x += (textWidth + 5.0f);
	}

	tag_pos.x = pos.x;
	tag_pos.y += textHeight;
	ImGui::SetCursorScreenPos( tag_pos );
	
	if ( (int)columnWidth ) {
		ImGui::PopStyleVar();
		ImGui::Columns(1);
		ImGui::PopID();
	}

	ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x, 0.0f ) );
}

static void vk_imgui_draw_inspector_shader_visualize_tcmod( const char *label, const textureBundle_t *bundle, float columnWidth ) {
	if ( !bundle->numTexMods )
		return;
	
	ImDrawList	*drawList = ImGui::GetWindowDrawList(); 
	float textHeight, textWidth;
	ImVec2 region, pos, tag_pos;
	uint32_t i;

	if ( (int)columnWidth ) {
		ImGui::PushID( label );
		ImGui::Columns(2);
		ImGui::SetColumnWidth( 0, columnWidth );
		ImGui::Text( label );
		ImGui::NextColumn();
		ImGui::PushMultiItemsWidths( 1, ImGui::CalcItemWidth() );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 } );
	}

	textHeight = (GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f) + 2.0f;
	pos = tag_pos = ImGui::GetCursorScreenPos(); 
	region = ImGui::GetContentRegionAvail();

	pos.y = tag_pos.y = pos.y + 8.0f;

	for ( i = 0; i < bundle->numTexMods; i++ ) 
	{
		if ( bundle->texMods[i].type == TMOD_NONE )
			break;

		const char *name = vk_tcmod_string[ (int)bundle->texMods[i].type ];
		textWidth = ImGui::CalcTextSize(name).x + 8.0f;

		// new line
		if ( tag_pos.x + textWidth > ((pos.x + region.x) - 10.0f) ) {
			tag_pos.x = pos.x;
			tag_pos.y += (textHeight + 5.0f );
		}

		drawList->AddRect(
			ImVec2(tag_pos.x, tag_pos.y), 
			ImVec2(tag_pos.x + textWidth, tag_pos.y + textHeight), 
			color_palette[i], 10.0f,  ImDrawFlags_RoundCornersAll, 1.0); 

		drawList->AddText( ImVec2( tag_pos.x + 4.0f, tag_pos.y + 3.0f ), color_palette[i], name );

		tag_pos.x += (textWidth + 5.0f);
	}

	tag_pos.x = pos.x;
	tag_pos.y += textHeight;
	ImGui::SetCursorScreenPos( tag_pos );

	if ( (int)columnWidth ) {
		ImGui::PopStyleVar();
		ImGui::Columns(1);
		ImGui::PopID();
	}

	ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x - 10.0f, 0.0f ) );	
}

static void vk_imgui_draw_inspector_shader_visualize( int index ) {
	const shader_t			*sh;
	const shaderStage_t		*pStage;
	const textureBundle_t	*bundle;

	uint32_t	i, j, k;
	ImDrawList	*drawList;
	ImVec2 region, pos; 

	sh = tr.shaders[index];

	if ( !sh )
		return;

	ImGuiIO& io = ImGui::GetIO();
	drawList = ImGui::GetWindowDrawList(); 

	imgui_draw_text_column( "Name", va("%s", sh->name), 100.0f );
	imgui_draw_text_column( "Index", va("%d", sh->index), 100.0f );
	imgui_draw_text_column( "Sort", vk_sort_string[ (int)sh->sort ], 100.0f );

	// surfaceParams tags
	vk_imgui_draw_inspector_shader_visualize_surface_params( "Surface", sh, 100.0f );
	
	// deformVertexes tags
	vk_imgui_draw_inspector_shader_visualize_deforms( "Deform", sh, 100.0f );

	ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 15.0f));

	// sky only
	if ( sh->sky ) {
		vk_imgui_draw_inspector_shader_visualize_sky( sh );

		return;
	}

#ifdef USE_RTX
	// dirty hack to quickly test textures
	{
		rtx_material_t *mat;


		mat = vk_rtx_get_material( (uint32_t)sh->index );


		ImGuiBeginGroupPanel( va(ICON_FA_LAYER_GROUP " RTX material"), ImVec2( -1.0f, 0.0f ) );

		ImGui::GetWindowDrawList()->ChannelsSplit( 3 );
		ImGui::GetWindowDrawList()->ChannelsSetCurrent( 1 );

		ImGui::PushItemWidth( ImGui::GetContentRegionAvail().x );
		ImGui::BeginGroup();
		ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x-10.0f, 0.0f ) );

		ImVec2 padding = { 20.0f, 10.0f };
		ImVec2 p0 = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos( p0 + padding );

		{
			ImGui::BeginGroup();
			ImGui::Dummy( ImVec2( 0.0f, 1.0f ) );

			vk_imgui_draw_inspector_shader_visualize_texture( tr.images[mat->albedo], va( "Albedo: %d", mat->albedo ) );

			vk_imgui_draw_inspector_shader_visualize_texture( tr.images[mat->emissive], va( "Emissive: %d", mat->emissive ) );

			ImGui::EndGroup();
		}

		ImGui::EndGroup();
		ImVec2 p1 = ImGui::GetItemRectMax() + padding;
			
		ImGui::Dummy( ImVec2( 0.0f, padding.y ) );
		p0.x += 10.0f;
		p1.x -= 20.0f;
			
		ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
		ImGui::GetWindowDrawList()->AddRectFilled( p0, p1, RGBA_LE(0x0f0f0fffu), 5.0f );
		ImGui::GetWindowDrawList()->ChannelsMerge();

		ImGuiEndGroupPanel( color_palette[0] );
	}
#endif

	for ( i = 0; i < MAX_SHADER_STAGES; i++ ) 
	{
		pStage = sh->stages[i];

		if ( !pStage || !pStage->active )
			break;

		ImGuiBeginGroupPanel( va(ICON_FA_LAYER_GROUP " Stage: %d", i), ImVec2( -1.0f, 0.0f ) );

		// visualize texture bundle(s)
		for ( j = 0; j < pStage->numTexBundles; j++ ) {
			bundle = &pStage->bundle[j];
		
			ImGui::GetWindowDrawList()->ChannelsSplit( 3 );
			ImGui::GetWindowDrawList()->ChannelsSetCurrent( 1 );

			ImGui::PushItemWidth( ImGui::GetContentRegionAvail().x );
			ImGui::BeginGroup();
			ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x-10.0f, 0.0f ) );

			ImVec2 padding = { 20.0f, 10.0f };
			ImVec2 p0 = ImGui::GetCursorScreenPos();
			ImGui::SetCursorScreenPos( p0 + padding );

			if ( bundle->image[0] != NULL ) {
				ImGui::BeginGroup();
				ImGui::Dummy( ImVec2( 0.0f, 1.0f ) );

				if ( bundle->numImageAnimations ) {
					for ( k = 0; k < bundle->numImageAnimations; k++ )
						vk_imgui_draw_inspector_shader_visualize_texture( bundle->image[k], va( "Animation: %d", k ) );
				} 
				else
					vk_imgui_draw_inspector_shader_visualize_texture( bundle->image[0], "Texture map" );

				// pbr currently restricted to bundle 0
				if ( j == 0 ) {	
					if ( pStage->vk_pbr_flags & PBR_HAS_NORMALMAP )
						 vk_imgui_draw_inspector_shader_visualize_texture( pStage->normalMap, "Normal map" );

					if ( pStage->vk_pbr_flags & PBR_HAS_PHYSICALMAP )
					{
						
						const char *type = va("Physical map: [%s]", textureMapTypes[pStage->physicalMap->type].suffix + 1);

						 vk_imgui_draw_inspector_shader_visualize_texture( pStage->physicalMap, type );
					}
					else if ( pStage->vk_pbr_flags & PBR_HAS_SPECULARMAP )
						vk_imgui_draw_inspector_shader_visualize_texture( pStage->physicalMap, "Specular map" );
				}
				ImGui::EndGroup();

				ImGui::SameLine( 75.0f, 0.0f );

				ImGui::BeginGroup();

				// lightmap, glow, surfacesprite icons
				{
					region = ImGui::GetContentRegionAvail();
					pos = ImGui::GetCursorScreenPos(); 
					pos.y -= 2;
					if ( bundle->isLightmap ) {
						drawList->AddText( ImVec2( pos.x + region.x - 30.0f, pos.y ), RGBA_LE(0x44454effu), ICON_FA_MAP );
						drawList->AddText( ImVec2( pos.x + region.x - 29.0f, pos.y - 5.0f ), RGBA_LE(0xede84effu), ICON_FA_SUN );
						ImGui::SetCursorScreenPos( ImVec2( pos.x + region.x - 30.0f, pos.y) );
						ImGui::Dummy( ImVec2(20.0f, 20.0f));
						if ( ImGui::IsItemHovered() )
							ImGui::SetTooltip( "Lightmap" );

						pos.x -= 18.0f;
						ImGui::SameLine();
					}
					pos.y -= 2;

					if ( pStage->glow ) {
						drawList->AddText( ImVec2( pos.x + region.x - 30.0f, pos.y ), RGBA_LE(0xf1f1f1ffu), ICON_FA_LIGHTBULB );
						ImGui::SetCursorScreenPos( ImVec2( pos.x + region.x - 30.0f, pos.y) );
						ImGui::Dummy( ImVec2(20.0f, 20.0f));
						if ( ImGui::IsItemHovered() )
							ImGui::SetTooltip( "Glow" );
						pos.x -= 18.0f;
						ImGui::SameLine();
					}

					if ( pStage->ss && pStage->ss->surfaceSpriteType ) {
						drawList->AddText( ImVec2( pos.x + region.x - 30.0f, pos.y ), RGBA_LE(0x6cfa61ffu), ICON_FA_LEAF );
						ImGui::SetCursorScreenPos( ImVec2( pos.x + region.x - 30.0f, pos.y) );
						ImGui::Dummy( ImVec2(20.0f, 20.0f));
						if ( ImGui::IsItemHovered() )
							ImGui::SetTooltip( "Surface sprite" );

						pos.x -= 18.0f;
						ImGui::SameLine();
					}
				}

				// rgbGen and alphaGen
				const char *rgbGen = 
					va("%s %s", vk_rgbgen_string[ (int)bundle->rgbGen ], 
					( bundle->rgbGen == CGEN_WAVEFORM ) ? 
						vk_wave_func_string[ (int)bundle->rgbWave.func ] : "");

				const char *alphaGen = 
					va("%s %s", vk_alphagen_string[ (int)bundle->alphaGen ], 
					( bundle->alphaGen == AGEN_WAVEFORM ) ? 
						vk_wave_func_string[ (int)bundle->alphaWave.func ] : "");

				// if cgen isn't explicitly specified, use either identity or identitylighting
				imgui_draw_text_column( "rgbGen", rgbGen, 100.0f );
				imgui_draw_text_column( "alphaGen", alphaGen, 100.0f );
	
				// blendFunc
				if ( bundle->blendSrcBits > 0 && bundle->blendDstBits > 0 ) 
				{
					const char *srcBlend = vk_get_src_blend_strings( bundle->blendSrcBits );
					const char *dstBlend = vk_get_dst_blend_strings( bundle->blendDstBits );
					imgui_draw_text_column( "blendFunc", va( "%s %s", srcBlend, dstBlend ), 100.0f );
					
					// note: want to catch edge case?
					// tr_shader.cpp: implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
				}
				
				// tcGen
				if ( bundle->tcGen != TCGEN_TEXTURE )
					imgui_draw_text_column( "tcGen", vk_tcgen_string[ (int)bundle->tcGen ], 100.0f );

				// tcMod tags
				vk_imgui_draw_inspector_shader_visualize_tcmod( "tcMod", bundle, 0 );

				ImGui::EndGroup();
			}

			ImGui::EndGroup();
			ImVec2 p1 = ImGui::GetItemRectMax() + padding;
			
			ImGui::Dummy( ImVec2( 0.0f, padding.y ) );
			p0.x += 10.0f;
			p1.x -= 20.0f;
			
			ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
			ImGui::GetWindowDrawList()->AddRectFilled( p0, p1, RGBA_LE(0x0f0f0fffu), 5.0f );
			ImGui::GetWindowDrawList()->ChannelsMerge();
		}

		//ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x, 50.0f ) );

		ImGuiEndGroupPanel( color_palette[i] );

		ImGui::Dummy(ImVec2(0.0f, 15.0f));
	}
}

void vk_imgui_draw_inspector_shader( void ) {
	if ( !inspector.shader.active )
		return;

	shader_t *sh = tr.shaders[inspector.shader.index];

	if ( !sh )
		return;

	shader_t *sh_remap = sh->remappedShader;
	shader_t *sh_updated = sh->updatedShader;

	if ( !sh )
		return;

	qboolean updated = sh->updatedShader ? qtrue : qfalse;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
	bool opened = ImGui::TreeNodeEx((void*)typeid(inspector.shader).hash_code(), flags, ICON_FA_FILL_DRIP " Shader");

	if ( !opened )
		return;

	if ( imgui_draw_text_with_button( "Name", sh->name, ICON_FA_EDIT, 50.0f ) ) {
		if ( !windows.shader.p_open )
			windows.shader.p_open = true;

		windows.shader.index = sh->index;
	}

	if ( sh->remappedShader ) {
		if ( imgui_draw_text_with_button( "Remap", sh_remap->name, ICON_FA_TIMES, 50.0f ) )
			R_RemoveRemap( sh->index, (qboolean)inspector.merge_shaders );
	}

	ImGui::Dummy( ImVec2( 0.0f, 6.0f ) );
		
	// blue
	//ImGui::PushStyleColor( ImGuiCol_TabActive, ImVec4(0.45f, 0.82f, 0.87f, 0.60f) );
	//ImGui::PushStyleColor( ImGuiCol_TabUnfocusedActive, ImVec4(0.45f, 0.82f, 0.87f, 0.40f) );
	
	// orange
	ImGui::PushStyleColor( ImGuiCol_TabActive, ImVec4(0.99f, 0.42f, 0.01f, 0.60f) );
	ImGui::PushStyleColor( ImGuiCol_TabUnfocusedActive, ImVec4(0.99f, 0.42f, 0.01f, 0.40f) );
		
	// draw tabbed shaders ( original, remapped, updated )
	if ( ImGui::BeginTabBar( "##ShaderVisualizer", ImGuiTabBarFlags_AutoSelectNewTabs ) ) {
		if ( ImGui::BeginTabItem( "Original" ) ) {
			vk_imgui_draw_inspector_shader_visualize( sh->index );
			ImGui::EndTabItem();
		}

		if ( sh_remap && ImGui::BeginTabItem( "Remap" ) ) {
			vk_imgui_draw_inspector_shader_visualize( sh_remap->index );
			ImGui::EndTabItem();
		}

		if ( sh_updated && ImGui::BeginTabItem( "Modified" ) ) {
			vk_imgui_draw_inspector_shader_visualize( sh_updated->index );
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();			
	}
	ImGui::PopStyleColor(2);

	ImGui::TreePop();
}

//
// object
//
static qboolean create_merge_shader_list( void )
{
	int i, j, hash;
	shader_t *sh;
	qboolean skip;

	if ( !inspector.merge_shaders )
		return qfalse;

	if ( inspector.num_shaders == tr.numShaders )
		return qtrue;

	Com_Memset( merge_shader_list, 0, sizeof(merge_shader_list) );
	Com_Memset( hashTable, 0, sizeof(hashTable) );

	for ( i = 0, j = 0; i < tr.numShaders; i++ ) 
	{
		hash = generateHashValue( tr.shaders[i]->name );
		sh = hashTable[hash];
		skip = qfalse;

		while ( sh ) {
			if ( Q_stricmp( sh->name, tr.shaders[i]->name ) == 0 ) {
				skip = qtrue;
				break;
			}
			sh = sh->next;
		}

		if ( skip )
			continue;

		hashTable[hash] = tr.shaders[i];
		merge_shader_list[j] = tr.shaders[i];
		j++;
	}

	inspector.num_shaders = tr.numShaders;

	return qtrue;
}

void vk_imgui_draw_objects_shaders( void )
{
	uint32_t i;
	ImGuiTreeNodeFlags flags;
	shader_t *sh;
	shader_t **shaders = tr.shaders;

	if ( create_merge_shader_list() )
		shaders = merge_shader_list;

	flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap;

	bool parentNode = ImGui::TreeNodeEx( "Shaders", flags );

	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100.0f);	
	ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 1.0f, 1.0f ) );
	if ( ImGui::Checkbox("merge", &inspector.merge_shaders ) )
		inspector.num_shaders = 0;	// re-populate on toggle
	ImGui::PopStyleVar();
	ImGui::SameLine(); HelpMarker("Identical shader (names) contain distinct surface information, like lightmap index. Enable merge shaders to update all in bulk.");

	if ( !parentNode ) 
		return;
	
	bool opened, selected;

	for ( i = 0; i < tr.numShaders; i++ ) 
	{
		selected = false;
		sh = shaders[i];

		if ( !sh )
			break;

		if ( sh->shaderText == NULL || sh->isUpdatedShader )
			continue;

		if ( inspector.search_keyword != NULL ) {
			if ( !strstr( sh->name, inspector.search_keyword ) )
				continue;
		}

		flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
				
		if ( inspector.selected.ptr == sh )
			selected = true;

		if ( selected ) {
			flags |= ImGuiTreeNodeFlags_Selected ;
			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4(0.99f, 0.42f, 0.01f, 1.0f) );
		}

		opened = ImGui::TreeNodeEx( (void*)sh, flags, sh->name );
		
		if ( ImGui::IsItemClicked() ) {
			inspector.selected.type = OT_SHADER;
			inspector.selected.ptr = sh;
		}

		if ( sh->remappedShader ) {
			ImGui::SameLine( 14.0f );
			ImGui::PushStyleColor( ImGuiCol_Text, RGBA_LE(0xa2708affu) );
			ImGui::Text( "r" );
			ImGui::PopStyleColor();	
		}	

		if ( sh->updatedShader ) {
			ImGui::SameLine( 1.0f );
			ImGui::PushStyleColor( ImGuiCol_Text, RGBA_LE(0xfc6b03ffu) );
			ImGui::Text("*");
			ImGui::PopStyleColor();	
		}
		
		if ( opened )
			ImGui::TreePop();

		if ( selected )
			ImGui::PopStyleColor();	
	}

	ImGui::TreePop();
}