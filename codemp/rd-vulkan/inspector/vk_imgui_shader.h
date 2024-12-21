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

//
// shader text-editor and node-editor
//

extern TextEditor text_editor;

static const char *editor_type[2] = { "node-based", "text-based" };

// general
#define N_CULL_TYPES \
	TYPE_DO( N_CULL_NONE,					none				) \
	TYPE_DO( N_CULL_DISABLE,				disable				) \
	TYPE_DO( N_CULL_TWOSIDED,				twosided			) \
	TYPE_DO( N_CULL_BACKSIDED,				backsided			) \
	TYPE_DO( N_CULL_BACKSIDE,				backside			) \
	TYPE_DO( N_CULL_BACK,					back				) \

#define N_SURFACEPARAMS_TYPES \
	TYPE_DO( N_SURFPARM_NONSOLID,			nonsolid			) \
	TYPE_DO( N_SURFPARM_NONOPAQUE,			nonopaque			) \
	TYPE_DO( N_SURFPARM_LAVA,				lava				) \
	TYPE_DO( N_SURFPARM_SLIME,				slime				) \
	TYPE_DO( N_SURFPARM_WATER,				water				) \
	TYPE_DO( N_SURFPARM_FOG,				fog					) \
	TYPE_DO( N_SURFPARM_SHOTCLIP,			shotclip			) \
	TYPE_DO( N_SURFPARM_PLAYERCLIP,			playerclip			) \
	TYPE_DO( N_SURFPARM_MONSTERCLIP,		monsterclip			) \
	TYPE_DO( N_SURFPARM_BOTCLIP,			botclip				) \
	TYPE_DO( N_SURFPARM_TRIGGER,			trigger				) \
	TYPE_DO( N_SURFPARM_NODROP,				nodrop				) \
	TYPE_DO( N_SURFPARM_TERRAIN,			terrain				) \
	TYPE_DO( N_SURFPARM_LADDER,				ladder				) \
	TYPE_DO( N_SURFPARM_ABSEIL,				abseil				) \
	TYPE_DO( N_SURFPARM_OUTSIDE,			outside				) \
	TYPE_DO( N_SURFPARM_INSIDE,				inside				) \
	\
	TYPE_DO( N_SURFPARM_DETAIL,				detail				) \
	TYPE_DO( N_SURFPARM_TRANSLUCENT,		trans				) \
	\
	TYPE_DO( N_SURFPARM_SKY,				sky					) \
	TYPE_DO( N_SURFPARM_SLICK,				slick				) \
	\
	TYPE_DO( N_SURFPARM_NODAMAGE,			nodamage			) \
	TYPE_DO( N_SURFPARM_NOIMPACT,			noimpact			) \
	TYPE_DO( N_SURFPARM_NOMARKS,			nomarks				) \
	TYPE_DO( N_SURFPARM_NODRAW,				nodraw				) \
	TYPE_DO( N_SURFPARM_NOSTEPS,			nosteps				) \
	TYPE_DO( N_SURFPARM_NODLIGHT,			nodlight			) \
	TYPE_DO( N_SURFPARM_METALSTEPS,			metalsteps			) \
	TYPE_DO( N_SURFPARM_NOMISCENTS,			nomiscents			) \
	TYPE_DO( N_SURFPARM_FORCEFIELD,			forcefield			) \
	TYPE_DO( N_SURFPARM_FORCESIGHT,			forcesight			) \

