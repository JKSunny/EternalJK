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

#ifndef VK_IMGUI_H
#define VK_IMGUI_H

// need to define  ImTextureID ImU64 in imconfig.h:105
// need to typedef ImTextureID ImU64 in cimgui.h:182

// inspector object types
#define OT_CUBEMAP			( 1 )
#define OT_FLARE			( 2 )
#define OT_SHADER			( 4 )
#define OT_NODE				( 8 )
#define OT_SURFACE			( 16 )
#define OT_ENTITY			( 32 )
#ifdef USE_RTX
#define OT_RTX_LIGHT_POLY	( 64 )
#endif

#define FILE_HASH_SIZE		1024

// define if profiler starts or ends with WaitForTargetFPS frame time
#define PROFILER_PREPEND_WAITMAXFPS

#ifdef USE_RTX
enum {
#define IMG_DO( _handle, ... ) _handle##_RENDER_MODE_RTX,
	LIST_RTX_RENDER_MODES
#undef IMG_DO
	NUM_BOUND_RENDER_MODES_RTX,    
	// images bound to rtx and compute descriptors stop here

    // reset to 0, using a different variable ( rtxImage and rtxDebug )
    SHADOW_MAP_RENDER_MODE_RTX = 0,

    NUM_UNBOUND_RENDER_MODES_RTX,
    NUM_TOTAL_RENDER_MODES_RTX = NUM_BOUND_RENDER_MODES_RTX + NUM_UNBOUND_RENDER_MODES_RTX
};
#endif

typedef struct {
	qboolean	init;
	char		search_keyword[MAX_QPATH];
	bool		merge_shaders;	// merge shaders with same name and update in bulk
	int			num_shaders;
	bool		outline_selected;

	struct {
		int				index;
		VkDescriptorSet	image;	// attachment image the engine renders to

#ifdef USE_RTX
		struct {
			VkDescriptorSet bound[2];		// rtx or compute descriptor bound images
			VkDescriptorSet unbound[NUM_UNBOUND_RENDER_MODES_RTX];	// direct images
#endif
		} rtx_image;
	} render_mode;

	struct {
		uint32_t	type;
		void		*ptr;
		void		*prev;
	} selected;

	struct {
		qboolean	active;
		vec3_t		*origin;
		float		*radius;
		float		*rotation;
	} transform;

	struct {
		qboolean	active;
		int			index;

	} shader;

	struct {
		qboolean		active;
		trRefEntity_t	*ptr;
	} entity;

	struct {
		qboolean	active;
		mnode_t		*node;
	} node;

	struct {
		qboolean	active;
		void		*surf;
	} surface;

#ifdef USE_RTX
	struct {
		qboolean		active;
		light_poly_t	*light;
	} rtx_light_poly;
#endif
} vk_imgui_inspector_t;

typedef struct {
	struct {
		bool	p_open;
		ImVec2	size;
	} viewport;

	struct {
		bool	p_open;
		bool	text_mode;
		int		index;
		int		prev;
	} shader;

	struct {
		bool	p_open;
	} profiler;
} vk_imgui_window_t;

extern vk_imgui_inspector_t inspector;
extern vk_imgui_window_t	windows;

static const char *yesno[] = {"no ", "yes"};

static const ImU32 color_palette[MAX_SHADER_STAGES] = {
	RGBA_LE(0x73d2deffu),
	RGBA_LE(0x218380ffu),
	RGBA_LE(0x8f2d56ffu),
	RGBA_LE(0xd81159ffu),
	RGBA_LE(0xffbc42ffu),
	RGBA_LE(0x824670ffu),
	RGBA_LE(0xa2708affu),
	RGBA_LE(0xc1F7dCffu),
};


static const char *vk_entitytype_string[12] = { 
	"model",
	"poly",
	"sprite",
	"orientated quad",
	"beam",
	"saber glow",
	"electricity",
	"portal surface",
	"line",
	"orientated line",
	"cylinder",
	"ent chain",
};

