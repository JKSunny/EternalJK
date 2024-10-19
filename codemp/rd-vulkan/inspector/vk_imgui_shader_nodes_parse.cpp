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
#include "vk_imgui_shader_nodes.h"
#include "vk_imgui_shader_nodes_stage.h"
#include "vk_imgui_shader_nodes_general.h"

#include <algorithm>
#include <regex>
#include <string>
#include <cmath>

//
// TODO
// 
// nodes:
// sort %i override
// light %p
// surfacfesprites and params
// pbr gloss, roughness, specular.. paralax.. normalsscale, specularscale
// add node through right-click popup menu - check defaults like blendfunc

// feature:
// tweak positioning
// tweak styling
// auto-indenting
// node-to-text parser - claenup whitespaces adn refactor

extern ImNodeFlow node_editor;

// contains general and stage nodes along with their position and height information.
struct shaderNodes {
	struct {
		Q3N_Stage	*node;
		ImVec2		position;
		float		height;
	} stage[MAX_SHADER_STAGES];	// stage nodes go down

	struct {
		float height;			// general nodes go up
	} general;

	Q3N_Shader	*shader;
};
static shaderNodes shader_nodes;	

static void clear_nodes( void )
{
	Com_Memset( &shader_nodes, 0, sizeof(shaderNodes) );

	for ( auto &node: node_editor.getNodes() ) 
	{
		node.second->setDestroyable( true );
		node.second->destroy();
	}

	node_editor.update();
}

/**
* @brief Sanitizes the input string by replacing whitespace sequences with 
*        a single space and removing leading/trailing spaces.
*
* @param in  The input string to sanitize (may contain spaces and tabs).
* @param out The output string that will contain the sanitized result.
*/
static void format_sanitize( const std::string& in, std::string& out ) 
{
	out.clear();
	out.reserve( in.size() );

	bool in_space = true; // whether we're in a space sequence

	for ( char ch : in ) 
	{
		if ( ch == '\t' ) {
			// convert tab to a single space
			if ( !in_space ) {
				out += ' ';
				in_space = true;
			}
			// rest is not added to output
		} 
		else if (std::isspace(ch)) {
			// space is not in a space sequence, add a space
			if ( !in_space ) {
				out += ' ';
				in_space = true;
			}
		} 
		else {
			// add non-space character to the output, and set not in_space
			out += ch;
			in_space = false;
		}
	}

	// remove trailing space
	if ( !out.empty() && in_space )
		out.pop_back();
};

/**
* @brief Trims trailing zeros from decimal numbers in a string.
* 
* Preserves non-numeric tokens while formatting numbers.
*
* @param str The shader text
* @return A string with formatted decimal numbers.
*/
static std::string format_decimals( const std::string& str ) 
{
	size_t length = str.length();
	std::string out;
    std::string token;

	out.reserve( length );
	token.reserve( 15 );

	const auto l_trim_decimals = []( const std::string& numberStr )
		{
			size_t dot = numberStr.find('.');

			if ( dot == std::string::npos ) return numberStr;

			std::string integer = numberStr.substr(0, dot);
			std::string decimal = numberStr.substr(dot + 1);

			while ( !decimal.empty() && decimal.back() == '0' ) 
				decimal.pop_back();

			return decimal.empty() ? integer + ".0" : integer + "." + decimal;
		};

    auto l_process_token = [&out, &l_trim_decimals]( const std::string& t ) {
			out += std::all_of( t.begin(), t.end(), []( char c ) { return std::isdigit( c ) || c == '.'; } ) 
				? l_trim_decimals( t ) : t;
		};

    for ( size_t i = 0; i < length; ++i ) 
	{
        char ch = str[i];

        if ( ch == ' ' ) {
            if ( !token.empty() ) {
                l_process_token( token );
                out += ' ';
                token.clear();
            }
        } else
            token += ch;
    }

    if ( !token.empty() ) l_process_token(token);

    if ( !out.empty() && out.back() == ' ' ) out.pop_back();

    return out;
}

