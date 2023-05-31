/*
===========================================================================
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

/*
=======================================================================

USER INTERFACE SABER LOADING & DISPLAY CODE

=======================================================================
*/

#include "ui_local.h"
#include "ui_shared.h"

#define IM_ARRAYSIZE(_ARR)          ((int)(sizeof(_ARR) / sizeof(*(_ARR))))  

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "icons/FontAwesome5/IconsFontAwesome5.h"

static ImGuiContext *igContext;

#define ImGuiPivot_CenterBottom		((ImVec2){ 0.5f, 1.0f })
#define ImGuiPivot_LeftCenter		((ImVec2){ 0.0f, 0.5f })
#define ImGuiPivot_LeftBottom		((ImVec2){ 0.0f, 1.0f })

typedef struct igMenuRef_s {
	const char	*name;
	void		(*ref)(void);
} igMenuRef_t;

igMenuRef_t	menuRef[MAX_MENUS];
uint32_t	menuRefCount;

typedef struct igColorCodesRef_s {
	char	*code;
	ImVec4	color;
} igColorCodesRef_t;

static const igColorCodesRef_t igColorCodeRef[10] = {
	S_COLOR_BLACK,		{ 0.0, 0.0, 0.0, 1.0 },	// black
	S_COLOR_RED,		{ 1.0, 0.0, 0.0, 1.0 },	// red
	S_COLOR_GREEN,		{ 0.0, 1.0, 0.0, 1.0 },	// green
	S_COLOR_YELLOW,		{ 1.0, 1.0, 0.0, 1.0 },	// yellow
	S_COLOR_BLUE,		{ 0.0, 0.0, 1.0, 1.0 },	// blue
	S_COLOR_CYAN,		{ 0.0, 1.0, 1.0, 1.0 },	// cyan
	S_COLOR_MAGENTA,	{ 1.0, 0.0, 1.0, 1.0 },	// magenta
	S_COLOR_WHITE,		{ 1.0, 1.0, 1.0, 1.0 },	// white
	S_COLOR_ORANGE,		{ 1.0, 0.5, 0.0, 1.0 }, // orange
	S_COLOR_GREY,		{ 0.5, 0.5, 0.5, 1.0 }	// md.grey
};

// create a seccond cache structure for ImTextureID/VkDescriptorSet
typedef struct cachedIgAssets_s {
	ImTextureID needPass;
	ImTextureID noForce;
	ImTextureID forceRestrict;
	ImTextureID saberOnly;
	ImTextureID trueJedi;

	ImTextureID defaultIcon;
	ImTextureID defaultIconRed;
	ImTextureID defaultIconBlue;
	ImTextureID defaultIconRGB;
} cachedIgAssets_t;

static cachedIgAssets_t cachedIgAssets;

static void UI_igExecuteCommand( const char *text ) {
	trap->Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", text ) );	
}

static void igTestImages( void ) {
    float my_tex_w = 32.0f;
    float my_tex_h = 32.0f;

	ImVec2 size = (ImVec2){32.0f, 32.0f};    
	ImVec2 uv0 = (ImVec2){0.0f, 0.0f};                        // UV coordinates for lower-left
	ImVec2 uv1 = (ImVec2){32.0f / my_tex_w, 32.0f / my_tex_h};// UV coordinates for (32,32) in our texture
	ImVec4 bg_col = (ImVec4){1.0f, 0.0f, 0.0f, 1.0f};         // Black background
	ImVec4 tint_col = (ImVec4){1.0f, 1.0f, 1.0f, 1.0f};       // No tint

	float x_offset = 0;
	if (igImageButton( "tex_needPass", cachedIgAssets.needPass, size, uv0, uv1, bg_col, tint_col))
		Com_Printf("hello\n");

	x_offset += 35.0f; igSameLine( x_offset, 0.0f);
	if (igImageButton( "tex_noForce", cachedIgAssets.noForce, size, uv0, uv1, bg_col, tint_col))
		Com_Printf("hello\n");

	x_offset += 35.0f; igSameLine( x_offset, 0.0f);
	if (igImageButton( "tex_forceRestrict", cachedIgAssets.forceRestrict, size, uv0, uv1, bg_col, tint_col))
		Com_Printf("hello\n");

	x_offset += 35.0f; igSameLine( x_offset, 0.0f);
	if (igImageButton( "tex_saberOnly", cachedIgAssets.saberOnly, size, uv0, uv1, bg_col, tint_col))
		Com_Printf("hello\n");

	x_offset += 35.0f; igSameLine( x_offset, 0.0f);
	if (igImageButton( "tex_trueJedi", cachedIgAssets.trueJedi, size, uv0, uv1, bg_col, tint_col))
		Com_Printf("hello\n");

	x_offset += 35.0f; igSameLine( x_offset, 0.0f);
	if (igImageButton( "tex_defaultIcon", cachedIgAssets.defaultIcon, size, uv0, uv1, bg_col, tint_col))
		Com_Printf("hello\n");

	x_offset += 35.0f; igSameLine( x_offset, 0.0f);
	if (igImageButton( "tex_defaultIconRed", cachedIgAssets.defaultIconRed, size, uv0, uv1, bg_col, tint_col))
		Com_Printf("hello\n");

	x_offset += 35.0f; igSameLine( x_offset, 0.0f);
	if (igImageButton( "tex_defaultIconBlue", cachedIgAssets.defaultIconBlue, size, uv0, uv1, bg_col, tint_col))
		Com_Printf("hello\n");

	x_offset += 35.0f; igSameLine( x_offset, 0.0f);
	if (igImageButton( "tex_defaultIconRGB", cachedIgAssets.defaultIconRGB, size, uv0, uv1, bg_col, tint_col))
		Com_Printf("hello\n");
}