// use MATERIALS with some magic macro to create this?
#define N_MATERIAL_TYPES \
	TYPE_DO( N_MATERIAL_NONE,				none				) /* for when the artist hasn't set anything up =) */ \
	TYPE_DO( N_MATERIAL_SOLIDWOOD,			solidwood			) /* freshly cut timber */ \
	TYPE_DO( N_MATERIAL_HOLLOWWOOD,			hollowwood			) /* termite infested creaky wood */ \
	TYPE_DO( N_MATERIAL_SOLIDMETAL,			solidmetal			) /* solid girders */ \
	TYPE_DO( N_MATERIAL_HOLLOWMETAL,		hollowmetal			) /* hollow metal machines */ \
	TYPE_DO( N_MATERIAL_SHORTGRASS,			shortgrass			) /* manicured lawn */ \
	TYPE_DO( N_MATERIAL_LONGGRASS,			longgrass			) /* long jungle grass */ \
	TYPE_DO( N_MATERIAL_DIRT,				dirt				) /* hard mud */ \
	TYPE_DO( N_MATERIAL_SAND,				sand				) /* sandy beach */ \
	TYPE_DO( N_MATERIAL_GRAVEL,				gravel				) /* lots of small stones */ \
	TYPE_DO( N_MATERIAL_GLASS,				glass				) \
	TYPE_DO( N_MATERIAL_CONCRETE,			concrete			) /* hardened concrete pavement */ \
	TYPE_DO( N_MATERIAL_MARBLE,				marble				) /* marble floors */ \
	TYPE_DO( N_MATERIAL_WATER,				water				) /* light covering of water on a surface */ \
	TYPE_DO( N_MATERIAL_SNOW,				snow				) /* freshly laid snow */ \
	TYPE_DO( N_MATERIAL_ICE,				ice					) /* packed snow/solid ice */ \
	TYPE_DO( N_MATERIAL_FLESH,				flesh				) /* hung meat, corpses in the world */ \
	TYPE_DO( N_MATERIAL_MUD,				mud					) /* wet soil */ \
	TYPE_DO( N_MATERIAL_BPGLASS,			bpglass				) /* bulletproof glass */ \
	TYPE_DO( N_MATERIAL_DRYLEAVES,			dryleaves			) /* dried up leaves on the floor */ \
	TYPE_DO( N_MATERIAL_GREENLEAVES,		greenleaves			) /* fresh leaves still on a tree */ \
	TYPE_DO( N_MATERIAL_FABRIC,				fabric				) /* Cotton sheets */ \
	TYPE_DO( N_MATERIAL_CANVAS,				canvas				) /* tent material */ \
	TYPE_DO( N_MATERIAL_ROCK,				rock				) \
	TYPE_DO( N_MATERIAL_RUBBER,				rubber				) /* hard tire like rubber */ \
	TYPE_DO( N_MATERIAL_PLASTIC,			plastic				) \
	TYPE_DO( N_MATERIAL_TILES,				tiles				) /* tiled floor */ \
	TYPE_DO( N_MATERIAL_CARPET,				carpet				) /* lush carpet */ \
	TYPE_DO( N_MATERIAL_PLASTER,			plaster				) /* drywall style plaster */ \
	TYPE_DO( N_MATERIAL_SHATTERGLASS,		shatterglass		) /* glass with the Crisis Zone style shattering */ \
	TYPE_DO( N_MATERIAL_ARMOR,				armor				) /* body armor */ \
	TYPE_DO( N_MATERIAL_COMPUTER,			computer			) /* computers/electronic equipment */ \

#define N_DEFORM_TYPES \
	TYPE_DO( N_DEFORM_WAVE,					wave 				) \
	TYPE_DO( N_DEFORM_NORMALS,				normal 				) \
	TYPE_DO( N_DEFORM_BULGE,				bulge 				) \
	TYPE_DO( N_DEFORM_MOVE,					move 				) \
	TYPE_DO( N_DEFORM_PROJECTION_SHADOW,	projectionShadow 	) \
	TYPE_DO( N_DEFORM_AUTOSPRITE,			autosprite 			) \
	TYPE_DO( N_DEFORM_AUTOSPRITE2,			autosprite2 		) \
	TYPE_DO( N_DEFORM_TEXT0,				text 				) \
	TYPE_DO( N_DEFORM_TEXT1,				text1 				) \
	TYPE_DO( N_DEFORM_TEXT2,				text2 				) \
	TYPE_DO( N_DEFORM_TEXT3,				text3 				) \
	TYPE_DO( N_DEFORM_TEXT4,				text4 				) \
	TYPE_DO( N_DEFORM_TEXT5,				text5 				) \
	TYPE_DO( N_DEFORM_TEXT6,				text6 				) \
	TYPE_DO( N_DEFORM_TEXT7,				text7 				) \