/**
* @brief Formats input string by validating syntax line-by-line.
* 
* Lines with specific characters or matching regex are retained as-is;
* others are commented with "//" prefix.
*
* @param in The shader text
*/
static void format_valid_syntax( std::string& str )
{
    std::string line;
    std::string out;
	out.reserve( str.size() );
    size_t pos = 0, end;
	std::smatch sm;

	const auto l_valid_syntax = []( const TextEditor::LanguageDefinition &lang, const std::string& line, std::string &out ) {
			std::smatch sm;

			if ( line.find_first_of("{}") != std::string::npos || line.find("//") != std::string::npos ) {
				out += line;
				return;
			}

			for ( const auto& regexinfo : lang.mFormatsRegex ) {
				if ( std::regex_match( line, sm, regexinfo.regex ) ) {
					out += line;
					return;
				}
			}

			out += "//" + line;
		};

	auto lang = text_editor.GetLanguageDefinition();
    while ( ( end = str.find('\n', pos) ) != std::string::npos ) 
	{
        format_sanitize( str.substr(pos, end - pos), line );

        l_valid_syntax( lang, line, out );
		out += "\n";
        pos = end + 1;
    }
	
	line = str.substr( pos ); // last line
	if ( !line.empty() )
		l_valid_syntax( lang, line, out );

	str = out;
}

