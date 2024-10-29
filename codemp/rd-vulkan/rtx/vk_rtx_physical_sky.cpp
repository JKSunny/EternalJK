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

static int width  = 1024, 
           height = 1024;

static int		skyNeedsUpdate = VK_TRUE;

static bool		has_envmap = false;
static time_t	latched_local_time;

static int		current_preset = 0;

#define BARRIER_COMPUTE( cmd_buf, img ) \
	do { \
		VkImageSubresourceRange range; \
		VkImageMemoryBarrier barrier; \
		Com_Memset( &barrier, 0, sizeof(VkImageMemoryBarrier) ); \
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; \
		range.baseMipLevel = 0; \
		range.levelCount = 1; \
		range.baseArrayLayer = 0; \
		range.layerCount = 1; \
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; \
		barrier.pNext = NULL; \
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; \
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; \
		barrier.image = img; \
		barrier.subresourceRange = range; \
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; \
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; \
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL; \
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL; \
		qvkCmdPipelineBarrier( cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, \
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, \
				1, &barrier); \
	} while(0)

int active_sun_preset( void )
{
	return sun_preset->integer;
}

void vkpt_physical_sky_latch_local_time( void )
{
	time( &latched_local_time );
}

static void change_image_layouts( VkImage image, const VkImageSubresourceRange* subresource_range )
{
	VkCommandBuffer cmd_buf = vk_begin_command_buffer();

	IMAGE_BARRIER( cmd_buf,
		image,	// vk.img_physical_sky[binding_offset].handle
		*subresource_range,
		0,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL
	);

	vk_end_command_buffer( cmd_buf );
	vk_wait_idle();
}

static void vk_rtx_destroy_env_texture( void )
{
	const int binding_offset = ( BINDING_OFFSET_PHYSICAL_SKY - ( BINDING_OFFSET_PHYSICAL_SKY ) );

	vk_rtx_destroy_image( &vk.img_physical_sky[binding_offset] );
#if 0
    if ( vk.img_physical_sky[binding_offset].view != VK_NULL_HANDLE ) {
        qvkDestroyImageView( vk.device, vk.img_physical_sky[binding_offset].view, NULL );
        vk.img_physical_sky[binding_offset].view = NULL;
    }
    if ( vk.img_physical_sky[binding_offset].handle != VK_NULL_HANDLE ) {
        qvkDestroyImage( vk.device, vk.img_physical_sky[binding_offset].handle, NULL );
        vk.img_physical_sky[binding_offset].handle = NULL;
    }
	if ( vk.img_physical_sky[binding_offset].memory != VK_NULL_HANDLE ) {
		qvkFreeMemory( vk.device, vk.img_physical_sky[binding_offset].memory, NULL );
		vk.img_physical_sky[binding_offset].memory = VK_NULL_HANDLE;
	}
#endif
}