static const char *vk_modeltype_string[5] = { 
	"bad",
	"brush",	// ignore
	"mesh",
	"mdxm / glm",
	"mdxa / gla",
};

static const char *vk_surfacetype_string[11] = { 
	"bad",
	"skip",	// ignore
	"face",
	"grid",
	"triangles",
	"poly",
	"mdv",
	"mdx",
	"flare",
	"entity",
	"vbo-mdv"
};

static const char *render_modes[23] = { 
	"Final Image", 
	"Diffuse", 
	"Specular", 
	"Roughness", 
	"Ambient Occlusion", 
	"Normals", 
	"Normals + Normalmap",  
	"Light direction",
	"View direction",
	"Tangents",
	"Light color",
	"Ambient color",
	"Reflectance",
	"Attenuation",
	"H - Half vector lightdir/viewdir",
	"Fd - CalcDiffuse",
	"Fs - CalcSpecular",
	"NdotE - Normal dot View direction",
	"NdotL - Normal dot Light direction",
	"LdotH - Light direction dot Half vector",
	"NdotH - Normal dor Half vector",
	"VdotH - View direction dot Half vector",
	"IBL Contribution"
};

#ifdef USE_RTX
static const char *rtx_render_modes[] = { 
	#define IMG_DO( _handle, _name, ... ) _name,
		LIST_RTX_RENDER_MODES
	#undef IMG_DO 
	// images bound to rtx and compute descriptors stop here

	"Shadowmap",
};
#endif

// main
void		vk_imgui_create_gui( void );

// globals
ImU32		imgui_set_alpha( const ImU32 &color, unsigned char alpha );
int			generateHashValue( const char *fname );
void		ImGuiBeginGroupPanel(const char* name, const ImVec2& size);
void		ImGuiEndGroupPanel( ImU32 border_color );
void		HelpMarker( const char* desc );
bool		imgui_draw_toggle_button( const char *label, bool *value, const char *l_icon, const char *r_icon, const ImU32 color );
qboolean	imgui_draw_vec3_control( const char *label, vec3_t &values, float resetValue, float columnWidth );
qboolean	imgui_draw_colorpicker3( const char *label, vec3_t values );
void		imgui_draw_text_column( const char *label, const char *content, float columnWidth );
qboolean	imgui_draw_text_with_button( const char *label, const char *value, const char *button, float columnWidth );

// context
void		vk_imgui_bind_game_color_image( void );

// cubemaps
void		vk_imgui_draw_objects_cubemaps( void );

// entity
void		vk_imgui_draw_objects_entities( void );
void		vk_imgui_draw_inspector_entity( void );

// flares
void		vk_imgui_draw_objects_flares( void );

// profiler
void		vk_imgui_draw_profiler( void );

// shader
void		vk_imgui_draw_objects_shaders();
void		vk_imgui_draw_inspector_shader( void );
void		vk_imgui_draw_shader_editor( void );
void		vk_imgui_reload_shader_editor( qboolean close );


// shader text editor
void		vk_imgui_shader_text_editor_initialize( void );
void		vk_imgui_draw_shader_editor_text( void );
void		vk_imgui_shader_text_editor_set_text( const char *str );

// shader node editor
void		vk_imgui_draw_shader_editor_node( void );
void		vk_imgui_parse_shader_nodes_to_text( void );

// world nodes
void		vk_imgui_draw_inspector_world_node( void );
void		vk_imgui_draw_inspector_world_node_surface( void );
void		vk_imgui_draw_objects_world_nodes( void );

#ifdef USE_RTX
// rtx
void vk_imgui_bind_rtx_cvars( void );
void vk_imgui_bind_rtx_draw_image( void );
void vk_imgui_rtx_add_unbound_texture( VkDescriptorSet *image, VkImageView view, VkSampler sampler );

void vk_imgui_draw_rtx_settings( void );
void vk_imgui_draw_rtx_feedback( void );

void vk_imgui_draw_inspector_rtx_light_poly( void );
void vk_rtx_imgui_draw_objects_lights( void );
#endif

#endif