// stage
#define N_MODIFIER_TYPES \
	TYPE_DO( N_SIN				, sin				) \
	TYPE_DO( N_TRIANGLE			, triangle			) \
	TYPE_DO( N_SQUARE			, square			) \
	TYPE_DO( N_SAWTOOTH			, sawtooth			) \
	TYPE_DO( N_INVERSE_SAWTOOTH	, inversesawtooth	) \
	TYPE_DO( N_NOISE			, noise				) \
	TYPE_DO( N_RANDOM			, random			) /* only on deform? */ \

#define N_MAP_TYPES \
	TYPE_DO( N_MAP				, Map				) \
	TYPE_DO( N_CLAMPMAP			, clampMap			) \
	TYPE_DO( N_VIDEOMAP			, videoMap			) \
	/* types allow multiple textures: */ \
	TYPE_DO( N_ANIMMAP			, animMap			) \
	TYPE_DO( N_CLAMPANIMMAP		, clampanimMap		) \
	TYPE_DO( N_ONESHOTANIMMAP	, oneshotanimMap	) \

#define N_NORMAL_MAP_TYPES \
	TYPE_DO( N_NORMALMAP		, normalMap			) \
	TYPE_DO( N_NORMALHEIGHTMAP	, normalHeightMap	) \

#define N_PHYSICAL_MAP_TYPES \
	TYPE_DO( N_SPECMAP			, specMap			) \
	TYPE_DO( N_SPECULARMAP		, specularMap		) \
	TYPE_DO( N_RMOMAP			, rmoMap			) \
	TYPE_DO( N_RMOSMAP			, rmosMap			) \
	TYPE_DO( N_MOXRMAP			, moxrMap			) \
	TYPE_DO( N_MOSRMAP			, moxsMap			) \
	TYPE_DO( N_ORMMAP			, ormMap			) \
	TYPE_DO( N_ORMSMAP			, ormsMap			) \

#define N_ALPHAFUNC_VALUES \
	TYPE_DO( N_GT0				, GT0				) \
	TYPE_DO( N_LT128			, LT128				) \
	TYPE_DO( N_GE128			, GE128				) \
	TYPE_DO( N_GE192			, GE192				) \

#define N_DEPTHFUNC_VALUES \
	TYPE_DO( N_EQUAL			, equal				) \
	TYPE_DO( N_LEQUAL			, lequal			) \
	TYPE_DO( N_DISABLE			, disable			) \

#define N_RGBGEN_TYPES \
	TYPE_DO( N_CGEN_IDENTITY_LIGHTING,			identityLighting		) \
	TYPE_DO( N_CGEN_IDENTITY,					identity				) \
	TYPE_DO( N_CGEN_ENTITY,						entity					) \
	TYPE_DO( N_CGEN_ONE_MINUS_ENTITY,			oneMinusEntity			) \
	TYPE_DO( N_CGEN_EXACT_VERTEX,				exactVertex				) \
	TYPE_DO( N_CGEN_VERTEX,						vertex					) \
	TYPE_DO( N_CGEN_ONE_MINUS_VERTEX,			oneMinusVertex			) \
	TYPE_DO( N_CGEN_LIGHTING_DIFFUSE,			lightingDiffuse			) \
	TYPE_DO( N_CGEN_LIGHTING_DIFFUSE_ENTITY,	lightingDiffuseEntity	) \

// with parameters
#define N_RGBGEN_TYPES_PARMS \
	TYPE_DO( N_CGEN_WAVEFORM,					wave					) \
	TYPE_DO( N_CGEN_CONST,						const					) \

