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

#define IM_ARRAYSIZE(_ARR)          ((int)(sizeof(_ARR) / sizeof(*(_ARR)))) 

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "icons/FontAwesome5/IconsFontAwesome5.h"

static ImGuiContext *igContext;

#define ImGuiPivot_CenterBottom		((ImVec2){ 0.5f, 1.0f })
#define ImGuiPivot_LeftCenter		((ImVec2){ 0.0f, 0.5f })
#define ImGuiPivot_LeftBottom		((ImVec2){ 0.0f, 1.0f })

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

//
// Chat begin
// 
static const char *igChatColorPopupLabel = "igChatColorPopup";
static const char *igChatColorEmojiLabel = "igChatEmojiPopup";

typedef struct igChatInputData_s {
	char		*insert;
	qboolean	focus;
	qboolean	send;
	qboolean	clear;
	qboolean	close;
} igChatInputData_t;

static igChatInputData_t igChatInputUserData = { 0 };

static uint32_t igChatColorPopup( char *buf, const ImVec2 pos ) {
	uint32_t color_index = -1;

	igSetNextWindowPos( pos, ImGuiCond_Appearing, ImGuiPivot_LeftBottom );
	
	if ( igBeginPopup( igChatColorPopupLabel, ImGuiWindowFlags_NoTitleBar 
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBackground ) ) 
	{
		uint32_t i;
		float y_offset = 0;
		const int num_colors = ARRAY_LEN( igColorCodeRef );
		const ImVec2 label_size = { 20.0f, 20.0f };

		for ( i = 0; i < num_colors; i++, y_offset += 50.0f ){
			igSameLine( y_offset, 0.0f);

			if ( igColorButton( va( "##color%d", i ), igColorCodeRef[i].color, 0, label_size ) ) {
				color_index = i;
			}
		}

		igEndPopup();
	}

	return color_index;
}

static uint32_t igChatEmojiPopup( char *buf, const ImVec2 pos ) {
	uint32_t emoji_index = -1;

	igSetNextWindowPos( pos, ImGuiCond_Appearing, ImGuiPivot_LeftBottom );
	
	const uint32_t items_per_row = 20;

	const ImVec2 size = (ImVec2){24.0f, 24.0f};    
	const ImVec2 uv0 = (ImVec2){0.0f, 0.0f};                       
	const ImVec2 uv1 = (ImVec2){1.0f, 1.0f};
	const ImVec4 bg_col = (ImVec4){0.0f, 0.0f, 0.0f, 0.0f};         // Black background
	const ImVec4 tint_col = (ImVec4){1.0f, 1.0f, 1.0f, 1.0f};       // No tint

	if ( igBeginPopup( igChatColorEmojiLabel, ImGuiWindowFlags_NoTitleBar 
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBackground ) ) 
	{
		uint32_t i, row_count;
		float x_offset = 0;

		for ( i = 0, row_count = 0; i < 256; i++, row_count++ ) {
			if ( VALIDSTRING( emojis[i].name ) ) {

				if( row_count != 0 ) {
					x_offset += 30.0f;

					if ( row_count == 1 )
						x_offset += 8.0; // ?

					if ( row_count < items_per_row)
						igSameLine( x_offset, 0.0f);
					else
						row_count = x_offset = 0;
				} 

				if ( igImageButton( va( "##emoji%d", i ), (ImTextureID)emojis[i].emoji_ig, size, uv0, uv1, bg_col, tint_col ) )
					emoji_index = i;
			}
		}

		igEndPopup();
	}

	return emoji_index;
}

static void igChatInputCallback( ImGuiInputTextCallbackData* data ) 
{
	if ( data->EventFlag != ImGuiInputTextFlags_CallbackAlways )
		return;

	igChatInputData_t *input = data->UserData;

	if ( input->insert != NULL ) {
		ImGuiInputTextCallbackData_InsertChars( data, data->CursorPos, input->insert, NULL );
		input->insert = NULL;
	}

	if ( input->send ) {
		CG_igExecuteCommand( va( "say %s", data->Buf ) );
		input->send = qfalse;
		input->clear = qtrue;
		input->close = qtrue;
	}

	if ( input->clear ) {
		ImGuiInputTextCallbackData_DeleteChars( data, 0, data->BufTextLen );
		input->clear = qfalse;
	} 
}

