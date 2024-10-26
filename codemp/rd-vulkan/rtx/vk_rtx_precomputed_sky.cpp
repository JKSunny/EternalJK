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

const struct AtmosphereParameters *Constants = NULL;

#define EARTH_SURFACE_RADIUS			(6360.0f)
#define EARTH_ATMOSPHERE_RADIUS			(6420.f)
#define STROGGOS_SURFACE_RADIUS			(6360.0f)
#define STROGGOS_ATMOSPHERE_RADIUS		(6520.f)
#define DIST_TO_HORIZON( LOW, HIGH )	( HIGH*HIGH - LOW*LOW )

static struct AtmosphereParameters Params_Earth = {
	{ 1.47399998f, 1.85039997f, 1.91198003f },
	0.00467499997f,

	{ 0.00580233941f, 0.0135577619f, 0.0331000052f },
	EARTH_SURFACE_RADIUS,

	{ 0.0014985f, 0.0014985f, 0.0014985f },
	EARTH_ATMOSPHERE_RADIUS,

	0.8f,
	DIST_TO_HORIZON(EARTH_SURFACE_RADIUS, EARTH_ATMOSPHERE_RADIUS),
	EARTH_ATMOSPHERE_RADIUS - EARTH_SURFACE_RADIUS,
	0.f
};

static struct AtmosphereParameters Params_Stroggos = {
	{ 2.47399998, 1.85039997, 1.01198006 },
	0.00934999995f,

	{ 0.0270983186, 0.0414223559, 0.0647224262 },
	STROGGOS_SURFACE_RADIUS,

	{ 0.00342514296, 0.00342514296, 0.00342514296 },
	STROGGOS_ATMOSPHERE_RADIUS,

	0.9f,
	DIST_TO_HORIZON(STROGGOS_SURFACE_RADIUS, STROGGOS_ATMOSPHERE_RADIUS),
	STROGGOS_ATMOSPHERE_RADIUS - STROGGOS_SURFACE_RADIUS,
	0.f
};


struct ImageGPUInfo
{
	VkImage			Image;
	VkDeviceMemory	DeviceMemory;
	VkImageView		View;
};

void ReleaseInfo( struct ImageGPUInfo *info, uint32_t binding )
{
	const int binding_offset = ( binding - ( BINDING_OFFSET_PHYSICAL_SKY ) );

	qvkFreeMemory( vk.device, vk.physicalSkyImages[binding_offset].memory, NULL );
	qvkDestroyImage( vk.device, vk.physicalSkyImages[binding_offset].handle, NULL );
	qvkDestroyImageView( vk.device, vk.physicalSkyImages[binding_offset].view, NULL );
	memset( info, 0, sizeof(*info) );
}

struct ImageGPUInfo	SkyTransmittance;
struct ImageGPUInfo	SkyInscatter;
struct ImageGPUInfo	SkyIrradiance;
struct ImageGPUInfo	SkyClouds;

struct ImageGPUInfo TerrainAlbedo;
struct ImageGPUInfo TerrainNormals;
struct ImageGPUInfo TerrainDepth;

VkDescriptorSetLayout	uniform_precomputed_descriptor_layout;
VkDescriptorSet         desc_set_precomputed_ubo;

#define PRECOMPUTED_SKY_BINDING_IDX					0
#define PRECOMPUTED_SKY_UBO_DESC_SET_IDX			3

static vkbuffer_t		atmosphere_params_buffer;
static VkDescriptorPool desc_pool_precomputed_ubo;

float terrain_shadowmap_viewproj[16] = { 0.f };