static VkResult vk_rtx_init_env_texture( int width, int height )
{
    vk_wait_idle();
    vk_rtx_destroy_env_texture();

	const int binding_offset = ( BINDING_OFFSET_PHYSICAL_SKY - ( BINDING_OFFSET_PHYSICAL_SKY ) );
    const int num_images = 6;

    // cube image

    VkImageCreateInfo img_info;
	Com_Memset( &img_info, 0, sizeof(VkImageCreateInfo) );
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.pNext = NULL;
    img_info.extent = {
        (uint32_t)width,
        (uint32_t)height,
        1,
    };
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    img_info.mipLevels = 1;
    img_info.arrayLayers = num_images;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.usage = VK_IMAGE_USAGE_STORAGE_BIT
                            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT;
    img_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img_info.queueFamilyIndexCount = 0;
	//img_info.queueFamilyIndexCount = vk.queue_family_index;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK( qvkCreateImage( vk.device, &img_info, NULL, &vk.img_physical_sky[binding_offset].handle ) );
    //ATTACH_LABEL_VARIABLE(img_envmap, IMAGE);

    VkMemoryRequirements mem_req;
    qvkGetImageMemoryRequirements( vk.device, vk.img_physical_sky[binding_offset].handle, &mem_req );
    //assert(mem_req.size >= (img_size * num_images));

	VK_CHECK( allocate_gpu_memory( mem_req, &vk.img_physical_sky[binding_offset].memory ) );

    VK_CHECK( qvkBindImageMemory( vk.device, vk.img_physical_sky[binding_offset].handle, vk.img_physical_sky[binding_offset].memory, 0) );

	VkImageSubresourceRange subresource_range;
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = num_images;

	change_image_layouts( vk.img_physical_sky[binding_offset].handle, &subresource_range );


    // image view
    VkImageViewCreateInfo img_view_info;
	Com_Memset( &img_view_info, 0, sizeof(VkImageViewCreateInfo) );
    img_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_info.pNext = NULL;
    img_view_info.flags = 0;
    img_view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    img_view_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    img_view_info.image = vk.img_physical_sky[binding_offset].handle;
    img_view_info.subresourceRange = subresource_range;
    img_view_info.components = {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A,
    };

    VK_CHECK( qvkCreateImageView( vk.device, &img_view_info, NULL, &vk.img_physical_sky[binding_offset].view ) );
    //ATTACH_LABEL_VARIABLE(imv_envmap, IMAGE_VIEW);

	int binding_offset_env_tex = ( BINDING_OFFSET_PHYSICAL_SKY - ( BINDING_OFFSET_PHYSICAL_SKY ) );

    // cube descriptor layout
    {
        VkDescriptorImageInfo desc_img_info;
        desc_img_info.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
        desc_img_info.imageView		= vk.img_physical_sky[binding_offset_env_tex].view;
        desc_img_info.sampler		= vk.tex_sampler;

        VkWriteDescriptorSet s;
		Com_Memset( &s, 0, sizeof(VkWriteDescriptorSet) );
        s.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		s.pNext				= NULL;
        s.dstSet			= vk.desc_set_textures_even;
        s.dstBinding		= BINDING_OFFSET_PHYSICAL_SKY;
        s.dstArrayElement	= 0;
        s.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        s.descriptorCount	= 1;
        s.pImageInfo		= &desc_img_info;

        qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );

        s.dstSet = vk.desc_set_textures_odd;
        qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );
    }

    // image descriptor
    {
        VkDescriptorImageInfo desc_img_info;
        desc_img_info.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
        desc_img_info.imageView		= vk.img_physical_sky[binding_offset_env_tex].view;
		desc_img_info.sampler		= VK_NULL_HANDLE;

        VkWriteDescriptorSet s;
		Com_Memset( &s, 0, sizeof(VkWriteDescriptorSet) );
        s.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		s.pNext				= NULL;
        s.dstSet			= vk.desc_set_textures_even;
        s.dstBinding		= BINDING_OFFSET_PHYSICAL_SKY_IMG;
        s.dstArrayElement	= 0;
        s.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        s.descriptorCount	= 1;
        s.pImageInfo		= &desc_img_info;

        qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );

        s.dstSet = vk.desc_set_textures_odd;
        qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );
    }
	return VK_SUCCESS;
}

VkResult vkpt_physical_sky_initialize( void ) 
{
	current_preset = 0;

	SkyInitializeDataGPU();

	// requires initialization to create image and sampler handles
	vk_rtx_init_sky_scatter();
	vk_rtx_init_env_texture( width, height );

	InitializeShadowmapResources();

	return VK_SUCCESS;
}

VkResult vkpt_physical_sky_destroy( void )
{
	current_preset = 0;

	ReleaseShadowmapResources();
	SkyReleaseDataGPU();

	vk_rtx_destroy_env_texture();

	return VK_SUCCESS;
}