//
// Text to nodes
//
void TextToNodesParser::parse_general( RegexMatchInfo &parms )
{
	if ( !shader_nodes.shader )
		return;

	auto base_shader = shader_nodes.shader;

	std::string keyword = parms.words[0];	// first word in syntax

	const auto l_parms_shift = [&]( void )
		{
			parms.shift( 1 );
		};

	const auto l_get_position = [&]( void )
		{
			ImVec2 pos = general_nodes_position;
			pos.y -= shader_nodes.general.height;

			return pos;
		};

	const auto l_set_general_height = [&]( const float height )
		{
			shader_nodes.general.height += ( height + node_margin.y );
		};

	const auto l_parse_type = [&]( BaseNodeExt *node )
		{
			for ( size_t i = 0; i < node->types_size; ++i )
				if ( Q_stricmp( parms.words[0].c_str(), node->types[i] ) == 0 ) 
					node->setType( i );
		};

	//
	// modifier sub-node
	//
	const auto l_parse_waveform_modifier = [&]( Pin *inPin, ImVec2 pos )
	{
		pos.x -= 300.0f;

		auto node = node_editor.addNode<Q3N_waveForm>( pos );
		node->outPin( "modifier" )->createLink( inPin );

		if ( !inPin->getParent()->isTypeLessModifierSubNode() )
		{
			l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
			l_parms_shift(); // next word/token
		}

		node->parseParamValues( parms );

		node->setDrawActive();

		l_set_general_height( 100.0f );

		return qtrue;
	};

	const auto l_type_cull = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "cull" ) != 0 )
			return;

		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_cull>( pos );
		node->outPin( "cull" )->createLink( base_shader->inPin( "cull" ) );
		
		l_parms_shift(); // next word/token

		l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
		l_parms_shift(); // next word/token

		node->setDrawActive();
		l_set_general_height( 50.0f );
	};

	const auto l_type_sort = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "sort" ) != 0 )
			return;

		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_sort>( pos );
		node->outPin( "sort" )->createLink( base_shader->inPin( "sort" ) );
		
		l_parms_shift(); // next word/token

		l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
		l_parms_shift(); // next word/token

		node->setDrawActive();
		l_set_general_height( 50.0f );
	};

	const auto l_type_surface_param = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "surfaceparm" ) != 0 )
			return;

		if ( base_shader->num_surface_parms == base_shader->MAX_SURFACE_PARMS )
			return;

		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_surfaceParam>( pos );
		node->outPin( "param" )->createLink( base_shader->inPin( (const char*)va( "surfaceparm %d", base_shader->num_surface_parms ) ) );
		
		l_parms_shift(); // next word/token

		l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
		l_parms_shift(); // next word/token

		node->setDrawActive();
		base_shader->num_surface_parms++;
		l_set_general_height( 50.0f );
	};

	const auto l_type_material = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "material" ) != 0 )
			return;

		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_material>( pos );
		node->outPin( "material" )->createLink( base_shader->inPin( "material" ) );
		
		l_parms_shift(); // next word/token

		l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
		l_parms_shift(); // next word/token

		node->setDrawActive();
		l_set_general_height( 50.0f );
	};

	const auto l_type_sun = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "sun" ) != 0 
			&& Q_stricmp( keyword.c_str(), "q3map_sun" ) != 0
			&& Q_stricmp( keyword.c_str(), "q3map_sunExt" ) != 0 )
			return;

		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_sun>( pos );
		node->outPin( "sun" )->createLink( base_shader->inPin( "sun" ) );
		
		l_parms_shift(); // next word/token

		// variables
		node->parseParamValues( parms ); 

		node->setDrawActive();
		l_set_general_height( 50.0f );
	};

	const auto l_type_fogParms = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "fogParms" ) != 0 )
			return;

		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_fogParms>( pos );
		node->outPin( "fog" )->createLink( base_shader->inPin( "fog" ) );
		
		l_parms_shift(); // next word/token

		// variables
		node->parseParamValues( parms ); 

		node->setDrawActive();
		l_set_general_height( 50.0f );
	};

	const auto l_deform = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "deform" ) != 0 && Q_stricmp( keyword.c_str(), "deformVertexes" ) != 0 )
			return;

		uint32_t i;
		ImVec2 pos = l_get_position();
		pos.x -= 50.0f;

		auto node = node_editor.addNode<Q3N_deform>( pos );
		node->outPin( "deform" )->createLink( base_shader->inPin( "deform" ) );
		
		l_parms_shift(); // next word/token

		l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
		l_parms_shift(); // next word/token

		// variables
		node->parseParamValues( parms ); 

		//  sub nodes
		for (  i = 0; i < parms.size; i++)
		{
			// modifier sub node
			if ( strcmp( parms.node_uid[i].c_str(), "modifier" ) == 0 )
			{
				l_parse_waveform_modifier( node->inPin( "modifier" ), pos );
				break;
			}
		}

		node->setDrawActive();
		l_set_general_height( 50.0f );
	};	

	const auto l_type_skyParms = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "skyParms" ) != 0 )
			return;

		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_skyParms>( pos );
		node->outPin( "sky" )->createLink( base_shader->inPin( "sky" ) );
		node->setDrawActive();

		l_parms_shift(); // next word/token
		// sub node: 
		// outerbox texture
		if ( strcmp( parms.words[0].c_str(), "-") ) 
		{
			pos.x -= 300.0f;
			auto outerbox = node_editor.addNode<Q3N_Texture>( pos );
			outerbox->outPin( "texture" )->createLink( node->inPin( "outerbox" ) );
			outerbox->setString<char[MAX_QPATH * 2]>( "##texture", parms.words[0].c_str() );
			outerbox->setDrawActive();
		}

		l_parms_shift(); // next word/token
		// value: cloudHeight
		if ( strcmp( parms.words[0].c_str(), "-") ) 
			node->setValue<int>( parms.node_uid[0].c_str(), parms.words[0].c_str() );

		l_parms_shift(); // next word/token
		// sub node: 
		// innerbox texture *unused

		l_set_general_height( 75.0f );
	};

	l_type_cull();
	l_type_sort();
	l_type_surface_param();
	l_type_material();
	l_type_sun();
	l_type_fogParms();
	l_type_skyParms();
	l_deform();

	//
	// parse singular key or key:value
	//
	for ( auto value : base_shader->getValues() )
	{
		if ( Q_stricmp( keyword.c_str(), value.second->getUid() ) == 0 )
		{
			if ( value.second->getDataType() == typeid( bool ) )
				base_shader->setValue<bool>( value.second->getUid(), qtrue );

			else if ( value.second->getDataType() == typeid( float ) ) {
				l_parms_shift();
				base_shader->setValue<float>( value.second->getUid(), parms.words[0].c_str() );
			}

			else if ( value.second->getDataType() == typeid( int ) ) {
				l_parms_shift();
				base_shader->setValue<int>( value.second->getUid(), parms.words[0].c_str() );
			}

			break;
		}
	}
}

