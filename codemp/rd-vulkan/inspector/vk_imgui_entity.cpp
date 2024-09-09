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

#include "ghoul2/g2_local.h"
#include <typeinfo>

qboolean	G2_SetupModelPointers( CGhoul2Info_v &ghoul2 );
void		G2_Sort_Models( CGhoul2Info_v &ghoul2, int * const modelList, int modelListCapacity, int * const modelCount );
int			G2_ComputeLOD( trRefEntity_t *ent, const model_t *currentModel, int lodBias );

static const char *vk_imgui_get_entity_name( trRefEntity_t *ent, model_t *model ) {
	qboolean valid_mdxm = qfalse;
	
	static char *bad = "bad";

	switch ( model->type ) {
		case MOD_MESH:
			return model->index != 0 ? model->name : vk_entitytype_string[ent->e.reType];
			break;
		case MOD_BRUSH:
			return va("brush %s", model->name);
			break;
		case MOD_MDXM:
			if ( ent->e.ghoul2 )
				valid_mdxm = qtrue;
			break;
		case MOD_BAD:		// null model axis
			if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && ( tr.viewParms.portalView == PV_NONE ) ) {
				if ( !( ent->e.renderfx & RF_SHADOW_ONLY ) )
					break;
			}

			if ( G2API_HaveWeGhoul2Models(*((CGhoul2Info_v*)ent->e.ghoul2)) )
				valid_mdxm = qtrue;
			break;
	}

	if ( valid_mdxm ) {
		CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)ent->e.ghoul2);

		if ( !ghoul2.IsValid() || !G2_SetupModelPointers( ghoul2 ) )
			return bad;

		if ( !ghoul2[0].currentModel || !ghoul2[0].animModel )
			return bad;
		
		return ghoul2[0].currentModel->name;
	}

	return bad;
}

//
//	inspector: 
//

static void vk_imgui_draw_inspector_entity_mesh( trRefEntity_t *ent, model_t *model ){
	uint32_t	i, j;
	shader_t	*surface_sh;
	ImVec2		pos, region;

	imgui_draw_text_column( "Name",			model->name, 100.0f );
	imgui_draw_text_column( "Type",			vk_modeltype_string[model->type], 100.0f );
	imgui_draw_text_column( "numLods",		va( "%d", model->numLods), 100.0f );
	imgui_draw_text_column( "numSurfaces",	va( "%d", model->data.mdv[0]->numSurfaces), 100.0f );
	
	ImGui::Dummy(ImVec2(0.0f, 15.0f));

	// TODO: add tab for every LOD
	// surfaces
	inspector.surface.surf = NULL;

	for ( i = 0; i < model->data.mdv[0]->numSurfaces; i++ ) 
	{
		mdvSurface_t *surf = &model->data.mdv[0]->surfaces[i];

		if ( !surf )
			continue;

		region = ImGui::GetContentRegionAvail();
		pos = ImGui::GetCursorScreenPos(); 
		ImGuiBeginGroupPanel( va(ICON_FA_VECTOR_SQUARE " %s", surf->name), ImVec2( -1.0f, 0.0f ) );

		// shaders
		for ( j = 0; j < surf->numShaderIndexes; j++ ) 
		{		
			surface_sh = tr.shaders[ surf->shaderIndexes[j] ];

			if ( !surface_sh )
				continue;

			if ( imgui_draw_text_with_button( va( "Shader##surf_%d%d", i,j ), surface_sh->name, ICON_FA_DOT_CIRCLE, 50.0f ) ) {
				// open in inspector
				inspector.selected.type = OT_SHADER;
				inspector.selected.ptr = surface_sh;
			}
		}

		ImGuiEndGroupPanel( color_palette[ i % 8 ] );

		ImGui::SetCursorScreenPos( ImVec2( pos.x, pos.y) );
		ImGui::Dummy( ImVec2(region.x, 45.0f));
		if ( ImGui::IsItemHovered() )
			inspector.surface.surf = (void*)surf;

		ImGui::Dummy(ImVec2(0.0f, 15.0f));
	}
}

