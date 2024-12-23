#============================================================================
# Copyright (C) 2013 - 2018, OpenJK contributors
#
# This file is part of the OpenJK source code.
#
# OpenJK is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#============================================================================

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

# Check for dependencies first

set(MPVulkanRendererIncludeDirectories ${MPDir})
set(MPVulkanRendererIncludeDirectories
	${MPVulkanRendererIncludeDirectories}
	${SharedDir}
	${MPDir}
	"${MPDir}/rd-vulkan"
	"${GSLIncludeDirectory}"
	)

set(MPVulkanRendererFiles
	"${MPDir}/rd-vulkan/G2_API.cpp"
	"${MPDir}/rd-vulkan/G2_bolts.cpp"
	"${MPDir}/rd-vulkan/G2_bones.cpp"
	"${MPDir}/rd-vulkan/G2_misc.cpp"
	"${MPDir}/rd-vulkan/G2_surfaces.cpp"
	"${MPDir}/rd-vulkan/tr_allocator.h"
	"${MPDir}/rd-vulkan/tr_allocator.cpp"
	"${MPDir}/rd-vulkan/tr_backend.cpp"
	"${MPDir}/rd-vulkan/tr_bsp.cpp"
	"${MPDir}/rd-vulkan/tr_cmds.cpp"
	"${MPDir}/rd-vulkan/tr_curve.cpp"
	"${MPDir}/rd-vulkan/tr_decals.cpp"
	"${MPDir}/rd-vulkan/tr_ghoul2.cpp"
	"${MPDir}/rd-vulkan/tr_image.cpp"
	"${MPDir}/rd-vulkan/tr_init.cpp"
	"${MPDir}/rd-vulkan/tr_light.cpp"
	"${MPDir}/rd-vulkan/tr_local.h"
	"${MPDir}/rd-vulkan/tr_main.cpp"
	"${MPDir}/rd-vulkan/tr_marks.cpp"
	"${MPDir}/rd-vulkan/tr_mesh.cpp"
	"${MPDir}/rd-vulkan/tr_model.cpp"
	"${MPDir}/rd-vulkan/tr_quicksprite.cpp"
	"${MPDir}/rd-vulkan/tr_quicksprite.h"
	"${MPDir}/rd-vulkan/tr_scene.cpp"
	"${MPDir}/rd-vulkan/tr_shade.cpp"
	"${MPDir}/rd-vulkan/tr_shader.cpp"
	"${MPDir}/rd-vulkan/tr_shade_calc.cpp"
	"${MPDir}/rd-vulkan/tr_shadows.cpp"
	"${MPDir}/rd-vulkan/tr_skin.cpp"
	"${MPDir}/rd-vulkan/tr_sky.cpp"
	"${MPDir}/rd-vulkan/tr_subs.cpp"
	"${MPDir}/rd-vulkan/tr_surface.cpp"
	"${MPDir}/rd-vulkan/tr_surfacesprites.cpp"
	"${MPDir}/rd-vulkan/tr_world.cpp"
	"${MPDir}/rd-vulkan/tr_WorldEffects.cpp"
	"${MPDir}/rd-vulkan/tr_WorldEffects.h"
	"${MPDir}/rd-vulkan/vk_attachments.cpp"
	"${MPDir}/rd-vulkan/vk_bloom.cpp"
	"${MPDir}/rd-vulkan/vk_cmd.cpp"
	"${MPDir}/rd-vulkan/vk_cubemap.cpp"
	"${MPDir}/rd-vulkan/vk_debug.cpp"
	"${MPDir}/rd-vulkan/vk_dynamic_glow.cpp"
	"${MPDir}/rd-vulkan/vk_flares.cpp"
	"${MPDir}/rd-vulkan/vk_frame.cpp"
	"${MPDir}/rd-vulkan/vk_image.cpp"
	"${MPDir}/rd-vulkan/vk_image_process.cpp"
	"${MPDir}/rd-vulkan/vk_info.cpp"
	"${MPDir}/rd-vulkan/vk_init.cpp"
	"${MPDir}/rd-vulkan/vk_instance.cpp"
	"${MPDir}/rd-vulkan/vk_local.h"
	"${MPDir}/rd-vulkan/vk_pipelines.cpp"
	"${MPDir}/rd-vulkan/vk_shaders.cpp"
	"${MPDir}/rd-vulkan/vk_shade_geometry.cpp"
	"${MPDir}/rd-vulkan/vk_swapchain.cpp"
	"${MPDir}/rd-vulkan/vk_vbo.cpp"
	)
source_group("renderer" FILES ${MPVulkanRendererFiles})

set(MPVulkanRendererGhoul2Files
	"${MPDir}/ghoul2/g2_local.h"
	"${MPDir}/ghoul2/ghoul2_shared.h"
	"${MPDir}/ghoul2/G2_gore.cpp"
	"${MPDir}/ghoul2/G2_gore.h")
