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

#include <icons/FontAwesome5/IconsFontAwesome5.h>
#include "icons/FontAwesome5/fa5solid900.cpp"

#include <iostream>

#include <cmath>

// cvars
#define UBO_CVAR_DO( _handle, _value ) static float value_sun_##_handle = _value;
	UBO_CVAR_LIST
#undef UBO_CVAR_DO
static float value_pt_caustics;
static float value_pt_dof;
static float value_pt_projection;

static float value_sun_elevation;
static float value_sun_azimuth;
static float value_sun_latitude;
static float value_sun_angle;
static float value_sun_bounce;
static float value_sun_brightness;
static vec3_t value_sun_color;

static float value_sun_preset;
static float value_sun_animate;


static float value_sky_transmittance;
static float value_sky_phase_g;
static float value_sky_amb_phase_g;
static float value_sky_scattering;
static float value_physical_sky_draw_clouds;

void vk_imgui_bind_rtx_cvars( void ) 
{
	value_pt_caustics		= pt_caustics->integer;
	value_pt_dof			= pt_dof->integer;
	value_pt_projection		= pt_projection->integer;

	value_sun_elevation		= sun_elevation->integer;
	value_sun_azimuth		= sun_azimuth->integer;
	value_sun_latitude		= sun_latitude->integer;
	value_sun_angle			= sun_angle->integer;
	value_sun_bounce		= sun_bounce->integer;
	value_sun_brightness	= sun_brightness->integer;
	value_sun_color[0]		= sun_color[0]->value;
	value_sun_color[1]		= sun_color[1]->value;
	value_sun_color[2]		= sun_color[2]->value;

	value_sun_preset		= sun_preset->value;
	value_sun_animate		= sun_animate->value;

	value_sky_transmittance	= sky_transmittance->value;
	value_sky_phase_g		= sky_phase_g->value;
	value_sky_amb_phase_g	= sky_amb_phase_g->value;
	value_sky_scattering	= sky_scattering->value;

	value_physical_sky_draw_clouds	= physical_sky_draw_clouds->value;
}

const char *rtx_sun_presets[] = { 
	#define SUN_PRESET_DO( _handle, ... ) #_handle,
		LIST_SUN_PRESET
	#undef SUN_PRESET_DO
};

