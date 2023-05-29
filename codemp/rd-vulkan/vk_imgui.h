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

// need to define  ImTextureID ImU64 in imconfig.h:105
// need to typedef ImTextureID ImU64 in cimgui.h:182
#include <imgui.h>
#include <imgui.cpp>
#include <imgui_draw.cpp>
#include <imgui_widgets.cpp>
#include <imgui_tables.cpp>
#include <backends/imgui_impl_vulkan.cpp>
#include <backends/imgui_impl_sdl.cpp>
#include <utils/legit-profiler/imgui_legit_profiler.h>
#include "icons/FontAwesome5/IconsFontAwesome5.h"
#include "icons/FontAwesome5/fa5solid900.cpp"

#include <SDL_video.h>
#include <typeinfo>


// inspector object types
#define OT_CUBEMAP			( 1 )
#define OT_FLARE			( 2 )
#define OT_SHADER			( 4 )

#define FILE_HASH_SIZE		1024

// define if profiler starts or ends with WaitForTargetFPS frame time
#define PROFILER_PREPEND_WAITMAXFPS

typedef struct {
	qboolean	init;
	char		search_keyword[MAX_QPATH];
	bool		unique_shaders;
	int			num_shaders;
	int			render_mode;

	struct {
		uint32_t	type;
		void		*ptr;
		void		*prev;
	} selected;

	struct {
		qboolean	active;
		vec3_t		*origin;
	} transform;

	struct {
		qboolean	active;
		int			index;

	} shader;

} vk_imgui_inspector_t;

typedef struct {
	struct {
		bool	p_open;
		int		index;
		int		prev;
	} shader;

	struct {
		bool	p_open;
	} profiler;
} vk_imgui_window_t;
 
typedef struct profilerFrame_s {
	std::chrono::high_resolution_clock::time_point frameStartTime;
	std::vector<profilerTask_s> cpu_tasks;
	size_t frame_interval_index;
} profilerFrame_t;

typedef struct {
	profilerFrame_t frames[2];

	struct {
		profilerFrame_t *current;
		profilerFrame_t *previous;
	} frame;
} imgui_profiler_t;

SDL_Window			*screen;
VkDescriptorPool	imguiPool;
ImGuiGlobal			imguiGlobal;
ImGuiContext		*ImContext;

static vk_imgui_inspector_t inspector;
static vk_imgui_window_t	windows;
static imgui_profiler_t		profiler;

static shader_t *hashTable[FILE_HASH_SIZE];
static shader_t *unique_shader_list[MAX_SHADERS];

static ImVector<ImRect> s_GroupPanelLabelStack;
ImGuiUtils::ProfilersWindow profilersWindow;

static const char *yesno[] = {"no ", "yes"};

static const ImU32 color_palette[MAX_SHADER_STAGES] = {
	RGBA_LE(0x73d2deffu),
	RGBA_LE(0x218380ffu),
	RGBA_LE(0x8f2d56ffu),
	RGBA_LE(0xd81159ffu),
	RGBA_LE(0xffbc42ffu),
	RGBA_LE(0x824670ffu),
	RGBA_LE(0xa2708affu),
	RGBA_LE(0xc1F7dCffu),
};

const char *render_modes[23] = { 
	"Final Image", 
	"Diffuse", 
	"Specular", 
	"Roughness", 
	"Ambient Occlusion", 
	"Normals", 
	"Normals + Normalmap",  
	"Light direction",
	"View direction",
	"Tangents",
	"Light color",
	"Ambient color",
	"Reflectance",
	"Attenuation",
	"H - Half vector lightdir/viewdir",
	"Fd - CalcDiffuse",
	"Fs - CalcSpecular",
	"NdotE - Normal dot View direction",
	"NdotL - Normal dot Light direction",
	"LdotH - Light direction dot Half vector",
	"NdotH - Normal dor Half vector",
	"VdotH - View direction dot Half vector",
	"IBL Contribution"
};

static const char *vk_deform_string[18] = {
	"none",
	"wave",
	"normal",
	"bulge",
	"bulge", // uniform speed == width == 0.0f
	"move",
	"projectionShadow",
	"autoSprite",
	"autoSprite2",
	"text",
	"text 1",
	"text 2",
	"text 3",
	"text 4",
	"text 5",
	"text 6",
	"text 7",
	"disintergrate", // no param
};

static const char *vk_sort_string[22] = {
	"bad",
	"portal",
	"environment",
	"opaque",
	"decal",
	"see through",
	"banner",
	"inside",
	"mid inside",
	"middle",
	"mid outside",
	"outside",
	"fog",
	"underwater",
	"blend 0",
	"blend 1",
	"blend 2",
	"blend 3",
	"blend 6",
	"stencil shadow",
	"almost nearest",
	"nearest"
};

static const char *vk_alphagen_string[13] = {
	"identity",
	"skip",
	"entity",
	"oneMinusEntity",
	"vertex",
	"oneMinusVertex",
	"lightingSpecular",
	"wave",
	"portal",
	"blend",
	"const",
	"dot",
	"oneMinusDot",
};

