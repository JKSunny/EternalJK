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

#ifndef VK_PHYSICAL_SKY_H
#define VK_PHYSICAL_SKY_H

#define LIST_PHYSICAL_SKY_SHADERS \
	SHADER_MODULE_DO( SHADER_PHYSICAL_SKY_COMP,				physical_sky_comp ) \
	SHADER_MODULE_DO( SHADER_SKY_BUFFER_RESOLVE_COMP,		sky_buffer_resolve_comp )

#define LIST_PHYSICAL_SKY_PIPELINES \
	PIPELINE_DO( PHYSICAL_SKY ) \
	PIPELINE_DO( SKY_BUFFER_RESOLVE )

#define LIST_SUN_PRESET \
	SUN_PRESET_DO( SUN_PRESET_NONE ) \
	SUN_PRESET_DO( SUN_PRESET_CURRENT_TIME ) \
	SUN_PRESET_DO( SUN_PRESET_FAST_TIME ) \
	SUN_PRESET_DO( SUN_PRESET_NIGHT ) \
	SUN_PRESET_DO( SUN_PRESET_DAWN ) \
	SUN_PRESET_DO( SUN_PRESET_MORNING ) \
	SUN_PRESET_DO( SUN_PRESET_NOON ) \
	SUN_PRESET_DO( SUN_PRESET_EVENING ) \
	SUN_PRESET_DO( SUN_PRESET_DUSK )

// shaders
enum {
#define SHADER_MODULE_DO( _index, ... ) _index,
	LIST_PHYSICAL_SKY_SHADERS
#undef SHADER_MODULE_DO
	NUM_PHYSICAL_SKY_SHADER_MODULES
};

// pipelines
enum {
#define PIPELINE_DO( _index ) _index,
	LIST_PHYSICAL_SKY_PIPELINES
#undef PIPELINE_DO
	PHYSICAL_SKY_NUM_PIPELINES
};

// sun preset
typedef enum
{
#define SUN_PRESET_DO( _index, ... ) _index,
	LIST_SUN_PRESET
#undef SUN_PRESET_DO
	SUN_PRESET_COUNT
} sun_preset_t;

typedef struct PhysicalSkyDesc {
    vec3_t		sunColor;
    float		sunAngularDiameter; // size of sun in sky
    vec3_t		groundAlbedo;
    uint32_t	flags;
	int			preset;
} PhysicalSkyDesc_t;

VkResult	vkpt_physical_sky_initialize( void );
VkResult	vkpt_physical_sky_destroy( void );
void		vk_create_physical_sky_pipelines( void );
void		vk_load_physical_sky_shaders( void );
void		vk_destroy_physical_sky_pipelines( void );

int			active_sun_preset( void );
void		vkpt_physical_sky_latch_local_time( void );
void		vk_rtx_reset_envmap( void );
qboolean	vkpt_physical_sky_needs_update( void );
VkResult	vkpt_physical_sky_record_cmd_buffer( VkCommandBuffer cmd_buf );

void		physical_sky_cvar_changed( void );
void		UpdatePhysicalSkyCVars( void );
const PhysicalSkyDesc_t		*GetSkyPreset( int index );
void		CalculateDirectionToSun( float DayOfYear, float TimeOfDay, float LatitudeDegrees, vec3_t result );
#endif // VK_PHYSICAL_SKY_H