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

// Per Vulkan spec, acceleration structure offset must be a multiple of 256
// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkAccelerationStructureCreateInfoKHR.html
#define ACCEL_STRUCT_ALIGNMENT 256

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

qboolean RB_IsSky(shader_t* shader)
{
	return (qboolean)(shader->isSky || shader->sun || (shader->surfaceFlags & SURF_SKY));
}

qboolean RB_IsDynamicGeometry( shader_t *shader ) 
{
	return (qboolean)((shader->numDeforms > 0) || (backEnd.currentEntity->e.frame > 0 || backEnd.currentEntity->e.oldframe > 0));
}

qboolean RB_IsDynamicMaterial( shader_t *shader ) {
	uint32_t i, j;
	qboolean changes = qfalse;

	for ( i = 0; i < MAX_SHADER_STAGES; i++ ) 
	{
		if ( shader->stages[i] != NULL && shader->stages[i]->active ) 
		{
			for ( j = 0; j < shader->stages[i]->numTexBundles; j++ ) 
			{

				if ( shader->stages[i]->bundle[j].numImageAnimations > 0 ) 
					return qtrue;

				if ( (shader->stages[i]->bundle[j].tcGen != TCGEN_BAD) && (shader->stages[i]->bundle[j].numTexMods > 0 ) ) 
					return qtrue;

				if ( shader->stages[i]->bundle[0].rgbGen == CGEN_WAVEFORM )
					return qtrue;
			}
		}
	}
	return changes;
}


static inline int accel_matches(vk_blas_match_info_t *match,
								int fast_build,
								uint32_t vertex_count,
								uint32_t index_count) {
	return match->fast_build == fast_build &&
		   match->vertex_count >= vertex_count &&
		   match->index_count >= index_count;
}

void vk_rtx_create_blas_bsp( accel_build_batch_t *batch, vk_geometry_data_t *geom )
{
	uint32_t i, type;

	for ( type = 0; type < BLAS_TYPE_COUNT; ++type ) 
	{
		if ( !(geom->blas_type_flags & (1 << type) ) )
			continue;

		vk_geometry_data_accel_t *accel = &geom->accel[type];

		uint32_t num_instances = geom->dynamic_flags ? NUM_COMMAND_BUFFERS : 1;

		for ( i = 0; i < num_instances; ++i )
		{
			vk_rtx_create_blas(
				batch,
				&geom->primitives[i], accel->offset_primitives,
				NULL, 0,
				accel->num_primitives, 0,
				&accel->blas[i],
				geom->dynamic_flags ? qtrue : qfalse, 
				geom->fast_build, geom->allow_update, geom->is_world,
				geom->vertex_data_offset,
				"bsp"
			);
		}
	}
}