void TextToNodesParser::parse_stage( const int stage, RegexMatchInfo &parms )
{
	if ( !shader_nodes.stage[stage].node )
		return;

	auto stage_node = shader_nodes.stage[stage].node;

	std::string keyword = parms.words[0];	// first word in syntax

	//
	// globals
	//
	const auto l_parms_shift = [&]( void )
		{
			parms.shift( 1 );
		};

	const auto l_get_position = [&]( void )
		{
			ImVec2 pos = shader_nodes.stage[stage].position;
			pos.x -= node_margin.x;
			pos.y += shader_nodes.stage[stage].height;

			return pos;
		};

	const auto l_set_stage_height = [&]( const float height )
		{
			shader_nodes.stage[stage].height += ( height + node_margin.y );
		};

	const auto l_parse_type = [&]( BaseNodeExt *node )
		{
			for ( size_t i = 0; i < node->types_size; ++i )
				if ( Q_stricmp( parms.words[0].c_str(), node->types[i] ) == 0 ) 
					node->setType( i );
		};

	//
	// texture nodes
	//
	const auto l_type_normal_map = [&]( void )
	{
		for ( uint32_t i = 0; i < ARRAY_LEN( n_normal_map_types_string ); ++i )
		{
			if ( Q_stricmp( keyword.c_str(), n_normal_map_types_string[i] ) != 0 )
				continue;

			ImVec2 pos = l_get_position();

			auto map = node_editor.addNode<Q3N_NormalMap>( pos );
			map->outPin( "map" )->createLink( stage_node->inPin( "normal map" ) );
			map->setType( i );
			map->setDrawActive();

			l_parms_shift(); // next word/token

			// sub node: texture
			pos.x -= 300.0f;
			auto texture = node_editor.addNode<Q3N_Texture>( pos );
			texture->outPin( "texture" )->createLink( map->inPin( "texture" ) );
			texture->setString<char[MAX_QPATH * 2]>( "##texture", parms.words[0].c_str() );
			texture->setDrawActive();

			l_set_stage_height( 50.0f );
		}
	};

	const auto l_type_physical_map = [&]( void )
	{
		for ( uint32_t i = 0; i < ARRAY_LEN( n_physical_map_types_string ); ++i )
		{
			if ( Q_stricmp( keyword.c_str(), n_physical_map_types_string[i] ) != 0 )
				continue;

			ImVec2 pos = l_get_position();

			auto map = node_editor.addNode<Q3N_PhyiscalMap>( pos );
			map->outPin( "map" )->createLink( stage_node->inPin( "physical map" ) );
			map->setType( i );
			map->setDrawActive();

			l_parms_shift(); // next word/token

			pos.x -= 300.0f;
			auto texture = node_editor.addNode<Q3N_Texture>( pos );
			texture->outPin( "texture" )->createLink( map->inPin( "texture" ) );
			texture->setString<char[MAX_QPATH * 2]>( "##texture", parms.words[0].c_str() );
			texture->setDrawActive();

			l_set_stage_height( 50.0f );
		}
	};

	const auto l_type_map = [&]( void )
	{
		uint32_t i, j, k = 0;

		for ( i = 0; i < ARRAY_LEN( n_map_types_string ); ++i )
		{
			if ( Q_stricmp( keyword.c_str(), n_map_types_string[i] ) != 0 )
				continue;

			ImVec2 pos = l_get_position();

			auto map = node_editor.addNode<Q3N_Map>( pos );
			map->outPin( "map" )->createLink( stage_node->inPin( "map" ) );
			map->setType( i );
			map->setDrawActive();
			l_parms_shift(); // next word/token

			// variables 
			map->parseParamValues( parms );

			// sub nodes
			for (  j = 0; j < parms.size; j++)
			{
				// also matches animMap "texture(s)" edge case
				if ( strcmp( parms.node_uid[j].c_str(), "texture" ) >= 0 )
				{
					std::vector<std::string> textures;
					textures.reserve( MAX_IMAGE_ANIMATIONS );
					size_t textures_found = text_editor.split( parms.words[j], " ", textures );

					pos.x -= 300.0f;

					for ( k = 0; k < textures_found; k++ )
					{
						if ( map->num_textures == MAX_IMAGE_ANIMATIONS )
							break;

						if ( k > 0 ) {
							pos.x -= 75.0f;
							pos.y += 75.0f;
						}
						const char *uid = va("texture %d", map->num_textures );
						auto texture = node_editor.addNode<Q3N_Texture>( pos );
						texture->outPin( "texture" )->createLink( map->inPin( uid ) );
						texture->setString<char[MAX_QPATH * 2]>( "##texture", textures[k].c_str() );
						
						texture->setDrawActive();
						map->num_textures++;
					}
				}
			}

			l_set_stage_height( 100.0f + ( k * 20.0f ) );
		}
	};

	l_type_map();
	l_type_normal_map();
	l_type_physical_map();

	//
	// func nodes
	//
	const auto l_alpha_func = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "alphaFunc" ) != 0 )
			return;

		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_alphaFunc>( pos );
		node->outPin( "alpha" )->createLink( stage_node->inPin( "alpha" ) );
		
		l_parms_shift(); // next word/token

		l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );

		node->setDrawActive();
		l_set_stage_height( 50.0f );
	};

	const auto l_depth_func = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "depthFunc" ) != 0 )
			return;

		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_depthFunc>( pos );
		node->outPin( "depth" )->createLink( stage_node->inPin( "depth" ) );
		
		l_parms_shift(); // next word/token

		l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );

		l_parms_shift(); // next word/token

		node->setDrawActive();
		l_set_stage_height( 50.0f );
	};

	const auto l_blend_func = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "blendFunc" ) != 0 )
			return;

		ImVec2 pos = l_get_position();
		pos.x -= 25.0f;

		auto node = node_editor.addNode<Q3N_blendFunc>( pos );
		node->outPin( "blend" )->createLink( stage_node->inPin( "blend" ) );
		
		l_parms_shift(); // next word/token

		// edge case, blendfunc has two types.
		for ( uint32_t i = 0; i < ARRAY_LEN( n_blend_func_src_type_string ); ++i )
			if ( Q_stricmp( parms.words[0].c_str(), n_blend_func_src_type_string[i] ) == 0 ) 
				node->setCombo<nodeValueCombo>( "src", i );

		l_parms_shift(); // next word/token

		if ( parms.size )
			for ( uint32_t i = 0; i < ARRAY_LEN( n_blend_func_dst_type_string ); ++i )
				if ( Q_stricmp( parms.words[0].c_str(), n_blend_func_dst_type_string[i] ) == 0 ) 
					node->setCombo<nodeValueCombo>( "dst", i );

		node->setDrawActive();
		l_set_stage_height( 75.0f );
	};

	l_alpha_func();
	l_depth_func();
	l_blend_func();

	//
	// modifier sub-node
	//
	const auto l_parse_waveform_modifier = [&]( Pin *inPin, ImVec2 pos )
	{
		pos.x -= 300.0f;

		auto node = node_editor.addNode<Q3N_waveForm>( pos );
		node->outPin( "modifier" )->createLink( inPin );

		if ( !inPin->getParent()->isTypeLessModifierSubNode() )
		{
			l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
			l_parms_shift(); // next word/token
		}

		node->parseParamValues( parms );
		node->setDrawActive();

		l_set_stage_height( 100.0f );

		return qtrue;
	};

	//
	// gen nodes
	//
	const auto l_rgb_gen = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "rgbGen" ) != 0 )
			return;

		uint32_t i;
		ImVec2 pos = l_get_position();

		auto node = node_editor.addNode<Q3N_rgbGen>( pos );
		node->outPin( "rgbGen" )->createLink( stage_node->inPin( "rgbGen" ) );
		
		l_parms_shift(); // next word/token

		l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
		l_parms_shift(); // next word/token

		// variables
		node->parseParamValues( parms ); 

		//  sub nodes
		for ( i = 0; i < parms.size; i++)
		{
			// modifier sub node
			if ( strcmp( parms.node_uid[i].c_str(), "modifier" ) == 0 )
			{
				l_parse_waveform_modifier( node->inPin( "modifier" ), pos );
				break;
			}
		}

		node->setDrawActive();
		l_set_stage_height( 50.0f );
	};

	const auto l_alpha_gen = [&]( void )
	{
		if ( Q_stricmp( keyword.c_str(), "alphaGen" ) != 0 )
			return;

		uint32_t i;
		ImVec2 pos = l_get_position();
		pos.x -= 50.0f;

		auto node = node_editor.addNode<Q3N_alphaGen>( pos );
		node->outPin( "alphaGen" )->createLink( stage_node->inPin( "alphaGen" ) );
		
		l_parms_shift(); // next word/token

		l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
		l_parms_shift(); // next word/token

		// variables
		node->parseParamValues( parms ); 

		//  sub nodes
		for ( i = 0; i < parms.size; i++)
		{
			// modifier sub node
			if ( strcmp( parms.node_uid[i].c_str(), "modifier" ) == 0 )
			{
				l_parse_waveform_modifier( node->inPin( "modifier" ), pos );
				break;
			}
		}

		node->setDrawActive();
		l_set_stage_height( 50.0f );
	};

	const auto l_tc_gen = [&](void)
		{
			if ( Q_stricmp( keyword.c_str(), "tcGen" ) != 0 && Q_stricmp( keyword.c_str(), "texgen" ) != 0 )
				return;

			ImVec2 pos = l_get_position();
			pos.x -= 50.0f;

			auto node = node_editor.addNode<Q3N_tcGen>( pos );
			node->outPin( "tcGen" )->createLink( stage_node->inPin( "tcGen" ) );

			l_parms_shift(); // next word/token

			l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
			l_parms_shift(); // next word/token

			// variables
			node->parseParamValues( parms );

			node->setDrawActive();
			l_set_stage_height( 50.0f );
		};

	const auto l_tc_mod = [&](void)
		{
			if ( Q_stricmp( keyword.c_str(), "tcMod" ) != 0 )
				return;

			if ( stage_node->num_texmods == TR_MAX_TEXMODS )
				return;

			qboolean modifier = qfalse;
			uint32_t i;
			ImVec2 pos = l_get_position();
			pos.x -= 50.0f;

			auto node = node_editor.addNode<Q3N_tcMod>( pos );
			node->outPin( "tcMod" )->createLink( stage_node->inPin( (const char*)va( "tcMod %d", stage_node->num_texmods ) ) );

			l_parms_shift(); // next word/token

			l_parse_type( static_cast<BaseNodeExt*>( node.get() ) );
			l_parms_shift(); // next word/token

			// tcmod 'turb' edge case ( typeless waveform )
			if ( node->getType() == N_TMOD_TURBULENT )
				modifier = l_parse_waveform_modifier( node->inPin( "modifier" ), pos );

			// variables
			node->parseParamValues( parms );

			//  sub nodes
			for ( i = 0; i < parms.size; i++ )
			{
				// modifier sub node
				if ( strcmp( parms.node_uid[i].c_str(), "modifier" ) == 0 )
				{
					modifier = l_parse_waveform_modifier( node->inPin( "modifier" ), pos );
					break;
				}
			}

			node->setDrawActive();
			stage_node->num_texmods++;

			float height = 50.0f;
			if ( node->getType() == N_TMOD_SCROLL || node->getType() == N_TMOD_SCALE )
				height = 75.0f;
			else if ( node->getType() == N_TMOD_TRANSFORM )
				height = 125.0f;

			height = modifier ? 0.0f : height;

			l_set_stage_height( height );
		};

	l_rgb_gen();
	l_alpha_gen();
	l_tc_gen();
	l_tc_mod();

	//
	// parse singular key or key:value
	//
	for ( auto value : stage_node->getValues() )
	{
		if ( Q_stricmp( keyword.c_str(), value.second->getUid() ) == 0 )
		{
			if ( value.second->getDataType() == typeid( bool ) )
				stage_node->setValue<bool>( value.second->getUid(), qtrue );

			else if ( value.second->getDataType() == typeid( float ) ) {
				l_parms_shift();
				stage_node->setValue<float>( value.second->getUid(), parms.words[0].c_str() );
			}

			else if ( value.second->getDataType() == typeid( int ) ) {
				l_parms_shift();
				stage_node->setValue<int>( value.second->getUid(), parms.words[0].c_str() );
			}

			break;
		}
	}
}