#define N_ALPHAGEN_TYPES \
	TYPE_DO( N_AGEN_LIGHTING_SPECULAR,			lightingSpecular		) \
	TYPE_DO( N_AGEN_ENTITY,						entity					) \
	TYPE_DO( N_AGEN_ONE_MINUS_ENTITY,			oneMinusEntity			) \
	TYPE_DO( N_AGEN_VERTEX,						vertex					) \
	TYPE_DO( N_AGEN_ONE_MINUS_VERTEX,			oneMinusVertex			) \
	TYPE_DO( N_AGEN_DOT,						dot						) \
	TYPE_DO( N_AGEN_ONE_MINUS_DOT,				oneMinusDot				) \

// with parameters
#define N_ALPHAGEN_TYPES_PARMS \
	TYPE_DO( N_AGEN_WAVEFORM,					wave					) \
	TYPE_DO( N_AGEN_CONST,						const					) \
	TYPE_DO( N_AGEN_PORTAL,						portal					) \

#define N_TCGEN_TYPES \
	TYPE_DO( N_TCGEN_ENVIRONMENT_MAPPED,		environment				) \
	TYPE_DO( N_TCGEN_LIGHTMAP,					lightmap				) \
	TYPE_DO( N_TCGEN_BASE,						base					) \
	TYPE_DO( N_TCGEN_TEXTURE,					texture					) \

// with parameters
#define N_TCGEN_TYPES_PARMS \
	TYPE_DO( N_TCGEN_VECTOR,					vector					)  /* S and T from world coordinates */ \

#define N_TCMOD_TYPES \
	TYPE_DO( N_TMOD_TRANSFORM,					transform				) \
	TYPE_DO( N_TMOD_TURBULENT,					turb					) \
	TYPE_DO( N_TMOD_SCROLL,						scroll					) \
	TYPE_DO( N_TMOD_SCALE,						scale					) \
	TYPE_DO( N_TMOD_STRETCH,					stretch					) \
	TYPE_DO( N_TMOD_ROTATE,						rotate					) \
	TYPE_DO( N_TMOD_ENTITY_TRANSLATE,			entityTranslate			) \

#define N_SORT_TYPES \
	TYPE_DO( N_SS_PORTAL,						portal					) \
	TYPE_DO( N_SS_ENVIRONMENT,					sky						) \
	TYPE_DO( N_SS_OPAQUE,						opaque					) \
	TYPE_DO( N_SS_DECAL,						decal					) \
	TYPE_DO( N_SS_SEE_THROUGH,					seeThrough				) \
	TYPE_DO( N_SS_BANNER,						banner					) \
	TYPE_DO( N_SS_BLEND1,						additive				) \
	TYPE_DO( N_SS_NEAREST,						nearest					) \
	TYPE_DO( N_SS_UNDERWATER,					underwater				) \
	TYPE_DO( N_SS_INSIDE,						inside					) \
	TYPE_DO( N_SS_MID_INSIDE,					mid_inside				) \
	TYPE_DO( N_SS_MIDDLE,						middle					) \
	TYPE_DO( N_SS_MID_OUTSIDE,					mid_outside				) \
	TYPE_DO( N_SS_OUTSIDE,						outside					) \

#define N_BLENDFUNC_SRC_MACRO_TYPES \
	TYPE_DO( N_SRC_BLENDFUNC_ADD,					add						) \
	TYPE_DO( N_SRC_BLENDFUNC_FILTER,				filter					) \
	TYPE_DO( N_SRC_BLENDFUNC_BLEND,					blend					) \