static void vk_imgui_draw_inspector_entity_mdxm_surface( int surfaceNum, surfaceInfo_v rootSList, model_t *currentModel, shader_t *cust_shader, skin_t *skin, int lod ) 
{
	uint32_t	i;
	shader_t	*sh = NULL;
	int			offFlags;
	ImVec2		pos, region;

	mdxmHeader_t			*mdxm			= currentModel->data.glm->header;
	mdxmSurface_t			*surface		= (mdxmSurface_t *)G2_FindSurface( currentModel, surfaceNum, lod );
	mdxmHierarchyOffsets_t	*surfIndexes	= (mdxmHierarchyOffsets_t *)((byte *)mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo		= (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);

	// see if we have an override surface in the surface list
	const surfaceInfo_t	*surfOverride = G2_FindOverrideSurface( surfaceNum, rootSList );

	// really, we should use the default flags for this surface unless it's been overriden
	offFlags = surfInfo->flags;

	// set the off flags if we have some
	if ( surfOverride )
		offFlags = surfOverride->offFlags;

	// if this surface is not off, add it to the shader render list
	if ( !offFlags ) 
	{
		if ( cust_shader )
			sh = cust_shader;

		else if ( skin ) 
		{
			for ( i = 0 ; i < skin->numSurfaces; i++ )
			{
				// the names have both been lowercased
				if ( !strcmp( skin->surfaces[i]->name, surfInfo->name ) )
				{
					sh = (shader_t*)skin->surfaces[i]->shader;
					break;
				}
			}
		}

		else
			sh = R_GetShaderByHandle( surfInfo->shaderIndex );

		if ( sh ) 
		{
			region = ImGui::GetContentRegionAvail();
			pos = ImGui::GetCursorScreenPos(); 
			ImGuiBeginGroupPanel( va(ICON_FA_VECTOR_SQUARE " %s - %d", surfInfo->name, surfaceNum), ImVec2( -1.0f, 0.0f ) );

			if ( imgui_draw_text_with_button( va( "Shader##surf_%d", surfaceNum ), sh->name, ICON_FA_DOT_CIRCLE, 50.0f ) ) {
				// open in inspector
				inspector.selected.type = OT_SHADER;
				inspector.selected.ptr = sh;
			}

			ImGuiEndGroupPanel( color_palette[ surfaceNum % 8 ] );

			ImGui::SetCursorScreenPos( ImVec2( pos.x, pos.y) );
			ImGui::Dummy( ImVec2(region.x, 45.0f));
			if ( ImGui::IsItemHovered() )
				inspector.surface.surf = (void*)surfInfo;

			ImGui::Dummy( ImVec2( 0.0f, 15.0f ) );
		}
	}

	// now recursively call for the children
	for ( i = 0; i < surfInfo->numChildren; i++ ) 
	{
		vk_imgui_draw_inspector_entity_mdxm_surface( 
			surfInfo->childIndexes[i], rootSList, currentModel, 
			cust_shader, skin,lod
		);
	}
}

static void vk_imgui_draw_inspector_entity_mdxm( trRefEntity_t *ent ) 
{
	int				i, j, lod;
	ImVec2			pos, region;
	CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)ent->e.ghoul2);
	shader_t		*cust_shader;
	skin_t			*skin;

	if ( !ent->e.ghoul2 || !ghoul2.IsValid() || !G2_SetupModelPointers( ghoul2 ) )
		return;

	int	modelCount;
	int modelList[256];
	modelList[255]=548;
	G2_Sort_Models( ghoul2, modelList, ARRAY_LEN(modelList), &modelCount );

	inspector.surface.surf = NULL;

	// walk each possible model for this entity and try rendering it out
	for ( i = 0; i < modelCount; i++ )
	{
		j = modelList[i];

		if ( ghoul2[j].mValid && !(ghoul2[j].mFlags & GHOUL2_NOMODEL) && !(ghoul2[j].mFlags & GHOUL2_NORENDER) )
		{
			cust_shader = NULL;
			skin = NULL;

			if ( ent->e.customShader )
				cust_shader = R_GetShaderByHandle( ent->e.customShader );

			else
			{
				if ( ghoul2[j].mCustomSkin )	// figure out the custom skin thing
					skin = R_GetSkinByHandle( ghoul2[j].mCustomSkin );

				else if ( ent->e.customSkin )
					skin = R_GetSkinByHandle( ent->e.customSkin );

				else if ( ghoul2[j].mSkin > 0 && ghoul2[j].mSkin < tr.numSkins )
					skin = R_GetSkinByHandle( ghoul2[j].mSkin );
			}

			lod = G2_ComputeLOD( ent, ghoul2[j].currentModel, ghoul2[j].mLodBias );
			
			G2_FindOverrideSurface( -1, ghoul2[j].mSlist ); //reset the quick surface override lookup;

			model_t				*mod_m = (model_t *)ghoul2[j].currentModel;
			model_t				*mod_a = (model_t *)ghoul2[j].animModel;
			mdxmHeader_t *mdxm = mod_m->data.glm->header;
			mdxaHeader_t *mdxa = mod_a->data.gla;

			imgui_draw_text_column( "Name",			mdxm->name, 100.0f );

			if ( skin )
				imgui_draw_text_column( "Skin",		skin->name, 100.0f );

			imgui_draw_text_column( "Type",			vk_modeltype_string[mod_m->type], 100.0f );
			imgui_draw_text_column( "numLods",		va("%d", mdxm->numLODs), 100.0f );
			imgui_draw_text_column( "numSurfaces",	va("%d", mdxm->numSurfaces), 100.0f );
			imgui_draw_text_column( "numBones",		va("%d", mdxm->numBones), 100.0f );
			imgui_draw_text_column( "Anim",		mdxa->name, 100.0f );


			// recursivly walk model surface and children
			vk_imgui_draw_inspector_entity_mdxm_surface( 
				ghoul2[j].mSurfaceRoot, ghoul2[j].mSlist, (model_t *)ghoul2[j].currentModel, 
				cust_shader, skin, lod
			);
		}
	}
}