static void vk_imgui_draw_rtx_sun_presets( void )
{
	static const char *current_sun_preset;
	
	current_sun_preset = rtx_sun_presets[ sun_preset->integer ];

	ImGui::SetNextItemWidth( 200 );

	ImGui::PushStyleColor( ImGuiCol_FrameBg , ImVec4(0.00f, 0.00f, 0.00f, 1.00f) );
	if ( ImGui::BeginCombo( "##RTXSunPresetInspector", current_sun_preset, ImGuiComboFlags_HeightLarge ) )
	{
		for ( uint32_t n = 0; n < IM_ARRAYSIZE( rtx_sun_presets ); n++ )
		{
			bool is_selected = ( current_sun_preset == rtx_sun_presets[n] );

			if ( ImGui::Selectable( rtx_sun_presets[n], is_selected ) ) {
				ri.Cvar_Set( "sun_preset", va( "%d", n ) );
			}

			if ( is_selected )
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::PopStyleColor();
}

static void vk_imgui_draw_rtx_settings_light( void ) 
{
	qboolean modified = qfalse;

	if ( !tr.world )
		return;

	if ( sun_preset->integer == SUN_PRESET_NONE )
	{
		if ( ImGui::DragFloat( "sun_elevation", &value_sun_elevation, 0.5f, 0.0f, 0.0f, "%.2f" ) )
			ri.Cvar_Set( "sun_elevation", va( "%f", value_sun_elevation ) );

		if ( ImGui::DragFloat( "sun_azimuth", &value_sun_azimuth, 0.5f, 0.0f, 0.0f, "%.2f" ) )
			ri.Cvar_Set( "sun_azimuth", va( "%f", value_sun_azimuth ) );

		if ( ImGui::DragFloat( "sun_latitude", &value_sun_latitude, 0.5f, 0.0f, 0.0f, "%.2f" ) )
			ri.Cvar_Set( "sun_latitude", va( "%f", value_sun_latitude ) );
	}

	ImGui::Separator();

	if ( ImGui::DragFloat( "sun_angle", &value_sun_angle, 0.5f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "sun_angle", va( "%f", value_sun_angle ) );

	if ( ImGui::DragFloat( "sun_bounce", &value_sun_bounce, 0.1f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "sun_bounce", va( "%f", value_sun_bounce ) );

	if ( ImGui::DragFloat( "sun_brightness", &value_sun_brightness, 0.1f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "sun_brightness", va( "%f", value_sun_brightness ) );

	ImGui::Separator();

	if ( ImGui::DragFloat( "sky_transmittance", &value_sky_transmittance, 0.1f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "sky_transmittance", va( "%f", value_sky_transmittance ) );

	if ( ImGui::DragFloat( "sky_phase_g", &value_sky_phase_g, 0.1f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "sky_phase_g", va( "%f", value_sky_phase_g ) );
	
	if ( ImGui::DragFloat( "sky_amb_phase_g", &value_sky_amb_phase_g, 0.1f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "sky_amb_phase_g", va( "%f", value_sky_amb_phase_g ) );

	if ( ImGui::DragFloat( "sky_scattering", &value_sky_scattering, 0.1f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "sky_scattering", va( "%f", value_sky_scattering ) );

	if ( ImGui::DragFloat( "draw_clouds", &value_physical_sky_draw_clouds, 0.5f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "draw_clouds", va( "%f", value_physical_sky_draw_clouds ) );

	ImGui::Separator();

	vk_imgui_draw_rtx_sun_presets();

	if ( ImGui::DragFloat( "sun_animate", &value_sun_animate, 0.001f, 0.0f, 0.0f, "%.4f" ) )
		ri.Cvar_Set( "sun_animate", va( "%f", value_sun_animate ) );

	ImGui::Separator();

	if ( imgui_draw_colorpicker3( "ambientLight", value_sun_color ) )
	{
		ri.Cvar_Set( "sun_color_r", va( "%f", value_sun_color[0] / 255.0f ) );
		ri.Cvar_Set( "sun_color_g", va( "%f", value_sun_color[1] / 255.0f ) );
		ri.Cvar_Set( "sun_color_b", va( "%f", value_sun_color[2] / 255.0f ) );

		physical_sky_cvar_changed();	// other sun cvars are handled with modified state.
	}

	ImGui::Separator();
	if ( ImGui::Button( ICON_FA_UNDO " TEST", ImVec2{ 70, 30 } ) ) {
		vk_rtx_destroy_shaders();
	}

	ImGui::Separator();
}

static void vk_imgui_draw_rtx_settings_cvars( void )  
{
	if ( ImGui::DragFloat( "pt_caustics", &value_pt_caustics, 0.1f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "pt_caustics", va( "%f", value_pt_caustics ) );

	if ( ImGui::DragFloat( "pt_dof", &value_pt_dof, 0.1f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "pt_dof", va( "%f", value_pt_dof ) );

	if ( ImGui::DragFloat( "pt_projection", &value_pt_projection, 0.1f, 0.0f, 0.0f, "%.2f" ) )
		ri.Cvar_Set( "pt_projection", va( "%f", value_pt_projection ) );

	// created from macros, float only
	#define UBO_CVAR_DO( _handle, _value ) \
		if ( ImGui::DragFloat( #_handle, &value_sun_##_handle, 0.1f, 0.0f, 0.0f, "%.2f" ) ) \
			ri.Cvar_Set( #_handle, va( "%f", value_sun_##_handle ) );
		UBO_CVAR_LIST
	#undef UBO_CVAR_DO
}

void vk_imgui_draw_rtx_settings( void ) 
{
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2{ 4.0f, 10.0f } );
	ImGui::Begin( "RTX Settings##Object" );

	vk_imgui_draw_rtx_settings_light();
	vk_imgui_draw_rtx_settings_cvars();

	ImGui::End();
	ImGui::PopStyleVar();
}

void vk_imgui_draw_rtx_feedback( void ) 
{
	if ( !vk.rtxActive )
		return;

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2{ 4.0f, 10.0f } );
	ImGui::Begin( "RTX Feedback##Window" );

	{
		ImGui::Text( va("View cluster: %d / %d", backEnd.refdef.feedback.viewcluster, vk.numClusters) );
		ImGui::Text( va("Look at cluster: %d / %d", backEnd.refdef.feedback.lookatcluster, vk.numClusters) );

		ImGui::Separator();

		ImGui::Text( va("View material: %s", backEnd.refdef.feedback.view_material) );
		ImGui::Text( va("View material override: %s", backEnd.refdef.feedback.view_material_override) );
	
		ImGui::Separator();

		ImGui::Text( va("Resolution scale: %d", backEnd.refdef.feedback.resolution_scale) );
		ImGui::Text( va("Num light polys: %d", backEnd.refdef.feedback.num_light_polys) );

		ImGui::Separator();

		ImGui::ColorEdit4("HDR color##3", (float*)&backEnd.refdef.feedback.hdr_color, 
			ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | 
			ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker );

		ImGui::Separator();
		ImGui::Text( va("Sun luminance: %f", backEnd.refdef.feedback.sun_luminance) );
		ImGui::Text( va("Adapted luminance: %f", backEnd.refdef.feedback.adapted_luminance) );
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

//
//	inspector: 
//
void vk_imgui_draw_inspector_rtx_light_poly( void )
{
	if ( !inspector.rtx_light_poly.active )
		return;

	light_poly_t *light = inspector.rtx_light_poly.light;

	if ( !light )
		return;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
	bool opened = ImGui::TreeNodeEx((void*)typeid(inspector.shader).hash_code(), flags, ICON_FA_FILL_DRIP " Light poly");

	if ( !opened )
		return;

	ImGui::Text( va("Cluster: %d", light->cluster) );
	ImGui::Text( va("Style: %d", light->style) );

	ImGui::Separator();

	vec3_t *pos1 = (vec3_t*)light->positions + 0;
	vec3_t *pos2 = (vec3_t*)light->positions + 3;
	vec3_t *pos3 = (vec3_t*)light->positions + 6;

	imgui_draw_vec3_control( "Vertex 1", *pos1, 0.0f, 100.0f );
	imgui_draw_vec3_control( "Vertex 2", *pos2, 0.0f, 100.0f );
	imgui_draw_vec3_control( "Vertex 3", *pos3, 0.0f, 100.0f );

	ImGui::Separator();

	imgui_draw_vec3_control( "Off center", light->off_center, 0.0f, 100.0f );

	if ( imgui_draw_colorpicker3( "ambientLight", light->color ) )
	{

	}

	ImGui::TreePop();
}

//
// object: 
//
static void vk_rtx_imgui_draw_objects_lights_polys( void )
{
	uint32_t i;
	ImGuiTreeNodeFlags flags;
	light_poly_t *light;

	flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap;

	bool parentNode = ImGui::TreeNodeEx( "RTX Light polys", flags );

	if ( !parentNode ) 
		return;
	
	bool opened, selected;

	for ( int nlight = 0; nlight < tr.world->num_light_polys; nlight++ )
	{
		selected = false;
		light = tr.world->light_polys + nlight;

		if ( !light )
			break;

		flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
				
		if ( inspector.selected.ptr == light )
			selected = true;

		if ( selected ) {
			flags |= ImGuiTreeNodeFlags_Selected ;
			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4(0.99f, 0.42f, 0.01f, 1.0f) );
		}

		opened = ImGui::TreeNodeEx( (void*)light, flags, 
			va("%d", light->cluster ) );

		if ( ImGui::IsItemClicked() ) {
			inspector.selected.type = OT_RTX_LIGHT_POLY;
			inspector.selected.ptr = light;
		}

		if ( opened )
			ImGui::TreePop();

		if ( selected )
			ImGui::PopStyleColor();	
	}

	ImGui::TreePop();
}

void vk_rtx_imgui_draw_objects_lights( void ) 
{
	vk_rtx_imgui_draw_objects_lights_polys();
}