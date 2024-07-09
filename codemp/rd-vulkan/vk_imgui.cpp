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
#include <vk_imgui.h>

#include "ghoul2/g2_local.h"
qboolean	G2_SetupModelPointers( CGhoul2Info_v &ghoul2 );
void		G2_Sort_Models( CGhoul2Info_v &ghoul2, int * const modelList, int modelListCapacity, int * const modelCount );
int			G2_ComputeLOD( trRefEntity_t *ent, const model_t *currentModel, int lodBias );

//
// profiler
//
double vk_imgui_profiler_get_time_secconds() {
	return double( std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::high_resolution_clock::now() - profiler.frame.current->frameStartTime ).count() ) / 1e6;
}

size_t vk_imgui_profiler_insert_task( const char *name, uint32_t color, double startTime, double endTime ) 
{
	if ( profiler.frame.current == nullptr )
		return -1;

	profilerFrame_t *frame;
	profilerTask_s	task;

	frame = profiler.frame.current;
	Com_Memset( &task, 0, sizeof(task) );

	Q_strncpyz( task.name, name, sizeof(task.name) );

	task.color		= color;
	task.startTime	= startTime;
	task.endTime	= endTime;

	frame->cpu_tasks.push_back( task );
	return frame->cpu_tasks.size();
}

size_t vk_imgui_profiler_start_task( const char *name, uint32_t color ) 
{
	return vk_imgui_profiler_insert_task( name, color, vk_imgui_profiler_get_time_secconds(), -1.0f );
}

void vk_imgui_profiler_end_task( uint32_t task_id ) {
	if ( profiler.frame.current != nullptr )
		profiler.frame.current->cpu_tasks.back().endTime = vk_imgui_profiler_get_time_secconds();
}

void vk_imgui_profiler_begin_frame( void ) 
{
#ifndef PROFILER_PREPEND_WAITMAXFPS
	if ( profiler.frame.current != nullptr )
		vk_imgui_profiler_end_task( profiler.frame.current->frame_interval_index );
#endif

	profiler.frame.current = &profiler.frames[ imguiGlobal.profiler_index++ ];
	imguiGlobal.profiler_index %= 2;

	profiler.frame.current->cpu_tasks.clear();
#ifndef PROFILER_PREPEND_WAITMAXFPS
	profiler.frame.current->frameStartTime = std::chrono::high_resolution_clock::now();
#else
	vk_imgui_profiler_insert_task( "WaitForTargetFPS", RGBA_LE(0x2c3e50ffu), 0.0, vk_imgui_profiler_get_time_secconds() );
#endif

	/*
	// display entire frame time
	ImGuiIO& io = ImGui::GetIO();
	double endTime = (1000.0f / io.Framerate) * 0.001;
	vk_imgui_profiler_insert_task( "Entire frame", RGBA_LE(0x2c3e50ffu), 0.0f, endTime );
	*/
}

void vk_imgui_profiler_end_frame( void ) 
{
#ifndef PROFILER_PREPEND_WAITMAXFPS
	profiler.frame.current->frame_interval_index = vk_imgui_profiler_start_task( "WaitForTargetFPS", RGBA_LE(0x2c3e50ffu) );
#else
	profiler.frames[ imguiGlobal.profiler_index ].frameStartTime = std::chrono::high_resolution_clock::now();
#endif

	profiler.frame.previous = profiler.frame.current;
}

static void vk_imgui_draw_profiler( void )
{
	if( !windows.profiler.p_open )
		return;

	if ( profiler.frame.previous == nullptr )
		return;

	if ( !profilersWindow.pause )
		profilersWindow.cpuGraph.LoadFrameData( profiler.frame.previous->cpu_tasks.data(), profiler.frame.previous->cpu_tasks.size() );
	
	profilersWindow.Render( &windows.profiler.p_open );	
}
//
// profiler end
//


//
// drawing methods
//
static qboolean imgui_draw_vec3_control( const char *label, vec3_t &values, float resetValue, float columnWidth ){
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

static qboolean imgui_draw_colorpicker3( const char *label, vec3_t values ){
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

static void imgui_draw_text_column( const char *label, const char *content, float columnWidth ) 
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

static qboolean imgui_draw_text_with_button( const char *label, const char *value, const char *button, float columnWidth ) 
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
//
// drawing method end
//

static inline void vk_imgui_draw_objects_node_surface( msurface_t *surf )
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
static inline void vk_imgui_draw_objects_nodes_recursive( mnode_t *node )
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
			vk_imgui_draw_objects_nodes_recursive( node->children[0] );

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

		vk_imgui_draw_objects_node_surface( surf );

		mark++;
	}
}