static void vk_create_physical_sky_pipeline( uint32_t pipeline_index, uint32_t shader_index, 
	VkSpecializationInfo *spec, uint32_t push_size ) 
{
	vkpipeline_t *pipeline = &vk.physical_sky_pipeline[pipeline_index];

	VkDescriptorSetLayout set_layouts[4] = {
		vk.computeDescriptor[0].layout,
		vk.desc_set_layout_textures,
		vk.imageDescriptor.layout,
		vk.desc_set_layout_ubo
	};

	vk_rtx_bind_pipeline_shader( pipeline, vk.physical_sky_shader[shader_index] );
	vk_rtx_bind_pipeline_desc_set_layouts( pipeline, set_layouts, ARRAY_LEN(set_layouts) );
	vk_rtx_create_compute_pipeline( pipeline, spec, push_size );

	skyNeedsUpdate = VK_TRUE;
}

void vk_create_physical_sky_pipelines( void ) 
{
	vk_load_physical_sky_shaders();

	vk_create_physical_sky_pipeline( PHYSICAL_SKY,			SHADER_PHYSICAL_SKY_COMP,		NULL, sizeof(float) * 16 );
	vk_create_physical_sky_pipeline( SKY_BUFFER_RESOLVE,	SHADER_SKY_BUFFER_RESOLVE_COMP, NULL, 0 );
}

void vk_destroy_physical_sky_pipelines( void ) 
{
	 uint32_t i;

	 for ( i = 0; i < PHYSICAL_SKY_NUM_PIPELINES; i++ )
		 vk_rtx_destroy_pipeline( &vk.physical_sky_pipeline[i] );
}

//
// traditional envmap begin
//
void vk_rtx_reset_envmap( void ) 
{
	vk_rtx_destroy_image( &vk.img_envmap );
	has_envmap = false;
}

void vk_rtx_set_envmap_descriptor_binding( void )
{
	VkDescriptorImageInfo desc_img_info;
	desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	desc_img_info.imageView   = vk.img_envmap.view;
	desc_img_info.sampler     = vk.tex_sampler;

	VkWriteDescriptorSet s;
	Com_Memset( &s, 0, sizeof(VkWriteDescriptorSet) );
	s.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	s.pNext				= NULL;
	s.dstSet			= vk.desc_set_textures_even;
	s.dstBinding		= BINDING_OFFSET_ENVMAP;
	s.dstArrayElement	= 0;
	s.descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	s.descriptorCount	= 1;
	s.pImageInfo		= &desc_img_info;

	qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );

	s.dstSet = vk.desc_set_textures_odd;
	qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );
}

void vk_rtx_prepare_envmap( world_t &worldData ) 
{
	uint32_t	i;
	int			width, height;
	byte		*pic;

	if ( has_envmap )
		return;

	for ( i = 0; i < worldData.numsurfaces; i++ ) 
	{ 
		shader_t *shader = tr.shaders[ worldData.surfaces[i].shader->index ];

		if ( !shader->isSky ) 
			continue;
		
		if ( shader->sky->outerbox[0] != NULL ) 
		{
			width = shader->sky->outerbox[0]->width;
			height = shader->sky->outerbox[0]->height;

			vk_rtx_create_cubemap( &vk.img_envmap, width, height,
				VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1 );

			R_LoadImage( shader->sky->outerbox[3]->imgName, &pic, &width, &height );

			if ( width == 0 || height == 0 ) 
				goto skyFromStage;

			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 0 ); // back
			ri.Z_Free( pic );

			R_LoadImage( shader->sky->outerbox[1]->imgName, &pic, &width, &height );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 1 ); // front
			ri.Z_Free( pic );

			R_LoadImage(shader->sky->outerbox[4]->imgName, &pic, &width, &height );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 2 ); // bottom
			ri.Z_Free( pic );

			R_LoadImage(shader->sky->outerbox[5]->imgName, &pic, &width, &height );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 3 ); // up
			ri.Z_Free( pic );

			R_LoadImage(shader->sky->outerbox[0]->imgName, &pic, &width, &height );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 4 ) ; // right
			ri.Z_Free( pic );

			R_LoadImage(shader->sky->outerbox[2]->imgName, &pic, &width, &height );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 5 ); // left
			ri.Z_Free( pic );
		}

		else if ( shader->stages[0] != NULL ) 
		{
		skyFromStage:
			width = shader->stages[0]->bundle[0].image[0]->width;
			height = shader->stages[0]->bundle[0].image[0]->height;

			vk_rtx_create_cubemap( &vk.img_envmap, width, height,
				VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1 );

			R_LoadImage( shader->stages[0]->bundle[0].image[0]->imgName, &pic, &width, &height );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 0 );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 1 );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 2 );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 3 );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 4 );
			vk_rtx_upload_image_data( &vk.img_envmap, width, height, pic, 4, 0, 5 );
		}

		vk_rtx_set_envmap_descriptor_binding();

		has_envmap = true;
		break;
	}
}
//
// traditional envmap end
//