#define N_BLENDFUNC_SRC_TYPES \
	TYPE_DO( N_SRC_BLENDFUNC_ONE,					GL_ONE					) \
	TYPE_DO( N_SRC_BLENDFUNC_ZERO,					GL_ZERO					) \
	TYPE_DO( N_SRC_BLENDFUNC_SRC_ALPHA,				GL_SRC_ALPHA			) \
	TYPE_DO( N_SRC_BLENDFUNC_ONE_MINUS_SRC_ALPHA,	GL_ONE_MINUS_SRC_ALPHA	) \
	TYPE_DO( N_SRC_BLENDFUNC_DST_ALPHA,				GL_DST_ALPHA			) \
	TYPE_DO( N_SRC_BLENDFUNC_ONE_MINUS_DST_ALPHA,	GL_ONE_MINUS_DST_ALPHA	) \
	TYPE_DO( N_SRC_BLENDFUNC_DST_COLOR,				GL_DST_COLOR			) \
	TYPE_DO( N_SRC_BLENDFUNC_ONE_MINUS_DST_COLOR,	GL_ONE_MINUS_DST_COLOR	) \
	TYPE_DO( N_SRC_BLENDFUNC_SRC_ALPHA_SATURATE,	GL_SRC_ALPHA_SATURATE	) \

#define N_BLENDFUNC_DST_TYPES \
	TYPE_DO( N_DST_BLENDFUNC_ONE,					GL_ONE					) \
	TYPE_DO( N_DST_BLENDFUNC_ZERO,					GL_ZERO					) \
	TYPE_DO( N_DST_BLENDFUNC_SRC_ALPHA,				GL_SRC_ALPHA			) \
	TYPE_DO( N_DST_BLENDFUNC_ONE_MINUS_SRC_ALPHA,	GL_ONE_MINUS_SRC_ALPHA	) \
	TYPE_DO( N_DST_BLENDFUNC_DST_ALPHA,				GL_DST_ALPHA			) \
	TYPE_DO( N_DST_BLENDFUNC_ONE_MINUS_DST_ALPHA,	GL_ONE_MINUS_DST_ALPHA	) \
	TYPE_DO( N_DST_BLENDFUNC_SRC_COLOR,				GL_SRC_COLOR			) \
	TYPE_DO( N_DST_BLENDFUNC_ONE_MINUS_SRC_COLOR,	GL_ONE_MINUS_SRC_COLOR	) \

#define STRUCT( _content, _name ) typedef enum { _content } _name;

#define TYPE_DO( _enum, _name ) _enum,
	// keywords types
	STRUCT( N_MAP_TYPES,			MapTypes			)
	STRUCT( N_NORMAL_MAP_TYPES,		NormalMapTypes		)
	STRUCT( N_PHYSICAL_MAP_TYPES,	PhysicalMapTypes	)

	// modifiers
	STRUCT( N_MODIFIER_TYPES,		modifierTypes		)

	// values
	STRUCT( N_ALPHAFUNC_VALUES,		AlphaFuncValues		)
	STRUCT( N_DEPTHFUNC_VALUES,		DepthFuncValues		)
	STRUCT( N_RGBGEN_TYPES N_RGBGEN_TYPES_PARMS,				RGBGenTypes			)
	STRUCT( N_ALPHAGEN_TYPES N_ALPHAGEN_TYPES_PARMS,			AlphaGenTypes		)
	STRUCT( N_TCGEN_TYPES N_TCGEN_TYPES_PARMS,					TcGenTypes			)
	STRUCT( N_TCMOD_TYPES,			TcModTypes			)

	// general
	STRUCT( N_CULL_TYPES,			CullTypes			)
	STRUCT( N_SORT_TYPES,			SortTypes			)
	STRUCT( N_SURFACEPARAMS_TYPES,	SurfaceParamTypes	)
	STRUCT( N_MATERIAL_TYPES,		MaterialTypes		)
	STRUCT( N_DEFORM_TYPES,			DeformTypes			)
	STRUCT( N_BLENDFUNC_SRC_MACRO_TYPES N_BLENDFUNC_SRC_TYPES,	BlendFuncSrcTypes	)
	STRUCT( N_BLENDFUNC_DST_TYPES,	BlendFuncDstTypes	)
#undef TYPE_DO