static void vk_imgui_draw_objects_nodes( void ) {
	ImGuiTreeNodeFlags flags;

	flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	bool parentNode = ImGui::TreeNodeEx( "Nodes", flags );

	if ( parentNode ) 
	{
		vk_imgui_draw_objects_nodes_recursive( tr.world->nodes );

		ImGui::TreePop();
	}
}

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

static void vk_imgui_draw_objects_entities( void ) {
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

static qboolean create_merge_shader_list( void ) {
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

static void vk_imgui_draw_objects_shaders( void ) {
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

static void vk_imgui_draw_objects_cubemaps( void ) {
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

static void vk_imgui_draw_objects_flares( void ) {
	uint32_t i;
	ImGuiTreeNodeFlags flags;

	if (!r_flares->integer)
		return;

	flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	bool parentNode = ImGui::TreeNodeEx( "Flares", flags );

	if ( parentNode ) 
	{
		flare_t *f;
		srfFlare_t *flare;
		bool opened, selected;

		for ( i = 0, f = r_activeFlares; f; f = f->next, i++ ) {
			if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->drawIntensity ) 
			{
				selected = false;
				flare = (srfFlare_t *)f->surface;

				flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
				
				if ( inspector.selected.ptr == flare )
					selected = true;

				if ( selected ) {
					flags |= ImGuiTreeNodeFlags_Selected ;
					ImGui::PushStyleColor( ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f) );
				}

				opened = ImGui::TreeNodeEx( (void*)flare, flags, 
					va( "[%f, %f, %f]", flare->origin[0], flare->origin[1], flare->origin[2] ) );

				// focus on object
				if ( ImGui::IsItemClicked() ) {
					inspector.selected.type = OT_FLARE;
					inspector.selected.ptr = flare;
				}

				if ( opened )
					ImGui::TreePop();

				if ( selected )
					ImGui::PopStyleColor();
			}
		}

		ImGui::TreePop();
	}
}

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
	if( ImGui::ListBoxHeader( "##Objects", ImGui::GetContentRegionAvail() ) )
	{	
		if ( tr.registered && tr.world ) {
			//vk_imgui_draw_objects_cubemaps();
			//vk_imgui_draw_objects_nodes();
			vk_imgui_draw_objects_entities();
			vk_imgui_draw_objects_flares();
		}

		vk_imgui_draw_objects_shaders();
		
		ImGui::ListBoxFooter();
	}
	ImGui::PopStyleColor();

	ImGui::End();
}

static int edtior_callback( ImGuiInputTextCallbackData *data ) {
	return 0;
}

static void vk_imgui_draw_shader_editor( void ) {

	if ( !windows.shader.p_open || !windows.shader.index )
		return;
	
	static char shaderText[4069];
	shader_t *sh = tr.shaders[ windows.shader.index ];

	if ( !sh )
		return;

	shader_t *sh_remap = sh->remappedShader;
	shader_t *sh_updated = sh->updatedShader;

	if ( windows.shader.index != windows.shader.prev ) 
	{	
		Com_Memset( &shaderText, 0, sizeof(shaderText));
		
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

		windows.shader.prev = windows.shader.index;
	}

	ImGui::Begin( "Shader", &windows.shader.p_open );

	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0, 1.0) );
	ImGui::PushStyleColor( ImGuiCol_Border, RGBA_LE(0x25c076ffu) );
	ImGui::PushStyleColor( ImGuiCol_Text, RGBA_LE(0x25c076ffu) );

	if ( ImGui::Button( ICON_FA_SYNC_ALT " Update", ImVec2{ 80, 30 } ) )
		R_UpdateShader( sh->index, shaderText, (qboolean)inspector.merge_shaders );

	ImGui::SameLine();

	ImGui::PopStyleColor(2);
	ImGui::PushStyleColor( ImGuiCol_Border, RGBA_LE(0xfe6a41ffu) );
	ImGui::PushStyleColor( ImGuiCol_Text, RGBA_LE(0xfe6a41ffu) );

	ImDrawList* drawList = ImGui::GetWindowDrawList(); 
	const ImVec2 region = ImGui::GetContentRegionAvail();
	const ImVec2 pos = ImGui::GetCursorScreenPos(); 
	float height = (GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f) + 15.0f;

	if ( region.x > 350.0f ) {
		drawList->AddRect(
			ImVec2(pos.x, pos.y), 
			ImVec2((pos.x + region.x) - (sh_updated ? 80.0f : 0), pos.y + height), 
			RGBA_LE(0x333436ffu), 2.0f,  ImDrawCornerFlags_Left, 1.0); 

		drawList->AddText( ImVec2( pos.x + 8.0f, pos.y + 8.0f ), RGBA_LE(0x44454effu), sh->name );
	}

	if( sh_updated ) {
		const ImVec2 pos = ImGui::GetCursorScreenPos(); 
		ImGui::SetCursorScreenPos( ImVec2( (pos.x + region.x) - 70.0f, pos.y ) );

		if ( ImGui::Button( ICON_FA_UNDO " Reset", ImVec2{ 70, 30 } ) ) {
			// either revert to original or remapped shader
			char *revert = NULL;

			// if possible, revert to remapped shader
			if ( sh_remap ) {
				if ( !sh_remap->shaderText ) // remove remap
					tr.shaders[ windows.shader.index ]->remappedShader = NULL;
				else
					revert = sh_remap->shaderText;
			} 
		
			if( !revert ) // use original shader
				revert = sh->shaderText;

			R_UpdateShader( sh->index, revert, (qboolean)inspector.merge_shaders );
			windows.shader.prev = NULL;
		}
	} 
	else
		ImGui::NewLine();

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);

	ImGui::PushStyleColor( ImGuiCol_Text , RGBA_LE(0x550000effu) );
	ImGui::PushStyleColor( ImGuiCol_FrameBg , ImVec4(0.7f, 0.7f, 0.7f, 1.0f) );
	ImVec2 size = ImGui::GetWindowSize();
	size.y -= 75.0f;
	size.x -= 16.0f;

	ImGui::InputTextMultiline("##source", shaderText, sizeof shaderText, size, 
		ImGuiInputTextFlags_CallbackAlways, edtior_callback);

	ImGui::PopStyleColor(2);

	ImGui::End();
}

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
			color_palette[ i % MAX_SHADER_STAGES ], 10.0f,  ImDrawCornerFlags_All, 1.0); 

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
			color_palette[ i % MAX_SHADER_STAGES ], 10.0f,  ImDrawCornerFlags_All, 1.0); 

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
			color_palette[i], 10.0f,  ImDrawCornerFlags_All, 1.0); 

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