void TextToNodesParser::parse( void ) 
{
	if ( !node_editor.hasFlag( SHADER_NODES_PARSE_TEXT ) )
		return; 

	clear_nodes();

	auto base_shader = node_editor.addNode<Q3N_Shader>( shader_node_position );
	base_shader->setDrawActive();
	shader_nodes.shader = base_shader.get();

	uint32_t i, j;
	auto lines = text_editor.GetTextLines();
	auto lang = text_editor.GetLanguageDefinition();

	int depth = 0;
	int stage = 0;

	for ( i = 0; i < lines.size(); ++i )
	{
		// handle depth and stage nodes
		if ( lines[i].find("{") != std::string::npos )
		{
			if ( depth == 1 )
			{
				ImVec2 pos = stage_node_position;

				if ( stage >= MAX_SHADER_STAGES)
					break;

				if ( stage > 0 )
					pos.y = shader_nodes.stage[( stage - 1 )].position.y + shader_nodes.stage[( stage - 1 )].height;

				shader_nodes.stage[stage].position = pos;

				auto stage_node = node_editor.addNode<Q3N_Stage>( pos );
				const char *stage_id = va("stage %d", stage );
				auto connect_to = base_shader->inPin(stage_id);
				stage_node->outPin("stage")->createLink( connect_to );
				stage_node->setDrawActive();
				shader_nodes.stage[stage].node = stage_node.get();

				stage++;
			}
			depth++;
		}
		else if ( lines[i].find("}") != std::string::npos )
		{
			depth--;
			if ( depth == 0 )
				break;
		}

		for ( auto& regexinfo : lang.mFormatsRegex ) 
		{
			std::smatch sm;
			std::string line;
			format_sanitize( lines[i], line );

			// line is valid shader syntax
			if ( std::regex_match( line, sm, regexinfo.regex ) )
			{
				RegexMatchInfo parms;

				// format/tokens
				std::vector<std::string> tokens;
				tokens.reserve( RegexMatchInfo::max );
				text_editor.split( regexinfo.format, " ", tokens );
	
				// node-uids
				// .. base 'RegexMatchInfo parms' size on 'node-uid' size 
				// .. thank *@! edge case 'animMap'..
				// .. which requires more format/tokens for text-editor autocomplete
				std::vector<std::string> uids;
				uids.reserve( RegexMatchInfo::max );
				parms.size = text_editor.split( regexinfo.node_uid, " ", uids );

				for ( size_t i = 0; i < parms.size; ++i ) {
					parms.tokens[i] = tokens[i];
					parms.node_uid[i] = uids[i];
				}

				size_t k = 0;
				for ( j = 1; j < sm.size(); ++j )
				{
					std::string match;
					format_sanitize( sm.str( j ), match );

					// split regex matched word to ensure 'RegexMatchInfo parms' holds matching sized lists
					// perhaps some regex magic can also handle this ..
					// 
					// to clarify:
					// syntax format: "rgbGen wave" is a treated as single string in smatch.
					{
						// edge case: animMap "uid:textures", which is split in 'parse_stage::l_type_map()'. 
						// since RegexMatchInfo holds matching sized lists
						if ( k < RegexMatchInfo::max && parms.node_uid[k] == "textures" ) 
						{
							parms.words[k] = match;
							k++;
							continue;
						}

						std::vector<std::string> words;
						uids.reserve( RegexMatchInfo::max );
						size_t words_size = text_editor.split( match, " ", words );

						for ( size_t i = 0; i < words_size; ++i, ++k ) 
							parms.words[k] = words[i];
					}
				}

				if ( depth == 1 )
					parse_general( parms );

				else if ( depth == 2 )
					parse_stage( ( stage - 1 ), parms );
				
				break;
			}
		}
	}

	node_editor.unsetFlag( SHADER_NODES_PARSE_TEXT );
}