static void vk_imgui_draw_inspector_entity_brush( trRefEntity_t *ent ) {
	bmodel_t		*bmodel;
	const model_t	*model;
	int				i;
	ImVec2			pos, region;

	model = R_GetModelByHandle( ent->e.hModel );

	bmodel = model->data.bmodel;

	imgui_draw_text_column( "Name",			model->name, 100.0f );
	imgui_draw_text_column( "Type",			vk_modeltype_string[model->type], 100.0f );
	imgui_draw_text_column( "numLods",		va( "%d", model->numLods), 100.0f );
	imgui_draw_text_column( "numSurfaces",	va( "%d",bmodel->numSurfaces), 100.0f );

	ImGui::Dummy( ImVec2( 0.0f, 15.0f ) );

	inspector.surface.surf = NULL;

	for ( i = 0; i < bmodel->numSurfaces; i++ ) 
	{
		msurface_t *surf = bmodel->firstSurface + i;

		region = ImGui::GetContentRegionAvail();
		pos = ImGui::GetCursorScreenPos(); 
		ImGuiBeginGroupPanel( va(ICON_FA_VECTOR_SQUARE " %s#%d", vk_surfacetype_string[*surf->data], i ), ImVec2( -1.0f, 0.0f ) );
		
		if ( imgui_draw_text_with_button( va( "Shader##surf_%d", i ), surf->shader->name, ICON_FA_DOT_CIRCLE, 50.0f ) ) {
			inspector.selected.type = OT_SHADER;
			inspector.selected.ptr = surf->shader;
		}

		ImGuiEndGroupPanel( color_palette[ i % 8 ] );

		ImGui::SetCursorScreenPos( ImVec2( pos.x, pos.y) );
		ImGui::Dummy( ImVec2( region.x, 45.0f ) );
		if ( ImGui::IsItemHovered() )
			inspector.surface.surf = (void*)surf;

		ImGui::Dummy( ImVec2( 0.0f, 15.0f ) );
	}
}

static void vk_imgui_draw_inspector_entity_model( trRefEntity_t *ent )
{
	model_t *model = R_GetModelByHandle( ent->e.hModel );

	switch ( model->type ) {
		case MOD_MESH:
			vk_imgui_draw_inspector_entity_mesh( ent, model );
			break;
		case MOD_BRUSH:
			vk_imgui_draw_inspector_entity_brush( ent ); //R_AddBrushModelSurfaces(ent);
			break;
		case MOD_MDXM:
			vk_imgui_draw_inspector_entity_mdxm( ent );
			break;
		case MOD_BAD:		// null model axis
			if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && ( tr.viewParms.portalView == PV_NONE ) ) {
				if ( !( ent->e.renderfx & RF_SHADOW_ONLY ) )
					break;
			}

			if ( G2API_HaveWeGhoul2Models(*((CGhoul2Info_v*)ent->e.ghoul2)) )
				vk_imgui_draw_inspector_entity_mdxm( ent );
			break;
		}
}

