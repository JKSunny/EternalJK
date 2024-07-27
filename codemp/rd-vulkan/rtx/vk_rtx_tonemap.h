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

#ifndef VK_TONEMAP_H
#define VK_TONEMAP_H

#define LIST_TONEMAP_SHADERS \
	SHADER_MODULE_DO( SHADER_TONE_MAPPING_HISTOGRAM_COMP,	tone_mapping_histogram_comp ) \
	SHADER_MODULE_DO( SHADER_TONE_MAPPING_CURVE_COMP,		tone_mapping_curve_comp	 ) \
	SHADER_MODULE_DO( SHADER_TONE_MAPPING_APPLY_COMP,		tone_mapping_apply_comp)

#define LIST_TONEMAP_PIPELINES \
	PIPELINE_DO( TONE_MAPPING_HISTOGRAM ) \
	PIPELINE_DO( TONE_MAPPING_CURVE ) \
	PIPELINE_DO( TONE_MAPPING_APPLY_SDR ) \
	PIPELINE_DO( TONE_MAPPING_APPLY_HDR ) \

// Here are each of the pipelines we'll be using, followed by an additional
// enum value to count the number of tone mapping pipelines.

// shaders
enum {
#define SHADER_MODULE_DO( _index, ... ) _index,
	LIST_TONEMAP_SHADERS
#undef SHADER_MODULE_DO
	TM_NUM_SHADERS
};

// pipelines
enum {
#define PIPELINE_DO( _index ) _index,
	LIST_TONEMAP_PIPELINES
#undef PIPELINE_DO
	TM_NUM_PIPELINES
};

void		vk_create_tonemap_pipelines( void );
void		vk_load_tonemap_shaders( void );
void		vk_destroy_tonemap_pipelines( void );

VkResult vkpt_tone_mapping_record_cmd_buffer( VkCommandBuffer cmd_buf, float frame_time );

#endif // VK_TONEMAP_H