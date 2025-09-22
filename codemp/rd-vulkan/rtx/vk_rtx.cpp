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

VkDescriptorSet vk_rtx_get_current_desc_set_textures()
{
#if 1
	return (vk.frame_counter & 1) ? vk.desc_set_textures_odd : vk.desc_set_textures_even;
#else
	return vk.swapchain_image_index ? vk.desc_set_textures_odd : vk.desc_set_textures_even;
#endif
}

byte* BSP_GetPvs( world_t *bsp, int cluster )
{
	if (!bsp->vis || !bsp->numClusters)
		return NULL;
	
	if (cluster < 0 || cluster >= bsp->numClusters)
		return NULL;

	return (byte*)bsp->vis + bsp->clusterBytes * cluster;
}

byte* BSP_GetPvs2( world_t *bsp, int cluster )
{
	if (!bsp->vis || !bsp->numClusters)
		return NULL;

	if (cluster < 0 || cluster >= bsp->numClusters)
		return NULL;

	return (byte*)bsp->vis2 + bsp->clusterBytes * cluster;
}

byte *BSP_ClusterVis( world_t *bsp, byte *mask, int cluster, int vis )
{
	if ( !bsp || !bsp->vis ) {
		memset( mask, 0xff, VIS_MAX_BYTES );
		return mask;
	}

    if ( cluster == -1 ) {
        memset( mask, 0, bsp->clusterBytes );
		return mask;
    }

    //if ( cluster < 0 || cluster >= bsp->numClusters ) {
    if ( cluster < 0 || cluster >= bsp->numClusters ) {
        Com_Error( ERR_DROP, "%s: bad cluster", __func__ );
    }

	if ( vis == DVIS_PVS2 )
	{
		if ( bsp->vis2 )
		{
			byte *row = BSP_GetPvs2( bsp, cluster );
			memcpy( mask, row, bsp->clusterBytes );
			return mask;
		}

		// fallback
		vis = DVIS_PVS;
	}

	if ( vis == DVIS_PVS/* && bsp->pvs_matrix*/ )
	{
		byte *row = BSP_GetPvs( bsp, cluster );
		memcpy(mask, row, bsp->clusterBytes);
		return mask;
	}

	return mask;
}

void vk_rtx_show_pvs_f( void )
{
	if ( !vk.rtxActive )
		return;

	if ( backEnd.refdef.feedback.lookatcluster < 0 )
	{
		memset(vk.cluster_debug_mask, 0, sizeof(vk.cluster_debug_mask));
		vk.cluster_debug_index = -1;
		return;
	}

	BSP_ClusterVis( tr.world, (byte*)vk.cluster_debug_mask, backEnd.refdef.feedback.lookatcluster, DVIS_PVS);
	vk.cluster_debug_index =  backEnd.refdef.feedback.lookatcluster;
}

static float halton( int base, int index ) {
	float f = 1.f;
	float r = 0.f;
	int i = index;

	while (i > 0)
	{
		f = f / base;
		r = r + f * (i % base);
		i = i / base;
	}
	return r;
};

static inline qboolean extents_equal(VkExtent2D a, VkExtent2D b)
{
	return ( a.width == b.width && a.height == b.height ) ? qtrue: qfalse;
}

static VkExtent2D vk_rtx_get_render_extent()
{
	// yup ..
	vk.extent_unscaled.width = glConfig.vidWidth;
	vk.extent_unscaled.height = glConfig.vidHeight;

	int scale = 100;
	//(drs_effective_scale != 0) ? drs_effective_scale : scr_viewsize->integer;

	VkExtent2D result;
	result.width = (uint32_t)(vk.extent_unscaled.width * (float)scale / 100.f);
	result.height = (uint32_t)(vk.extent_unscaled.height * (float)scale / 100.f);

	result.width = (result.width + 1) & ~1;

	return result;
}

static VkExtent2D vk_rtx_get_screen_image_extent()
{
	VkExtent2D result;
	/*if ( cvar_drs_enable->integer )
	{
		//int drs_maxscale = MAX(sun_drs_minscale->integer, sun_drs_maxscale->integer);
		int drs_maxscale = MAX(50, 100);
		result.width = (uint32_t)(vk.extent_unscaled.width * (float)drs_maxscale / 100.f);
		result.height = (uint32_t)(vk.extent_unscaled.height * (float)drs_maxscale / 100.f);
	}
	else*/
	{
		result.width = MAX(vk.extent_render.width, vk.extent_unscaled.width);
		result.height = MAX(vk.extent_render.height, vk.extent_unscaled.height);
	}

	result.width = (result.width + 1) & ~1;

	return result;
}

