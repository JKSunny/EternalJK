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
#include <utils/ImGuiColorTextEdit/TextEditor.h>
#include <icons/FontAwesome5/IconsFontAwesome5.h>
#include "icons/FontAwesome5/fa5solid900.cpp"

#include "vk_imgui_shader.h"

static qboolean initialized = qfalse;

void vk_imgui_shader_text_editor_initialize( void ) 
{
	if ( initialized )
		return;

	auto lang = TextEditor::LanguageDefinition::Q3Shader();

	lang.mFormats.push_back( { 1, g_shaderGeneralFormats } );
	lang.mFormats.push_back( { 2, g_shaderStageFormats,	 } );

	text_editor.SetLanguageDefinition( lang );
	text_editor.FormatInit();

	initialized = qtrue;
}

void vk_imgui_draw_shader_editor_text( void )
{
	text_editor.Render( "##shader_text_editor" );
}

#define INDENT_SIZE 4;
static std::string auto_indentation( const std::string& str ) 
{
    uint32_t depth = 0;
    size_t pos = 0;

    std::string out;
	out.reserve( 768 );

    while ( pos < str.length() ) 
	{
        std::string line;
		line.reserve( 100 );
        while ( pos < str.length() && str[pos] != '\n' )
            line += str[pos++];


        if ( pos < str.length() && str[pos] == '\n' )
            pos++; // skip newline

        size_t start = line.find_first_not_of(" \t");
        if ( start != std::string::npos ) {
            line = line.substr( start ); // trim leading whitespace
        } else {
            // only whitespace, continue (skip empty lines)
            out += '\n';
            continue;
        }

        if ( line == "}" ) 
		{
            depth -= INDENT_SIZE;
            out += std::string( depth, ' ' ) + line + '\n';
            continue;
        }

        // concat syntx with indentation
        out += std::string(depth, ' ') + line + '\n';

        for ( char ch : line ) 
		{
            if ( ch == '{' ) {
                depth += INDENT_SIZE;
                break;
            }
        }
    }

    return out;
}
#undef INDENT_SIZE

void vk_imgui_shader_text_editor_set_text( const char *str ) 
{
	text_editor.SetText( auto_indentation( str ) );
}