//
// Nodes to text
//
// parse singular key or key:value
void NodesToTextParser::parse_singular_values( BaseNodeExt &node )
	{
		for ( auto value : node.getValues() )
		{
			if ( value.second->getDataType() == typeid( bool ) )
			{
				if ( node.getValue<bool>( value.second->getUid() ) )
					shaderText.append( va( "%s\n", value.second->getUid() ) );
			}

			else if ( value.second->getDataType() == typeid( float ) )
			{
				auto val = node.getValue<float>( value.second->getUid() );

				if ( val != 0 )
					shaderText.append( va( "%s %f\n", value.second->getUid(), val ) );
			}

			else if ( value.second->getDataType() == typeid( int ) )
			{
				auto val = node.getValue<int>( value.second->getUid() );

				if ( val != 0 )
					shaderText.append( va( "%s %d\n", value.second->getUid(), val ) );
			}
		}
	};

void NodesToTextParser::parse_general( BaseNodeExt *base_shader )
{
	if ( !base_shader )
		return;

	parse_singular_values( *base_shader );

	for ( auto pin : base_shader->getIns() )
	{
		if ( !strncmp( pin->getName().c_str(), "stage", 5 ) ) // skip stage nodes
			continue;

		auto value = base_shader->getInVal<std::string>( pin->getName().c_str() );
		if ( !value.empty() )
			shaderText.append( format_decimals( value ) + "\n" );
	}
}