void vk_rtx_cvar_handler( void ) 
{
	#define CVAR_CHANGED( _handle, _action ) \
	if ( _handle->modified ) { \
		_action(); \
		_handle->modified = qfalse; \
	}

	// temporal
	CVAR_CHANGED( sun_flt_temporal_hf,		temporal_cvar_changed )
	CVAR_CHANGED( sun_flt_temporal_lf,		temporal_cvar_changed )
	CVAR_CHANGED( sun_flt_temporal_spec,	temporal_cvar_changed )
	CVAR_CHANGED( sun_flt_enable,			temporal_cvar_changed )

	// sky and sun
	CVAR_CHANGED( sun_elevation,				physical_sky_cvar_changed )
	CVAR_CHANGED( sun_azimuth,					physical_sky_cvar_changed )
	CVAR_CHANGED( sun_angle,					physical_sky_cvar_changed )
	CVAR_CHANGED( sun_brightness,				physical_sky_cvar_changed )

	CVAR_CHANGED( sky_scattering,				physical_sky_cvar_changed )
	CVAR_CHANGED( sky_transmittance,			physical_sky_cvar_changed )
	CVAR_CHANGED( sky_phase_g,					physical_sky_cvar_changed )
	CVAR_CHANGED( sky_amb_phase_g,				physical_sky_cvar_changed )

	CVAR_CHANGED( sun_bounce,					physical_sky_cvar_changed )
	CVAR_CHANGED( sun_animate,					physical_sky_cvar_changed )
	CVAR_CHANGED( sun_preset,					physical_sky_cvar_changed )
	CVAR_CHANGED( sun_latitude,					physical_sky_cvar_changed )
	CVAR_CHANGED( sun_bounce,					physical_sky_cvar_changed )

	CVAR_CHANGED( physical_sky,					physical_sky_cvar_changed )
	CVAR_CHANGED( physical_sky_draw_clouds,		physical_sky_cvar_changed )
	CVAR_CHANGED( physical_sky_space,			physical_sky_cvar_changed )
	CVAR_CHANGED( physical_sky_brightness,		physical_sky_cvar_changed )
#undef CVAR_CHANGED
}

// on vid restart or level change
void vk_rtx_begin_registration( void )
{
	vkpt_physical_sky_latch_local_time();

	vk.cluster_debug_index = -1;
	memset( vk. cluster_debug_mask, 0, sizeof(vk. cluster_debug_mask) );
}

void vk_rtx_begin_frame( void ) 
{
	VK_BeginRenderClear();

	vk.extent_render = vk_rtx_get_render_extent();
	vk.gpu_slice_width = (vk.extent_render.width + vk.device_count - 1) / vk.device_count;
	
	VkExtent2D extent_screen_images = vk_rtx_get_screen_image_extent();

	if ( !extents_equal( extent_screen_images, vk.extent_screen_images ) )
	{
		vk.extent_screen_images = extent_screen_images;
	}

	vk.extent_taa_images.width = MAX(vk.extent_screen_images.width, vk.extent_unscaled.width);
	vk.extent_taa_images.height = MAX(vk.extent_screen_images.height, vk.extent_unscaled.height);
}

void vk_rtx_initialize( void )
{
	if ( !vk.rtxActive ) 
		return;

	uint32_t i;
	const VkImageUsageFlags imgFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	vk.extent_render = vk_rtx_get_render_extent();
	vk.extent_screen_images = vk_rtx_get_screen_image_extent();

	vk.extent_taa_images.width = MAX(vk.extent_screen_images.width, vk.extent_unscaled.width);
	vk.extent_taa_images.height = MAX(vk.extent_screen_images.height, vk.extent_unscaled.height);

	vk.device_count = 1;	// right
	vk.gpu_slice_width = (vk.extent_render.width + vk.device_count - 1) / vk.device_count;
	
	vk.shaderGroupBaseAlignment = vk.ray_pipeline_properties.shaderGroupBaseAlignment;
	vk.shaderGroupHandleSize = vk.ray_pipeline_properties.shaderGroupHandleSize;
	vk.minAccelerationStructureScratchOffsetAlignment = vk.accel_struct_properties.minAccelerationStructureScratchOffsetAlignment;

	for ( i = 0; i < NUM_TAA_SAMPLES; i++ )
	{
		vk.taa_samples[i][0] = halton(2, i + 1) - 0.5f;
		vk.taa_samples[i][1] = halton(3, i + 1) - 0.5f;
	}

	vk_rtx_initialize_images();
	vk_rtx_create_images();
	vk_rtx_create_buffers();
	vk_rtx_create_model_vbo_ibo_descriptor();

	// game textures descriptor
	Com_Memset( &vk.imageDescriptor, 0, sizeof(vkdescriptor_t) );
	vk.imageDescriptor.lastBindingVariableSizeExt = qtrue;
	vk_rtx_add_descriptor_sampler( &vk.imageDescriptor, 0, (VkShaderStageFlagBits)VK_GLOBAL_IMAGEARRAY_SHADER_STAGE_FLAGS, MAX_DRAWIMAGES, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	vk_rtx_create_descriptor( &vk.imageDescriptor );

	vk_rtx_clear_material_list();
}

void vk_rtx_shutdown( void ) 
{
	if ( !vk.rtxActive ) 
		return;

	uint32_t i;

	VK_DestroyBuffer( &vk.buf_shader_binding_table );

	vk_rtx_destroy_shaders();
	vk_rtx_destroy_buffers();

	if( tr.world )
	{ 
		vk_rtx_destroy_compute_pipelines();
		vk_rtx_destroy_rt_descriptors();
		vk_rtx_destroy_rt_pipelines();

		vk_rtx_destroy_accel_all();
	}

	vk_rtx_clear_material_list();

	vk.scratch_buf_ptr = 0;
}