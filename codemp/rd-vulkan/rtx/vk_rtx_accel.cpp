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

// Buffer Layout:
//
// | Section              | Description                                                       | Size (per element)          |
// |----------------------|-------------------------------------------------------------------|-----------------------------|
// | VBOPrimitive         | Flattened surface polys containing:                               | 176 bytes                   |
// |                      | - Position                                                        |                             |
// |                      | - Normal                                                          |                             |
// |                      | - Tangent                                                         |                             |
// |                      | - Material info                                                   |                             |
// |                      | (used for rendering)                                              |                             |
// | vertex_buffer_offset | Offset to this vertex buffer in GPU memory                        | -                           |
// | prim_positions_t     | List of vertex positions (used for BLAS generation)               | 36 bytes (mat3 / 3x vec3_t) |
// | BLAS Accel Structure | Acceleration data for bottom-level AS							  | [follows prim_positions_t]  |
//

// Per Vulkan spec, acceleration structure offset must be a multiple of 256
// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkAccelerationStructureCreateInfoKHR.html
#define ACCEL_STRUCT_ALIGNMENT 256

#define AS_PRIMITIVE_BUFFER_FLAGS ( \
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | \
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | \
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | \
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | \
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | \
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR )

static size_t Q_snprintf(char *dest, size_t size, const char *fmt, ...)
{
    va_list argptr;
    size_t  ret;

    va_start(argptr, fmt);
    ret = Q_vsnprintf(dest, size, fmt, argptr);
    va_end(argptr);

    return ret;
}

static inline size_t align(size_t x, size_t alignment)
{
	return (x + (alignment - 1)) & ~(alignment - 1);
}

static inline int accel_matches(vk_blas_match_info_t *match,
								int fast_build,
								uint32_t vertex_count,
								uint32_t index_count) {
	return match->fast_build == fast_build &&
		   match->vertex_count >= vertex_count &&
		   match->index_count >= index_count;
}

// ~sunny, to be deprecated/reworked
#if 0
static inline bool vk_rtx_blas_needs_rebuild(const vk_blas_t* blas, vk_geometry_data_accel_t* accel)
{
	return (accel->num_primitives * 3 > blas->create_num_vertices  );
}
#endif

// ~sunny, to be deprecated/reworked
void vk_rtx_update_blas( VkCommandBuffer cmd_buf, vk_geometry_data_t *geom, vk_geometry_data_accel_t *accel,
	vk_blas_t *blas, vk_blas_t *old_blas )
{
#if 0
	const int frame_idx = vk.frame_counter & 1;

	const bool needs_rebuild = vk_rtx_blas_needs_rebuild( blas, accel );

	// rebuild blas
	if ( needs_rebuild ) 
	{
		vk_rtx_destroy_blas( blas );

		vk_rtx_create_blas(
			NULL,
			&geom->xyz[frame_idx], accel->xyz_offset,
			&geom->idx[frame_idx], accel->idx_offset,
			accel->xyz_count, accel->idx_count,
			blas,
			qtrue /*geom->dynamic_flags*/, geom->fast_build, geom->allow_update, geom->is_world,
			0, "update"
		);

		blas->create_num_vertices = accel->xyz_count;
		blas->create_num_indices = accel->idx_count;

		MEM_BARRIER_BUILD_ACCEL( cmd_buf );
		return;
	}

	// update blas
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
	memset(&buildInfo, 0, sizeof(buildInfo));
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo.pNext = NULL;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &blas->geometries;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
	buildInfo.dstAccelerationStructure = blas->accel;
	buildInfo.srcAccelerationStructure = old_blas ? old_blas->accel : VK_NULL_HANDLE;
	buildInfo.scratchData.deviceAddress = vk.buf_accel_scratch.address + vk.scratch_buf_ptr;

	const VkAccelerationStructureBuildRangeInfoKHR* range_ptr = &blas->range;
	qvkCmdBuildAccelerationStructuresKHR( cmd_buf, 1, &buildInfo, &range_ptr );

	MEM_BARRIER_BUILD_ACCEL( cmd_buf );
#endif
}