#define TYPE_DO( _enum, _name ) #_name,
	// keywords types
	STRUCT_TO_STRING( N_MAP_TYPES,				n_map_types_string				)
	STRUCT_TO_STRING( N_NORMAL_MAP_TYPES,		n_normal_map_types_string		)
	STRUCT_TO_STRING( N_PHYSICAL_MAP_TYPES,		n_physical_map_types_string		)

	// modifiers
	STRUCT_TO_STRING( N_MODIFIER_TYPES,			n_modifier_string				)

	// values
	STRUCT_TO_STRING( N_ALPHAFUNC_VALUES,		n_alpha_func_string				)
	STRUCT_TO_STRING( N_DEPTHFUNC_VALUES,		n_depth_func_string				)
	STRUCT_TO_STRING( N_RGBGEN_TYPES N_RGBGEN_TYPES_PARMS,					n_rgbgen_type_string			)	/* diffrent from vk_rgbgen_string	!! only user definable types */
	STRUCT_TO_STRING( N_ALPHAGEN_TYPES N_ALPHAGEN_TYPES_PARMS,				n_alphagen_type_string			)	/* diffrent from vk_alphagen_string !! only user definable types */
	STRUCT_TO_STRING( N_TCGEN_TYPES N_TCGEN_TYPES_PARMS,					n_tcgen_type_string				)	/* diffrent from vk_tcgen_string	!! only user definable types */
	STRUCT_TO_STRING( N_TCMOD_TYPES,			n_tcmod_type_string				)	/* diffrent from vk_tcmod_string	!! only user definable types */
	STRUCT_TO_STRING( N_BLENDFUNC_SRC_MACRO_TYPES N_BLENDFUNC_SRC_TYPES,	n_blend_func_src_type_string	)	/* diffrent from vk_src_blend_func_string	!! only user definable types */
	STRUCT_TO_STRING( N_BLENDFUNC_DST_TYPES,	n_blend_func_dst_type_string	)	/* diffrent from vk_src_blend_func_string	!! order */

	// general
	STRUCT_TO_STRING( N_CULL_TYPES,				n_cull_type_string				)
	STRUCT_TO_STRING( N_SORT_TYPES,				n_sort_type_string				)	/* diffrent from vk_sort_string		!! only user definable types */
	STRUCT_TO_STRING( N_SURFACEPARAMS_TYPES,	n_surface_params_type_string	)
	STRUCT_TO_STRING( N_MATERIAL_TYPES,			n_material_type_string			)
	STRUCT_TO_STRING( N_DEFORM_TYPES,			n_deform_type_string			)	/* diffrent from vk_deform_string		!! only user definable types */
#undef TYPE_DO
#undef STRUCT
#undef STRUCT_TO_STRING

#define TYPE_DO( enum, _name ) #_name,