static void vk_imgui_draw_inspector_shader( void ) {
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

static void vk_imgui_draw_inspector_transform( void ) {
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

static void vk_imgui_draw_inspector_entity( void ) {
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

static void vk_imgui_draw_inspector_node_surface( void ) {
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

static void vk_imgui_draw_inspector_node( void ) {
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

static void vk_imgui_draw_inspector( void ) {
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2{ 4.0f, 10.0f } );
	ImGui::Begin( "Inspector##Object" );

	if ( tr.registered && tr.world && inspector.selected.ptr != nullptr ) {
		vk_imgui_draw_inspector_transform();
		vk_imgui_draw_inspector_entity();
		vk_imgui_draw_inspector_node();
		vk_imgui_draw_inspector_node_surface();
	}

	vk_imgui_draw_inspector_shader();

	ImGui::End();
	ImGui::PopStyleVar();
}

static void vk_imgui_draw_render_mode( const char **modes, uint32_t num_render_modes ) {
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

static void vk_imgui_draw_viewport( void ) {
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
	vk_imgui_draw_render_mode( render_modes, IM_ARRAYSIZE( render_modes ) );

	ImGui::Image( (ImU64)inspector.render_mode.image, { (float)gls.windowWidth, (float)gls.windowHeight } );
	ImGui::End();
}

//
// bind image descriptors
//
static void vk_imgui_bind_game_color_image( void ) {
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

void vk_imgui_swapchain_restarted( void ) {
	vk_imgui_bind_game_color_image();

	glConfig.vidWidth = (int)windows.viewport.size.x;
    glConfig.vidHeight = (int)windows.viewport.size.y - 46.0f;
}

static void vk_imgui_create_gui( void ) 
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

		// render image
		vk_imgui_bind_game_color_image();

		inspector.init = qtrue;
	}

	vk_imgui_draw_bars();
	vk_imgui_draw_objects();

	vk_imgui_selected_object_handler();

	vk_imgui_draw_inspector();
	vk_imgui_draw_shader_editor();
	vk_imgui_draw_profiler();
	vk_imgui_draw_viewport();

	ImGui::End();
}

//
// ImGui implementation
//
static SDL_Window *vk_imgui_get_sdl_window( void )
{
	//return SDL_GetWindowFromID( window.id );
	return (SDL_Window*)window.sdl_handle;
}

void *R_GetImGuiContext( void  ) {
	return ImContext;
}

uint64_t R_GetImGuiTexture( qhandle_t hShader ) {
	shader_t *shader = R_GetShaderByHandle( hShader );
	image_t *image = shader->stages[0]->bundle[0].image[0];

	if ( image->descriptor_set != VK_NULL_HANDLE )
		return (uint64_t)image->descriptor_set;

	return NULL;
}

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