// used for entities
void vk_rtx_create_blas( accel_build_batch_t *batch,  
								 vkbuffer_t *vertex_buffer, VkDeviceAddress vertex_offset,
								 vkbuffer_t *index_buffer, VkDeviceAddress index_offset,
								 uint32_t num_vertices, uint32_t num_indices,
								 vk_blas_t *blas, qboolean is_dynamic, qboolean fast_build, 
								 qboolean allow_update, size_t first_vertex_offset,
								 const char* debug_label ) 
{	
	if (num_vertices == 0)
	{
		blas->present = false;
		return;
	}

	assert(batch->numBuilds < MAX_BATCH_ACCEL_BUILDS);
	uint32_t buildIdx = batch->numBuilds++;

	assert(vertex_buffer->address);
	if ( index_buffer ) 
		assert(index_buffer->address);

	bool doFree = qfalse;
	bool doAlloc = qfalse;

	if ( !is_dynamic || !accel_matches(&blas->match, fast_build, num_vertices, num_indices) || blas->accel == VK_NULL_HANDLE ) {
		doAlloc = qtrue;
		doFree = (blas->accel != VK_NULL_HANDLE);
	}

	if ( doFree ) 
		vk_rtx_destroy_blas( blas );

	VkAccelerationStructureGeometryTrianglesDataKHR triangles;
	Com_Memset( &triangles, 0, sizeof(VkAccelerationStructureGeometryTrianglesDataKHR) );
	triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	triangles.pNext = NULL;
	triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	triangles.vertexData.hostAddress = VK_NULL_HANDLE;

	// models and bsp vertex data differ in format/stride
	VkDeviceSize stride = sizeof(vec3_t);

	blas->vertex_address = vertex_buffer->address + vertex_offset;
	blas->index_address = index_buffer ? (index_buffer->address + (index_offset * sizeof(uint32_t))) : 0;

	triangles.vertexData.deviceAddress = blas->vertex_address;
	triangles.vertexStride = stride;
	triangles.maxVertex = num_vertices - 1;
	triangles.indexData.hostAddress = VK_NULL_HANDLE;
	triangles.indexData.deviceAddress = blas->index_address;
	triangles.indexType = index_buffer ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_NONE_KHR;

	VkAccelerationStructureGeometryDataKHR geometry_data;
	Com_Memset( &geometry_data, 0, sizeof(VkAccelerationStructureGeometryDataKHR) );
	geometry_data.triangles = triangles;

	//VkAccelerationStructureGeometryKHR geometry;
	blas->geometries.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	blas->geometries.pNext = NULL,
	blas->geometries.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
	blas->geometries.geometry = geometry_data;
	blas->geometries.flags = 0;
	
	batch->geometries[buildIdx] = blas->geometries;
	VkAccelerationStructureBuildGeometryInfoKHR* buildInfo = &batch->buildInfos[buildIdx];

	// Prepare build info now, acceleration is filled later
	buildInfo->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo->pNext = NULL;
	buildInfo->type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	VkBuildAccelerationStructureFlagsKHR flags;
	flags = fast_build ? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR : VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

	if ( allow_update )
		flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;

	buildInfo->flags = flags;
	buildInfo->mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo->srcAccelerationStructure = VK_NULL_HANDLE;
	buildInfo->dstAccelerationStructure = VK_NULL_HANDLE;
	buildInfo->geometryCount = 1;
	buildInfo->pGeometries = &batch->geometries[buildIdx] ;
	buildInfo->ppGeometries = NULL;

	// Find size to build on the device
	uint32_t max_primitive_count = MAX(num_vertices, num_indices) / 3; // number of tris

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo;
	Com_Memset( &sizeInfo, 0, sizeof(VkAccelerationStructureBuildSizesInfoKHR) );
	sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	qvkGetAccelerationStructureBuildSizesKHR(vk.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, buildInfo, &max_primitive_count, &sizeInfo);

	if ( doAlloc ) 
	{
		int num_vertices_to_allocate = num_vertices;
		int num_indices_to_allocate = num_indices;
		blas->create_num_vertices = num_vertices_to_allocate;
		blas->create_num_indices = num_indices_to_allocate;

		// Allocate more memory / larger BLAS for dynamic objects
		if ( is_dynamic )
		{
			num_vertices_to_allocate *= 2;
			num_indices_to_allocate *= 2;

			max_primitive_count = MAX(num_vertices_to_allocate, num_indices_to_allocate) / 3;
			qvkGetAccelerationStructureBuildSizesKHR(vk.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, buildInfo, &max_primitive_count, &sizeInfo);
		}

		// Create acceleration structure
		VkAccelerationStructureCreateInfoKHR createInfo;
		Com_Memset( &createInfo, 0, sizeof(VkAccelerationStructureCreateInfoKHR) );
		createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.pNext = NULL;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		createInfo.size = sizeInfo.accelerationStructureSize;

		// Create the buffer for the acceleration structure
		vk_rtx_buffer_create( &blas->mem, sizeInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		createInfo.buffer = blas->mem.buffer;
		vk_rtx_buffer_attach_name(&blas->mem, va("blas: %s", debug_label));

		// Create the acceleration structure
		qvkCreateAccelerationStructureKHR( vk.device, &createInfo, NULL, &blas->accel );

		blas->match.fast_build = fast_build;
		blas->match.vertex_count = num_vertices_to_allocate;
		blas->match.index_count = num_indices_to_allocate;
	}

	// set where the build lands
	buildInfo->dstAccelerationStructure = blas->accel;

	buildInfo->scratchData.deviceAddress = vk.buf_accel_scratch.address + vk.scratch_buf_ptr;
	assert(vk.buf_accel_scratch.address);

	vk.scratch_buf_ptr += sizeInfo.buildScratchSize;
	vk.scratch_buf_ptr = align(vk.scratch_buf_ptr, vk.minAccelerationStructureScratchOffsetAlignment);

	// build range
	Com_Memset( &blas->range, 0, sizeof(VkAccelerationStructureBuildRangeInfoKHR) );

	blas->range.primitiveCount = MAX(num_vertices, num_indices) / 3;

	batch->rangeInfos[buildIdx] = blas->range;
	batch->rangeInfoPtrs[buildIdx] = &batch->rangeInfos[buildIdx];

	blas->present = true;
}


void vk_rtx_create_tlas( accel_build_batch_t *batch, vk_tlas_t *as, VkDeviceAddress instance_data, uint32_t num_instances ) 
{
	assert(batch->numBuilds < MAX_BATCH_ACCEL_BUILDS);
	uint32_t buildIdx = batch->numBuilds++;

	// Build the TLAS
	VkAccelerationStructureGeometryDataKHR geometry;
	Com_Memset( &geometry, 0, sizeof(VkAccelerationStructureGeometryDataKHR) );
	geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.instances.pNext = NULL;
	geometry.instances.data.deviceAddress = instance_data;

	geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;

	assert(instance_data);

	VkAccelerationStructureGeometryKHR topASGeometry;
	Com_Memset( &topASGeometry, 0, sizeof(VkAccelerationStructureGeometryKHR) );
	topASGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	topASGeometry.pNext = NULL;
	topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	topASGeometry.geometry = geometry;
	batch->geometries[buildIdx] = topASGeometry;

	// Find size to build on the device
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
	Com_Memset( &buildInfo, 0, sizeof(VkAccelerationStructureBuildGeometryInfoKHR) );
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo.pNext = NULL;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &batch->geometries[buildIdx];
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
	batch->buildInfos[buildIdx] = buildInfo;

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo;
	Com_Memset( &sizeInfo, 0, sizeof(VkAccelerationStructureBuildSizesInfoKHR) );
	sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	sizeInfo.pNext = NULL;
	qvkGetAccelerationStructureBuildSizesKHR( vk.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &num_instances, &sizeInfo);
	//assert( sizeInfo.accelerationStructureSize < SIZE_buf_accel_scratch );

	//if (accel_top_match[idx].instanceCount < num_instances) {
	//	vkpt_pt_destroy_toplevel(idx);

		// Create the buffer for the acceleration structure
		vk_rtx_buffer_create( &as->mem, sizeInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Create TLAS
		// Create acceleration structure
		VkAccelerationStructureCreateInfoKHR createInfo;
		Com_Memset( &createInfo, 0, sizeof(VkAccelerationStructureCreateInfoKHR) );
		createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.pNext = NULL;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		createInfo.size = sizeInfo.accelerationStructureSize;
		createInfo.buffer = as->mem.buffer;

		// Create the acceleration structure
		qvkCreateAccelerationStructureKHR( vk.device, &createInfo, NULL, &as->accel );

		// Get handle
		VkAccelerationStructureDeviceAddressInfoKHR  as_device_address_info;
		as_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		as_device_address_info.pNext = NULL;
		as_device_address_info.accelerationStructure = as->accel;
		as->handle = qvkGetAccelerationStructureDeviceAddressKHR(vk.device, &as_device_address_info);
	//}

	// Update build information
	batch->buildInfos[buildIdx].dstAccelerationStructure = as->accel;
	batch->buildInfos[buildIdx].scratchData.deviceAddress = vk.buf_accel_scratch.address;
	assert(vk.buf_accel_scratch.address);

	VkAccelerationStructureBuildRangeInfoKHR offset;
	Com_Memset( &offset, 0, sizeof(VkAccelerationStructureBuildRangeInfoKHR) );
	offset.primitiveCount = num_instances;
	VkAccelerationStructureBuildRangeInfoKHR* offsets = &offset;

	batch->rangeInfos[buildIdx] = offset;
	batch->rangeInfoPtrs[buildIdx] = &batch->rangeInfos[buildIdx];
}

void vk_rtx_destroy_tlas( vk_tlas_t *as ) 
{
	vk_rtx_buffer_destroy( &as->mem );

	if ( as->accel != VK_NULL_HANDLE ) 
	{
		qvkDestroyAccelerationStructureKHR( vk.device, as->accel, NULL );
		as->accel = VK_NULL_HANDLE;
	}

	memset( &as->accel, 0, sizeof(vk_tlas_t) );

}

void vk_rtx_destroy_blas( vk_blas_t *blas ) 
{
	vk_rtx_buffer_destroy( &blas->mem );

	if ( blas->accel != VK_NULL_HANDLE ) 
	{
		qvkDestroyAccelerationStructureKHR( vk.device, blas->accel, NULL );
		blas->accel = VK_NULL_HANDLE;
	}

	memset( blas, 0, sizeof(vk_blas_t) );
}

void vk_rtx_destroy_accel_all() {
	uint32_t i;

	if ( tr.world )
	{
		vk_rtx_reset_world_geometries( tr.world );
	}

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		vk_rtx_destroy_blas( &vk.model_instance.blas.dynamic[i] );
		vk_rtx_destroy_blas( &vk.model_instance.blas.transparent_models[i] );
		vk_rtx_destroy_blas( &vk.model_instance.blas.viewer_models[i] );
		vk_rtx_destroy_blas( &vk.model_instance.blas.viewer_weapon[i] );
		vk_rtx_destroy_blas( &vk.model_instance.blas.explosions[i] );

		vk_rtx_destroy_tlas( &vk.tlas_geometry[i] );
	}
}

//
// 
//
void vkpt_init_model_geometry(model_geometry_t* info, uint32_t max_geometries)
{
	assert(info->geometry_storage == NULL); // avoid double allocation

	if (max_geometries == 0)
		return;

	size_t size_geometries = max_geometries * sizeof(VkAccelerationStructureGeometryKHR);
	size_t size_build_ranges = max_geometries * sizeof(VkAccelerationStructureBuildRangeInfoKHR);
	size_t size_prims = max_geometries * sizeof(uint32_t);

	info->geometry_storage = (uint8_t*)Z_Malloc(size_geometries + size_build_ranges + size_prims * 2, TAG_GENERAL, qtrue);

	info->geometries = (VkAccelerationStructureGeometryKHR*)info->geometry_storage;
	info->build_ranges = (VkAccelerationStructureBuildRangeInfoKHR*)(info->geometry_storage + size_geometries);
	info->prim_counts = (uint32_t*)(info->geometry_storage + size_geometries + size_build_ranges);
	info->prim_offsets = (uint32_t*)(info->geometry_storage + size_geometries + size_build_ranges + size_prims);

	info->max_geometries = max_geometries;
}

void vkpt_append_model_geometry(model_geometry_t* info, uint32_t num_prims, uint32_t prim_offset, const char* model_name)
{
	if (num_prims == 0)
		return;

	if (info->num_geometries >= info->max_geometries)
	{
		Com_Printf("Model '%s' exceeds the maximum supported number of meshes (%d)\n", model_name, info->max_geometries);
		return;
	}

	VkAccelerationStructureGeometryTrianglesDataKHR triangles;
	Com_Memset( &triangles, 0, sizeof(VkAccelerationStructureGeometryTrianglesDataKHR) );
	triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	triangles.pNext = NULL;
	triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	triangles.vertexStride = sizeof(float) * 3,
	triangles.maxVertex = num_prims * 3;
	triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

	VkAccelerationStructureGeometryDataKHR geometry_data;
	Com_Memset( &geometry_data, 0, sizeof(VkAccelerationStructureGeometryDataKHR) );
	geometry_data.triangles = triangles;

	VkAccelerationStructureGeometryKHR geometry;
	Com_Memset( &geometry, 0, sizeof(VkAccelerationStructureGeometryKHR) );
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = NULL;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.geometry = geometry_data;
	geometry.flags = 0;


	VkAccelerationStructureBuildRangeInfoKHR build_range;
	Com_Memset( &build_range, 0, sizeof(VkAccelerationStructureBuildRangeInfoKHR) );
	build_range.primitiveCount = num_prims;


	uint32_t geom_index = info->num_geometries;

	info->geometries[geom_index] = geometry;
	info->build_ranges[geom_index] = build_range;
	info->prim_counts[geom_index] = num_prims;
	info->prim_offsets[geom_index] = prim_offset;

	++info->num_geometries;
}

void suballocate_model_blas_memory(model_geometry_t* info, size_t* vbo_size, const char* model_name)
{
	VkAccelerationStructureBuildSizesInfoKHR build_sizes;
	Com_Memset( &build_sizes, 0, sizeof(VkAccelerationStructureBuildSizesInfoKHR) );
	build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	info->build_sizes = build_sizes;

	if (info->num_geometries == 0)
		return;

	VkAccelerationStructureBuildGeometryInfoKHR blasBuildinfo;
	memset(&blasBuildinfo, 0, sizeof(VkAccelerationStructureBuildGeometryInfoKHR));
	blasBuildinfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	blasBuildinfo.pNext = NULL;
	blasBuildinfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	blasBuildinfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	blasBuildinfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	blasBuildinfo.geometryCount = info->num_geometries;
	blasBuildinfo.pGeometries = info->geometries;

	qvkGetAccelerationStructureBuildSizesKHR(vk.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&blasBuildinfo, info->prim_counts, &info->build_sizes);

	if (info->build_sizes.buildScratchSize > vk.buf_accel_scratch.size)
	{
		Com_Printf("Model '%s' requires %lu bytes scratch buffer to build its BLAS, while only %zu are available.\n",
			model_name, info->build_sizes.buildScratchSize, vk.buf_accel_scratch.size);

		info->num_geometries = 0;
	}
	else
	{
		*vbo_size = align(*vbo_size, ACCEL_STRUCT_ALIGNMENT);

		info->blas_data_offset = *vbo_size;
		*vbo_size += info->build_sizes.accelerationStructureSize;
	}
}

void create_model_blas(model_geometry_t* info, VkBuffer buffer, const char* name)
{
	if (info->num_geometries == 0)
		return;

	VkAccelerationStructureCreateInfoKHR blasCreateInfo;
	Com_Memset( &blasCreateInfo, 0, sizeof(VkAccelerationStructureCreateInfoKHR) );
	blasCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	blasCreateInfo.pNext = NULL;
	blasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	blasCreateInfo.buffer = buffer;
	blasCreateInfo.offset = info->blas_data_offset;
	blasCreateInfo.size = info->build_sizes.accelerationStructureSize;

	VK_CHECK(qvkCreateAccelerationStructureKHR(vk.device, &blasCreateInfo, NULL, &info->accel));
	
	VkAccelerationStructureDeviceAddressInfoKHR  as_device_address_info;
	as_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	as_device_address_info.pNext = NULL;
	as_device_address_info.accelerationStructure = info->accel;

	info->blas_device_address = qvkGetAccelerationStructureDeviceAddressKHR(vk.device, &as_device_address_info);

	//if (name)
	//	ATTACH_LABEL_VARIABLE_NAME(info->accel, ACCELERATION_STRUCTURE_KHR, name);
}

void build_model_blas(VkCommandBuffer cmd_buf, model_geometry_t* info, size_t first_vertex_offset, const vkbuffer_t* buffer)
{
	if (!info->accel)
		return;

	assert(buffer->address);

	uint32_t total_prims = 0;

	for (uint32_t index = 0; index < info->num_geometries; index++)
	{
		VkAccelerationStructureGeometryKHR* geometry = info->geometries + index;

		geometry->geometry.triangles.vertexData.deviceAddress = buffer->address
			+ info->prim_offsets[index] * sizeof(prim_positions_t) + first_vertex_offset;

		total_prims += info->prim_counts[index];
	}

	VkAccelerationStructureBuildGeometryInfoKHR blasBuildinfo;
	memset(&blasBuildinfo, 0, sizeof(VkAccelerationStructureBuildGeometryInfoKHR));
	blasBuildinfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	blasBuildinfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	blasBuildinfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	blasBuildinfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	blasBuildinfo.geometryCount = info->num_geometries;
	blasBuildinfo.pGeometries = info->geometries;
	blasBuildinfo.dstAccelerationStructure = info->accel;
	blasBuildinfo.scratchData.deviceAddress = vk.buf_accel_scratch.address;
	assert(vk.buf_accel_scratch.address);

	const VkAccelerationStructureBuildRangeInfoKHR* pBlasBuildRange = info->build_ranges;

	qvkCmdBuildAccelerationStructuresKHR(cmd_buf, 1, &blasBuildinfo, &pBlasBuildRange);

	VkMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR
					| VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;


	VkPipelineStageFlags blas_dst_stage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
	qvkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		blas_dst_stage, 0, 1,
		&barrier, 0, 0, 0, 0);
}