void prepare_sky_matrix( float time, vec3_t sky_matrix[3] ) 
{
#if 0
	if (sky_rotation != 0.f)
	{
		SetupRotationMatrix(sky_matrix, sky_axis, time * sky_rotation);
	}
	else
	{
#endif
		VectorSet( sky_matrix[0], 1.f, 0.f, 0.f );
		VectorSet( sky_matrix[1], 0.f, 1.f, 0.f );
		VectorSet( sky_matrix[2], 0.f, 0.f, 1.f );
//	}
}

static void 
reset_sun_color_buffer(VkCommandBuffer cmd_buf)
{
	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_UNIFORM_READ_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		vk.buf_sun_color.buffer,
		0,
		VK_WHOLE_SIZE
	);

	qvkCmdFillBuffer(cmd_buf, vk.buf_sun_color.buffer,
		0, VK_WHOLE_SIZE, 0);

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_WRITE_BIT,
		vk.buf_sun_color.buffer,
		0,
		VK_WHOLE_SIZE
	);
}

qboolean vkpt_physical_sky_needs_update( void ) {
	return (qboolean)skyNeedsUpdate;
}

extern float terrain_shadowmap_viewproj[16];

VkResult vkpt_physical_sky_record_cmd_buffer( VkCommandBuffer cmd_buf )
{
	if ( !skyNeedsUpdate )
		return VK_SUCCESS;

	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	VkDescriptorSet desc_sets[] = {
		vk.computeDescriptor[idx].set,
		vk_rtx_get_current_desc_set_textures(),
		vk.imageDescriptor.set,
		vk.desc_set_ubo
	};

	vk_rtx_create_shadow_map_framebuffer( &vk.precomputed_shadowmapdata );

	RecordCommandBufferShadowmap( cmd_buf );

	reset_sun_color_buffer( cmd_buf );

	{
		uint32_t pipeline_index = PHYSICAL_SKY;

		if ( physical_sky_space->integer > 0 )
			pipeline_index = PHYSICAL_SKY;
		else
			pipeline_index = PHYSICAL_SKY;

		qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.physical_sky_pipeline[pipeline_index].handle );

		qvkCmdPushConstants( cmd_buf, vk.physical_sky_pipeline[pipeline_index].layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float) * 16, terrain_shadowmap_viewproj );

		qvkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
				vk.physical_sky_pipeline[pipeline_index].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0);

		const int group_size = 16;
		qvkCmdDispatch( cmd_buf, width / group_size, height / group_size, 6 );
	}

	//BARRIER_COMPUTE( cmd_buf, img_envmap );

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_SHADER_WRITE_BIT,
		vk.buf_sun_color.buffer,
		0,
		VK_WHOLE_SIZE
	);

	{
		qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, vk.physical_sky_pipeline[SKY_BUFFER_RESOLVE].handle );
		
		qvkCmdBindDescriptorSets( cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
				vk.physical_sky_pipeline[SKY_BUFFER_RESOLVE].layout, 0, ARRAY_LEN(desc_sets), desc_sets, 0, 0 );


		qvkCmdDispatch(cmd_buf, 1, 1, 1);
	}

	BUFFER_BARRIER( cmd_buf,
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_UNIFORM_READ_BIT,
		vk.buf_sun_color.buffer,
		0,
		VK_WHOLE_SIZE
	);

	skyNeedsUpdate = VK_FALSE;

	return VK_SUCCESS;
}