void layer_main( void ) {
	if ( !igContext || !igContext->WithinFrameScope )
		return;

	const ImVec2 size = { 200.0f, 30.0f };
	
	igBegin("main", NULL, 0);
	igText("Main menu");

	if ( igButton( ICON_FA_JEDI, size ) )
		UI_igExecuteCommand( "map mp/duel1" );

	if ( igButton( "Refresh", size ) )
		UI_igExecuteCommand( "vid_Restart" );

	igEnd();
}

void layer_joinserver( void ) {
	if ( !igContext || !igContext->WithinFrameScope )
		return;

	igBegin("join", NULL, 0);
	igText("Join server");

	ImVec2 size = { 125.0f, 30.0f };
	if ( igButton( "...", size ) )
		UI_igExecuteCommand( "map mp/duel1" );

	igEnd();
}

void layer_quitMenu( void ) {
	if ( !igContext || !igContext->WithinFrameScope )
		return;

	igBegin("quit", NULL, 0);
	igText("Quit game");

	ImVec2 size = { 125.0f, 30.0f };
	if ( igButton( "quit", size ) )
		UI_igExecuteCommand( "quit" );

	igEnd();
}

static void UI_igCreateMenu( const char *name, void *ref ){
	igMenuRef_t *mr;

	mr = &menuRef[menuRefCount++];
	mr->name = name;
	mr->ref = ref;
}

void UI_ImGuiInitMenus( qboolean reset ) {
	if ( reset ) {
		Com_Memset( &menuRef, 0, sizeof(igMenuRef_t) * MAX_MENUS );
		menuRefCount = 0;
	}

	//UI_igCreateMenu( "main", layer_main );
	//UI_igCreateMenu( "joinserver", layer_joinserver );
	//UI_igCreateMenu( "quitMenu", layer_quitMenu );
}

void UI_ImGuiFindMenu( menuDef_t *menu ) {
	uint32_t i;

	for ( i = 0; i < menuRefCount; i++ ) {
		if ( Q_stricmp( menu->window.name, menuRef[i].name ) == 0)
			menu->igDraw = *menuRef[i].ref;
	}
}

static void UI_igDebugMenuItem( windowDef_t *window, qboolean visible ) {

	ImVec4 color = { 0.0f, 255.0f, 0.0f, 255.0f};

	if ( visible )
		igPushStyleColor_Vec4( ImGuiCol_Text, color );

	igText( "- %s", window->name );
	
	if ( visible )
		igPopStyleColor( 1 );
}

static void UI_igDebugMenus(){
	uint32_t i;
	menuDef_t *Menus = Menu_Pointer();

	igBegin( "debug_menus",NULL,ImGuiWindowFlags_NoTitleBar );

	if (Menu_Count() > 0) {
		for ( i = 0; i < Menu_Count(); i++ )
			UI_igDebugMenuItem( &Menus[i].window, (qboolean)( Menus[i].window.flags & WINDOW_VISIBLE ) );
	} 	
	else
		igText( "Empty" );

	igEnd();
}

void UI_igAssetCache( qboolean inGameLoad ) {
	Com_Memset( &cachedIgAssets, 0, sizeof(cachedIgAssets_t) );

	// Icons for various server settings.
	cachedIgAssets.needPass			= trap->R_GetImGuiTexture( uiInfo.uiDC.Assets.needPass );
	cachedIgAssets.noForce			= trap->R_GetImGuiTexture( uiInfo.uiDC.Assets.noForce );
	cachedIgAssets.forceRestrict	= trap->R_GetImGuiTexture( uiInfo.uiDC.Assets.forceRestrict );
	cachedIgAssets.saberOnly		= trap->R_GetImGuiTexture( uiInfo.uiDC.Assets.saberOnly );
	cachedIgAssets.trueJedi			= trap->R_GetImGuiTexture( uiInfo.uiDC.Assets.trueJedi );

	//default icons for profile menu
	cachedIgAssets.defaultIcon		= trap->R_GetImGuiTexture( uiInfo.uiDC.Assets.defaultIcon );
	cachedIgAssets.defaultIconRed	= trap->R_GetImGuiTexture( uiInfo.uiDC.Assets.defaultIconRed );
	cachedIgAssets.defaultIconBlue	= trap->R_GetImGuiTexture( uiInfo.uiDC.Assets.defaultIconBlue );
	cachedIgAssets.defaultIconRGB	= trap->R_GetImGuiTexture( uiInfo.uiDC.Assets.defaultIconRGB );
}

void UI_ImGuiInit( qboolean inGameLoad ) {
	igContext = (ImGuiContext*)trap->R_GetImGuiContext();

	if ( !igContext || !igContext->Initialized )
		return;

	igSetCurrentContext( igContext );

	UI_igAssetCache( inGameLoad );
}

void UI_ImGuiFrame( void ) {
	if ( !igContext || !igContext->Initialized || !igContext->WithinFrameScope )
		return;

	/*ImGuiIO *io = igGetIO();

	igBegin( "ui_module",NULL,ImGuiWindowFlags_NoTitleBar );
    igText( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate );
	igTestImages();		// Test asset rendering
	igEnd();

	// Show loaded .menu files
	UI_igDebugMenus();*/
}