static VkResult vkpt_vertex_buffer_upload_bsp_subnmodel_mesh( VkCommandBuffer cmd_buf, world_t &worldData )
{
	uint32_t i;
	char name[MAX_QPATH];

	vk_geometry_data_t *world_static = &worldData.geometry.world_static;
	vk_geometry_data_t *geom = &worldData.geometry.world_submodels;

	size_t vbo_size = geom->num_primitives * sizeof(VboPrimitive);
	geom->vertex_data_offset = vbo_size;
	vbo_size += geom->num_primitives * sizeof(prim_positions_t);
	size_t staging_size = vbo_size;

	for ( i = 0; i < worldData.num_bmodels; i++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[i];
		Q_snprintf(name, sizeof(name), "bsp:models[%d]", i);

		suballocate_model_blas_memory(&bmodel->geometry, &vbo_size, name);
	}

	if ( vbo_size <= 0 )
		return VK_INCOMPLETE;

	// only first buffer now, not dynamic ring yet..
	VkResult res = vk_rtx_buffer_create(&geom->buffer[0], vbo_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (res != VK_SUCCESS) return res;


	res = vk_rtx_buffer_create(&geom->staging_buffer, staging_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (res != VK_SUCCESS) return res;

	for ( i = 0; i < worldData.num_bmodels; i++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[i];

		Q_snprintf(name, sizeof(name), "bsp:models[%d]", i);

		create_model_blas(&bmodel->geometry, geom->buffer[0].buffer, name);
	}

	uint8_t* staging_data = (uint8_t*)buffer_map(&geom->staging_buffer);
	memcpy(staging_data, geom->primitives, geom->num_primitives * sizeof(VboPrimitive));

	prim_positions_t* positions = (prim_positions_t*)(staging_data + geom->vertex_data_offset);  // NOLINT(clang-diagnostic-cast-align)
	for (uint32_t prim = 0; prim < geom->num_primitives; ++prim)
	{
		VectorCopy(geom->primitives[prim].pos0, positions[prim][0]);
		VectorCopy(geom->primitives[prim].pos1, positions[prim][1]);
		VectorCopy(geom->primitives[prim].pos2, positions[prim][2]);
	}

	buffer_unmap(&geom->staging_buffer);

	//VkCommandBuffer cmd_buf = vkpt_begin_command_buffer( &vk.cmd_buffers_graphics );
	VkBufferCopy copyRegion;
	Com_Memset( &copyRegion, 0, sizeof(VkBufferCopy) );
	copyRegion.size = geom->staging_buffer.size;

	qvkCmdCopyBuffer( cmd_buf, geom->staging_buffer.buffer, geom->buffer[0].buffer, 1, &copyRegion );
	//if ( geom->dynamic_flags != BLAS_DYNAMIC_FLAG_NONE )
	//	qvkCmdCopyBuffer(cmd_buf, staging_buffer.buffer, geom->buffer[1].buffer, 1, &copyRegion);

	BUFFER_BARRIER(cmd_buf,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		geom->buffer[0].buffer,
		0,
		VK_WHOLE_SIZE
	);

	for ( i = 0; i < worldData.num_bmodels; i++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[i];
		build_model_blas( cmd_buf, &bmodel->geometry, geom->vertex_data_offset, &geom->buffer[0] );

		bmodel->geometry.instance_mask = bmodel->transparent ? world_static->geom_transparent.instance_mask : world_static->geom_opaque.instance_mask;
		bmodel->geometry.instance_flags = bmodel->masked ? world_static->geom_masked.instance_flags : ( bmodel->transparent ? world_static->geom_transparent.instance_flags : world_static->geom_opaque.instance_flags);
		bmodel->geometry.sbt_offset = bmodel->masked ? world_static->geom_masked.sbt_offset : world_static->geom_opaque.sbt_offset;
	}

	//vkpt_submit_command_buffer(cmd_buf, vk.queue_graphics, (1 << vk.device_count) - 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL);

	return VK_SUCCESS;
}

static VkResult vk_rtx_vertex_buffer_upload_bsp_mesh_geom( VkCommandBuffer cmd_buf, vk_geometry_data_t *geom )
{
	qvkDeviceWaitIdle( vk.device );

	bool is_sky_static = (geom == &tr.world->geometry.sky_static);

	size_t vbo_size = geom->num_primitives * sizeof(VboPrimitive);
	geom->vertex_data_offset = vbo_size;
	vbo_size += geom->num_primitives * sizeof(prim_positions_t);
	size_t staging_size = vbo_size;

	suballocate_model_blas_memory( &geom->geom_opaque,      &vbo_size, "bsp:opaque");
	if ( !is_sky_static ) {
		suballocate_model_blas_memory( &geom->geom_transparent, &vbo_size, "bsp:transparent");
		suballocate_model_blas_memory( &geom->geom_masked,      &vbo_size, "bsp:masked");
	}

	if ( vbo_size <= 0 )
	{
		size_t vbo_size = 1 * sizeof(VboPrimitive);

		VkResult res = vk_rtx_buffer_create(&geom->buffer[0], vbo_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		return VK_INCOMPLETE;
	}

	// only first buffer now, not dynamic ring yet..
	VkResult res = vk_rtx_buffer_create(&geom->buffer[0], vbo_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (res != VK_SUCCESS) return res;

	res = vk_rtx_buffer_create(&geom->staging_buffer, staging_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (res != VK_SUCCESS) return res;

	// only first blas now, not dynamic ring yet..
	create_model_blas(&geom->geom_opaque,			geom->buffer[0].buffer, "bsp:opaque");
	if ( !is_sky_static ) {
		create_model_blas(&geom->geom_transparent,	geom->buffer[0].buffer, "bsp:transparent");
		create_model_blas(&geom->geom_masked,		geom->buffer[0].buffer, "bsp:masked");
	}

	uint8_t* staging_data = (uint8_t*)buffer_map(&geom->staging_buffer);
	memcpy(staging_data, geom->primitives, geom->num_primitives * sizeof(VboPrimitive));

	prim_positions_t* positions = (prim_positions_t*)(staging_data + geom->vertex_data_offset);  // NOLINT(clang-diagnostic-cast-align)
	for (uint32_t prim = 0; prim < geom->num_primitives; ++prim)
	{
		VectorCopy(geom->primitives[prim].pos0, positions[prim][0]);
		VectorCopy(geom->primitives[prim].pos1, positions[prim][1]);
		VectorCopy(geom->primitives[prim].pos2, positions[prim][2]);
	}

	buffer_unmap(&geom->staging_buffer);

	//VkCommandBuffer cmd_buf = vkpt_begin_command_buffer( &vk.cmd_buffers_graphics );
	VkBufferCopy copyRegion;
	Com_Memset( &copyRegion, 0, sizeof(VkBufferCopy) );
	copyRegion.size = geom->staging_buffer.size;

	qvkCmdCopyBuffer( cmd_buf, geom->staging_buffer.buffer, geom->buffer[0].buffer, 1, &copyRegion );
	//if ( geom->dynamic_flags != BLAS_DYNAMIC_FLAG_NONE )
	//	qvkCmdCopyBuffer(cmd_buf, staging_buffer.buffer, geom->buffer[1].buffer, 1, &copyRegion);

	BUFFER_BARRIER(cmd_buf,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		geom->buffer[0].buffer,
		0,
		VK_WHOLE_SIZE
	);

	build_model_blas( cmd_buf, &geom->geom_opaque,      geom->vertex_data_offset, &geom->buffer[0] );
	if ( !is_sky_static ) {
		build_model_blas( cmd_buf, &geom->geom_transparent, geom->vertex_data_offset, &geom->buffer[0] );
		build_model_blas( cmd_buf, &geom->geom_masked,      geom->vertex_data_offset, &geom->buffer[0] );
	}

	geom->geom_opaque.instance_mask = is_sky_static ? AS_FLAG_SKY : AS_FLAG_OPAQUE;
	geom->geom_opaque.instance_flags = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
	geom->geom_opaque.sbt_offset = SBTO_OPAQUE;

	if ( !is_sky_static ) {
		geom->geom_transparent.instance_mask = AS_FLAG_TRANSPARENT;
		geom->geom_transparent.instance_flags = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
		geom->geom_transparent.sbt_offset = SBTO_OPAQUE;

		geom->geom_masked.instance_mask = AS_FLAG_OPAQUE;
		geom->geom_masked.instance_flags = VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR | VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		geom->geom_masked.sbt_offset = SBTO_MASKED;
	}

	//vkpt_submit_command_buffer(cmd_buf, vk.queue_graphics, (1 << vk.device_count) - 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL);


	return VK_SUCCESS;
}

void vkpt_vertex_buffer_upload_bsp_mesh( world_t &worldData )
{
	uint32_t i;

	vk_geometry_data_t *sky_static				= &worldData.geometry.sky_static;
	vk_geometry_data_t *world_static			= &worldData.geometry.world_static;
	vk_geometry_data_t *world_dynamic_material	= &worldData.geometry.world_dynamic_material;
	vk_geometry_data_t *world_dynamic_geometry	= &worldData.geometry.world_dynamic_geometry;
	vk_geometry_data_t *world_submodels			= &worldData.geometry.world_submodels;
#ifdef DEBUG_POLY_LIGHTS 
	vk_geometry_data_t *debug_light_polys		= &worldData.geometry.debug_light_polys;
#endif

	VkCommandBuffer cmd_buf = vkpt_begin_command_buffer(&vk.cmd_buffers_graphics);

	vk_rtx_vertex_buffer_upload_bsp_mesh_geom( cmd_buf, sky_static );
	vk_rtx_vertex_buffer_upload_bsp_mesh_geom( cmd_buf, world_static );
	vk_rtx_vertex_buffer_upload_bsp_mesh_geom( cmd_buf, world_dynamic_material );
	vk_rtx_vertex_buffer_upload_bsp_mesh_geom( cmd_buf, world_dynamic_geometry );
#ifdef DEBUG_POLY_LIGHTS 
	vk_rtx_vertex_buffer_upload_bsp_mesh_geom( cmd_buf, debug_light_polys );
#endif

	// build submodel buffer and blas
	vkpt_vertex_buffer_upload_bsp_subnmodel_mesh( cmd_buf, worldData );
	
	//
	// fill light buffer
	//
	assert( vk.buf_light_stats[0].buffer != NULL );

	for ( i = 0; i < NUM_LIGHT_STATS_BUFFERS; i++ )
		qvkCmdFillBuffer( cmd_buf, vk.buf_light_stats[i].buffer, 0, vk.buf_light_stats[i].size, 0 );
	
	vkpt_submit_command_buffer(cmd_buf, vk.queue_graphics, (1 << vk.device_count) - 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL);

	qvkDeviceWaitIdle( vk.device );

	vk_rtx_buffer_destroy( &sky_static->staging_buffer );
	vk_rtx_buffer_destroy( &world_static->staging_buffer );
	vk_rtx_buffer_destroy( &world_dynamic_material->staging_buffer );
	vk_rtx_buffer_destroy( &world_dynamic_geometry->staging_buffer );
	vk_rtx_buffer_destroy( &world_submodels->staging_buffer );
#ifdef DEBUG_POLY_LIGHTS 
	vk_rtx_buffer_destroy( &debug_light_polys->staging_buffer );
#endif

	vk.scratch_buf_ptr = 0;
}