// unused
void vkpt_next_sun_preset()
{
	int preset;

	if ( sun_preset->integer < SUN_PRESET_NIGHT || sun_preset->integer > SUN_PRESET_DUSK )
		preset = SUN_PRESET_MORNING;
	else
	{
		preset = sun_preset->integer + 1;
		if ( preset > SUN_PRESET_DUSK )
			preset = SUN_PRESET_NIGHT;
	}

	ri.Cvar_Set( "sun_preset", va( "%d", preset ) );

#ifdef USE_VK_IMGUI
	vk_imgui_bind_rtx_cvars();
#endif
}

void vk_rtx_init_sky_scatter( void ) 
{
	PhysicalSkyDesc_t const *skyDesc = GetSkyPreset( physical_sky->integer );

	if ( (skyDesc->flags & PHYSICAL_SKY_FLAG_USE_SKYBOX) != 0 )
		return;

	SkyLoadScatterParameters( skyDesc->preset );

	current_preset = physical_sky->integer;
}

void vk_rtx_evaluate_sun_light( sun_light_t *light, const vec3_t sky_matrix[3], float time )
{
	static uint16_t skyIndex = -1;

	if ( physical_sky->integer != skyIndex )
	{   // update cvars with presets if the user changed the sky
		UpdatePhysicalSkyCVars();
		skyIndex = physical_sky->integer;
	}

	PhysicalSkyDesc_t const * skyDesc = GetSkyPreset(skyIndex);

	if ( (skyDesc->flags & PHYSICAL_SKY_FLAG_USE_SKYBOX) != 0 )
	{
		// physical sky is disabled - no direct sun light in this mode
		memset(light, 0, sizeof(*light));
		return;
	}

	if ( skyIndex != current_preset )
	{
		//vkQueueWaitIdle(vk.queue_graphics);
		vk_wait_idle();
		SkyLoadScatterParameters( skyDesc->preset );
		current_preset = skyIndex;
	}

#if 0
	process_gamepad_input();
#endif
	float azimuth, elevation;


	static float start_time = 0.0f, sun_animate_changed = 0.0f;
	if ( sun_animate->value != sun_animate_changed )
	{
		start_time = time;
		sun_animate_changed = sun_animate->value;
	}

	const int preset = active_sun_preset();


	if ( (preset == SUN_PRESET_CURRENT_TIME) || (preset == SUN_PRESET_FAST_TIME) )
	{
		qboolean fast_time = (preset == SUN_PRESET_FAST_TIME) ? qtrue : qfalse;

		struct tm* local_time;

		if ( fast_time )
			local_time = gmtime( &latched_local_time );
		else
			local_time = localtime( &latched_local_time );

		float time_of_day = local_time->tm_hour + local_time->tm_min / 60.f;
		if ( fast_time )
			time_of_day *= 12.f;
		else if ( local_time->tm_isdst )
			time_of_day -= 1.f;

		CalculateDirectionToSun( local_time->tm_yday, time_of_day, sun_latitude->value, light->direction_envmap );
	}
	else
	{
		switch ( preset )
		{	
			case SUN_PRESET_NIGHT:
				elevation = -90.f;
				azimuth = 0.f;
				break;

			case SUN_PRESET_DAWN:
				elevation = -3.f;
				azimuth = 0.f;
				break;

			case SUN_PRESET_MORNING:
				elevation = 25.f;
				azimuth = -15.f;
				break;

			case SUN_PRESET_NOON:
				elevation = 80.f;
				azimuth = -75.f;
				break;

			case SUN_PRESET_EVENING:
				elevation = 15.f;
				azimuth = 190.f;
				break;

			case SUN_PRESET_DUSK:
				elevation = -6.f;
				azimuth = 205.f;
				break;

			default:

				if ( sun_animate->value > 0.0f )
				{
					float elapsed = ( time - start_time ) * 1000.f * sun_animate->value;

					azimuth = fmod( sun_azimuth->value + elapsed / (24.f * 60.f * 60.f), 360.0f );

					float e = fmod( sun_elevation->value + elapsed / (60.f * 60.f), 360.0f );
					if (e > 270.f)
						elevation = -( 360.f - e );
					else if (e > 180.0f)
					{
						elevation = -( e - 180.0f );
						azimuth = fmod( azimuth + 180.f, 360.f );
					}
					else if (e > 90.0f)
					{
						elevation = 180.0f - e;
						azimuth = fmod( azimuth + 180.f, 360.f );
					}
					else
						elevation = e;
					elevation = MAX( -90, MIN( 90, elevation ) );

					skyNeedsUpdate = VK_TRUE;
				}
				else
				{
					azimuth = sun_azimuth->value;
					elevation = sun_elevation->value;
				}
			break;
		}

		float elevation_rad = elevation * M_PI / 180.0f; //max(-20.f, min(90.f, elevation)) * M_PI / 180.f;
		float azimuth_rad = azimuth * M_PI / 180.f;
		light->direction_envmap[0] = cosf(azimuth_rad) * cosf(elevation_rad);
		light->direction_envmap[1] = sinf(azimuth_rad) * cosf(elevation_rad);
		light->direction_envmap[2] = sinf(elevation_rad);
	}

	light->angular_size_rad = MAX(1.f, MIN(10.f, sun_angle->value)) * M_PI / 180.f;

	light->use_physical_sky = qtrue;

	// color before occlusion
	vec3_t sunColor = { sun_color[0]->value, sun_color[1]->value, sun_color[2]->value };
	VectorScale(sunColor, sun_brightness->value, light->color);

	// potentially visible - can be overridden if readback data says it's occluded
	if (physical_sky_space->integer)
		light->visible = qtrue;
	else
		light->visible = (qboolean)(light->direction_envmap[2] >= -sinf(light->angular_size_rad * 0.5f));

	vec3_t sun_direction_world = { 0.f };
	sun_direction_world[0] = light->direction_envmap[0] * sky_matrix[0][0] + light->direction_envmap[1] * sky_matrix[0][1] + light->direction_envmap[2] * sky_matrix[0][2];
	sun_direction_world[1] = light->direction_envmap[0] * sky_matrix[1][0] + light->direction_envmap[1] * sky_matrix[1][1] + light->direction_envmap[2] * sky_matrix[1][2];
	sun_direction_world[2] = light->direction_envmap[0] * sky_matrix[2][0] + light->direction_envmap[1] * sky_matrix[2][1] + light->direction_envmap[2] * sky_matrix[2][2];
	VectorCopy(sun_direction_world, light->direction);
}