// quickly put together chat window for demo purposes
static void CG_igShowChatInput( void ) {
	if( !cg.igShowMessagemode )
		return;
	
	ImGuiIO *io = igGetIO();

	if( io->DisplaySize.x < 1.0f && io->DisplaySize.y < 1.0f )
		return;
	
	static char message[128] = "";
	const ImVec4 border_color = { .4f, .43f, .5, 1.0 };
	ImVec2 position = { ( ( io->DisplaySize.x * 0.5f ) ), ( io->DisplaySize.y ) };	
	
	igSetNextWindowPos( position, ImGuiCond_Appearing, ImGuiPivot_CenterBottom );
	igSetNextWindowSize( (ImVec2){ 600.0f, 125.0f }, ImGuiCond_Appearing );

	igBegin( "cg_igChat", NULL, ImGuiWindowFlags_NoTitleBar 
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBackground );

	
	// replace this with a seperate font with larger scale later..
	ImFont *font = igGetDefaultFont();
	float currentFontScale = font->Scale;
	font->Scale = 1.2f;
	igPushFont( font );


	// emoji and color button
	igPushStyleColor_Vec4( ImGuiCol_Text,			border_color );
	igPushStyleColor_Vec4( ImGuiCol_Border,			border_color );
	igPushStyleColor_Vec4( ImGuiCol_Button,			(ImVec4){0,0,0,0} );
	igPushStyleColor_Vec4( ImGuiCol_ButtonHovered,	(ImVec4){0,0,0,0} );
	igPushStyleColor_Vec4( ImGuiCol_ButtonActive,	(ImVec4){0,0,0,0} );

	if( igButton( ICON_FA_LAUGH_BEAM, (ImVec2){ 30.0f, 30.0f } ) ) {
		ImGuiID popupId = igGetID_Str( igChatColorEmojiLabel );
		igOpenPopup_ID( popupId, 0 );
	}

	int emoji_index = igChatEmojiPopup( message, igContext->CurrentWindow->Pos );

	if ( emoji_index != -1 ) {
		igChatInputUserData.insert = emojis[emoji_index].name;
		igChatInputUserData.focus = qtrue;
	}

	igSameLine( 50.0f, 0.0f);
	if ( igButton( ICON_FA_PALETTE, (ImVec2){ 30.0f, 30.0f } ) ) {
		ImGuiID popupId = igGetID_Str( igChatColorPopupLabel );
		igOpenPopup_ID( popupId, 0 );
	}

	int color_index = igChatColorPopup( message, igContext->CurrentWindow->Pos );

	if ( color_index != -1 ) {
		igChatInputUserData.insert = igColorCodeRef[color_index].code;
		igChatInputUserData.focus = qtrue;
	}
	
	igPopStyleColor( 5 );

	
	// return to previous font scale
	igPopFont();
	font->Scale = currentFontScale;
	igPushFont( font );
	igPopFont();

	
	// chat input field
	igPushItemWidth( -1 );
	igPushStyleVar_Vec2( ImGuiStyleVar_FramePadding,		(ImVec2){ 15.0f, 20.0f } );
	igPushStyleVar_Float( ImGuiStyleVar_FrameRounding,		30.0f );
	igPushStyleVar_Float( ImGuiStyleVar_FrameBorderSize,	3.0f );
	igPushStyleColor_Vec4( ImGuiCol_Border,					border_color );
	igPushStyleColor_Vec4( ImGuiCol_Text,					border_color );

	if ( igChatInputUserData.focus || cg.igFocusMessagemode ) {
		igSetKeyboardFocusHere( 0 );
		igChatInputUserData.focus = qfalse;

		if ( cg.igFocusMessagemode )
			cg.igFocusMessagemode = qfalse;
	}

	ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways;
	if ( igInputText( "##cg_igChat_message", message, IM_ARRAYSIZE(message), flags, igChatInputCallback, &igChatInputUserData ) ) {
		igChatInputUserData.send = qtrue;
		igChatInputUserData.focus = qtrue;
	}
	
	igPopStyleVar( 3 );
	igPopStyleColor( 2 );

	igEnd();

	// return to game input, and close
	if ( igChatInputUserData.close ) {	
		cg.igShowMessagemode = qfalse;
		igChatInputUserData.close = qfalse;

		// clear ImGui input state 
		// game takes over next frame, andd ImGui keys are "never" released
		CG_igExecuteCommand( "in_imgui 0\n" );
		ImGuiIO_ClearInputKeys( io );
	}
}
// 
// Chat end
//

void CG_ImGuiInit( void ) {
	igContext = (ImGuiContext*)trap->R_GetImGuiContext();

	if ( !igContext || !igContext->Initialized )
		return;

	igSetCurrentContext( igContext );
}

void CG_ImGuiFrame( void ) {
	if ( !igContext || !igContext->Initialized || !igContext->WithinFrameScope )
		return;

	/*ImGuiIO *io = igGetIO();

	igBegin("cgame_module",NULL,ImGuiWindowFlags_NoTitleBar);
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
	igEnd();
	
	CG_igDrawEmotes();
	CG_igShowScoreboard();
	CG_igShowChatInput();*/
}