//	
// text-editor autocomplete and this shader format is
// based on netradiant-custom implementation of auto-complete
// https://github.com/Garux/netradiant-custom/commit/9c2fbc9d1dd4029c0f76aec2830f991fcb2c259e
//
static const std::vector<TextEditor::Format> g_shaderGeneralFormats 
{
	{ "surfaceparm %s",					"key type", { N_SURFACEPARAMS_TYPES } },
	{ "cull %s",						"key type", { N_CULL_TYPES } },
	{ "noPicMip",						"key" },
	{ "noMipMaps",						"key" },
	{ "noTC",							"key" },	
	{ "noglfog",						"key" },
	{ "entityMergable",					"key" },
	{ "surfacelight %i",				"key integer" },
	// q3map_surfacelight deprecated as of 16 Jul 01
	//{
	//	"q3map_surfaceLight %f", "key float"
	//},
	{ "polygonOffset",					"key" },
	{ "portal",							"key" },
	{ "skyParms %t %i -",				"key texture cloudHeight void" },
	{ "skyParms %t - -",				"key texture cloudHeight void" },
	{ "skyParms - %i -",				"key texture cloudHeight void" },
	{ "skyParms - - -",					"key texture cloudHeight void" },
	{ "fogParms ( %c %c %c ) %f",		"key ( color:0 color:1 color:2 ) depthForOpaque" },

	{ "sun %c %c %c %f %f %f",					"key color:0 color:1 color:2 direction:0 direction:1 direction:2" },
	{ "q3map_sun %c %c %c %f %f %f",			"key color:1 color:2 direction:0 direction:1 direction:2" },
	{ "q3map_sunExt %c %c %c %f %f %f %f %i",	"key color:1 color:2 direction:0 direction:1 direction:2 void" },
	
	{ "sort %s",						"key type", { N_SORT_TYPES } },
	{ "sort %i",						"key integer" },	// todo

	{ "deform wave %f %s %f %f %f %f",			"key type spread modifier base amplitude phase frequency", { N_MODIFIER_TYPES } },
	{ "deform move %f %f %f %s %f %f %f %f",	"key type displace:0 displace:1 displace:2 modifier base amplitude phase frequency", { N_MODIFIER_TYPES } },
	{
		"deform %s %f %f",				"key type amplitude frequency", {
			"normal",
		}
	},
	{
		"deform bulge %f %f %f",		"key type bulgeWidth bulgeHeight bulgeSpeed", {
			"bulge",
		}
	},
	{
		"deform %s",					"key type", {
			"autosprite2",
			"autosprite", 
			"projectionShadow",
			"text",
			"text1",
			"text2",
			"text3",
			"text4",
			"text5",
			"text6",
			"text7", //this last, so it doesn't take precendence, while matching longer version
		}
	},
	// same as deform ..
	{ "deformVertexes wave %f %s %f %f %f %f",			"key type spread modifier base amplitude phase frequency", { N_MODIFIER_TYPES } },
	{ "deformVertexes move %f %f %f %s %f %f %f %f",	"key type displace:0 displace:1 displace:2 modifier base amplitude phase frequency", { N_MODIFIER_TYPES } },
	{
		"deformVertexes %s %f %f",		"key type amplitude frequency", {
			"normal",
		}
	},
	{
		"deformVertexes bulge %f %f %f", "key type bulgeWidth bulgeHeight bulgeSpeed", {
			"bulge",
		}
	},
	{
		"deformVertexes %s",			"key type", {
			"autosprite2",
			"autosprite", 
			"projectionShadow",
			"text",
			"text1",
			"text2",
			"text3",
			"text4",
			"text5",
			"text6",
			"text7", //this last, so it doesn't take precendence, while matching longer version
		}
	},
	{ "light %p",						"key texture" },
	{ "tessSize %f",					"key float" },
	{ "clampTime %f",					"key float" },
	{ "material %s",					"key type", { N_MATERIAL_TYPES } },
};

