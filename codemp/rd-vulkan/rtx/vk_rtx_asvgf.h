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

#ifndef VK_ASVGF_H
#define VK_ASVGF_H

#define LIST_ASVGF_SHADERS \
	SHADER_MODULE_DO( SHADER_ASVGF_GRADIENT_IMG_COMP,			asvgf_gradient_img_comp		 ) \
	SHADER_MODULE_DO( SHADER_ASVGF_GRADIENT_ATROUS_COMP,		asvgf_gradient_atrous_comp	 ) \
	SHADER_MODULE_DO( SHADER_ASVGF_GRADIENT_REPROJECT_COMP,		asvgf_gradient_reproject_comp) \
	SHADER_MODULE_DO( SHADER_ASVGF_ATROUS_COMP,					asvgf_atrous_comp			 ) \
	SHADER_MODULE_DO( SHADER_ASVGF_LF_COMP,						asvgf_lf_comp				 ) \
	SHADER_MODULE_DO( SHADER_ASVGF_TEMPORAL_COMP,				asvgf_temporal_comp			 ) \
	SHADER_MODULE_DO( SHADER_CHECKERBOARD_INTERLEAVE_COMP,		checkerboard_interleave_comp ) \
	SHADER_MODULE_DO( SHADER_ASVGF_TAAU_COMP,					asvgf_taau_comp				 ) \
	SHADER_MODULE_DO( SHADER_COMPOSITING_COMP,					compositing_comp			 ) \
	SHADER_MODULE_DO( SHADER_SHADOW_MAP_VERT,					shadow_map_vert				 ) \
	SHADER_MODULE_DO( SHADER_GOD_RAYS_COMP,						god_rays_comp				 ) \
	SHADER_MODULE_DO( SHADER_GOD_RAYS_FILTER_COMP,				god_rays_filter_comp		 ) \
	SHADER_MODULE_DO( SHADER_INSTANCE_GEOMETRY_COMP,			instance_geometry_comp		 ) \

#define LIST_ASVGF_PIPELINES \
	PIPELINE_DO( GRADIENT_IMAGE ) \
	PIPELINE_DO( GRADIENT_ATROUS ) \
	PIPELINE_DO( GRADIENT_REPROJECT ) \
	PIPELINE_DO( TEMPORAL ) \
	PIPELINE_DO( ATROUS_LF ) \
	PIPELINE_DO( ATROUS_ITER_0 ) \
	PIPELINE_DO( ATROUS_ITER_1 ) \
	PIPELINE_DO( ATROUS_ITER_2 ) \
	PIPELINE_DO( ATROUS_ITER_3 ) \
	PIPELINE_DO( CHECKERBOARD_INTERLEAVE ) \
	PIPELINE_DO( TAAU ) \
	PIPELINE_DO( COMPOSITING ) \

// shaders
enum {
#define SHADER_MODULE_DO( _index, ... ) _index,
	LIST_ASVGF_SHADERS
#undef SHADER_MODULE_DO
	NUM_ASVGF_SHADER_MODULES
};

// pipelines
enum {
#define PIPELINE_DO( _index ) _index,
	LIST_ASVGF_PIPELINES
#undef PIPELINE_DO
	ASVGF_NUM_PIPELINES
};

void		vk_create_asvgf_pipelines( void );
void		vk_load_asvgf_shaders( void );
void		vk_destroy_asvgf_pipelines( void );

VkResult	vkpt_asvgf_gradient_reproject( VkCommandBuffer cmd_buf );
VkResult	vkpt_asvgf_filter( VkCommandBuffer cmd_buf, qboolean enable_lf );
VkResult	vkpt_interleave( VkCommandBuffer cmd_buf );
VkResult	vkpt_taa( VkCommandBuffer cmd_buf );
VkResult	vkpt_compositing( VkCommandBuffer cmd_buf );
#endif // VK_ASVGF_H