VkResult vk_rtx_physical_sky_update_ubo( vkUniformRTX_t *ubo, const sun_light_t *light, qboolean render_world ) 
{
	PhysicalSkyDesc_t const *skyDesc = GetSkyPreset(physical_sky->integer);

	if ( physical_sky_space->integer ) {
		ubo->pt_env_scale = 0.3f;
	}
	else
	{
		const float min_brightness = -10.f;
		const float max_brightness = 2.f;

		float brightness = MAX( min_brightness, MIN( max_brightness, physical_sky_brightness->value ) );
		ubo->pt_env_scale = exp2f( brightness - 2.f );
	}

    // sun
    ubo->sun_bounce_scale = sun_bounce->value;
	ubo->sun_tan_half_angle = tanf(light->angular_size_rad * 0.5f);
	ubo->sun_cos_half_angle = cosf(light->angular_size_rad * 0.5f);
	ubo->sun_solid_angle = 2 * M_PI * (float)(1.0 - cos(light->angular_size_rad * 0.5)); // use double for precision
	//ubo->sun_solid_angle = max(ubo->sun_solid_angle, 1e-3f);

	VectorCopy( light->color, ubo->sun_color );
	VectorCopy( light->direction_envmap, ubo->sun_direction_envmap) ;
	VectorCopy( light->direction, ubo->sun_direction );

    if ( light->direction[2] >= 0.99f )
    {
        VectorSet(ubo->sun_tangent, 1.f, 0.f, 0.f);
        VectorSet(ubo->sun_bitangent, 0.f, 1.f, 0.f);
    }
    else
    {
        vec3_t up;
        VectorSet(up, 0.f, 0.f, 1.f);
        CrossProduct(light->direction, up, ubo->sun_tangent);
        VectorNormalize(ubo->sun_tangent);
        CrossProduct(light->direction, ubo->sun_tangent, ubo->sun_bitangent);
        VectorNormalize(ubo->sun_bitangent);
    }

	// clouds
	ubo->sky_transmittance	= sky_transmittance->value;
	ubo->sky_phase_g		= sky_phase_g->value;
	ubo->sky_amb_phase_g	= sky_amb_phase_g->value;
	ubo->sky_scattering		= sky_scattering->value;

    // atmosphere
	if ( !render_world )				
		ubo->environment_type = ENVIRONMENT_NONE;
	else if ( light->use_physical_sky )	
		ubo->environment_type = ENVIRONMENT_DYNAMIC;
	else								
		ubo->environment_type = ENVIRONMENT_STATIC;

    if ( light->use_physical_sky )
    {
		uint32_t flags = PHYSICAL_SKY_FLAG_NONE;
        // adjust flags from cvars here

        if ( physical_sky_draw_clouds->value > 0.0f )
            flags = flags | PHYSICAL_SKY_FLAG_DRAW_CLOUDS;
        else
            flags = flags & (~PHYSICAL_SKY_FLAG_DRAW_CLOUDS);

        ubo->physical_sky_flags = flags;

        // compute approximation of reflected radiance from ground
        vec3_t ground_radiance;

        VectorCopy( skyDesc->groundAlbedo, ground_radiance );
        VectorScale( ground_radiance, MAX( 0.f, light->direction_envmap[2] ), ground_radiance ); // N.L
        VectorVectorScale( ground_radiance, light->color, ground_radiance );
    }
	else
		skyNeedsUpdate = VK_FALSE;

#if 0
    // planet
    ubo->planet_albedo_map = physical_sky_planet_albedo_map;
    ubo->planet_normal_map = physical_sky_planet_normal_map;
#endif

	ubo->sun_visible = light->visible;

#if 0
	if (render_world && !(skyDesc->flags & PHYSICAL_SKY_FLAG_USE_SKYBOX))
	{
		vec3_t forward;
		VectorScale(light->direction_envmap, -1.0f, forward);
		UpdateTerrainShadowMapView(forward);
	}
#endif
	return VK_SUCCESS;
}

