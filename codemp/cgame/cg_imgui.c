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
#include "cg_local.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

static ImGuiContext *igContext;

static void CG_igExecuteCommand( const char *text ){
	trap->SendConsoleCommand( va( "%s\n", text ) );
}

static void CG_igDrawEmoteItem( const char *label, const char *command ){
	if ( igPieMenuItem( label, true ) )
		CG_igExecuteCommand( command );
}

static void CG_igDrawEmotes( void ) {
	if ( !cg.igShowEmotes )
		return;

	bool released = false;

	if ( cg.igHideEmotes )
		released = true;

	bool is_open = true;
	igBegin("Emotes", &is_open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground );

	ImGuiWindow *window = igGetCurrentWindow();
	ImVec2 window_size = { 1.0f, 1.0f };

	//igSetWindowSize_WindowPtr(window, (ImVec2){ 1.0f, 1.0f }, 0);

	ImGuiID popupId = igGetID_Str("PieMenu");

	igOpenPopup_ID( popupId, 0 );
	ImVec2 center = { ( cgs.glconfig.vidWidth / 2), ( cgs.glconfig.vidHeight / 2 ) };

	//https://jediacademy.fandom.com/wiki/Emoting
	if ( igBeginPiePopup( "PieMenu", center, 0, released ) ) {

		if ( igBeginPieMenu( "Interact", true ) ) {
			CG_igDrawEmoteItem( "Hello",			"amHello" );
			CG_igDrawEmoteItem( "Hug",				"amHug" );
			CG_igDrawEmoteItem( "Kiss",			"amKiss" );
			CG_igDrawEmoteItem( "Shake",			"amShake" );
			CG_igDrawEmoteItem( "Beg",				"amBeg" );
			CG_igDrawEmoteItem( "Nod",				"amNod" );

			igEndPieMenu();
		}

		if ( igBeginPieMenu( "Stance", true ) ) {
			if ( igBeginPieMenu( "Sit", true ) ) {
				CG_igDrawEmoteItem( "Meditate",	"meditate" );
				CG_igDrawEmoteItem( "Sit",			"amSit" );
				CG_igDrawEmoteItem( "Sit 2",		"amSit2" );
				CG_igDrawEmoteItem( "Sit 3",		"amSit3" );
				CG_igDrawEmoteItem( "Sit 4",		"amSit4" );
	
				igEndPieMenu();
			}

			if ( igBeginPieMenu( "Pose", true ) ) {
				CG_igDrawEmoteItem( "Wait",		"amWait" );
				CG_igDrawEmoteItem( "Hips",		"amHips" );
				CG_igDrawEmoteItem( "Neo",			"amNeo" );
				CG_igDrawEmoteItem( "Tease",		"amATease" );
				CG_igDrawEmoteItem( "Noisy",		"amNoisy" );
				CG_igDrawEmoteItem( "Sleep",		"amSleep" );	
				igEndPieMenu();
			}

			if ( igBeginPieMenu( "Battle", true ) ) {
				CG_igDrawEmoteItem( "Surrender",	"amSurrender" );
				CG_igDrawEmoteItem( "Won",			"amWon" );
				CG_igDrawEmoteItem( "Victory",		"amVictory" );
				CG_igDrawEmoteItem( "Finish",		"amFinishingHim" );
				CG_igDrawEmoteItem( "Kneel",		"amKneel" );
				igEndPieMenu();
			}

			igEndPieMenu();
		}

		if ( igBeginPieMenu( "Hilt", true ) ) {
			CG_igDrawEmoteItem( "Flip",			"amFlip" );
			CG_igDrawEmoteItem( "Throw",			"amHiltThrow1" );
			CG_igDrawEmoteItem( "Throw 2",			"amHiltThrow2" );

			igEndPieMenu();
		}

		if ( igBeginPieMenu( "Dance", true ) ) {
			CG_igDrawEmoteItem( "Dance",			"amBreakDance" );
			CG_igDrawEmoteItem( "Dance 2",			"amBreakDance2" );

			igEndPieMenu();
		}

		igEndPiePopup();
	}

	igEnd();

	if ( released ) {
		cg.igShowEmotes = qfalse;
		cg.igHideEmotes = qfalse;

		trap->Cvar_Set("in_imgui", "0");
		trap->Cvar_Update(&in_imgui);
	}
}

static void CG_igShowScoreboard( void ) {
	if( !cg.showScores )
		return;  

	uint32_t i;
	clientInfo_t	*ci;

	igBegin("scoreboard",NULL,ImGuiWindowFlags_NoTitleBar);
	igText( "name" );
	igSameLine( 250.0f, 1.0f );
	igText( "score" );
	igSameLine( 300.0f, 1.0f );
	igText( "deaths" );
	igSameLine( 350.0f, 1.0f );
	igText( "ping" );

	for ( i = 0 ; i < cg.numScores; i++ ) {
		ci = &cgs.clientinfo[cg.scores[i].client];

		igText( ci->name );
		igSameLine( 250.0f, 1.0f );
		igText( "%d", cg.scores[i].score );
		igSameLine( 300.0f, 1.0f );
		igText( "%d", cg.scores[i].deaths );
		igSameLine( 350.0f, 1.0f );
		igText( "%d", cg.scores[i].ping );
	}

	igEnd();
}

void CG_ImGuiFrame( void ) {
	igContext = (ImGuiContext*)trap->R_GetImGuiContext();

	if ( !igContext || !igContext->Initialized )
		return;

	igSetCurrentContext( igContext );
	ImGuiIO *io = igGetIO();

	igBegin("cgame_module",NULL,ImGuiWindowFlags_NoTitleBar);
    static float f = 0.0f;
    igText("I'm rendering from cgame");
    igSliderFloat("float", &f, 0.0f, 1.0f, "%.3f", 0);
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
	igEnd();
	
	CG_igDrawEmotes();
	CG_igShowScoreboard();
}