void vk_imgui_draw_inspector_entity( void )
{
	if ( !inspector.entity.active )
		return;

	trRefEntity_t *ent = (trRefEntity_t*)inspector.selected.ptr;

	if( !ent )
		return;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
	bool opened = ImGui::TreeNodeEx((void*)typeid(inspector.entity).hash_code(), flags, ICON_FA_FILL_DRIP " Entity");

	if ( !opened )
		return;

	shader_t *sh = tr.shaders[ent->e.customShader];
	if ( sh && sh->index > 0 ) {
		if ( imgui_draw_text_with_button( "Shader", sh->name, ICON_FA_DOT_CIRCLE, 50.0f ) ) {
			if ( !windows.shader.p_open )
				windows.shader.p_open = true;

			windows.shader.index = sh->index;
		}
	}

	ImGui::PushStyleColor( ImGuiCol_TabActive, ImVec4(0.99f, 0.42f, 0.01f, 0.60f) );
	ImGui::PushStyleColor( ImGuiCol_TabUnfocusedActive, ImVec4(0.99f, 0.42f, 0.01f, 0.40f) );
		
	// draw tabbed shaders ( original, remapped, updated )
	if ( ImGui::BeginTabBar( "##EntityVisualizer", ImGuiTabBarFlags_AutoSelectNewTabs ) ) {

		// lighting
		if ( ImGui::BeginTabItem( "Lighting" ) ) 
		{
			imgui_draw_vec3_control( "lightDir", ent->lightDir, 0.0f, 100.0f );
			imgui_draw_vec3_control( "modelLightDir", ent->modelLightDir, 0.0f, 100.0f );
			imgui_draw_vec3_control( "directedLight", ent->directedLight, 0.0f, 100.0f );
			imgui_draw_vec3_control( "shadowLightDir", ent->shadowLightDir, 0.0f, 100.0f );
			
			imgui_draw_colorpicker3( "ambientLight", ent->ambientLight );

			ImGui::EndTabItem();
		}

		// model
		if ( ent->e.reType == RT_MODEL && ImGui::BeginTabItem( "Model" ) ) 
		{
			vk_imgui_draw_inspector_entity_model( ent );

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();			
	}
	ImGui::PopStyleColor(2);

	ImGui::TreePop();
}

//
// object: 
//

void vk_imgui_draw_objects_entities( void )
{
	uint32_t			i;
	trRefEntity_t		*ent;
	ImGuiTreeNodeFlags	flags;

	if ( !cl_paused->integer || !tr.world )
		return;

	flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	bool parentNode = ImGui::TreeNodeEx( "Entities", flags );

	if ( parentNode ) 
	{
		bool opened, selected;

		flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;

		for ( i = 0; i < tr.refdef.num_entities; i++ ) 
		{
			ent = &tr.refdef.entities[i];
			//ent = &backEndData->entities[i];

			if ( !ent )
				continue;

			// skip line, sprite, cylinder etc.
			if ( ent->e.reType != RT_MODEL )
				continue;

			model_t *model = R_GetModelByHandle( ent->e.hModel );

			// skip brushes
			if ( model->type == MOD_BRUSH )
				continue;

			const char *name = vk_imgui_get_entity_name( ent, model );

			if ( inspector.search_keyword != NULL ) {
				if ( !strstr( name, inspector.search_keyword ) )
					continue;
			}

			selected = false;

			if ( inspector.selected.ptr == ent )
				selected = true;

			if ( selected ) {
				flags |= ImGuiTreeNodeFlags_Selected ;
				ImGui::PushStyleColor( ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f) );
			}

			opened = ImGui::TreeNodeEx( (void*)ent, flags, name );

			// focus on object
			if ( ImGui::IsItemClicked() ) {
				inspector.selected.type = OT_ENTITY;
				inspector.selected.ptr = ent;
			}

			if ( selected )
				ImGui::PopStyleColor();

			if ( opened )
				ImGui::TreePop();
		}

		ImGui::TreePop();
	}
}