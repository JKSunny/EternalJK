/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef  _INSPECTOR_H_
#define  _INSPECTOR_H_

//#ifdef USE_RTX_INSPECT_TANGENTS
// if this isnt used, PT_TANGENT can be removed
//#endif

#ifdef USE_VK_IMGUI
#define LIST_RTX_RENDER_MODES \
	IMG_DO( TAA_OUTPUT,			"Final Image",				0,	true ) \
	IMG_DO( ALBEDO,				"Base color",				1,	true ) \
	IMG_DO( NORMAL,				"Normals",					2,	true ) \
	IMG_DO( TANGENT,			"Tangents",					3,	true ) \
	IMG_DO( SPECULAR,			"Specular",					4,	true ) \
	IMG_DO( ROUGHNESS,			"Roughness",				5,	true ) \
	IMG_DO( METALLIC,			"Metallic",					6,	true ) \
	IMG_DO( TRANSPARANCY,		"Transparent",				7,	true ) \
	IMG_DO( POSITION,			"Position",					8,	true ) \
	IMG_DO( VIEW_DIRECTION,		"View direction",			9,	true ) \
	IMG_DO( COLOR_SPEC,			"Specular contribution",	10,	true ) \
	IMG_DO( DIRECT_ILL,			"Direct illumination",		11,	true ) \
	IMG_DO( INDIRECT_ILL,		"Indirect illumination",	12,	true ) \
	IMG_DO( MOTION,				"Motion",					13,	true ) \
	IMG_DO( THROUGHPUT,			"Troughput",				14,	true ) \
	IMG_DO( BOUNCE_THROUGHPUT,	"Bounce troughput",			15,	true ) \
	IMG_DO( FLAT_COLOR,	        "Flat color",			    16,	true ) \

#ifdef GLSL
const bool render_mode_interleave[] = {
    #define IMG_DO( _handle, _name, _binding, _interleave ) _interleave,
        LIST_RTX_RENDER_MODES
    #undef IMG_DO
};

#define IMG_DO( _handle, _name, _binding, _interleave ) \
	const int	IS_##_handle = _binding;
    LIST_RTX_RENDER_MODES
#undef IMG_DO

vec4 prepare_render_mode_input( in int index, in ivec2 ipos, out bool is_interleaved )
{
    vec4 color = vec4( 0.0, 0.0, 0.0, 1.0 );

    switch( index ) 
    {
        case IS_ALBEDO:             color.rgb = texelFetch( TEX_PT_BASE_COLOR_A,            ipos, 0 ).rgb;              break;
        case IS_NORMAL:             color.rgb = decode_normal(texelFetch(TEX_PT_NORMAL_A,   ipos, 0 ).x) * 0.5 + 0.5;   break;
#ifdef USE_RTX_INSPECT_TANGENTS		
        case IS_TANGENT:            color.rgb = decode_normal(texelFetch(TEX_PT_TANGENT_A,  ipos, 0 ).x);  				break;
#endif
        case IS_SPECULAR:           color.rgb = texelFetch( TEX_PT_METALLIC_A,              ipos, 0 ).bbb;              break;
        case IS_ROUGHNESS:          color.rgb = texelFetch( TEX_PT_METALLIC_A,              ipos, 0 ).ggg;              break;
        case IS_METALLIC:           color.rgb = texelFetch( TEX_PT_METALLIC_A,              ipos, 0 ).rrr;              break;
        case IS_TRANSPARANCY:       color.rgb = texelFetch( TEX_PT_TRANSPARENT,             ipos, 0 ).rgb;              break;
        case IS_POSITION:           color.rgb = texelFetch( TEX_PT_SHADING_POSITION,        ipos, 0 ).rgb;              break;
        case IS_VIEW_DIRECTION:     color.rgb = texelFetch( TEX_PT_VIEW_DIRECTION,          ipos, 0 ).rgb;              break;
        case IS_COLOR_SPEC:         color.rgb = unpackRGBE( texelFetch( TEX_PT_COLOR_SPEC,  ipos, 0 ).x ).rgb / STORAGE_SCALE_SPEC; break;
        case IS_DIRECT_ILL:         color.rgb = unpackRGBE( texelFetch( TEX_PT_COLOR_HF,    ipos, 0 ).x ).rgb / STORAGE_SCALE_HF;   break;
        case IS_INDIRECT_ILL:       color.rgb = texelFetch( TEX_PT_COLOR_LF_SH, ipos, 0 ).rgb / STORAGE_SCALE_LF;                   break;
        case IS_MOTION:             color.rg  = texelFetch( TEX_FLAT_MOTION,                ipos, 0 ).rg;               break;
        case IS_THROUGHPUT:         color.rgb = texelFetch( TEX_PT_THROUGHPUT,              ipos, 0 ).rgb;              break;
        case IS_BOUNCE_THROUGHPUT:  color.rgb = texelFetch( TEX_PT_BOUNCE_THROUGHPUT,       ipos, 0 ).rgb;              break;
        case IS_FLAT_COLOR:         color.rgb = texelFetch( TEX_FLAT_COLOR,                 ipos, 0 ).rgb;              break;

        case IS_TAA_OUTPUT:
        default: color.rgba = imageLoad( IMG_ASVGF_COLOR, ipos ); break;
    }

    is_interleaved = render_mode_interleave[index];
    return color;
}
#endif // GLSL

#endif // USE_VK_IMGUI

#endif // _INSPECTOR_H_