void physical_sky_cvar_changed( void )
{   // cvar callback to trigger a re-render of skybox
    skyNeedsUpdate = VK_TRUE;
#if 0
    vkpt_reset_accumulation();
#endif
}

void UpdatePhysicalSkyCVars( void )
{
    PhysicalSkyDesc_t const * sky = GetSkyPreset( physical_sky->integer );

	ri.Cvar_Set( "sun_color_r", va( "%f", sky->sunColor[0] ) );
	ri.Cvar_Set( "sun_color_g", va( "%f", sky->sunColor[1] ) );
	ri.Cvar_Set( "sun_color_b", va( "%f", sky->sunColor[2] ) );

	ri.Cvar_Set( "sun_angle", va( "%f", sky->sunAngularDiameter ) );

    skyNeedsUpdate = VK_TRUE;

#ifdef USE_VK_IMGUI
	vk_imgui_bind_rtx_cvars();
#endif
}

//
// Sky presets
//
static PhysicalSkyDesc_t skyPresets[3] = {
    { 
		{ 0.0f, 0.0f, 0.0f },
		0.0f,
		{ 0.3f, 0.15f, 0.14f },
		PHYSICAL_SKY_FLAG_USE_SKYBOX,
		SKY_NONE,
    },
    { 
		{ 1.45f, 1.29f, 1.27f },  // earth : G2 sun, red scatter, 30% ground albedo
		1.f,
		{ 0.3f, 0.15f, 0.14f },
		PHYSICAL_SKY_FLAG_DRAW_MOUNTAINS,
		SKY_EARTH,
	},
    { 
		{ 0.315f, 0.137f, 0.033f },  // red sun
		1.f,
		{ 0.78, 0.78, 0.78 },
		PHYSICAL_SKY_FLAG_DRAW_MOUNTAINS,
		SKY_STROGGOS,
	},
};

