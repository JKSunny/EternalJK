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


#ifndef VK_IMGUI_SHADER_H
#define VK_IMGUI_SHADER_H

#define STRUCT_TO_STRING( _content, _name ) static const char *_name[] { _content };

//
// inspector
//
#define ENUM_DO( _enum, _name ) #_name,
	STRUCT_TO_STRING( DEFORM_LIST,				vk_deform_string				)
	STRUCT_TO_STRING( ALPHAGEN_LIST,			vk_alphagen_string				)
	STRUCT_TO_STRING( COLORGEN_LIST,			vk_rgbgen_string				)
	STRUCT_TO_STRING( TCMOD_LIST,				vk_tcmod_string					)
	STRUCT_TO_STRING( SHADER_SORT_LIST,			vk_sort_string					)
	STRUCT_TO_STRING( TCGEN_LIST,				vk_tcgen_string					)
	STRUCT_TO_STRING( GEN_FUNC_LIST,			vk_wave_func_string				)
#undef ENUM_DO

static const char *vk_src_blend_func_string[9] = {
	"ZERO",
	"ONE",
	"DST_COLOR",
	"ONE_MINUS_DST_COLOR",
	"SRC_ALPHA",
	"ONE_MINUS_SRC_ALPHA",
	"DST_ALPHA",
	"ONE_MINUS_DST_ALPHA",
	"SRC_ALPHA_SATURATE",
};

static const char *vk_dst_blend_func_string[8] = {
	"ZERO",
	"ONE",
	"SRC_COLOR",
	"ONE_MINUS_SRC_COLOR",
	"SRC_ALPHA",
	"ONE_MINUS_SRC_ALPHA",
	"DST_ALPHA",
	"ONE_MINUS_DST_ALPHA",
};

#define SRC_BLEND_STRING( bits, bit, i ) if( ( bits & GLS_SRCBLEND_BITS ) == bit ) return vk_src_blend_func_string[i]
#define DST_BLEND_STRING( bits, bit, i ) if( ( bits & GLS_DSTBLEND_BITS ) == bit ) return vk_dst_blend_func_string[i]

static const char *vk_get_src_blend_strings( const int bits ) {
	SRC_BLEND_STRING( bits, GLS_SRCBLEND_ZERO,					0);
	SRC_BLEND_STRING( bits, GLS_SRCBLEND_ONE,					1);
	SRC_BLEND_STRING( bits, GLS_SRCBLEND_DST_COLOR,				2);
	SRC_BLEND_STRING( bits, GLS_SRCBLEND_ONE_MINUS_DST_COLOR,	3);
	SRC_BLEND_STRING( bits, GLS_SRCBLEND_SRC_ALPHA,				4);
	SRC_BLEND_STRING( bits, GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA,	5);
	SRC_BLEND_STRING( bits, GLS_SRCBLEND_DST_ALPHA,				6);
	SRC_BLEND_STRING( bits, GLS_SRCBLEND_ONE_MINUS_DST_ALPHA,	7);
	SRC_BLEND_STRING( bits, GLS_SRCBLEND_ALPHA_SATURATE,		8);

	return "BAD";
}

static const char *vk_get_dst_blend_strings( const int bits ) {

	DST_BLEND_STRING( bits, GLS_DSTBLEND_ZERO,					0);
	DST_BLEND_STRING( bits, GLS_DSTBLEND_ONE,					1);
	DST_BLEND_STRING( bits, GLS_DSTBLEND_SRC_COLOR,				2);
	DST_BLEND_STRING( bits, GLS_DSTBLEND_ONE_MINUS_SRC_COLOR,	3);
	DST_BLEND_STRING( bits, GLS_DSTBLEND_SRC_ALPHA,				4);
	DST_BLEND_STRING( bits, GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA,	5);
	DST_BLEND_STRING( bits, GLS_DSTBLEND_DST_ALPHA,				6);
	DST_BLEND_STRING( bits, GLS_DSTBLEND_ONE_MINUS_DST_ALPHA,	7);

	return "BAD";
}

#endif // VK_IMGUI_SHADER_H