static const std::vector<TextEditor::Format> g_shaderStageFormats
{
	{
		"map %s",						"key texture", {
			"$lightmap",	// these do not work for highlighting, $ is special
			"$whiteimage",	// next rule highlights and this works for completion - acceptable
		}
	},
	{ "map %t",							"key texture" },
	{ "clampMap %t",					"key texture" },
	{ "videoMap %p",					"key texture" },
	{ "animMap %f %t %t %t %t %t %t %t %t",			"key speed textures" },	// texture will be split by spaces
	{ "clampanimMap %f %t %t %t %t %t %t %t %t",	"key speed textures" },	// texture will be split by spaces
	{ "oneshotanimMap %f %t %t %t %t %t %t %t %t",	"key speed textures" },	// texture will be split by spaces
	{ "blendFunc %s",					"key src", { N_BLENDFUNC_SRC_MACRO_TYPES } },
	{
		"blendFunc %s %s",				"key src dst", {
			N_BLENDFUNC_SRC_TYPES
			"__next_token__",
			N_BLENDFUNC_DST_TYPES
		}
	},
	{ "rgbGen %s",						"key type", { N_RGBGEN_TYPES } },
	{ "rgbGen wave %s %f %f %f %f",		"key type modifier base amplitude phase frequency", { N_MODIFIER_TYPES } },
	{ "rgbGen const ( %c %c %c )",		"key type ( color:0 color:1 color:2 )" },
	{ "alphaGen %s",					"key type", { N_ALPHAGEN_TYPES } },
	{ "alphaGen wave %s %f %f %f %f",	"key type modifier base amplitude phase frequency", { N_MODIFIER_TYPES } },
	{ "alphaGen const %f",				"key type alpha" },
	{ "alphaGen portal %f",				"key type range" },
	{ "tcGen %s",						"key type", { N_TCGEN_TYPES } },
	{ "tcGen vector ( %f %f %f ) ( %f %f %f )", "key type ( axis_s:0 axis_s:1 axis_s:2 ) ( axis_t:0 axis_t:1 axis_t:2 )" },
	// same as tcGen
	{ "texgen %s",						"key type", { N_TCGEN_TYPES } },
	{ "texgen vector ( %f %f %f ) ( %f %f %f )", "key type ( axis_s:0 axis_s:1 axis_s:2 ) ( axis_t:0 axis_t:1 axis_t:2 )" },
	{ "tcMod entityTranslate",			"key type" },
	{ "tcMod rotate %f",				"key type rotate" },
	{
		"tcMod %s %f %f",				"key type translate:0 translate:1", {
			"scale",
			"scroll",
		}
	},
	{ "tcMod stretch %s %f %f %f %f",		"key type modifier base amplitude phase frequency", { N_MODIFIER_TYPES } },
	{ "tcMod transform %f %f %f %f %f %f",	"key type matrix_x:0 matrix_x:1 matrix_y:0 matrix_y:1 translate:0 translate:1" },
	{ "tcMod turb %f %f %f %f",				"key type base amplitude phase frequency" },
	{ "depthFunc %s",						"key type", { N_DEPTHFUNC_VALUES } },
	{ "depthWrite",							"key" },
	{ "detail",								"key" },
	{ "glow",								"key" },
	{ "alphaFunc %s",						"key type", { N_ALPHAFUNC_VALUES } },
	// ss not implemented
	{
		"surfaceSprites %s %f %f %f %f",	"key type float_1 float_2 float_3", {
			"vertical",
			"oriented",
			"effect",
			"flattened",
		}
	},
	{ "ssFademax %f",					"key float_1" },
	{ "ssFadescale %f",					"key float_1" },
	{ "ssVariance %f %f",				"key float_1 float_2" },
	{ "ssHangdown",						"key" },
	{ "ssAnyangle",						"key" },
	{ "ssFaceup",						"key" },
	{ "ssWind %f",						"key float_1" },
	{ "ssWindIdle %f",					"key float_1" },
	{ "ssDuration %f",					"key float_1" },
	{ "ssGrow %f %f",					"key float_1 float_2" },
	{ "ssWeather",						"key" },

	// pbr
#ifdef USE_VK_PBR
	{ "normalMap %t",					"key normal_map" },
	{ "normalHeightMap %t",				"key normal_height_map" },
	{ "specMap %t", 					"key physical_map" },
	{ "specularMap %t", 				"key physical_map" },
	{ "rmoMap %t", 						"key physical_map" },
	{ "rmosMap %t", 					"key physical_map" },
	{ "moxrMap %t", 					"key physical_map" },
	{ "mosrMap %t", 					"key physical_map" },
	{ "ormMap %t", 						"key physical_map" },
	{ "ormsMap %t", 					"key physical_map" },
	{ "gloss %f",						"key float" },
	{ "roughness %f",					"key float" },
	{ "specularreflectance %f",			"key float" },
	{ "specularexponent %f",			"key float" },
	{ "parallaxdepth %f",				"key float" },
	{ "parallaxbias %f",				"key float" },
	{ "normalscale %f %f %f",			"key float1 float2 float3" },
	{ "specularscale %f %f %f %f",		"key float1 float2 float3 float4" },
#endif
};
#undef TYPEDO

#endif // VK_IMGUI_SHADER_H