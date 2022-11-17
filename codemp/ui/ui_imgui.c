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

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

static ImGuiContext *igContext;

typedef struct igMenuRef_s {
	const char	*name;
	void		(*ref)(void);
} igMenuRef_t;

igMenuRef_t	menuRef[MAX_MENUS];
uint32_t	menuRefCount;

static void UI_igExecuteCommand( const char *text ){
	trap->Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", text ) );	
}

void layer_main( void ) {
	igBegin("main", NULL, 0);
	igText("Main menu");

	ImVec2 size = { 125.0f, 30.0f };
	if( igButton( "Load duel1", size ) )
		UI_igExecuteCommand( "map mp/duel1" );

	igEnd();
}

void layer_joinserver( void ) {
	igBegin("join", NULL, 0);
	igText("Join server");

	ImVec2 size = { 125.0f, 30.0f };
	if( igButton( "...", size ) )
		UI_igExecuteCommand( "map mp/duel1" );

	igEnd();
}
void layer_quitMenu( void ) {
	igBegin("quit", NULL, 0);
	igText("Quit game");

	ImVec2 size = { 125.0f, 30.0f };
	if( igButton( "quit", size ) )
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
		Com_Memset( &menuRef, 0, sizeof(igMenuRef_t) * 128 );
		menuRefCount = 0;
	}

	UI_igCreateMenu( "main", layer_main );
	UI_igCreateMenu( "joinserver", layer_joinserver );
	UI_igCreateMenu( "quitMenu", layer_quitMenu );
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

void UI_ImGuiFrame( void ) {
	igContext = (ImGuiContext*)trap->R_GetImGuiContext();

	if ( !igContext || !igContext->Initialized )
		return;

	igSetCurrentContext( igContext );
	ImGuiIO *io = igGetIO();

	igBegin( "ui_module",NULL,ImGuiWindowFlags_NoTitleBar );
    static float f = 0.0f;
    igText( "I'm rendering from ui" );
    igSliderFloat( "float", &f, 0.0f, 1.0f, "%.3f", 0 );
    igText( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate );
	igEnd();

	// Show loaded .menu files
	UI_igDebugMenus();
}