void vk_rtx_create_blas( accel_build_batch_t *batch,  
								 vkbuffer_t *vertex_buffer, VkDeviceAddress vertex_offset,
								 vkbuffer_t *index_buffer, VkDeviceAddress index_offset,
								 uint32_t num_vertices, uint32_t num_indices,
								 vk_blas_t *blas, qboolean is_dynamic, qboolean fast_build, 
								 qboolean allow_update, qboolean is_world, size_t first_vertex_offset,
								 const char* debug_label ) 
{	
#if 0
	if ( num_vertices <= 0 || num_indices <= 0) {
        printf("Skipping BLAS: xyz_count=%u, idx_count=%u\n", num_vertices, num_indices);
        //return;
    }
#endif

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
	//VkDeviceSize stride = is_world ? sizeof(VboPrimitive) : sizeof(float) * 3;
	//VkDeviceSize stride = is_world ? sizeof(VertexBuffer) : sizeof(float) * 3;

	//blas->vertex_address = vertex_buffer->address + ( vertex_offset * stride );
	blas->vertex_address = vertex_buffer->address + vertex_offset;
	if ( is_world) // ~sunny, -.-
		 blas->vertex_address = vertex_buffer->address + first_vertex_offset + vertex_offset * sizeof(prim_positions_t) ;

	blas->index_address = index_buffer ? (index_buffer->address + (index_offset * sizeof(uint32_t))) : 0;

	triangles.vertexData.deviceAddress = blas->vertex_address;
	triangles.vertexStride = stride;

	triangles.maxVertex = num_vertices - 1;
	if ( is_world ) // ~sunny, -.-
		triangles.maxVertex = num_vertices * 3; // num_vertices = num_prims

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

	if ( is_world ) // ~sunny, -.-
		max_primitive_count = num_vertices * 3;


	VkAccelerationStructureBuildSizesInfoKHR sizeInfo;
	Com_Memset( &sizeInfo, 0, sizeof(VkAccelerationStructureBuildSizesInfoKHR) );
	sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	qvkGetAccelerationStructureBuildSizesKHR(vk.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, buildInfo, &max_primitive_count, &sizeInfo);

	if ( doAlloc ) 
	{
		int num_vertices_to_allocate = num_vertices;
		int num_indices_to_allocate = num_indices;

		if ( is_world ) // ~sunny, -.-
			num_vertices_to_allocate = num_vertices * 3;

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

	if ( is_world ) // ~sunny, -.-
		blas->range.primitiveCount = num_vertices;

	batch->rangeInfos[buildIdx] = blas->range;
	batch->rangeInfoPtrs[buildIdx] = &batch->rangeInfos[buildIdx];
}

static inline bool vk_rtx_blas_needs_rebuild( const vk_blas_t* blas, vk_geometry_data_accel_t *accel ) 
{
	return (accel->xyz_count > blas->create_num_vertices || accel->idx_count > blas->create_num_indices);
}

void vk_rtx_update_blas( VkCommandBuffer cmd_buf, vk_geometry_data_t *geom, vk_geometry_data_accel_t *accel,
	vk_blas_t *blas, vk_blas_t *old_blas )
{
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
	uint32_t i, j, idx;

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
#define AS_VERTEX_BUFFER_FLAGS ( \
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | \
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | \
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | \
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | \
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR )

#define AS_INDEX_BUFFER_FLAGS ( \
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | \
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | \
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | \
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | \
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR )

#define AS_CLUSTER_BUFFER_FLAGS ( \
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | \
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT )

#define AS_PRIMITIVE_BUFFER_FLAGS ( \
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | \
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | \
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | \
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | \
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | \
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR )

void vkpt_init_model_geometry(model_geometry_t* info, uint32_t max_geometries)
{
	assert(info->geometry_storage == NULL); // avoid double allocation

	if (max_geometries == 0)
		return;

	size_t size_geometries = max_geometries * sizeof(VkAccelerationStructureGeometryKHR);
	size_t size_build_ranges = max_geometries * sizeof(VkAccelerationStructureBuildRangeInfoKHR);
	size_t size_prims = max_geometries * sizeof(uint32_t);

	//worldData.cluster_lights = (int*)Z_Malloc(worldData.num_cluster_lights * sizeof(int), TAG_GENERAL, qtrue );

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

static void vk_rtx_upload_world_staging( VkCommandBuffer cmd_buf, vkbuffer_t *staging, vkbuffer_t *device )
{
	if ( !staging || !device ) {
		Com_Error(ERR_DROP, "Incorrect BSP buffers\n");
		return;
	}

	VkBufferCopy copyRegion;
	Com_Memset( &copyRegion, 0, sizeof(VkBufferCopy) );
	copyRegion.size = staging->size;

	qvkCmdCopyBuffer(cmd_buf, staging->buffer, device->buffer, 1, &copyRegion);
}

static void vk_rtx_build_geometry_buffer( VkCommandBuffer cmd_buf, vk_geometry_data_t *geom, size_t blas_size ) 
{
	uint32_t i, type;
	uint32_t idx_count = 0;
	uint32_t xyz_count = 0;
	uint32_t cluster_count = 0;
	uint32_t num_primitives = 0;
	const uint32_t offset = 0 ;

	for ( type = 0; type < BLAS_TYPE_COUNT; ++type ) 
	{
		idx_count		+= geom->accel[type].idx_count;
		xyz_count		+= geom->accel[type].xyz_count;
		cluster_count	+= geom->accel[type].cluster_count;
		num_primitives	+= geom->accel[type].num_primitives;
	}

	qboolean has_primtives = num_primitives > 0 ? qtrue : qfalse;
	num_primitives = MAX( 1, num_primitives);

	// dynamic geometries overallocation
	uint32_t alloc_idx_count		= idx_count;
	uint32_t alloc_xyz_count		= xyz_count;
	uint32_t alloc_cluster_count	= cluster_count;
	uint32_t alloc_num_primitives	= num_primitives;
	if ( geom->dynamic_flags & BLAS_DYNAMIC_FLAG_ALL )
	{
		alloc_idx_count *= 2;
		alloc_xyz_count *= 2;
		alloc_cluster_count *= 2;
		alloc_num_primitives *= 2;
	}

	// ~sunny, primitive buffer rewrite :)
	size_t vbo_size = alloc_num_primitives * sizeof(VboPrimitive);
	geom->vertex_data_offset = vbo_size;
	vbo_size += alloc_num_primitives * sizeof(prim_positions_t);
	size_t staging_size = vbo_size;

	qboolean is_host_visible = qfalse;

	if ( geom->dynamic_flags == BLAS_DYNAMIC_FLAG_NONE )
	{
		VK_CreateAttributeBuffer( &geom->idx[0],			MAX( 1, alloc_idx_count ) * sizeof(uint32_t), (VkBufferUsageFlagBits) AS_INDEX_BUFFER_FLAGS,		is_host_visible );
		VK_CreateAttributeBuffer( &geom->xyz[0],			MAX( 1, alloc_xyz_count ) * sizeof(VertexBuffer), (VkBufferUsageFlagBits)AS_VERTEX_BUFFER_FLAGS,	is_host_visible );
		VK_CreateAttributeBuffer( &geom->cluster[0],		MAX( 1, alloc_cluster_count ) * sizeof(uint32_t), (VkBufferUsageFlagBits)AS_CLUSTER_BUFFER_FLAGS,	is_host_visible );
		VK_CreateAttributeBuffer( &geom->primitives[0],		staging_size + blas_size, (VkBufferUsageFlagBits)AS_PRIMITIVE_BUFFER_FLAGS,	is_host_visible );

		if ( idx_count <= 0 || xyz_count <= 0 )
			return;

		is_host_visible = qtrue;
		VK_CreateAttributeBuffer( &geom->staging_idx,			MAX( 1, alloc_idx_count ) * sizeof(uint32_t),		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,	is_host_visible );
		VK_CreateAttributeBuffer( &geom->staging_xyz,			MAX( 1, alloc_xyz_count ) * sizeof(VertexBuffer),	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,	is_host_visible );
		VK_CreateAttributeBuffer( &geom->staging_cluster,		MAX( 1, alloc_cluster_count ) * sizeof(uint32_t),	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,	is_host_visible );
		VK_CreateAttributeBuffer( &geom->staging_primitives,	staging_size,	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,	is_host_visible );
		
		vk_rtx_upload_buffer_data_offset( &geom->staging_idx,			0, idx_count * sizeof(uint32_t),			(const byte*)geom->host.idx + offset * sizeof(uint32_t) );
		vk_rtx_upload_buffer_data_offset( &geom->staging_xyz,			0, xyz_count * sizeof(VertexBuffer),		(const byte*)geom->host.xyz + offset * sizeof(VertexBuffer) );
		vk_rtx_upload_buffer_data_offset( &geom->staging_cluster,		0, cluster_count * sizeof(uint32_t),		(const byte*)geom->host.cluster + offset * sizeof(uint32_t) );
		vk_rtx_upload_buffer_data_offset( &geom->staging_primitives,	0, vbo_size,								(const byte*)geom->host.primitives + offset * sizeof(VboPrimitive) );
	
		if ( has_primtives )
		{
			vkbuffer_t *staging_buffer = &geom->staging_primitives;
			byte *staging_data = (byte *)buffer_map(staging_buffer);

			prim_positions_t* positions = (prim_positions_t*)(staging_data + geom->vertex_data_offset);  // NOLINT(clang-diagnostic-cast-align)
			for (uint32_t prim = 0; prim < num_primitives; ++prim)
			{
				VectorCopy(geom->host.primitives[prim].pos0, positions[prim][0]);
				VectorCopy(geom->host.primitives[prim].pos1, positions[prim][1]);
				VectorCopy(geom->host.primitives[prim].pos2, positions[prim][2]);
			}

			buffer_unmap(staging_buffer);
		}

		vk_rtx_upload_world_staging( cmd_buf, &geom->staging_idx,			&geom->idx[0] );
		vk_rtx_upload_world_staging( cmd_buf, &geom->staging_xyz,			&geom->xyz[0] );
		vk_rtx_upload_world_staging( cmd_buf, &geom->staging_cluster,		&geom->cluster[0] );
		vk_rtx_upload_world_staging( cmd_buf, &geom->staging_primitives,	&geom->primitives[0] );

		/*vk_rtx_buffer_destroy( &geom->staging_idx );
		vk_rtx_buffer_destroy( &geom->staging_xyz );
		vk_rtx_buffer_destroy( &geom->staging_cluster );*/

		return;
	}
	
	uint32_t num_instances = geom->dynamic_flags ? NUM_COMMAND_BUFFERS : 1;
	is_host_visible = qtrue;

	for ( i = 0; i < num_instances; ++i )
	{
		VK_CreateAttributeBuffer( &geom->idx[i], MAX( 1, alloc_idx_count ) * sizeof(uint32_t), (VkBufferUsageFlagBits) AS_INDEX_BUFFER_FLAGS,			is_host_visible );
		VK_CreateAttributeBuffer( &geom->xyz[i], MAX( 1, alloc_xyz_count ) * sizeof(VertexBuffer), (VkBufferUsageFlagBits)AS_VERTEX_BUFFER_FLAGS,		is_host_visible );
		VK_CreateAttributeBuffer( &geom->cluster[i], MAX( 1, alloc_cluster_count ) * sizeof(uint32_t), (VkBufferUsageFlagBits)AS_CLUSTER_BUFFER_FLAGS,	is_host_visible );
		VK_CreateAttributeBuffer( &geom->primitives[i], staging_size + blas_size, (VkBufferUsageFlagBits)AS_PRIMITIVE_BUFFER_FLAGS,	is_host_visible );

		vk_rtx_upload_buffer_data_offset( &geom->idx[i], 0, idx_count * sizeof(uint32_t), (const byte*)geom->host.idx + offset * sizeof(uint32_t) );
		vk_rtx_upload_buffer_data_offset( &geom->xyz[i], 0, xyz_count * sizeof(VertexBuffer), (const byte*)geom->host.xyz + offset * sizeof(VertexBuffer) );
		vk_rtx_upload_buffer_data_offset( &geom->cluster[i], 0, cluster_count * sizeof(uint32_t), (const byte*)geom->host.cluster + offset * sizeof(uint32_t) );
		vk_rtx_upload_buffer_data_offset( &geom->primitives[i], 0, num_primitives * sizeof(VboPrimitive), (const byte*)geom->host.primitives + offset * sizeof(VboPrimitive) );
	
	
		if ( has_primtives )
		{
			vkbuffer_t *staging_buffer = &geom->primitives[i];
			byte *staging_data = (byte *)buffer_map(staging_buffer);

			prim_positions_t* positions = (prim_positions_t*)(staging_data + geom->vertex_data_offset);  // NOLINT(clang-diagnostic-cast-align)
			for (uint32_t prim = 0; prim < num_primitives; ++prim)
			{
				VectorCopy(geom->host.primitives[prim].pos0, positions[prim][0]);
				VectorCopy(geom->host.primitives[prim].pos1, positions[prim][1]);
				VectorCopy(geom->host.primitives[prim].pos2, positions[prim][2]);
			}

			buffer_unmap(staging_buffer);
		}
	}
}

static void vkpt_vertex_buffer_upload_bsp_subnmodel_mesh( world_t &worldData, VkCommandBuffer cmd_buf )
{
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
// | BLAS Accel Structure | Acceleration data for bottom-level AS (only sub-brush models)     | [follows prim_positions_t]  |
//

	uint32_t i;
	char name[MAX_QPATH];

	// gather blas size to tail prim_positions_t ()
	size_t blas_size = 0; 
	
	vk_geometry_data_t *world_submodels	= &worldData.geometry.world_submodels;

	for ( i = 0; i < worldData.num_bmodels; i++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[i];
		Q_snprintf(name, sizeof(name), "bsp:models[%d]", i);

		suballocate_model_blas_memory(&bmodel->geometry, &blas_size, name);
	}

	vk_rtx_build_geometry_buffer( cmd_buf, world_submodels, blas_size );

	for ( i = 0; i < worldData.num_bmodels; i++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[i];

		Q_snprintf(name, sizeof(name), "bsp:models[%d]", i);

		create_model_blas(&bmodel->geometry, world_submodels->primitives->buffer, name);
	}

	for ( i = 0; i < worldData.num_bmodels; i++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[i];
		build_model_blas( cmd_buf, &bmodel->geometry, world_submodels->vertex_data_offset, world_submodels->primitives );

		bmodel->geometry.instance_mask = AS_FLAG_OPAQUE;
		bmodel->geometry.instance_flags = 0;
		bmodel->geometry.sbt_offset = 0;
	}
}

void vkpt_vertex_buffer_upload_bsp_mesh( world_t &worldData )
{
	uint32_t i;

	VkCommandBuffer cmd_buf = vkpt_begin_command_buffer(&vk.cmd_buffers_graphics);

	vk_geometry_data_t *sky_static				= &worldData.geometry.sky_static;
	vk_geometry_data_t *world_static			= &worldData.geometry.world_static;
	vk_geometry_data_t *world_dynamic_material	= &worldData.geometry.world_dynamic_material;
	vk_geometry_data_t *world_dynamic_geometry	= &worldData.geometry.world_dynamic_geometry;
	
	accel_build_batch_t batch;
	Com_Memset( &batch, 0, sizeof(accel_build_batch_t) );

	// ~sunny, rework this to follow same layout a described in 
	// header of this method, tail blas accels in one unified buffer
	// no more separete blases
	// see: vkpt_vertex_buffer_upload_bsp_subnmodel_mesh()
	//
	vk_rtx_build_geometry_buffer( cmd_buf, sky_static, 0 );
	vk_rtx_build_geometry_buffer( cmd_buf, world_static, 0 );
	vk_rtx_build_geometry_buffer( cmd_buf, world_dynamic_material, 0 );
	vk_rtx_build_geometry_buffer( cmd_buf, world_dynamic_geometry, 0 );
		
	vk_rtx_create_blas_bsp( &batch, sky_static				);
	vk_rtx_create_blas_bsp( &batch, world_static			);
	vk_rtx_create_blas_bsp( &batch, world_dynamic_material	);
	vk_rtx_create_blas_bsp( &batch, world_dynamic_geometry	);

	qvkCmdBuildAccelerationStructuresKHR( cmd_buf, batch.numBuilds, batch.buildInfos, batch.rangeInfoPtrs );
	MEM_BARRIER_BUILD_ACCEL(cmd_buf); /* probably not needed here but doesn't matter */

	// build submodel buffer and blas
	vkpt_vertex_buffer_upload_bsp_subnmodel_mesh( worldData, cmd_buf );

	//
	// fill light buffer
	//
	assert( vk.buf_light_stats[0].buffer != NULL );

	for ( i = 0; i < NUM_LIGHT_STATS_BUFFERS; i++ )
		qvkCmdFillBuffer( cmd_buf, vk.buf_light_stats[i].buffer, 0, vk.buf_light_stats[i].size, 0 );
	
	vkpt_submit_command_buffer(cmd_buf, vk.queue_graphics, (1 << vk.device_count) - 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL);

	vk.scratch_buf_ptr = 0;
}