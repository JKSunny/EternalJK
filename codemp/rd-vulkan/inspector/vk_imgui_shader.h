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
// shader text-editor
//
extern TextEditor editor;

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
	TYPE_DO( N_SURFPARM_FOG,fog,			fog					) \
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
	TYPE_DO( N_CGEN_WAVEFORM,					wave					) \
	TYPE_DO( N_CGEN_LIGHTING_DIFFUSE,			lightingDiffuse			) \
	TYPE_DO( N_CGEN_LIGHTING_DIFFUSE_ENTITY,	lightingDiffuseEntity	) \
	TYPE_DO( N_CGEN_CONST,						const					) \

#define N_ALPHAGEN_TYPES \
	TYPE_DO( N_AGEN_LIGHTING_SPECULAR,			lightingSpecular		) \
	TYPE_DO( N_AGEN_ENTITY,						entity					) \
	TYPE_DO( N_AGEN_ONE_MINUS_ENTITY,			oneMinusEntity			) \
	TYPE_DO( N_AGEN_VERTEX,						vertex					) \
	TYPE_DO( N_AGEN_ONE_MINUS_VERTEX,			oneMinusVertex			) \
	TYPE_DO( N_AGEN_DOT,						dot						) \
	TYPE_DO( N_AGEN_ONE_MINUS_DOT,				oneMinusDot				) \
	TYPE_DO( N_AGEN_WAVEFORM,					wave					) \
	TYPE_DO( N_AGEN_CONST,						const					) \
	TYPE_DO( N_AGEN_PORTAL,						portal					) \

#define N_TCGEN_TYPES \
	TYPE_DO( N_TCGEN_ENVIRONMENT_MAPPED,		environment				) \
	TYPE_DO( N_TCGEN_LIGHTMAP,					lightmap				) \
	TYPE_DO( N_TCGEN_BASE,						base					) \
	TYPE_DO( N_TCGEN_TEXTURE,					texture					) \
	TYPE_DO( N_TCGEN_VECTOR,					vector					)  /* S and T from world coordinates */ \

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
	STRUCT( N_RGBGEN_TYPES,			RGBGenTypes			)
	STRUCT( N_ALPHAGEN_TYPES,		AlphaGenTypes		)
	STRUCT( N_TCGEN_TYPES,			TcGenTypes			)

	// general
	STRUCT( N_CULL_TYPES,			CullTypes			)
	STRUCT( N_SORT_TYPES,			SortTypes			)
	STRUCT( N_SURFACEPARAMS_TYPES,	SurfaceParamTypes	)
	STRUCT( N_MATERIAL_TYPES,		MaterialTypes		)
	STRUCT( N_BLENDFUNC_SRC_MACRO_TYPES N_BLENDFUNC_SRC_TYPES,	BlendFuncSrcTypes	)
	STRUCT( N_BLENDFUNC_DST_TYPES,	BlendFuncDstTypes	)
#undef TYPE_DO
#undef STRUCT
#undef STRUCT_TO_STRING