source_group("ghoul2" FILES ${MPVulkanRendererGhoul2Files})
set(MPVulkanRendererFiles ${MPVulkanRendererFiles} ${MPVulkanRendererGhoul2Files})

set(MPVulkanRendererRdCommonFiles
	"${MPDir}/rd-common/mdx_format.h"
	"${MPDir}/rd-common/tr_common.h"
	"${MPDir}/rd-common/tr_font.cpp"
	"${MPDir}/rd-common/tr_font.h"
	"${MPDir}/rd-common/tr_image_load.cpp"
	"${MPDir}/rd-common/tr_image_jpg.cpp"
	"${MPDir}/rd-common/tr_image_tga.cpp"
	"${MPDir}/rd-common/tr_image_png.cpp"
	"${MPDir}/rd-common/tr_noise.cpp"
	"${MPDir}/rd-common/tr_public.h"
	"${MPDir}/rd-common/tr_types.h")
source_group("rd-common" FILES ${MPVulkanRendererRdCommonFiles})
set(MPVulkanRendererFiles ${MPVulkanRendererFiles} ${MPVulkanRendererRdCommonFiles})

set(MPVulkanRendererVulkanAssets
	"${MPDir}/rd-vulkan/vulkan/vk_platform.h"
	"${MPDir}/rd-vulkan/vulkan/vulkan.h"
	"${MPDir}/rd-vulkan/vulkan/vulkan_core.h"
	"${MPDir}/rd-vulkan/vulkan/vulkan_win32.h"
	"${MPDir}/rd-vulkan/vulkan/vulkan_xlib.h"
	"${MPDir}/rd-vulkan/vulkan/vulkan_xlib_xrandr.h")
source_group("renderer\\vulkan" FILES ${MPVulkanRendererVulkanAssets})
set(MPVulkanRendererFiles ${MPVulkanRendererFiles} ${MPVulkanRendererVulkanAssets})

set(MPVulkanRendererUtils
	"${MPDir}/rd-vulkan/utils/stb_dxt.h"
	"${MPDir}/rd-vulkan/utils/stb_image.h"
	"${MPDir}/rd-vulkan/utils/legit-profiler/imgui_legit_profiler.h"
	"${MPDir}/rd-vulkan/utils/ImNodeFlow/include/ImNodeFlow.h"
	"${MPDir}/rd-vulkan/utils/ImNodeFlow/src/context_wrapper.h"
	"${MPDir}/rd-vulkan/utils/ImNodeFlow/src/imgui_bezier_math.h"
	"${MPDir}/rd-vulkan/utils/ImNodeFlow/src/imgui_bezier_math.inl"
	"${MPDir}/rd-vulkan/utils/ImNodeFlow/src/imgui_extra_math.h"
	"${MPDir}/rd-vulkan/utils/ImNodeFlow/src/imgui_extra_math.inl"
	"${MPDir}/rd-vulkan/utils/ImNodeFlow/src/ImNodeFlow.cpp"
	"${MPDir}/rd-vulkan/utils/ImNodeFlow/src/ImNodeFlow.inl"
	"${MPDir}/rd-vulkan/utils/ImGuiColorTextEdit/TextEditor.h"
	"${MPDir}/rd-vulkan/utils/ImGuiColorTextEdit/TextEditor.cpp")
source_group("renderer\\utils" FILES ${MPVulkanRendererUtils})
set(MPVulkanRendererFiles ${MPVulkanRendererFiles} ${MPVulkanRendererUtils})

set(MPVulkanRendererRTX
	"${MPDir}/rd-vulkan/rtx/conversion.h"
	"${MPDir}/rd-vulkan/rtx/vk_rtx.h"
	"${MPDir}/rd-vulkan/rtx/vk_rtx.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_as.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_asvgf.h"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_asvgf.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_buffer.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_bsp.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_descriptor.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_god_rays.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_physical_sky.h"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_physical_sky.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_material.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_matrix.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_model.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_pipelines.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_precomputed_sky.h"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_precomputed_sky.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_shader.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_shade.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_shadow_map.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_tonemap.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_tonemap.h"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_uniform.cpp"
	"${MPDir}/rd-vulkan/rtx/vk_rtx_image.cpp")
source_group("renderer\\rtx" FILES ${MPVulkanRendererRTX})
set(MPVulkanRendererFiles ${MPVulkanRendererFiles} ${MPVulkanRendererRTX})

set(MPVulkanRendererInspector
	"${MPDir}/rd-vulkan/inspector/vk_imgui.h"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_context.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_cubemaps.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_entity.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_flares.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_globals.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_profiler.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_shader.h"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_shader.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_shader_nodes.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_shader_nodes.h"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_shader_nodes_general.h"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_shader_nodes_stage.h"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_shader_nodes_parse.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_shader_text.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_world_nodes.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui_rtx.cpp"
	"${MPDir}/rd-vulkan/inspector/vk_imgui.cpp")