PhysicalSkyDesc_t const *GetSkyPreset( int index )
{
    if ( index >= 0 && index < ARRAY_LEN(skyPresets) )
        return &skyPresets[index];

    return &skyPresets[0];
}

static void rotationQuat(vec3_t const axis, float radians, vec4_t result)
{
    // Note: assumes axis is normalized
    float sinHalfTheta = sinf(0.5f * radians);
    float cosHalfTheta = cosf(0.5f * radians);
    result[0] = cosHalfTheta;
    result[1] = axis[0] * sinHalfTheta;
    result[2] = axis[1] * sinHalfTheta;
    result[3] = axis[2] * sinHalfTheta;
}

static float dotQuat(vec4_t const q0, vec4_t const q1)
{
    return q0[0]*q1[0] + q0[1]*q1[1] +  q0[2]*q1[2] +  q0[3]*q1[3];
}

static void normalizeQuat(vec4_t q)
{
    float l = sqrtf(dotQuat(q, q));
    for (int i = 0; i < 4; ++i)
        q[i] /= l;
}

static void conjugateQuat(vec4_t q)
{
    q[0] =  q[0];
    q[1] = -q[1];
    q[2] = -q[2];
    q[3] = -q[3];
}

static void inverseQuat(vec4_t q)
{
    float l2 = dotQuat(q, q);
    conjugateQuat(q);
    for (int i = 0; i < 4; ++i)
        q[i] /= l2;
}

static void quatMult(vec4_t const a, vec4_t const b, vec4_t result)
{
    result[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
    result[1] = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2];
    result[2] = a[0] * b[2] + a[2] * b[0] + a[3] * b[1] - a[1] * b[3];
    result[3] = a[0] * b[3] + a[3] * b[0] + a[1] * b[2] - a[2] * b[1];
}

void CalculateDirectionToSun(float DayOfYear, float TimeOfDay, float LatitudeDegrees, vec3_t result)
{
    const float AxialTilt = 23.439f;
    const float DaysInYear = 365.25f;
    const float SpringEquinoxDay = 79.0f; // Mar 20

    float altitudeAtNoon = 90 - LatitudeDegrees + AxialTilt * sinf((DayOfYear - SpringEquinoxDay) / DaysInYear * 2.0f * M_PI);
    altitudeAtNoon = altitudeAtNoon * M_PI / 180.f;

    float altitudeOfEarthAxis = 180 - LatitudeDegrees;
    altitudeOfEarthAxis = altitudeOfEarthAxis * M_PI / 180.f;

    // X -> North
    // Y -> Zenith
    // Z -> East

    vec3_t noonVector = { cosf(altitudeAtNoon), sinf(altitudeAtNoon), 0.0f };
    vec3_t earthAxis = { cosf(altitudeOfEarthAxis), sinf(altitudeOfEarthAxis), 0.0f };

    float angleFromNoon = (TimeOfDay - 12.0f) / 24.0f * 2.f * M_PI;
 
    vec4_t dayRotation;
    rotationQuat(earthAxis, -angleFromNoon, dayRotation);
  
    vec4_t dayRotationInv = { dayRotation[0], dayRotation[1], dayRotation[2], dayRotation[3] };
    inverseQuat(dayRotationInv);


    // = normalize(dayRotationInv * makequat(0.f, noonVector) * dayRotation);
    vec4_t sunDirection = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec4_t noonQuat = { 0.0f, noonVector[0], noonVector[1], noonVector[2] };
    quatMult(dayRotationInv, noonQuat, sunDirection);
    quatMult(sunDirection, dayRotation, sunDirection);
    normalizeQuat(sunDirection);

    result[0] = sunDirection[1];
    result[1] = sunDirection[3];
    result[2] = sunDirection[2];
}