void NodesToTextParser::parse_stage( BaseNodeExt *stage )
{
	if ( !stage )
		return;

	shaderText.append( "{\n" );

	parse_singular_values( *stage );

	for ( auto pin : stage->getIns() )
	{
		auto value = stage->getInVal<std::string>( pin->getName().c_str() );
		if ( !value.empty() )
			shaderText.append( format_decimals( value ) + "\n" );
	}

	shaderText.append( "}\n" );
}

static Q3N_Shader *getBaseShader( void )
{
	for ( const auto &node : node_editor.getNodes() )
	{
		if ( auto shaderNode = dynamic_cast<Q3N_Shader*>( node.second.get() ) ) 
			return shaderNode;
	}

	return nullptr;
}

void NodesToTextParser::parse( void )
{
#ifdef _DEBUG
	Com_Printf( S_COLOR_YELLOW "Parse nodes to text \n" );
#endif
	auto base_shader = getBaseShader();

	if ( !base_shader ) {
		Com_Printf( S_COLOR_RED "Shader editor: Base shader node is missing!\n" );
		return;
	}

	shaderText = "{\n";

	// general nodes
	parse_general( static_cast<BaseNodeExt*>( base_shader ) );

	// stage nodes
	for ( auto pin : base_shader->getIns() )
	{	
		if ( strncmp( pin->getName().c_str(), "stage", 5 ) ) // skip non-stage nodes
			continue;

		for ( auto node : pin->getConnectedPinNodes() )
			parse_stage( static_cast<BaseNodeExt*>( node ) );
	}

	shaderText.append( "}" );

	// comment out invalid lines 
	format_valid_syntax( shaderText );

	vk_imgui_shader_text_editor_set_text( shaderText.c_str() );
}