VkResult UploadImage( void *FirstPixel, size_t total_size, unsigned int Width, unsigned int Height, unsigned int Depth, unsigned int ArraySize, unsigned char Cube, VkFormat PixelFormat, uint32_t Binding, struct ImageGPUInfo *Info, const char *DebugName )
{
	const int binding_offset = ( Binding - ( BINDING_OFFSET_PHYSICAL_SKY ) );

	size_t img_size = Width * Height * Depth;

	vkbuffer_t buf_img_upload;
	vk_rtx_buffer_create(&buf_img_upload, total_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* bn_tex = buffer_map(&buf_img_upload);

	memcpy(bn_tex, FirstPixel, total_size);

	buffer_unmap(&buf_img_upload);
	bn_tex = NULL;

	enum VkImageType ImageType = VK_IMAGE_TYPE_2D;
	if (Depth == 0)
		Depth = 1;
	if (Depth > 1)
		ImageType = VK_IMAGE_TYPE_3D;

	VkImageCreateInfo img_info;
	Com_Memset( &img_info, 0, sizeof(VkImageCreateInfo) );
	img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img_info.pNext = NULL;
	img_info.flags = 0;
	img_info.extent = {
		Width,
		Height,
		Depth,
	};
	img_info.imageType = ImageType;
	img_info.format = PixelFormat;
	img_info.mipLevels = 1;
	img_info.arrayLayers = ArraySize;
	img_info.samples = VK_SAMPLE_COUNT_1_BIT;
	img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	img_info.usage = VK_IMAGE_USAGE_STORAGE_BIT
	| VK_IMAGE_USAGE_TRANSFER_DST_BIT
		| VK_IMAGE_USAGE_SAMPLED_BIT;
	img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img_info.queueFamilyIndexCount = 0;
	//img_info.queueFamilyIndexCount = vk.queue_family_index;
	img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	

	if (Cube)
		img_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VK_CHECK( qvkCreateImage( vk.device, &img_info, NULL, &vk.physicalSkyImages[binding_offset].handle ) );
	VK_SET_OBJECT_NAME( &vk.physicalSkyImages[binding_offset].handle, DebugName, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );

	VkMemoryRequirements mem_req;
	qvkGetImageMemoryRequirements( vk.device, vk.physicalSkyImages[binding_offset].handle, &mem_req );
	assert( mem_req.size >= buf_img_upload.allocSize );

	VkMemoryAllocateInfo mem_alloc_info;
	mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc_info.pNext = NULL;
	mem_alloc_info.allocationSize = mem_req.size;
	mem_alloc_info.memoryTypeIndex = vk_find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


#ifdef VKPT_DEVICE_GROUPS
	VkMemoryAllocateFlagsInfo mem_alloc_flags = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT,
		.deviceMask = (1 << qvk.device_count) - 1
	};

	if (qvk.device_count > 1) {
		mem_alloc_info.pNext = &mem_alloc_flags;
	}
#endif

	VK_CHECK( qvkAllocateMemory( vk.device, &mem_alloc_info, NULL, &vk.physicalSkyImages[binding_offset].memory ) );

	VK_CHECK( qvkBindImageMemory( vk.device, vk.physicalSkyImages[binding_offset].handle, vk.physicalSkyImages[binding_offset].memory, 0 ) );

	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	if (Depth > 1)
		viewType = VK_IMAGE_VIEW_TYPE_3D;
	if (ArraySize == 6 && Cube)
		viewType = VK_IMAGE_VIEW_TYPE_CUBE;

	VkImageViewCreateInfo img_view_info;
	Com_Memset( &img_view_info, 0, sizeof(VkImageViewCreateInfo) );
	img_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	img_view_info.pNext = NULL;
	img_view_info.flags = 0;
	img_view_info.viewType = viewType;
	img_view_info.format = PixelFormat;
	img_view_info.image = vk.physicalSkyImages[binding_offset].handle;
	img_view_info.subresourceRange = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		0,
		1,
		0,
		ArraySize,
	};
	img_view_info.components = {
		VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A,			
	};

	VK_CHECK( qvkCreateImageView( vk.device, &img_view_info, NULL, &vk.physicalSkyImages[binding_offset].view ) );
	VK_SET_OBJECT_NAME( &vk.physicalSkyImages[binding_offset].view, DebugName, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );

	VkCommandBuffer cmd_buf = vk_begin_command_buffer();

	VkImageSubresourceRange subresource_range;
	subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel = 0;
	subresource_range.levelCount = 1;
	subresource_range.baseArrayLayer = 0;
	subresource_range.layerCount = ArraySize;


	IMAGE_BARRIER(cmd_buf,
		vk.physicalSkyImages[binding_offset].handle,
		subresource_range,
		0,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	VkBufferImageCopy copy_info;
	Com_Memset( &copy_info, 0, sizeof(VkBufferImageCopy) );
	copy_info.bufferOffset = 0;
	copy_info.imageSubresource = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		0,
		0,
		ArraySize
	};
	copy_info.imageOffset = { 0, 0, 0 };
	copy_info.imageExtent = { Width, Height, Depth };

	qvkCmdCopyBufferToImage( cmd_buf, buf_img_upload.buffer, vk.physicalSkyImages[binding_offset].handle,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info );


	IMAGE_BARRIER(cmd_buf,
		vk.physicalSkyImages[binding_offset].handle,
		subresource_range,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

	vk_end_command_buffer( cmd_buf );

	VkDescriptorImageInfo desc_img_info;
	desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	desc_img_info.imageView = vk.physicalSkyImages[binding_offset].view;
	desc_img_info.sampler = vk.tex_sampler;

	VkWriteDescriptorSet s;
	Com_Memset( &s, 0, sizeof(VkWriteDescriptorSet) );
	s.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	s.pNext = NULL;
	s.dstSet = vk.desc_set_textures_even;
	s.dstBinding = Binding;
	s.dstArrayElement = 0;
	s.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	s.descriptorCount = 1;
	s.pImageInfo = &desc_img_info;
	s.pBufferInfo = NULL;
	s.pTexelBufferView = NULL;

	qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );

	s.dstSet = vk.desc_set_textures_odd;
	qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );

	//vk_wait_idle();
	qvkQueueWaitIdle( vk.queue );

	vk_rtx_buffer_destroy( &buf_img_upload );

	return VK_SUCCESS;
}

#define ISBITMASK(header,r,g,b,a) ( header.RBitMask == r && header.GBitMask == g && header.BBitMask == b && header.ABitMask == a )