source_group("renderer\\inspector" FILES ${MPVulkanRendererInspector})
set(MPVulkanRendererFiles ${MPVulkanRendererFiles} ${MPVulkanRendererInspector})

set(MPVulkanRendererCommonFiles
	"${MPDir}/qcommon/matcomp.cpp"
	"${MPDir}/qcommon/q_shared.cpp"
	
	${SharedCommonFiles})
source_group("common" FILES ${MPVulkanRendererCommonFiles})
set(MPVulkanRendererFiles ${MPVulkanRendererFiles} ${MPVulkanRendererCommonFiles})

set(MPVulkanRendererCommonSafeFiles
	${SharedCommonSafeFiles}
	)
source_group("common/safe" FILES ${MPVulkanRendererCommonSafeFiles})
set(MPVulkanRendererFiles ${MPVulkanRendererFiles} ${MPVulkanRendererCommonSafeFiles})

# Transparently use either bundled or system libjpeg.
list(APPEND MPVulkanRendererIncludeDirectories ${JPEG_INCLUDE_DIR})
list(APPEND MPVulkanRendererLibraries          ${JPEG_LIBRARIES})

# Transparently use either bundled or system libpng.  Order is important --
# libpng used zlib, so it must come before it on the linker command line.
list(APPEND MPVulkanRendererIncludeDirectories ${PNG_INCLUDE_DIRS})
list(APPEND MPVulkanRendererLibraries          ${PNG_LIBRARIES})

# Transparently use either bundled or system zlib.
list(APPEND MPVulkanRendererIncludeDirectories ${ZLIB_INCLUDE_DIR})
list(APPEND MPVulkanRendererLibraries          ${ZLIB_LIBRARIES})

# Transparently use our bundled minizip.
list(APPEND MPVulkanRendererIncludeDirectories ${MINIZIP_INCLUDE_DIRS})
list(APPEND MPVulkanRendererLibraries          ${MINIZIP_LIBRARIES})

# Transparently use our bundled imgui library.
list(APPEND MPVulkanRendererIncludeDirectories ${IMGUI_INCLUDE_DIRS})
list(APPEND MPVulkanRendererLibraries          ${IMGUI_LIBRARIES})

# Add SDL2
if(UseInternalSDL2)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		list(APPEND MPVulkanRendererLibraries ${OpenJKLibDir}/SDL2/lib/x86/SDL2main.lib)
		list(APPEND MPVulkanRendererLibraries ${OpenJKLibDir}/SDL2/lib/x86/SDL2.lib)		
	else()
		list(APPEND MPVulkanRendererLibraries ${OpenJKLibDir}/SDL2/lib/x64/SDL2main.x86_64.lib)
		list(APPEND MPVulkanRendererLibraries ${OpenJKLibDir}/SDL2/lib/x64/SDL2.x86_64.lib)	
	endif()

	list(APPEND MPVulkanRendererIncludeDirectories ${OpenJKLibDir}/SDL2/include)
else()
	find_package(SDL2 REQUIRED)
	list(APPEND MPVulkanRendererIncludeDirectories ${SDL2_INCLUDE_DIR})
	list(APPEND MPVulkanRendererLibraries ${SDL2_LIBRARY})
endif()

#find_package(OpenGL REQUIRED)
#set(MPVulkanRendererIncludeDirectories ${MPVulkanRendererIncludeDirectories} ${OPENGL_INCLUDE_DIR})
#set(MPVulkanRendererLibraries ${MPVulkanRendererLibraries} ${OPENGL_LIBRARIES})

set(MPVulkanRendererIncludeDirectories ${MPVulkanRendererIncludeDirectories} ${OpenJKLibDir})
add_library(${MPVulkanRenderer} SHARED ${MPVulkanRendererFiles})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPVulkanRenderer} PROPERTIES PREFIX "")
endif()

if(WIN32)
	install(TARGETS ${MPVulkanRenderer}
		RUNTIME
		DESTINATION ${JKAInstallDir}
		COMPONENT ${JKAMPClientComponent})
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPVulkanRenderer}
			LIBRARY
			DESTINATION "${JKAInstallDir}/${MPEngine}.app/Contents/MacOS/"
			COMPONENT ${JKAMPClientComponent})
	else()
		install(TARGETS ${MPVulkanRenderer}
			LIBRARY
			DESTINATION ${JKAInstallDir}
			COMPONENT ${JKAMPClientComponent})
	endif()
endif()

set_target_properties(${MPVulkanRenderer} PROPERTIES COMPILE_DEFINITIONS "${SharedDefines}")

# Hide symbols not explicitly marked public.
set_property(TARGET ${MPVulkanRenderer} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

set_target_properties(${MPVulkanRenderer} PROPERTIES INCLUDE_DIRECTORIES "${MPVulkanRendererIncludeDirectories}")
set_target_properties(${MPVulkanRenderer} PROPERTIES PROJECT_LABEL "MP Vulkan Renderer")
target_link_libraries(${MPVulkanRenderer} ${MPVulkanRendererLibraries})