#define TYPE_DO( enum, _name ) #_name,
// based on netradiant-custom implementation of auto-complete
// https://github.com/Garux/netradiant-custom
static const std::vector<TextEditor::Format> g_shaderGeneralFormats {
	{ "surfaceparm %s",					{ N_SURFACEPARAMS_TYPES } },
	{ "cull %s",						{ N_CULL_TYPES } },
	{ "noPicMip"						},
	{ "noMipMaps"						},
	{ "noTC"							},	
	{ "noglfog"							},
	{ "entityMergable"					},
	{ "surfacelight %i"					},
	// q3map_surfacelight deprecated as of 16 Jul 01
	//{
	//	"q3map_surfaceLight %f", "key float"
	//},
	{ "polygonOffset"					},
	{ "portal"							},
	{ "skyParms %t %i -"				},
	{ "skyParms %t - -"					},
	{ "skyParms - %i -"					},
	{ "skyParms - - -"					},
	{ "fogParms ( %c %c %c ) %i"		},

	{ "sun %c %c %c %f %f %f"					},
	{ "q3map_sun %c %c %c %f %f %f"				},
	{ "q3map_sunExt %c %c %c %f %f %f %f %i"	},
	
	{ "sort %s",						{ N_SORT_TYPES } },
	{ "sort %i"							},	

	{ "deform wave %f %s %f %f %f %f",			{ N_MODIFIER_TYPES } },
	{ "deform move %f %f %f %s %f %f %f %f",	{ N_MODIFIER_TYPES } },
	{
		"deform %s %f %f",				{
			"normal",
		}
	},
	{
		"deform bulge %f %f %f",		{
			"bulge",
		}
	},
	{
		"deform %s",					{
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
	{ "deformVertexes wave %f %s %f %f %f %f",			{ N_MODIFIER_TYPES } },
	{ "deformVertexes move %f %f %f %s %f %f %f %f",	{ N_MODIFIER_TYPES } },
	{
		"deformVertexes %s %f %f",		{
			"normal",
		}
	},
	{
		"deformVertexes bulge %f %f %f", {
			"bulge",
		}
	},
	{
		"deformVertexes %s",			{
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
	{ "light %p"						},
	{ "tessSize %f"						},
	{ "clampTime %f"					},
	{ "material %s",					{ N_MATERIAL_TYPES } },
};

static const std::vector<TextEditor::Format> g_shaderStageFormats{
	{
		"map %s",						{
			"$lightmap",	// these do not work for highlighting, $ is special
			"$whiteimage",	// next rule highlights and this works for completion - acceptable
		}
	},
	{ "map %t"							},
	{ "clampMap %t"						},
	{ "videoMap %p"						},
	{ "animMap %f %t %t %t %t %t %t %t %t" },	// texture will be split by spaces
	{ "blendFunc %s",					{ N_BLENDFUNC_SRC_MACRO_TYPES } },
	{
		"blendFunc %s %s",				{
			N_BLENDFUNC_SRC_TYPES
			"__next_token__",
			N_BLENDFUNC_DST_TYPES
		}
	},
	{ "rgbGen %s",						{ N_RGBGEN_TYPES } },
	{ "rgbGen wave %s %f %f %f %f",		{ N_MODIFIER_TYPES } },
	{ "rgbGen const ( %c %c %c )"		},
	{ "alphaGen %s",					{ N_ALPHAGEN_TYPES } },
	{ "alphaGen wave %s %f %f %f %f",	{ N_MODIFIER_TYPES } },
	{ "alphaGen const %f"				},
	{ "alphaGen portal %f"				},
	{ "tcGen %s",						{ N_TCGEN_TYPES } },
	{ "tcGen vector ( %f %f %f ) ( %f %f %f )" },
	// same as tcGen
	{ "texgen %s",						{ N_TCGEN_TYPES } },
	{ "texgen vector ( %f %f %f ) ( %f %f %f )" },
	{ "tcMod entityTranslate"			},
	{ "tcMod rotate %f"					},
	{
		"tcMod %s %f %f",				{
			"scale",
			"scroll",
		}
	},
	{ "tcMod stretch %s %f %f %f %f",	{ N_MODIFIER_TYPES } },
	{ "tcMod transform %f %f %f %f %f %f"	},
	{ "tcMod turb %f %f %f %f"			},
	{ "depthFunc %s",					{ N_DEPTHFUNC_VALUES } },
	{ "depthWrite"						},
	{ "detail"							},
	{ "glow"							},
	{ "alphaFunc %s",					{ N_ALPHAFUNC_VALUES } },
	// ss not implemented
	{
		"surfaceSprites %s %f %f %f %f", {
			"vertical",
			"oriented",
			"effect",
			"flattened",
		}
	},
	{ "ssFademax %f"					},
	{ "ssFadescale %f"					},
	{ "ssVariance %f %f"				},
	{ "ssHangdown"						},
	{ "ssAnyangle"						},
	{ "ssFaceup"						},
	{ "ssWind %f"						},
	{ "ssWindIdle %f"					},
	{ "ssDuration %f"					},
	{ "ssGrow %f %f"					},
	{ "ssWeather"						},

	// pbr
#ifdef USE_VK_PBR
	{ "normalMap %t"					},
	{ "normalHeightMap %t"				},
	{ "specMap %t" 						},
	{ "specularMap %t" 					},
	{ "rmoMap %t" 						},
	{ "rmosMap %t" 						},
	{ "moxrMap %t" 						},
	{ "mosrMap %t" 						},
	{ "ormMap %t" 						},
	{ "ormsMap %t" 						},
	{ "gloss %f"						},
	{ "roughness %f"					},
	{ "specularreflectance %f"			},
	{ "specularexponent %f"				},
	{ "parallaxdepth %f"				},
	{ "parallaxbias %f"					},
	{ "normalscale %f %f %f"			},
	{ "specularscale %f %f %f %f"		},
#endif
};
#undef TYPEDO

#endif // VK_IMGUI_SHADER_H