static const char *vk_rgbgen_string[16] = {
	"bad",
	"identityLighting",
	"identity",
	"entity",
	"oneMinusEntity",
	"exactVertex",
	"vertex",
	"oneMinusVertex",
	"wave",
	"lightingDiffuse",
	"lightingDiffuseEntity",
	"fog",
	"const",
	"lightmapstyle",
	"disintegration 1",
	"disintegration 2"
};

static const char *vk_tcmod_string[8] = {
	"none",
	"transform",
	"turb",
	"scroll",
	"scale",
	"stretch",
	"rotate",
	"entityTranslate"
};

static const char *vk_tcgen_string[10] = {
	"bad",
	"identity", 
	"lightmap",
	"lightmap1",
	"lightmap2",
	"lightmap3",
	"texture",		// default
	"environment",
	"fog",
	"vector"
};

static const char *vk_wave_func_string[8] = {
	"none",
	"sin",
	"square",
	"triangle",
	"sawtooth",
	"inversesawtooth",
	"noise",
	"rand"	
};

static const char *vk_src_blend_func_string[10] = {
	"NONE",
	"ONE",
	"DST_COLOR",
	"ONE_MINUS_DST_COLOR",
	"SRC_ALPHA",
	"ONE_MINUS_SRC_ALPHA",
	"DST_ALPHA",
	"ONE_MINUS_DST_ALPHA",
	"SRC_ALPHA_SATURATE",
	"BAD"
};

static const char *vk_dst_blend_func_string[9] = {
	"ZERO",
	"ONE",
	"SRC_COLOR",
	"ONE_MINUS_SRC_COLOR",
	"SRC_ALPHA",
	"ONE_MINUS_SRC_ALPHA",
	"DST_ALPHA",
	"ONE_MINUS_DST_ALPHA",
	"NONE"
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

	return vk_src_blend_func_string[9];
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

	return vk_dst_blend_func_string[8];
}

//
// extended methods
// 
static void vk_imgui_execute_cmd( const char *text ){
	ri.Cbuf_ExecuteText( EXEC_APPEND, va( "%s\n", text ) );	
}

int vk_imgui_get_render_mode( void ){
	return inspector.render_mode;
}

int vk_imgui_get_shader_editor_index ( void ) {
	return inspector.shader.index;
}

void vk_imgui_reload_shader_editor( qboolean close ) {
	windows.shader.prev = NULL;
	windows.shader.p_open = (bool)close;
}

static int generateHashValue( const char *fname )
{
    uint32_t i = 0;
    int	hash = 0;

    while (fname[i] != '\0') {
        char letter = tolower(fname[i]);
        if (letter == '.') break;		// don't include extension
        if (letter == '\\') letter = '/';	// damn path names
        hash += (long)(letter) * (i + 119);
        i++;
    }
    hash &= (FILE_HASH_SIZE - 1);
    return hash;
}

// https://github.com/ocornut/imgui/issues/1496#issuecomment-569465473
void ImGuiBeginGroupPanel(const char* name, const ImVec2& size)
{
    ImGui::BeginGroup();

    auto cursorPos = ImGui::GetCursorScreenPos();
    auto itemSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();
    ImGui::BeginGroup();

    ImVec2 effectiveSize = size;
    if (size.x < 0.0f)
        effectiveSize.x = ImGui::GetContentRegionAvail().x;
    else
        effectiveSize.x = size.x;
    ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(name);
    auto labelMin = ImGui::GetItemRectMin();
    auto labelMax = ImGui::GetItemRectMax();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
    ImGui::BeginGroup();

    //ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x          -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x         -= frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x                   -= frameHeight;

    auto itemWidth = ImGui::CalcItemWidth();
    ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));

    s_GroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
}

void ImGuiEndGroupPanel( ImU32 border_color )
{
    ImGui::PopItemWidth();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();

    ImGui::EndGroup();

    //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 255, 0, 64), 4.0f);

    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

    ImGui::EndGroup();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();
    //ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

    auto labelRect = s_GroupPanelLabelStack.back();
    s_GroupPanelLabelStack.pop_back();

    ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
    ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));
    labelRect.Min.x -= itemSpacing.x;
    labelRect.Max.x += itemSpacing.x;
    for (int i = 0; i < 4; ++i)
    {
        switch (i)
        {
            // left half-plane
            case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true); break;
            // right half-plane
            case 1: ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true); break;
            // top
            case 2: ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true); break;
            // bottom
            case 3: ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true); break;
        }

        ImGui::GetWindowDrawList()->AddRect(
            frameRect.Min, frameRect.Max,
            border_color,
            halfFrame.x);

        ImGui::PopClipRect();
    }

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x          += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x         += frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x                   += frameHeight;

    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    ImGui::EndGroup();
}

static void HelpMarker( const char* desc )
{
    ImGui::TextDisabled("(?)");
    if ( ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) )
    {
		ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}