qboolean LoadImageFromDDS(const char* FileName, uint32_t Binding, struct ImageGPUInfo* Info, const char* DebugName)
{
	unsigned char* data = NULL;


	ptrdiff_t len = ri.FS_ReadFile(FileName, (void**)&data);

	if (!data)
	{
		ri.Error( ERR_DROP, "Couldn't read file %s\n", FileName );
		return qfalse;
	}

	qboolean retval = qfalse;

	const DDS_HEADER* dds = (DDS_HEADER*)data;
	const DDS_HEADER_DXT10* dxt10 = (DDS_HEADER_DXT10*)(data + sizeof(DDS_HEADER));

	if (dds->magic != DDS_MAGIC || dds->size != sizeof(DDS_HEADER) - 4)
	{
		ri.Error( ERR_DROP, "File %s does not have the expected DDS file format\n", FileName);
		goto done;
	}

	int Cube = 0;
	int ArraySize = 1;
	VkFormat PixelFormat = VK_FORMAT_UNDEFINED;
	size_t dds_header_size = sizeof(DDS_HEADER);

	if (dds->ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0'))
	{
		if (dxt10->dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM)
			PixelFormat = VK_FORMAT_R8G8B8A8_UNORM;
		else if (dxt10->dxgiFormat == DXGI_FORMAT_R32_FLOAT)
			PixelFormat = VK_FORMAT_R32_SFLOAT;
		else if (dxt10->dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
			PixelFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
		else
		{
			ri.Error( ERR_DROP, "File %s uses an unsupported pixel format (%d)\n", FileName, dxt10->dxgiFormat);
			goto done;
		}

		Cube = (dxt10->miscFlag & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0;
		ArraySize = dxt10->arraySize * (Cube ? 6 : 1);
		dds_header_size = sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
	}
	else
	{
		if (dds->caps2 & DDS_CUBEMAP && ((dds->caps2 & DDS_CUBEMAP_ALLFACES) == DDS_CUBEMAP_ALLFACES))
		{
			Cube = 1;
			ArraySize = 6;
		}

		if (ISBITMASK(dds->ddspf, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
			PixelFormat = VK_FORMAT_R8G8B8A8_UNORM;
		else if (ISBITMASK(dds->ddspf, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
			PixelFormat = VK_FORMAT_R8G8B8A8_UNORM;
		else if (ISBITMASK(dds->ddspf, 0xffffffff, 0x00000000, 0x00000000, 0x00000000))
			PixelFormat = VK_FORMAT_R32_SFLOAT;
		else
			ri.Error( ERR_DROP, "File %s uses an unsupported pixel format.\n", FileName);
	}

	UploadImage(data + dds_header_size, len - dds_header_size, dds->width, dds->height, dds->depth, ArraySize, Cube, PixelFormat, Binding, Info, DebugName);

done:
	ri.FS_FreeFile(data);
	return retval;
}

VkDescriptorSetLayout *SkyGetDescriptorLayout( void )
{
	return &uniform_precomputed_descriptor_layout;
}

VkDescriptorSet SkyGetDescriptorSet( void )
{
	return desc_set_precomputed_ubo;
}

VkBuffer vk_rtx_get_atmospheric_buffer( void ) 
{
	return atmosphere_params_buffer.buffer;
}

static VkResult vk_rtx_uniform_precomputed_buffer_create( void )
{
	// clean me up. some things are not used ..
	// out of time ..

	VkDescriptorSetLayoutBinding ubo_layout_binding;
	Com_Memset( &ubo_layout_binding, 0, sizeof(VkDescriptorSetLayoutBinding) );
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.binding = PRECOMPUTED_SKY_BINDING_IDX;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_ALL;

	VkDescriptorSetLayoutCreateInfo layout_info;
	Com_Memset( &layout_info, 0, sizeof(VkDescriptorSetLayoutCreateInfo) );
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.pNext = NULL;
	layout_info.bindingCount = 1;
	layout_info.pBindings = &ubo_layout_binding;

	VK_CHECK( qvkCreateDescriptorSetLayout( vk.device, &layout_info, NULL, &uniform_precomputed_descriptor_layout ) );

	{
		vk_rtx_buffer_create( &atmosphere_params_buffer, sizeof(struct AtmosphereParameters),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		//VK_SET_OBJECT_NAME( atmosphere_params_buffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );
		//ATTACH_LABEL_VARIABLE_NAME(atmosphere_params_buffer.buffer, IMAGE_VIEW, "AtmosphereParameters");
	}

	VkDescriptorPoolSize pool_size;
	pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_size.descriptorCount = VK_MAX_SWAPCHAIN_SIZE;

	VkDescriptorPoolCreateInfo pool_info;
	Com_Memset( &pool_info, 0, sizeof(VkDescriptorPoolCreateInfo) );
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.pNext = NULL;
	pool_info.flags = 0;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = &pool_size;
	pool_info.maxSets = VK_MAX_SWAPCHAIN_SIZE;

	VK_CHECK( qvkCreateDescriptorPool( vk.device, &pool_info, NULL, &desc_pool_precomputed_ubo ) );

	VkDescriptorSetAllocateInfo descriptor_set_alloc_info;
	descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_alloc_info.pNext = NULL;
	descriptor_set_alloc_info.descriptorPool = desc_pool_precomputed_ubo;
	descriptor_set_alloc_info.descriptorSetCount = 1;
	descriptor_set_alloc_info.pSetLayouts = &uniform_precomputed_descriptor_layout;

	{
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &descriptor_set_alloc_info, &desc_set_precomputed_ubo ) );

		vkbuffer_t *ubo = &atmosphere_params_buffer;

		VkDescriptorBufferInfo buf_info;
		buf_info.buffer = ubo->buffer;
		buf_info.offset = 0;
		buf_info.range = sizeof(struct AtmosphereParameters);

		VkWriteDescriptorSet output_buf_write;
		Com_Memset( &output_buf_write, 0, sizeof(VkWriteDescriptorSet) );
		output_buf_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		output_buf_write.dstSet = desc_set_precomputed_ubo;
		output_buf_write.dstBinding = PRECOMPUTED_SKY_BINDING_IDX;
		output_buf_write.dstArrayElement = 0;
		output_buf_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		output_buf_write.descriptorCount = 1;
		output_buf_write.pBufferInfo = &buf_info;

		qvkUpdateDescriptorSets( vk.device, 1, &output_buf_write, 0, NULL );
	}

	return VK_SUCCESS;
}

static VkResult vk_rtx_uniform_precomputed_buffer_destroy( void )
{
	qvkDestroyDescriptorPool( vk.device, desc_pool_precomputed_ubo, NULL );
	qvkDestroyDescriptorSetLayout( vk.device, uniform_precomputed_descriptor_layout, NULL );
	desc_pool_precomputed_ubo = VK_NULL_HANDLE;
	uniform_precomputed_descriptor_layout = VK_NULL_HANDLE;

	vk_rtx_buffer_destroy( &atmosphere_params_buffer );

	return VK_SUCCESS;
}

static VkResult vk_rtx_uniform_precomputed_buffer_update( void )
{
	vkbuffer_t *ubo = &atmosphere_params_buffer;

	uint32_t idx, prev_idx;
	vk_rtx_get_descriptor_index( idx, prev_idx );

	assert( ubo->memory != VK_NULL_HANDLE );
	assert( ubo->buffer != VK_NULL_HANDLE );
	assert( idx < VK_MAX_SWAPCHAIN_SIZE );

	struct AtmosphereParameters *mapped_ubo = (AtmosphereParameters*)buffer_map(ubo);
	assert( mapped_ubo );
	memcpy( mapped_ubo, Constants, sizeof(struct AtmosphereParameters) );
	buffer_unmap( ubo );
	mapped_ubo = NULL;

	return VK_SUCCESS;
}

#define MATRIX(row, col) (row * 4 + col)

void create_identity_matrix(float matrix[16])
{
	uint32_t size = 16 * sizeof(float);
	memset(matrix, 0, size);
	matrix[MATRIX(0, 0)] = 1.0f;
	matrix[MATRIX(1, 1)] = 1.0f;
	matrix[MATRIX(2, 2)] = 1.0f;
	matrix[MATRIX(3, 3)] = 1.0f;
}

void create_look_at_matrix(float matrix[16], vec3_t EyePosition, vec3_t EyeDirection, vec3_t UpDirection)
{
	vec3_t f; 
	VectorNormalize2(EyeDirection, f);

	vec3_t s;
	CrossProduct(UpDirection, f, s);
	VectorNormalize2(s, s);

	vec3_t u;
	CrossProduct(f, s, u);

	float D0 = DotProduct(s, EyePosition);
	float D1 = DotProduct(u, EyePosition);
	float D2 = DotProduct(f, EyePosition);

	// Set identity
	create_identity_matrix(matrix);

	matrix[MATRIX(0, 0)] = s[0];
	matrix[MATRIX(1, 0)] = s[1];
	matrix[MATRIX(2, 0)] = s[2];
	matrix[MATRIX(0, 1)] = u[0];

	matrix[MATRIX(1, 1)] = u[1];
	matrix[MATRIX(2, 1)] = u[2];
	matrix[MATRIX(0, 2)] = f[0];
	matrix[MATRIX(1, 2)] = f[1];

	matrix[MATRIX(2, 2)] = f[2];
	matrix[MATRIX(3, 0)] = -D0;
	matrix[MATRIX(3, 1)] = -D1;
	matrix[MATRIX(3, 2)] = -D2;
}

void
create_centered_orthographic_matrix(float matrix[16], float xmin, float xmax,
	float ymin, float ymax, float znear, float zfar)
{
	float width, height;

	width = xmax - xmin;
	height = ymax - ymin;
	//float fRange = 1.0f / (znear - zfar);
	float fRange = 1.0f / (zfar - znear);

	matrix[0] = 2 / width;
	matrix[4] = 0;
	matrix[8] = 0;
	matrix[12] = -(xmax + xmin) / width;

	matrix[1] = 0;
	matrix[5] = 2 / height;
	matrix[9] = 0;
	matrix[13] = -(ymax + ymin) / height;

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = fRange;
	matrix[14] = -fRange * znear;

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
}

void UpdateTerrainShadowMapView(vec3_t forward)
{
	float Proj[16];
	float View[16];

	float BoundingOffset = ShadowmapWorldSize * 0.75f;
	create_centered_orthographic_matrix(Proj, -BoundingOffset, BoundingOffset, -BoundingOffset, BoundingOffset, -ShadowmapWorldSize * 0.75f, ShadowmapWorldSize * 0.75f);
	vec3_t up = { 0, 0, 1.0f };
	vec3_t origin = { 0.f };
	create_look_at_matrix(View, origin, forward, up);

	mult_matrix_matrix(terrain_shadowmap_viewproj, Proj, View);
}

static size_t Q_snprintf(char *dest, size_t size, const char *fmt, ...)
{
    va_list argptr;
    size_t  ret;

    va_start(argptr, fmt);
    ret = Q_vsnprintf(dest, size, fmt, argptr);
    va_end(argptr);

    return ret;
}

VkResult SkyLoadScatterParameters( int preset )
{
	const char* Planet;

	if ( preset == SKY_EARTH )
	{
		Planet = "earth";
		Constants = &Params_Earth;
	}
	else if ( preset == SKY_STROGGOS )
	{
		Planet = "stroggos";
		Constants = &Params_Stroggos;
	}

	ReleaseInfo( &SkyTransmittance, BINDING_OFFSET_SKY_TRANSMITTANCE );
	ReleaseInfo( &SkyInscatter,		BINDING_OFFSET_SKY_SCATTERING );
	ReleaseInfo( &SkyIrradiance,	BINDING_OFFSET_SKY_IRRADIANCE );

	char FileBuf[MAX_QPATH];
	Q_snprintf( FileBuf, sizeof(FileBuf), "env/transmittance_%s.dds", Planet );
	LoadImageFromDDS( FileBuf, BINDING_OFFSET_SKY_TRANSMITTANCE, &SkyTransmittance, "SkyTransmittance" );

	Q_snprintf( FileBuf, sizeof(FileBuf), "env/inscatter_%s.dds", Planet );
	LoadImageFromDDS( FileBuf, BINDING_OFFSET_SKY_SCATTERING,	&SkyInscatter,		"SkyInscatter" );

	Q_snprintf( FileBuf, sizeof(FileBuf), "env/irradiance_%s.dds", Planet );
	LoadImageFromDDS( FileBuf, BINDING_OFFSET_SKY_IRRADIANCE,	&SkyIrradiance,		"SkyIrradiance" );

	vk_rtx_uniform_precomputed_buffer_update();

	return VK_SUCCESS;
}

VkResult SkyInitializeDataGPU( void )
{
	LoadImageFromDDS("env/clouds.dds",			BINDING_OFFSET_SKY_CLOUDS,		&SkyClouds,			"SkyClouds");

	LoadImageFromDDS("env/terrain_albedo.dds",	BINDING_OFFSET_TERRAIN_ALBEDO,	&TerrainAlbedo,		"SkyGBufferAlbedo");
	LoadImageFromDDS("env/terrain_normal.dds",	BINDING_OFFSET_TERRAIN_NORMALS,	&TerrainNormals,	"SkyGBufferNormals");
	LoadImageFromDDS("env/terrain_depth.dds",	BINDING_OFFSET_TERRAIN_DEPTH,	&TerrainDepth,		"SkyGBufferDepth");
	
	vk_rtx_uniform_precomputed_buffer_create();

	return VK_SUCCESS;
}

void SkyReleaseDataGPU( void )
{
	ReleaseInfo( &SkyTransmittance, BINDING_OFFSET_SKY_TRANSMITTANCE );
	ReleaseInfo( &SkyInscatter,		BINDING_OFFSET_SKY_SCATTERING );
	ReleaseInfo( &SkyIrradiance,	BINDING_OFFSET_SKY_IRRADIANCE );

	ReleaseInfo( &SkyClouds,		BINDING_OFFSET_SKY_CLOUDS );
	ReleaseInfo( &TerrainAlbedo,	BINDING_OFFSET_TERRAIN_ALBEDO );
	ReleaseInfo( &TerrainNormals,	BINDING_OFFSET_TERRAIN_NORMALS );
	ReleaseInfo( &TerrainDepth,		BINDING_OFFSET_TERRAIN_DEPTH);

	vk_rtx_uniform_precomputed_buffer_destroy();
}

struct ShadowVertex
{
	float x;
	float y;
	float z;
};

struct ShadowFace
{
	uint32_t A;
	uint32_t B;
	uint32_t C;
};

struct ShadowmapGeometry
{
	vkbuffer_t		Vertexes;
	vkbuffer_t		Indexes;
	uint32_t		IndexCount;
};

// ----------------------------------------------------------------------------
//				Data
// ----------------------------------------------------------------------------

struct ShadowmapGeometry		ShadowmapGrid;

extern VkPipelineLayout        pipeline_layout_smap;
extern VkRenderPass            render_pass_smap;
extern VkPipeline              pipeline_smap;

// ----------------------------------------------------------------------------
//				Functions
// ----------------------------------------------------------------------------

static inline size_t align(size_t x, size_t alignment)
{
	return (x + (alignment - 1)) & ~(alignment - 1);
}


struct ShadowmapGeometry FillVertexAndIndexBuffers( const char* FileName, unsigned int SideSize, float size_km )
{
	struct  ShadowmapGeometry result = { 0 };


	unsigned char* file_data = NULL;
	//ssize_t file_len = FS_LoadFile(FileName, (void**)&file_data);
	ptrdiff_t len = ri.FS_ReadFile(FileName, (void**)&file_data);

	if (!file_data)
	{
		Com_Error( ERR_DROP, "Couldn't read file %s\n", FileName);
		goto done;
	}

	DDS_HEADER* dds = (DDS_HEADER*)file_data;
	DDS_HEADER_DXT10* dxt10 = (DDS_HEADER_DXT10*)(file_data + sizeof(DDS_HEADER));

	if (dds->magic != DDS_MAGIC || dds->size != sizeof(DDS_HEADER) - 4 || dds->ddspf.fourCC != MAKEFOURCC('D', 'X', '1', '0') || dxt10->dxgiFormat != DXGI_FORMAT_R32_FLOAT)
	{
		Com_Error( ERR_DROP, "File %s does not have the expected DDS file format\n", FileName);
		goto done;
	}

	float* file_pixels = (float*)(file_data + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10));
	

	size_t VertexBufferSize = SideSize * SideSize * sizeof(struct ShadowVertex);	
	size_t IndexBufferSize = (SideSize - 1) * (SideSize - 1) * 2 * sizeof(struct ShadowFace);
	size_t IndexCount = IndexBufferSize / sizeof(uint32_t);

	VertexBufferSize = align(VertexBufferSize, 64 * 1024);
	IndexBufferSize = align(IndexBufferSize, 64 * 1024);

	vkbuffer_t upload_buffer;
	vk_rtx_buffer_create(&upload_buffer, VertexBufferSize + IndexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	byte *mapped_upload_buf = (byte*)buffer_map(&upload_buffer);

	struct ShadowVertex* vertexes = (struct ShadowVertex*)mapped_upload_buf;
	struct ShadowFace* indexes = (struct ShadowFace*)(mapped_upload_buf + VertexBufferSize);

	if (!vertexes || !indexes)
		return result;

	result.IndexCount = IndexCount;

	float delta = size_km / (float)SideSize;
	float X = -0.5f * size_km;
	float Y = -0.5f * size_km;

	for (unsigned int y = 0; y < SideSize; y++)
	{
		for (unsigned int x = 0; x < SideSize; x++)
		{
			unsigned int index = y * SideSize + x;

			float Z = file_pixels[index];

			vertexes[index].x = X;
			vertexes[index].y = Y;
			vertexes[index].z = (x == 0) || (y == 0) || (x == SideSize - 1) || (y == SideSize - 1) ? -6.f : Z * 3.f;
			X += delta;
		}
		X = -0.5f * size_km;
		Y += delta;
	}

	unsigned int i = 0;
	for (unsigned int y = 0; y < SideSize-1; y++)
	{
		for (unsigned int x = 0; x < SideSize-1; x++)
		{
			unsigned int upper_index = (y + 1) * SideSize + x;
			unsigned int lower_index = y * SideSize + x;
			indexes[i].A = lower_index;
			indexes[i].B = upper_index + 1;
			indexes[i].C = lower_index + 1;
			i++;
			indexes[i].A = lower_index;
			indexes[i].B = upper_index;
			indexes[i].C = upper_index + 1;
			i++;
		}
	}

	buffer_unmap(&upload_buffer);


	vk_rtx_buffer_create(&result.Vertexes, VertexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//ATTACH_LABEL_VARIABLE_NAME(result.Vertexes.buffer, BUFFER, "Shadowmap Vertex Buffer");

	vk_rtx_buffer_create(&result.Indexes, IndexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//ATTACH_LABEL_VARIABLE_NAME(result.Indexes.buffer, BUFFER, "Shadowmap Index Buffer");

#if 0
	VkCommandBuffer cmd_buf = vkpt_begin_command_buffer(&qvk.cmd_buffers_transfer);
#else
	VkCommandBuffer cmd_buf = vk_begin_command_buffer();
#endif

	BUFFER_BARRIER( cmd_buf, 
		VK_ACCESS_NONE, 
		VK_ACCESS_TRANSFER_READ_BIT,
		upload_buffer.buffer,
		0,
		VK_WHOLE_SIZE
	);

	BUFFER_BARRIER( cmd_buf, 
		VK_ACCESS_NONE, 
		VK_ACCESS_TRANSFER_WRITE_BIT,
		result.Vertexes.buffer,
		0,
		VK_WHOLE_SIZE
	);

	BUFFER_BARRIER( cmd_buf, 
		VK_ACCESS_NONE, 
		VK_ACCESS_TRANSFER_WRITE_BIT,
		result.Indexes.buffer,
		0,
		VK_WHOLE_SIZE
	);

	VkBufferCopy region;
	region.srcOffset = 0;
	region.dstOffset = 0;
	region.size = VertexBufferSize;

	qvkCmdCopyBuffer(cmd_buf, upload_buffer.buffer, result.Vertexes.buffer, 1, &region);

	region.srcOffset = VertexBufferSize;
	region.size = IndexBufferSize;

	qvkCmdCopyBuffer(cmd_buf, upload_buffer.buffer, result.Indexes.buffer, 1, &region);

	BUFFER_BARRIER( cmd_buf, 
		VK_ACCESS_NONE, 
		VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
		result.Vertexes.buffer,
		0,
		VK_WHOLE_SIZE
	);

	BUFFER_BARRIER( cmd_buf, 
		VK_ACCESS_NONE, 
		VK_ACCESS_INDEX_READ_BIT,
		result.Indexes.buffer,
		0,
		VK_WHOLE_SIZE
	);

	const int binding_offset = ( BINDING_OFFSET_TERRAIN_SHADOWMAP - ( BINDING_OFFSET_PHYSICAL_SKY ) );

	VkImageSubresourceRange range;
	Com_Memset( &range, 0, sizeof(VkImageSubresourceRange) ); 
	range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	range.levelCount = 1;
	range.layerCount = 1;

	IMAGE_BARRIER( cmd_buf,
		vk.physicalSkyImages[binding_offset].handle,
		range,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

#if 0
	vkpt_submit_command_buffer_simple(cmd_buf, qvk.queue_transfer, qtrue);
	qvkQueueWaitIdle(qvk.queue_transfer);
#else
	vk_end_command_buffer( cmd_buf );
	qvkQueueWaitIdle( vk.queue );
#endif

	vk_rtx_buffer_destroy(&upload_buffer);

done:
	if (file_data)
		ri.FS_FreeFile(file_data);

	return result;
}


// ----------------------------------------------------------------------------

void ReleaseShadowmap( struct Shadowmap *InOutShadowmap )
{
	const int binding_offset = ( BINDING_OFFSET_TERRAIN_SHADOWMAP - ( BINDING_OFFSET_PHYSICAL_SKY ) );

	if ( vk.physicalSkyImages[binding_offset].view )
		qvkDestroyImageView( vk.device, vk.physicalSkyImages[binding_offset].view, NULL );

	if ( vk.physicalSkyImages[binding_offset].handle )
		qvkDestroyImage( vk.device, vk.physicalSkyImages[binding_offset].handle, NULL );

	if ( vk.physicalSkyImages[binding_offset].memory )
		qvkFreeMemory( vk.device, vk.physicalSkyImages[binding_offset].memory, NULL );

	if ( InOutShadowmap->framebuffer )
		qvkDestroyFramebuffer( vk.device, InOutShadowmap->framebuffer, NULL );
}

void vk_rtx_create_shadow_map_framebuffer( struct Shadowmap *InOutShadowmap ) 
{

	if ( InOutShadowmap->framebuffer )
		return;

	const int binding_offset = ( BINDING_OFFSET_TERRAIN_SHADOWMAP - ( BINDING_OFFSET_PHYSICAL_SKY ) );

	VkFramebufferCreateInfo fbufCreateInfo;
	Com_Memset( &fbufCreateInfo, 0, sizeof(VkFramebufferCreateInfo) );
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.pNext = NULL;
	fbufCreateInfo.renderPass = render_pass_smap;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = &vk.physicalSkyImages[binding_offset].view;
	fbufCreateInfo.width = InOutShadowmap->width;
	fbufCreateInfo.height = InOutShadowmap->height;
	fbufCreateInfo.layers = 1;

	VK_CHECK( qvkCreateFramebuffer( vk.device, &fbufCreateInfo, NULL, &InOutShadowmap->framebuffer ) );
}

void CreateShadowMap( struct Shadowmap *InOutShadowmap )
{
	const int binding_offset = ( BINDING_OFFSET_TERRAIN_SHADOWMAP - ( BINDING_OFFSET_PHYSICAL_SKY ) );

	InOutShadowmap->format = VK_FORMAT_D32_SFLOAT;
	InOutShadowmap->width = ShadowmapSize;
	InOutShadowmap->height = ShadowmapSize;

	VkImageCreateInfo ShadowTexInfo;
	Com_Memset( &ShadowTexInfo, 0, sizeof(VkImageCreateInfo) );
	ShadowTexInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ShadowTexInfo.pNext = NULL;
	ShadowTexInfo.imageType = VK_IMAGE_TYPE_2D;
	ShadowTexInfo.extent.width = InOutShadowmap->width;
	ShadowTexInfo.extent.height = InOutShadowmap->height;
	ShadowTexInfo.extent.depth = 1;
	ShadowTexInfo.mipLevels = 1;
	ShadowTexInfo.arrayLayers = 1;
	ShadowTexInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ShadowTexInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ShadowTexInfo.format = InOutShadowmap->format;																// Depth stencil attachment
	ShadowTexInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// We will sample directly from the depth attachment for the shadow mapping

	VK_CHECK( qvkCreateImage( vk.device, &ShadowTexInfo, NULL, &vk.physicalSkyImages[binding_offset].handle ) );
	//ATTACH_LABEL_VARIABLE_NAME(vk.physicalSkyImages[binding_offset].handle, IMAGE_VIEW, "EnvShadowMap");

	VkMemoryRequirements memReqs = {0};
	qvkGetImageMemoryRequirements( vk.device, vk.physicalSkyImages[binding_offset].handle, &memReqs );

	VkMemoryAllocateInfo memAlloc;
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlloc.pNext = NULL;
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vk_find_memory_type( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK_CHECK( qvkAllocateMemory( vk.device, &memAlloc, NULL, &vk.physicalSkyImages[binding_offset].memory ) );
	VK_CHECK( qvkBindImageMemory( vk.device, vk.physicalSkyImages[binding_offset].handle, vk.physicalSkyImages[binding_offset].memory, 0 ) );

	VkImageViewCreateInfo depthStencilView;
	Com_Memset( &depthStencilView, 0, sizeof(VkImageViewCreateInfo) );
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.pNext = NULL;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = InOutShadowmap->format;
	depthStencilView.subresourceRange = { 0 }; // hmm
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = vk.physicalSkyImages[binding_offset].handle;

	VK_CHECK( qvkCreateImageView( vk.device, &depthStencilView, NULL, &vk.physicalSkyImages[binding_offset].view ) );

	Vk_Sampler_Def sd;
	Com_Memset( &sd, 0, sizeof(sd) );
	sd.gl_mag_filter = sd.gl_min_filter = GL_NEAREST;
	sd.address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sd.max_lod_1_0 = qtrue;
	sd.noAnisotropy = qtrue;

	VkDescriptorImageInfo desc_img_info;
	desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	desc_img_info.imageView = vk.physicalSkyImages[binding_offset].view;
	desc_img_info.sampler = vk.tex_sampler;

	VkWriteDescriptorSet s;
	Com_Memset( &s, 0, sizeof(VkWriteDescriptorSet) );
	s.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	s.pNext = NULL;
	s.dstSet = vk.desc_set_textures_even,
	s.dstBinding = BINDING_OFFSET_TERRAIN_SHADOWMAP;
	s.dstArrayElement = 0;
	s.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	s.descriptorCount = 1;
	s.pImageInfo = &desc_img_info;
	s.pBufferInfo = NULL;
	s.pTexelBufferView = NULL;

	qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );

	s.dstSet = vk.desc_set_textures_odd;
	qvkUpdateDescriptorSets( vk.device, 1, &s, 0, NULL );

#if 0
	// seems deprecated ?
	VkSamplerCreateInfo sampler;
	Com_Memset( &sampler, 0, sizeof(VkSamplerCreateInfo) );
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.pNext = NULL;
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 1.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

	VK_CHECK( qvkCreateSampler( vk.device, &sampler, NULL, &InOutShadowmap->DepthSampler ) );
#endif
}

// ----------------------------------------------------------------------------

void InitializeShadowmapResources( void )
{
	CreateShadowMap( &vk.precomputed_shadowmapdata );
	ShadowmapGrid = FillVertexAndIndexBuffers( "env/terrain_heightmap.dds", ShadowmapGridSize, ShadowmapWorldSize );
}

void ReleaseShadowmapResources( void )
{
	vk_rtx_buffer_destroy(&ShadowmapGrid.Indexes);
	vk_rtx_buffer_destroy(&ShadowmapGrid.Vertexes);
	memset(&ShadowmapGrid, 0, sizeof(ShadowmapGrid));

	ReleaseShadowmap(&vk.precomputed_shadowmapdata);
}
// ----------------------------------------------------------------------------

void RecordCommandBufferShadowmap( VkCommandBuffer cmd_buf )
{

	VkImageSubresourceRange range;
	Com_Memset( &range, 0, sizeof(VkImageSubresourceRange) ); 
	range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	range.levelCount = 1;
	range.layerCount = 1;

	Shadowmap *map = &vk.precomputed_shadowmapdata;
	const int binding_offset = ( BINDING_OFFSET_TERRAIN_SHADOWMAP - ( BINDING_OFFSET_PHYSICAL_SKY ) );

	IMAGE_BARRIER( cmd_buf,
		vk.physicalSkyImages[binding_offset].handle,
		range,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	);
	
	VkClearValue clearValues;
	clearValues.depthStencil.depth = 1.0f;
	clearValues.depthStencil.stencil = 0;

	VkRenderPassBeginInfo renderPassBeginInfo;
	Com_Memset( &renderPassBeginInfo, 0, sizeof(VkRenderPassBeginInfo) );
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = render_pass_smap;
	renderPassBeginInfo.framebuffer = map->framebuffer;
	renderPassBeginInfo.renderArea.extent.width = map->width;
	renderPassBeginInfo.renderArea.extent.height = map->height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearValues;

	qvkCmdBeginRenderPass( cmd_buf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

	qvkCmdBindPipeline( cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_smap );

	VkViewport viewport;
	Com_Memset( &viewport, 0, sizeof(VkViewport) );
	viewport.width = map->width;
	viewport.height = map->height;
	viewport.minDepth = 0;
	viewport.maxDepth = 1.0f;

	qvkCmdSetViewport( cmd_buf, 0, 1, &viewport );

	VkRect2D rect2D; 
	Com_Memset( &rect2D, 0, sizeof(VkRect2D) );
	rect2D.extent.width = map->width;
	rect2D.extent.height = map->height;
	rect2D.offset.x = 0;
	rect2D.offset.y = 0;


	qvkCmdSetScissor( cmd_buf, 0, 1, &rect2D );
	float depthBiasConstant = 0.f;
	float depthBiasSlope = 0.f;

	qvkCmdPushConstants( cmd_buf, pipeline_layout_smap, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16, terrain_shadowmap_viewproj );

	VkDeviceSize offsets[1] = { 0 };
	qvkCmdBindVertexBuffers( cmd_buf, 0, 1, &ShadowmapGrid.Vertexes.buffer, offsets );
	qvkCmdBindIndexBuffer( cmd_buf, ShadowmapGrid.Indexes.buffer, 0, VK_INDEX_TYPE_UINT32 );
	qvkCmdDrawIndexed( cmd_buf, ShadowmapGrid.IndexCount, 1, 0, 0, 0 );
	qvkCmdEndRenderPass( cmd_buf );

	IMAGE_BARRIER( cmd_buf,
		vk.physicalSkyImages[binding_offset].handle,
		range,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
}