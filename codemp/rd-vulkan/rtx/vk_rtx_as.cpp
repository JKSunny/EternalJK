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

static inline size_t align(size_t x, size_t alignment)
{
	return (x + (alignment - 1)) & ~(alignment - 1);
}

qboolean RB_ASDynamic( shader_t *shader ) 
{
	return (qboolean)((shader->numDeforms > 0) || (backEnd.currentEntity->e.frame > 0 || backEnd.currentEntity->e.oldframe > 0));
}

qboolean RB_ASDataDynamic( shader_t *shader ) {
	uint32_t i, j;
	qboolean changes = qfalse;

	for ( i = 0; i < MAX_SHADER_STAGES; i++ ) 
	{
		if ( tess.shader->stages[i] != NULL && tess.shader->stages[i]->active ) 
		{
			for ( j = 0; j < NUM_TEXTURE_BUNDLES; j++ ) 
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

void vk_rtx_reset_accel_offsets( void ) 
{
	uint32_t i;

	// world
	vk.geometry.idx_world_static_offset = 0;
	vk.geometry.xyz_world_static_offset = 0;
	vk.geometry.idx_world_dynamic_data_offset = 0;
	vk.geometry.xyz_world_dynamic_data_offset = 0;

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		vk.geometry.idx_world_dynamic_as_offset[i] = 0;
		vk.geometry.xyz_world_dynamic_as_offset[i] = 0;
	}

	// cluster
	vk.geometry.cluster_world_static_offset = 0;
	vk.geometry.cluster_sky_static_offset = 0;
	vk.geometry.cluster_world_dynamic_data_offset = 0;
	vk.geometry.cluster_world_dynamic_as_offset = 0;
}

static inline int accel_matches(vk_blas_match_info_t *match,
								int fast_build,
								uint32_t vertex_count,
								uint32_t index_count) {
	return match->fast_build == fast_build &&
		   match->vertex_count >= vertex_count &&
		   match->index_count >= index_count;
}

void vk_rtx_create_blas( accel_build_batch_t *batch,  
								 vkbuffer_t *vertex_buffer, VkDeviceAddress vertex_offset,
								 vkbuffer_t *index_buffer, VkDeviceAddress index_offset,
								 uint32_t num_vertices, uint32_t num_indices,
								 vk_blas_t *blas, qboolean is_dynamic, qboolean fast_build, 
								 qboolean allow_update, qboolean instanced ) 
{	
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
	VkDeviceSize stride = instanced ? sizeof(float) * 3 : sizeof(VertexBuffer);

	triangles.vertexData.deviceAddress = vertex_buffer->address + ( vertex_offset * stride );
	triangles.vertexStride = stride;

	triangles.maxVertex = num_vertices - 1;
	triangles.indexData.hostAddress = VK_NULL_HANDLE;
	triangles.indexData.deviceAddress = index_buffer ? (index_buffer->address + (index_offset * sizeof(uint32_t))) : 0;
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

		blas->create_num_vertices = num_vertices;
		blas->create_num_indices = num_indices;

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

	// build offset
	VkAccelerationStructureBuildRangeInfoKHR offset;
	Com_Memset( &offset, 0, sizeof(VkAccelerationStructureBuildRangeInfoKHR) );
	offset.primitiveCount = MAX(num_vertices, num_indices) / 3;
	VkAccelerationStructureBuildRangeInfoKHR *offsets = &offset;

	batch->rangeInfos[buildIdx] = offset;
	batch->rangeInfoPtrs[buildIdx] = &batch->rangeInfos[buildIdx];
}

void vk_rtx_update_blas( VkCommandBuffer cmd_buf, 
								 vk_blas_t* oldBas, vk_blas_t* newBas, 
								 VkDeviceSize* offset, VkBuildAccelerationStructureFlagsKHR flag) 
{
	uint32_t num_vertices = newBas->create_num_vertices;
	uint32_t num_indices = newBas->create_num_indices;

	bool fast_build = false;

	// Prepare build info now, acceleration is filled later
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
	Com_Memset( &buildInfo, 0, sizeof(VkAccelerationStructureBuildGeometryInfoKHR) );
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildInfo.pNext = NULL;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildInfo.flags = fast_build ? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR : VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
	buildInfo.dstAccelerationStructure = VK_NULL_HANDLE;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &newBas->geometries;
	buildInfo.ppGeometries = NULL;

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo;

	if (oldBas == newBas) {
		// Find size to build on the device
		uint32_t max_primitive_count = MAX(num_vertices, num_indices) / 3; // number of tris
		Com_Memset( &sizeInfo, 0, sizeof(VkAccelerationStructureBuildSizesInfoKHR) );
		sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		qvkGetAccelerationStructureBuildSizesKHR(vk.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &max_primitive_count, &sizeInfo);
	
		// do this here for now
		{
			// set where the build lands
			buildInfo.dstAccelerationStructure = newBas->accel;
			buildInfo.srcAccelerationStructure = oldBas->accel;
			buildInfo.scratchData.deviceAddress = vk.buf_accel_scratch.address + vk.scratch_buf_ptr;

			vk.scratch_buf_ptr += sizeInfo.buildScratchSize;
			vk.scratch_buf_ptr = align(vk.scratch_buf_ptr, vk.minAccelerationStructureScratchOffsetAlignment);
		}

		// build offset
		VkAccelerationStructureBuildRangeInfoKHR offset;
		Com_Memset( &offset, 0, sizeof(VkAccelerationStructureBuildRangeInfoKHR) );
		offset.primitiveCount = MAX(num_vertices, num_indices) / 3;
		VkAccelerationStructureBuildRangeInfoKHR *offsets = &offset;

		qvkCmdBuildAccelerationStructuresKHR( cmd_buf, 1, &buildInfo, &offsets );
	}

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
	VK_DestroyBuffer( &as->mem );

	if ( as->accel != VK_NULL_HANDLE ) 
	{
		qvkDestroyAccelerationStructureKHR( vk.device, as->accel, NULL );
		as->accel = VK_NULL_HANDLE;
	}

	memset( &as->accel, 0, sizeof(vk_tlas_t) );

}

void vk_rtx_destroy_blas( vk_blas_t *blas ) 
{
	VK_DestroyBuffer( &blas->mem );

	if ( blas->accel != VK_NULL_HANDLE ) 
	{
		qvkDestroyAccelerationStructureKHR( vk.device, blas->accel, NULL );
		blas->accel = VK_NULL_HANDLE;
	}

	memset( blas, 0, sizeof(vk_blas_t) );
}

void vk_rtx_destroy_accel_all() {
	uint32_t idx;

	for ( idx = 0; idx < vk.swapchain_image_count; idx++ ) {
		vk_rtx_destroy_tlas( &vk.tlas_geometry[idx] );
	}

	vk_rtx_destroy_blas( &vk.blas_static.world );
	// add the rest
}