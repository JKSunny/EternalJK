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

static VkBuffer shade_bufs[12];	// +4 qtangents, lightdir, bones, weight

static int bind_base;
static int bind_count;

void vk_select_texture( const int index ) 
{
	if (vk.ctmu == index)
		return;

	if ( index >= glConfig.maxActiveTextures )
		ri.Error(ERR_DROP, "%s: texture unit overflow = %i", __func__, index);

	vk.ctmu = index;
}

void vk_set_depthrange( const Vk_Depth_Range depthRange ) 
{
	tess.depthRange = depthRange;
}

VkBuffer vk_get_vertex_buffer( void )
{
	return vk.cmd->vertex_buffer;
}
 
static void get_mvp_transform( float *mvp )
{
	if (backEnd.projection2D)
	{
		float mvp0 = 2.0f / SCREEN_WIDTH;
		float mvp5 = 2.0f / SCREEN_HEIGHT;

		mvp[0] = mvp0; mvp[1] = 0.0f; mvp[2] = 0.0f; mvp[3] = 0.0f;
		mvp[4] = 0.0f; mvp[5] = mvp5; mvp[6] = 0.0f; mvp[7] = 0.0f;
#ifdef USE_REVERSED_DEPTH
		mvp[8] = 0.0f; mvp[9] = 0.0f; mvp[10] = 0.0f; mvp[11] = 0.0f;
		mvp[12] = -1.0f; mvp[13] = -1.0f; mvp[14] = 1.0f; mvp[15] = 1.0f;
#else
		mvp[8] = 0.0f; mvp[9] = 0.0f; mvp[10] = 1.0f; mvp[11] = 0.0f;
		mvp[12] = -1.0f; mvp[13] = -1.0f; mvp[14] = 0.0f; mvp[15] = 1.0f;
#endif
	}
	else
	{
		const float* p = backEnd.viewParms.projectionMatrix;
		float proj[16];
		Com_Memcpy(proj, p, 64);

		proj[5] = -p[5];
		myGlMultMatrix(vk_world.modelview_transform, proj, mvp);
	}
}

void vk_update_mvp( const float *m ) {
	float push_constants[16]; // mvp transform

	// Specify push constants.
	if (m)
		Com_Memcpy(push_constants, m, sizeof(push_constants));
	else
		get_mvp_transform(push_constants);

	qvkCmdPushConstants(vk.cmd->command_buffer, vk.pipeline_layout, 
		VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants), push_constants);

#ifdef USE_VK_STATS
	vk.stats.push_size += sizeof(push_constants);
#endif
}

void vk_set_2d( void ) 
{
	backEnd.projection2D = qtrue;

	vk_update_mvp(NULL);

	// force depth range and viewport/scissor updates
	vk.cmd->depth_range = DEPTH_RANGE_COUNT;

	// set 2D virtual screen size
	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds() * ri.Cvar_VariableValue("timescale");
	backEnd.refdef.floatTime = (double)backEnd.refdef.time * 0.001; // -EC-: cast to double

	return;
}

static void vk_bind_index_attr( int index )
{
	if (bind_base == -1) {
		bind_base = index;
		bind_count = 1;
	}
	else {
		bind_count = index - bind_base + 1;
	}
}

static void vk_bind_attr( int index, unsigned int item_size, const void *src ) {
	const uint32_t offset = PAD(vk.cmd->vertex_buffer_offset, 32);
	const uint32_t size = tess.numVertexes * item_size;

	if (offset + size > vk.geometry_buffer_size) {
		// schedule geometry buffer resize
		vk.geometry_buffer_size_new = log2pad(offset + size, 1);
	}
	else {
		vk.cmd->buf_offset[index] = offset;
		Com_Memcpy(vk.cmd->vertex_buffer_ptr + offset, src, size);
		vk.cmd->vertex_buffer_offset = (VkDeviceSize)offset + size;
	}

	vk_bind_index_attr(index);
}

uint32_t vk_tess_index( uint32_t numIndexes, const void *src ) {
	const uint32_t offset = vk.cmd->vertex_buffer_offset;
	const uint32_t size = numIndexes * sizeof(tess.indexes[0]);

	if (offset + size > vk.geometry_buffer_size) {
		// schedule geometry buffer resize
		vk.geometry_buffer_size_new = log2pad(offset + size, 1);
		return ~0U;
	}
	else {
		Com_Memcpy(vk.cmd->vertex_buffer_ptr + offset, src, size);
		vk.cmd->vertex_buffer_offset = (VkDeviceSize)offset + size;
		return offset;
	}
}

void vk_bind_index_buffer( VkBuffer buffer, uint32_t offset )
{
	if (vk.cmd->curr_index_buffer != buffer || vk.cmd->curr_index_offset != offset)
		qvkCmdBindIndexBuffer(vk.cmd->command_buffer, buffer, offset, VK_INDEX_TYPE_UINT32);

	vk.cmd->curr_index_buffer = buffer;
	vk.cmd->curr_index_offset = offset;
}

void vk_bind_index( void )
{
#ifdef USE_VBO
	if (tess.vboIndex) {
		vk.cmd->num_indexes = 0;
		//qvkCmdBindIndexBuffer( vk.cmd->command_buffer, vk.vbo.index_buffer, tess.shader->iboOffset, VK_INDEX_TYPE_UINT32 );
		return;
	}
#endif

	vk_bind_index_ext(tess.numIndexes, tess.indexes);
}

void vk_bind_index_ext( const int numIndexes, const uint32_t *indexes )
{
	uint32_t offset	= vk_tess_index( numIndexes, indexes );

	if ( offset != ~0U ) {
		vk_bind_index_buffer( vk.cmd->vertex_buffer, offset );
		vk.cmd->num_indexes = numIndexes;
	} else {
		// overflowed
		vk.cmd->num_indexes = 0;
	}
}

#ifdef USE_VBO_MDV
static void vk_vbo_bind_geometry_mdv( int32_t flags )
{
	shade_bufs[8] = shade_bufs[9] = shade_bufs[10] = shade_bufs[11] = vk.vbo[vk.vbo_index].vertex_buffer;

	mdvVBOMesh_t *mesh_ptr = (mdvVBOMesh_t*)tess.vboMeshPtr;

	vk.cmd->vbo_offset[0] = mesh_ptr->vboOffset + 0; // possibly add framenum here
	vk.cmd->vbo_offset[2] = mesh_ptr->texOffset;
	vk.cmd->vbo_offset[5] = mesh_ptr->normalOffset; // possibly add framenum here

	if (flags & TESS_ST1)
		vk.cmd->vbo_offset[3] = mesh_ptr->texOffset;

	if (flags & TESS_ST2)
		vk.cmd->vbo_offset[4] = mesh_ptr->texOffset;

#ifdef USE_VK_PBR
	if (flags & TESS_PBR) {
		vk.cmd->vbo_offset[8] = mesh_ptr->qtangentOffset;
	}
#endif

	qvkCmdBindVertexBuffers( vk.cmd->command_buffer, 0, 12, shade_bufs, vk.cmd->vbo_offset + 0 );
}
#endif

#ifdef USE_VBO_GHOUL2
static void vk_vbo_bind_geometry_ghoul2( uint32_t flags )
{
	shade_bufs[8] = shade_bufs[9] = shade_bufs[10] = shade_bufs[11] = vk.vbo[vk.vbo_index].vertex_buffer;

	mdxmVBOMesh_t *mesh_ptr = (mdxmVBOMesh_t*)tess.vboMeshPtr;

	vk.cmd->vbo_offset[0] = mesh_ptr->vboOffset + 0;
	vk.cmd->vbo_offset[2] = mesh_ptr->texOffset;
	vk.cmd->vbo_offset[5] = mesh_ptr->normalOffset;
	vk.cmd->vbo_offset[10] = mesh_ptr->boneOffset;
	vk.cmd->vbo_offset[11] = mesh_ptr->weightOffset;

	if (flags & TESS_ST1)
		vk.cmd->vbo_offset[3] = mesh_ptr->texOffset;

	if (flags & TESS_ST2)
		vk.cmd->vbo_offset[4] = mesh_ptr->texOffset;

#ifdef USE_VK_PBR
	if (flags & TESS_PBR) {
		vk.cmd->vbo_offset[8] = mesh_ptr->qtangentOffset;
	}
#endif

	qvkCmdBindVertexBuffers( vk.cmd->command_buffer, 0, 12, shade_bufs, vk.cmd->vbo_offset + 0 );

#if 0
	// use the vertex buffer for rgba ..
	// or would ( #else const vec4 in_color0 = vec4(0.0); ) in glsl suffice
	// if vertex is not used. rather, is it ever used for ghoul2?
	if ( flags & TESS_RGBA0 ) { 
		vk_bind_attr_ghoul2_vbo( 1, sizeof(color4ub_t), tess.svars.colors[0] );
		qvkCmdBindVertexBuffers( vk.cmd->command_buffer, 1, 1, &vk.cmd->vertex_buffer, vk.cmd->buf_offset + 1 );
	}

	if ( flags & TESS_RGBA1 ) {
		vk_bind_attr_ghoul2_vbo( 6, sizeof(color4ub_t), tess.svars.colors[1] );
		qvkCmdBindVertexBuffers( vk.cmd->command_buffer, 6, 1, &vk.cmd->vertex_buffer, vk.cmd->buf_offset + 6 );
	}

	if ( flags & TESS_RGBA2 ) {
		vk_bind_attr_ghoul2_vbo( 7, sizeof(color4ub_t), tess.svars.colors[2] );
		qvkCmdBindVertexBuffers( vk.cmd->command_buffer, 7, 1, &vk.cmd->vertex_buffer, vk.cmd->buf_offset + 7 );
	}
#endif
}
#endif

void vk_bind_geometry( uint32_t flags )
{
	bind_base = -1;
	bind_count = 0;

	if ((flags & (TESS_XYZ | TESS_RGBA0 | TESS_ST0 | TESS_ST1 | TESS_ST2 | TESS_NNN | TESS_RGBA1 | TESS_RGBA2)) == 0)
		return;

#ifdef USE_VBO
	if (tess.vboIndex) {

		shade_bufs[0] = shade_bufs[1] = shade_bufs[2] = shade_bufs[3] = shade_bufs[4] = shade_bufs[5] = shade_bufs[6] = shade_bufs[7] = vk.vbo[vk.vbo_index].vertex_buffer;

#if defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV)  
		// clear, because vbo buffers don't share sizes
		// previously set offsets might be out of scope
		if ( vk.vboGhoul2Active || vk.vboMdvActive ) {
			Com_Memset( vk.cmd->vbo_offset, 0, sizeof(vk.cmd->vbo_offset) );

			if ( tess.surfType == SF_MDX )
				return vk_vbo_bind_geometry_ghoul2( flags );

			if ( tess.surfType == SF_VBO_MDVMESH )
				return vk_vbo_bind_geometry_mdv( flags );
		}
#endif

#ifdef USE_VK_PBR
		shade_bufs[8] = vk.vbo[vk.vbo_index].vertex_buffer;
		shade_bufs[9] = vk.vbo[vk.vbo_index].vertex_buffer;
#endif

		if (flags & TESS_XYZ) {  // 0
			vk.cmd->vbo_offset[0] = tess.shader->vboOffset + 0;
			vk_bind_index_attr(0);
		}

		if (flags & TESS_RGBA0) { // 1
			vk.cmd->vbo_offset[1] = tess.shader->stages[tess.vboStage]->rgb_offset[0];
			vk_bind_index_attr(1);
		}

		if (flags & TESS_ST0) {  // 2
			vk.cmd->vbo_offset[2] = tess.shader->stages[tess.vboStage]->tex_offset[0];
			vk_bind_index_attr(2);
		}

		if (flags & TESS_ST1) {  // 3
			vk.cmd->vbo_offset[3] = tess.shader->stages[tess.vboStage]->tex_offset[1];
			vk_bind_index_attr(3);
		}

		if (flags & TESS_ST2) {  // 4
			vk.cmd->vbo_offset[4] = tess.shader->stages[tess.vboStage]->tex_offset[2];
			vk_bind_index_attr(4);
		}

		if (flags & TESS_NNN) { // 5
			vk.cmd->vbo_offset[5] = tess.shader->normalOffset;
			vk_bind_index_attr(5);
		}

		if (flags & TESS_RGBA1) { // 6
			vk.cmd->vbo_offset[6] = tess.shader->stages[tess.vboStage]->rgb_offset[1];
			vk_bind_index_attr(6);
		}

		if (flags & TESS_RGBA2) { // 7
			vk.cmd->vbo_offset[7] = tess.shader->stages[tess.vboStage]->rgb_offset[2];
			vk_bind_index_attr(7);
		}
#ifdef USE_VK_PBR
		if (flags & TESS_PBR) {
			vk.cmd->vbo_offset[8] = tess.shader->qtangentOffset;
			vk_bind_index_attr(8);

			vk.cmd->vbo_offset[9] = tess.shader->lightdirOffset;
			vk_bind_index_attr(9);
		}
#endif
		qvkCmdBindVertexBuffers(vk.cmd->command_buffer, bind_base, bind_count, shade_bufs, vk.cmd->vbo_offset + bind_base);
	}
	else
#endif // USE_VBO
	{
		shade_bufs[0] = shade_bufs[1] = shade_bufs[2] = shade_bufs[3] = shade_bufs[4] = shade_bufs[5] = shade_bufs[6] = shade_bufs[7] = vk.cmd->vertex_buffer;
#ifdef USE_VK_PBR
		shade_bufs[8] = vk.cmd->vertex_buffer;
		shade_bufs[9] = vk.cmd->vertex_buffer;
#endif

		if (flags & TESS_XYZ)
			vk_bind_attr(0, sizeof(tess.xyz[0]), &tess.xyz[0]);

		if (flags & TESS_RGBA0)
			vk_bind_attr(1, sizeof(color4ub_t), tess.svars.colors[0]);

		if (flags & TESS_ST0)
			vk_bind_attr(2, sizeof(vec2_t), tess.svars.texcoordPtr[0]);

		if (flags & TESS_ST1)
			vk_bind_attr(3, sizeof(vec2_t), tess.svars.texcoordPtr[1]);

		if (flags & TESS_ST2)
			vk_bind_attr(4, sizeof(vec2_t), tess.svars.texcoordPtr[2]);

		if (flags & TESS_NNN)
			vk_bind_attr(5, sizeof(tess.normal[0]), tess.normal);

		if (flags & TESS_RGBA1)
			vk_bind_attr(6, sizeof(color4ub_t), tess.svars.colors[1]);

		if (flags & TESS_RGBA2)
			vk_bind_attr(7, sizeof(color4ub_t), tess.svars.colors[2]);
#ifdef USE_VK_PBR
		if (flags & TESS_PBR) {
			vk_bind_attr(8, sizeof(tess.qtangent[0]), tess.qtangent);
			vk_bind_attr(9, sizeof(tess.lightdir[0]), tess.lightdir);
		}
#endif
		qvkCmdBindVertexBuffers(vk.cmd->command_buffer, bind_base, bind_count, shade_bufs, vk.cmd->buf_offset + bind_base);
	}
}

void vk_bind_lighting( int stage, int bundle )
{
	bind_base = -1;
	bind_count = 0;

#ifdef USE_VBO
	if (tess.vboIndex) {

		shade_bufs[0] = shade_bufs[1] = shade_bufs[2] = vk.vbo[vk.vbo_index].vertex_buffer;

		vk.cmd->vbo_offset[0] = tess.shader->vboOffset + 0;
		vk.cmd->vbo_offset[1] = tess.shader->stages[stage]->tex_offset[bundle];
		vk.cmd->vbo_offset[2] = tess.shader->normalOffset;

		qvkCmdBindVertexBuffers(vk.cmd->command_buffer, 0, 3, shade_bufs, vk.cmd->vbo_offset + 0);

	}
	else
#endif // USE_VBO
	{
		shade_bufs[0] = shade_bufs[1] = shade_bufs[2] = vk.cmd->vertex_buffer;

		vk_bind_attr(0, sizeof(tess.xyz[0]), &tess.xyz[0]);
		vk_bind_attr(1, sizeof(vec2_t), tess.svars.texcoordPtr[bundle]);
		vk_bind_attr(2, sizeof(tess.normal[0]), tess.normal);

		qvkCmdBindVertexBuffers(vk.cmd->command_buffer, bind_base, bind_count, shade_bufs, vk.cmd->buf_offset + bind_base);
	}
}

static void vk_write_uniform_descriptor( VkWriteDescriptorSet *desc, VkDescriptorBufferInfo *info, VkDescriptorSet descriptor, const uint32_t binding )
{
	desc->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc->dstSet = descriptor;
	desc->dstBinding = binding;
	desc->dstArrayElement = 0;
	desc->descriptorCount = 1;
	desc->pNext = NULL;
	desc->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	desc->pImageInfo = NULL;
	desc->pBufferInfo = info;
	desc->pTexelBufferView = NULL;
}

void vk_update_uniform_descriptor( VkDescriptorSet descriptor, VkBuffer buffer )
{
	uint32_t count = 0;
	VkDescriptorBufferInfo info[4];
	VkWriteDescriptorSet desc[4];

	info[count].buffer = buffer;
	info[count].offset = 0;
	info[count].range = sizeof(vkUniform_t);

	vk_write_uniform_descriptor( desc + 0, info + 0, descriptor, 0 );
	count++;

	// camera
	info[count].buffer = buffer;
	info[count].offset = 0;
	info[count].range = sizeof(vkUniformCamera_t);

	vk_write_uniform_descriptor( desc + 1, info + 1, descriptor, 1 );
	count++;

#ifdef USE_VBO_GHOUL2
	if ( vk.vboGhoul2Active ) {
		// data
		info[count].buffer = buffer;
		info[count].offset = 0;
		info[count].range = sizeof(vkUniformData_t);

		vk_write_uniform_descriptor( desc + 2, info + 2, descriptor, 2 );
		count++;

		// bones
		info[count].buffer = buffer;
		info[count].offset = 0;
		info[count].range = sizeof(vkUniformGhoul_t);

		vk_write_uniform_descriptor( desc + 3, info + 3, descriptor, 3 );
		count++;
	}
#endif

	qvkUpdateDescriptorSets(vk.device, count, desc, 0, NULL);
}

void vk_create_storage_buffer( uint32_t size )
{
	VkMemoryRequirements memory_requirements;
	VkMemoryAllocateInfo alloc_info;
	VkBufferCreateInfo desc;
	uint32_t memory_type_bits;
	uint32_t memory_type;

	desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.queueFamilyIndexCount = 0;
	desc.pQueueFamilyIndices = NULL;
	
	Com_Memset(&memory_requirements, 0, sizeof(memory_requirements));
	
	desc.size = size;
	desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	VK_CHECK(qvkCreateBuffer(vk.device, &desc, NULL, &vk.storage.buffer));
	
	qvkGetBufferMemoryRequirements(vk.device, vk.storage.buffer, &memory_requirements);

	memory_type_bits = memory_requirements.memoryTypeBits;
	memory_type = vk_find_memory_type(memory_type_bits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = memory_type;
	VK_CHECK(qvkAllocateMemory(vk.device, &alloc_info, NULL, &vk.storage.memory));
	VK_CHECK(qvkMapMemory(vk.device, vk.storage.memory, 0, VK_WHOLE_SIZE, 0, (void**)&vk.storage.buffer_ptr));

	Com_Memset(vk.storage.buffer_ptr, 0, memory_requirements.size);

	qvkBindBufferMemory(vk.device, vk.storage.buffer, vk.storage.memory, 0);

	VK_SET_OBJECT_NAME(vk.storage.buffer, "storage buffer", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
	VK_SET_OBJECT_NAME(vk.storage.descriptor, "storage buffer", VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT);
	VK_SET_OBJECT_NAME(vk.storage.memory, "storage buffer memory", VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT);
}

void vk_update_attachment_descriptors( void ) {

	if ( vk.color_image_view )
	{
		VkDescriptorImageInfo info;
		VkWriteDescriptorSet desc;
		Vk_Sampler_Def sd;

		Com_Memset( &sd, 0, sizeof(sd) );
		sd.gl_mag_filter = sd.gl_min_filter = vk.blitFilter;
		sd.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sd.max_lod_1_0 = qtrue;
		sd.noAnisotropy = qtrue;

		info.sampler = vk_find_sampler( &sd );
		info.imageView = vk.color_image_view;
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc.dstSet = vk.color_descriptor;
		desc.dstBinding = 0;
		desc.dstArrayElement = 0;
		desc.descriptorCount = 1;
		desc.pNext = NULL;
		desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		desc.pImageInfo = &info;
		desc.pBufferInfo = NULL;
		desc.pTexelBufferView = NULL;

		qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );

		// screenmap
		sd.gl_mag_filter = sd.gl_min_filter = GL_LINEAR;
		sd.max_lod_1_0 = qfalse;
		sd.noAnisotropy = qtrue;

		info.sampler = vk_find_sampler( &sd );

		info.imageView = vk.screenMap.color_image_view;
		desc.dstSet = vk.screenMap.color_descriptor;

		qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );

		// bloom images
		if ( vk.bloomActive )
		{
			uint32_t i;
			for (i = 0; i < ARRAY_LEN( vk.bloom_image_descriptor ); i++)
			{
				info.imageView = vk.bloom_image_view[i];
				desc.dstSet = vk.bloom_image_descriptor[i];

				qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );
			}
		}

		// dglow images
		if ( vk.dglowActive )
		{
			uint32_t i;
			for ( i = 0; i < ARRAY_LEN( vk.dglow_image_descriptor ); i++ )
			{
				info.imageView = vk.dglow_image_view[i];
				desc.dstSet = vk.dglow_image_descriptor[i];

				qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );
			}
		}

#ifdef VK_PBR_BRDFLUT
		if( vk.pbrActive )
		{
			// brdf
			info.imageView = vk.brdflut_image_view;
			desc.dstSet = vk.brdflut_image_descriptor;

			qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );	

			// cubemap
			info.imageView = vk.cubeMap.color_image_view[0];
			desc.dstSet = vk.cubeMap.color_descriptor;
			qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );	
		}
#endif
	}
}

void vk_init_descriptors( void ) {
	VkDescriptorSetAllocateInfo alloc;
	VkDescriptorBufferInfo info;
	VkWriteDescriptorSet desc;
	uint32_t i;

	alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc.pNext = NULL;
	alloc.descriptorPool = vk.descriptor_pool;
	alloc.descriptorSetCount = 1;
	alloc.pSetLayouts = &vk.set_layout_storage;
	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.storage.descriptor ) );

	info.buffer = vk.storage.buffer;
	info.offset = 0;
	info.range = sizeof(uint32_t);

	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet = vk.storage.descriptor;
	desc.dstBinding = 0;
	desc.dstArrayElement = 0;
	desc.descriptorCount = 1;
	desc.pNext = NULL;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	desc.pImageInfo = NULL;
	desc.pBufferInfo = &info;
	desc.pTexelBufferView = NULL;

	qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );

	for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ )
	{
		alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc.pNext = NULL;
		alloc.descriptorPool = vk.descriptor_pool;
		alloc.descriptorSetCount = 1;
		alloc.pSetLayouts = &vk.set_layout_uniform;
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.tess[i].uniform_descriptor ) );

		vk_update_uniform_descriptor( vk.tess[i].uniform_descriptor, vk.tess[i].vertex_buffer );
		VK_SET_OBJECT_NAME( vk.tess[i].uniform_descriptor, "uniform descriptor", VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT );
	}

	if ( vk.color_image_view )
	{
		alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc.pNext = NULL;
		alloc.descriptorPool = vk.descriptor_pool;
		alloc.descriptorSetCount = 1;
		alloc.pSetLayouts = &vk.set_layout_sampler;
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.color_descriptor ) );

		// bloom images
		if ( vk.bloomActive ) {
			for ( i = 0; i < ARRAY_LEN( vk.bloom_image_descriptor ); i++ )
				VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.bloom_image_descriptor[i] ) );
		}

		// dglow images
		if ( vk.dglowActive ) {
			for ( i = 0; i < ARRAY_LEN( vk.dglow_image_descriptor ); i++ )
				VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.dglow_image_descriptor[i] ) );
		}

		alloc.descriptorSetCount = 1;
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.screenMap.color_descriptor ) ); // screenmap

#ifdef VK_PBR_BRDFLUT
		if( vk.pbrActive )
			VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.brdflut_image_descriptor ) );
#endif

		// cubemap
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &vk.cubeMap.color_descriptor ) );

		vk_update_attachment_descriptors();
	}
}

void vk_create_vertex_buffer( VkDeviceSize size )
{
	VkMemoryRequirements vb_memory_requirements;
	VkDeviceSize vertex_buffer_offset;
	VkMemoryAllocateInfo alloc_info;
	VkBufferCreateInfo desc;
	uint32_t memory_type_bits;
	void *data;
	int i;

	vk_debug("Create vertex buffer: vk.cmd->vertex_buffer \n");
	
	desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.queueFamilyIndexCount = 0;
	desc.pQueueFamilyIndices = NULL;
	
	Com_Memset(&vb_memory_requirements, 0, sizeof(vb_memory_requirements));

	for (i = 0; i < NUM_COMMAND_BUFFERS; i++) {
		desc.size = size;
		desc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		VK_CHECK(qvkCreateBuffer(vk.device, &desc, NULL, &vk.tess[i].vertex_buffer));

		qvkGetBufferMemoryRequirements(vk.device, vk.tess[i].vertex_buffer, &vb_memory_requirements);
	}

	memory_type_bits = vb_memory_requirements.memoryTypeBits;

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = vb_memory_requirements.size * NUM_COMMAND_BUFFERS;
	alloc_info.memoryTypeIndex = vk_find_memory_type(memory_type_bits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vk_debug("Allocate device memory for Vertex Buffer: %ld bytes. \n", alloc_info.allocationSize);

	VK_CHECK(qvkAllocateMemory(vk.device, &alloc_info, NULL, &vk.geometry_buffer_memory));
	VK_CHECK(qvkMapMemory(vk.device, vk.geometry_buffer_memory, 0, VK_WHOLE_SIZE, 0, &data));

	vertex_buffer_offset = 0;

	for (i = 0; i < NUM_COMMAND_BUFFERS; i++) {
		qvkBindBufferMemory(vk.device, vk.tess[i].vertex_buffer, vk.geometry_buffer_memory, vertex_buffer_offset);
		vk.tess[i].vertex_buffer_ptr = (byte*)data + vertex_buffer_offset;
		vk.tess[i].vertex_buffer_offset = 0;
		vertex_buffer_offset += vb_memory_requirements.size;

		VK_SET_OBJECT_NAME(vk.tess[i].vertex_buffer, "vertex_buffer", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
	}

	VK_SET_OBJECT_NAME(vk.geometry_buffer_memory, "geometry buffer memory", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);

	vk.geometry_buffer_size = vb_memory_requirements.size;
	vk.geometry_buffer_size_new = 0;

	Com_Memset(&vk.stats, 0, sizeof(vk.stats));
}

void vk_reset_descriptor( int index )
{
	vk.cmd->descriptor_set.current[index] = VK_NULL_HANDLE;
}

void vk_update_descriptor( int tmu, VkDescriptorSet curDesSet )
{
	if (vk.cmd->descriptor_set.current[tmu] != curDesSet) {
		vk.cmd->descriptor_set.start = 
			(tmu < vk.cmd->descriptor_set.start) ? tmu : vk.cmd->descriptor_set.start;
		vk.cmd->descriptor_set.end = 
			(tmu > vk.cmd->descriptor_set.end) ? tmu : vk.cmd->descriptor_set.end;
	}

	vk.cmd->descriptor_set.current[tmu] = curDesSet;
}

#ifdef USE_VK_PBR
static void vk_set_valid_preceeding_descriptor( const int tmu )
{
	if( vk.cmd->descriptor_set.current[tmu] != VK_NULL_HANDLE )
		return;

	vk_update_descriptor(tmu, tr.emptyImage->descriptor_set);

	if( tmu > 0 )
		vk_set_valid_preceeding_descriptor( tmu - 1 );
}
static void vk_update_pbr_descriptor( const int tmu, VkDescriptorSet curDesSet ){
	
	vk_set_valid_preceeding_descriptor( tmu - 1 );

	vk_update_descriptor( tmu, curDesSet );
}
#endif

void vk_update_descriptor_offset( int index, uint32_t offset )
{
	vk.cmd->descriptor_set.offset[index] = offset;
}

void vk_bind_descriptor_sets( void ) 
{
	uint32_t offsets[5], offset_count;
	uint32_t start, end, count;

	start = vk.cmd->descriptor_set.start;
	if (start == ~0U)
		return;

	end = vk.cmd->descriptor_set.end;

	offset_count = 0;
	if (start <= 1) { // uniform offset or storage offset
		offsets[offset_count++] = vk.cmd->descriptor_set.offset[start];

		if ( start == 1 ){
			offsets[offset_count++] = vk.cmd->descriptor_set.offset[start+1];

#ifdef USE_VBO_GHOUL2
			if ( vk.vboGhoul2Active ){
				offsets[offset_count++] = vk.cmd->descriptor_set.offset[start+2];
				offsets[offset_count++] = vk.cmd->descriptor_set.offset[start+3];
			}
#endif
		}
	}

	count = end - start + 1;

	qvkCmdBindDescriptorSets(vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk.pipeline_layout, start, count, vk.cmd->descriptor_set.current + start, offset_count, offsets);

	vk.cmd->descriptor_set.end = 0;
	vk.cmd->descriptor_set.start = ~0U;
}

void vk_bind_pipeline( uint32_t pipeline ) {
	VkPipeline vkpipe;

	vkpipe = vk_gen_pipeline(pipeline);

	if (vkpipe != vk.cmd->last_pipeline) {
		qvkCmdBindPipeline(vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipe);
		vk.cmd->last_pipeline = vkpipe;
	}

	vk_world.dirty_depth_attachment |= (vk.pipelines[pipeline].def.state_bits & GLS_DEPTHMASK_TRUE);
}

void vk_draw_geometry( Vk_Depth_Range depRg, qboolean indexed )
{
	VkViewport viewport;
	VkRect2D scissor_rect;

	// geometry buffer overflow happened this frame
	if (vk.geometry_buffer_size_new)
		return;

	vk_bind_descriptor_sets();

	// configure pipeline's dynamic state
	if (vk.cmd->depth_range != depRg) {
		vk.cmd->depth_range = depRg;

		get_scissor_rect(&scissor_rect);

		if (memcmp(&vk.cmd->scissor_rect, &scissor_rect, sizeof(scissor_rect)) != 0) {
			qvkCmdSetScissor(vk.cmd->command_buffer, 0, 1, &scissor_rect);
			vk.cmd->scissor_rect = scissor_rect;
		}

		get_viewport(&viewport, depRg);
		qvkCmdSetViewport(vk.cmd->command_buffer, 0, 1, &viewport);
	}

	if (tess.shader->polygonOffset) {
		qvkCmdSetDepthBias(vk.cmd->command_buffer, r_offsetUnits->value, 0.0f, r_offsetFactor->value);
	}

	// issue draw call(s)
#ifdef USE_VBO
	if (tess.vboIndex)
		VBO_RenderIBOItems();
	else
#endif
	{
		if (indexed)
			qvkCmdDrawIndexed(vk.cmd->command_buffer, vk.cmd->num_indexes, 1, 0, 0, 0);
		else
			qvkCmdDraw(vk.cmd->command_buffer, tess.numVertexes, 1, 0, 0);
	}
}

void ComputeColors( const int b, color4ub_t *dest, const shaderStage_t *pStage, int forceRGBGen )
{
	int			i;
	qboolean killGen = qfalse;
	alphaGen_t forceAlphaGen = pStage->bundle[b].alphaGen;//set this up so we can override below

	if (!tess.numVertexes)
		return;

	if (tess.shader != tr.projectionShadowShader && tess.shader != tr.shadowShader &&
		(backEnd.currentEntity->e.renderfx & (RF_DISINTEGRATE1 | RF_DISINTEGRATE2)))
	{
		RB_CalcDisintegrateColors( (unsigned char*) dest );
		RB_CalcDisintegrateVertDeform();

		// We've done some custom alpha and color stuff, so we can skip the rest.  Let it do fog though
		killGen = qtrue;
	}

	//
	// rgbGen
	//
	if (!forceRGBGen)
	{
		forceRGBGen = pStage->bundle[b].rgbGen;
	}

	// does not work for rotated models, technically, this should also be a CGEN type.
	// But that would entail adding new shader commands....which is too much work for one thing
	if (backEnd.currentEntity->e.renderfx & RF_VOLUMETRIC) 
	{
		int			i;
		float* normal, dot;
		unsigned char* color;
		int			numVertexes;

		normal = tess.normal[0];
		color = tess.svars.colors[0][0];

		numVertexes = tess.numVertexes;

		for (i = 0; i < numVertexes; i++, normal += 4, color += 4)
		{
			dot = DotProduct(normal, backEnd.refdef.viewaxis[0]);

			dot *= dot * dot * dot;

			if (dot < 0.2f) // so low, so just clamp it
			{
				dot = 0.0f;
			}

			color[0] = color[1] = color[2] = color[3] = Q_ftol(backEnd.currentEntity->e.shaderRGBA[0] * (1 - dot));
		}

		killGen = qtrue;
	}

	if (killGen)
	{
		goto avoidGen;
	}

	//
	// rgbGen
	//
	switch (forceRGBGen)
	{
	case CGEN_IDENTITY:
		Com_Memset(dest, 0xff, tess.numVertexes * 4);
		break;
	default:
	case CGEN_IDENTITY_LIGHTING:
		Com_Memset(dest, tr.identityLightByte, tess.numVertexes * 4);
		break;
	case CGEN_LIGHTING_DIFFUSE:
		RB_CalcDiffuseColor( (unsigned char*) dest );
		break;
	case CGEN_LIGHTING_DIFFUSE_ENTITY:
		RB_CalcDiffuseEntityColor( (unsigned char*) dest );
		if (forceAlphaGen == AGEN_IDENTITY &&
			backEnd.currentEntity->e.shaderRGBA[3] == 0xff
			)
		{
			forceAlphaGen = AGEN_SKIP;	//already got it in this set since it does all 4 components
		}
		break;
	case CGEN_EXACT_VERTEX:
		Com_Memcpy(dest, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
		break;
	case CGEN_CONST:
		for (i = 0; i < tess.numVertexes; i++) {
			*(int *)dest[i] = *(int *)pStage->bundle[b].constantColor;
		}
		break;
	case CGEN_VERTEX:
		if (tr.identityLight == 1)
		{
			Com_Memcpy(dest, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
		}
		else
		{
			for ( i = 0; i < tess.numVertexes; i++ )
			{
				dest[i][0] = tess.vertexColors[i][0] * tr.identityLight;
				dest[i][1] = tess.vertexColors[i][1] * tr.identityLight;
				dest[i][2] = tess.vertexColors[i][2] * tr.identityLight;
				dest[i][3] = tess.vertexColors[i][3];
			}
		}
		break;
	case CGEN_ONE_MINUS_VERTEX:
		if (tr.identityLight == 1)
		{
			for (i = 0; i < tess.numVertexes; i++)
			{
				dest[i][0] = 255 - tess.vertexColors[i][0];
				dest[i][1] = 255 - tess.vertexColors[i][1];
				dest[i][2] = 255 - tess.vertexColors[i][2];
			}
		}
		else
		{
			for (i = 0; i < tess.numVertexes; i++)
			{
				dest[i][0] = (255 - tess.vertexColors[i][0]) * tr.identityLight;
				dest[i][1] = (255 - tess.vertexColors[i][1]) * tr.identityLight;
				dest[i][2] = (255 - tess.vertexColors[i][2]) * tr.identityLight;
			}
		}
		break;
	case CGEN_FOG:
	{
		const fog_t *fog;

		fog = tr.world->fogs + tess.fogNum;

		for (i = 0; i < tess.numVertexes; i++) {
			*(int *)&dest[i] = fog->colorInt;
		}
	}
	break;
	case CGEN_WAVEFORM:
		RB_CalcWaveColor(&pStage->bundle[b].rgbWave, (unsigned char*) dest);
		break;
	case CGEN_ENTITY:
		RB_CalcColorFromEntity( (unsigned char*) dest );
		if (forceAlphaGen == AGEN_IDENTITY && backEnd.currentEntity->e.shaderRGBA[3] == 0xff)
		{
			forceAlphaGen = AGEN_SKIP;	//already got it in this set since it does all 4 components
		}
		break;
	case CGEN_ONE_MINUS_ENTITY:
		RB_CalcColorFromOneMinusEntity( (unsigned char*) dest );
		break;
	case CGEN_LIGHTMAPSTYLE:
		for (i = 0; i < tess.numVertexes; i++)
		{
			*(int *)dest[i] = *(int *)styleColors[pStage->lightmapStyle];
		}
		break;
	}

	//
	// alphaGen
	//
	switch ( forceAlphaGen )
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		if (forceRGBGen != CGEN_IDENTITY) {
			if ((forceRGBGen == CGEN_VERTEX && tr.identityLight != 1) ||
				forceRGBGen != CGEN_VERTEX) {
				for (i = 0; i < tess.numVertexes; i++) {
					dest[i][3] = 0xff;
				}
			}
		}
		break;
	case AGEN_CONST:
		if (forceRGBGen != CGEN_CONST) {
			for (i = 0; i < tess.numVertexes; i++) {
				dest[i][3] = pStage->bundle[b].constantColor[3];
			}
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha(&pStage->bundle[b].alphaWave, (unsigned char*) dest );
		break;
	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha( (unsigned char*) dest );
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( (unsigned char*) dest );
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( (unsigned char*) dest );
		break;
	case AGEN_VERTEX:
		if (forceRGBGen != CGEN_VERTEX) {
			for (i = 0; i < tess.numVertexes; i++) {
				dest[i][3] = tess.vertexColors[i][3];
			}
		}
		break;
	case AGEN_ONE_MINUS_VERTEX:
		for (i = 0; i < tess.numVertexes; i++)
		{
			dest[i][3] = 255 - tess.vertexColors[i][3];
		}
		break;
	case AGEN_PORTAL:
	{
		for (i = 0; i < tess.numVertexes; i++)
		{
			unsigned char alpha;
			float len;
			vec3_t v;

			VectorSubtract(tess.xyz[i], backEnd.viewParms.ori.origin, v);
			len = VectorLength( v ) * tess.shader->portalRangeR;

			if ( len > 1 )
			{
				alpha = 0xff;
			}
			else
			{
				alpha = len * 0xff;
			}

			dest[i][3] = alpha;
		}
	}
	break;
	case AGEN_BLEND:
		if (forceRGBGen != CGEN_VERTEX)
		{
			for (i = 0; i < tess.numVertexes; i++)
			{
				dest[i][3] = tess.vertexAlphas[i][pStage->index]; //rwwRMG - added support
			}
		}
		break;
	default:
		break;
	}
avoidGen:
	//
	// fog adjustment for colors to fade out as fog increases
	//
	if (tess.fogNum)
	{
		switch (pStage->bundle[b].adjustColorsForFog)
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( (unsigned char*) dest );
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( (unsigned char*) dest );
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( (unsigned char*) dest );
			break;
		case ACFF_NONE:
			break;
		}
	}
}

static uint32_t vk_push_uniform( const vkUniform_t *uniform ) {
	const uint32_t offset = vk.cmd->uniform_read_offset = PAD(vk.cmd->vertex_buffer_offset, vk.uniform_alignment);

	if ( offset + vk.uniform_item_size > vk.geometry_buffer_size )
		return ~0U;

	// push uniform
	Com_Memcpy( vk.cmd->vertex_buffer_ptr + offset, uniform, sizeof(*uniform) );
	vk.cmd->vertex_buffer_offset = offset + vk.uniform_item_size;

	vk_reset_descriptor( 1 );
	vk_update_descriptor( 1, vk.cmd->uniform_descriptor );
	vk_update_descriptor_offset( 1, vk.cmd->uniform_read_offset );

	return offset;
}

#if defined(USE_VBO_GHOUL2) && defined(USE_VK_PBR)
static uint32_t vk_push_uniform_camera( const vkUniformCamera_t *uniform ) {	
	const uint32_t offset = vk.cmd->uniform_read_offset = PAD(vk.cmd->vertex_buffer_offset, vk.uniform_alignment);

	if ( offset + vk.uniform_camera_item_size > vk.geometry_buffer_size )
		return ~0U;

	Com_Memcpy( vk.cmd->vertex_buffer_ptr + offset, uniform, sizeof(*uniform) );
	vk.cmd->vertex_buffer_offset = offset + vk.uniform_camera_item_size;

	vk_reset_descriptor( 1 );
	vk_update_descriptor( 1, vk.cmd->uniform_descriptor );
	vk_update_descriptor_offset( 1, 0 );
	vk_update_descriptor_offset( 2, vk.cmd->uniform_read_offset );

	return offset;
}
#endif

#ifdef USE_VBO_GHOUL2
static uint32_t vk_push_uniform_data( const vkUniformData_t *uniform ) {	
	const uint32_t offset = vk.cmd->uniform_read_offset = PAD(vk.cmd->vertex_buffer_offset, vk.uniform_alignment);

	if ( offset + vk.uniform_data_item_size > vk.geometry_buffer_size )
		return ~0U;

	Com_Memcpy( vk.cmd->vertex_buffer_ptr + offset, uniform, sizeof(*uniform) );
	vk.cmd->vertex_buffer_offset = offset + vk.uniform_data_item_size;

	vk_reset_descriptor( 1 );
	vk_update_descriptor( 1, vk.cmd->uniform_descriptor );
	vk_update_descriptor_offset( 1, 0 );
	vk_update_descriptor_offset( 3, vk.cmd->uniform_read_offset );

	return offset;
}

static uint32_t vk_push_uniform_ghoul2( const vkUniformGhoul_t *uniform ) {	
	const uint32_t offset = vk.cmd->uniform_read_offset = PAD( vk.cmd->vertex_buffer_offset, vk.uniform_alignment );

	if ( offset + vk.uniform_ghoul_item_size > vk.geometry_buffer_size )
		return ~0U;

	Com_Memcpy( vk.cmd->vertex_buffer_ptr + offset, uniform, sizeof(*uniform) );
	vk.cmd->vertex_buffer_offset = offset + vk.uniform_ghoul_item_size;

	vk_reset_descriptor( 1 );
	vk_update_descriptor( 1, vk.cmd->uniform_descriptor );
	vk_update_descriptor_offset( 1, 0 );
	vk_update_descriptor_offset( 4, vk.cmd->uniform_read_offset );

	return offset;
}
#endif

/*
========================
RB_CalcFogProgramParms
========================
*/
const fogProgramParms_t *RB_CalcFogProgramParms( void )
{
	static fogProgramParms_t parm;
	const fog_t* fog;
	vec3_t		local;

	Com_Memset(parm.fogDepthVector, 0, sizeof(parm.fogDepthVector));

	fog = tr.world->fogs + tess.fogNum;

	// all fogging distance is based on world Z units
	VectorSubtract(backEnd.ori.origin, backEnd.viewParms.ori.origin, local);
	parm.fogDistanceVector[0] = -backEnd.ori.modelViewMatrix[2];
	parm.fogDistanceVector[1] = -backEnd.ori.modelViewMatrix[6];
	parm.fogDistanceVector[2] = -backEnd.ori.modelViewMatrix[10];
	parm.fogDistanceVector[3] = DotProduct(local, backEnd.viewParms.ori.axis[0]);

	// scale the fog vectors based on the fog's thickness
	parm.fogDistanceVector[0] *= fog->tcScale;
	parm.fogDistanceVector[1] *= fog->tcScale;
	parm.fogDistanceVector[2] *= fog->tcScale;
	parm.fogDistanceVector[3] *= fog->tcScale;

	// rotate the gradient vector for this orientation
	if (fog->hasSurface) {
		parm.fogDepthVector[0] = fog->surface[0] * backEnd.ori.axis[0][0] +
			fog->surface[1] * backEnd.ori.axis[0][1] + fog->surface[2] * backEnd.ori.axis[0][2];
		parm.fogDepthVector[1] = fog->surface[0] * backEnd.ori.axis[1][0] +
			fog->surface[1] * backEnd.ori.axis[1][1] + fog->surface[2] * backEnd.ori.axis[1][2];
		parm.fogDepthVector[2] = fog->surface[0] * backEnd.ori.axis[2][0] +
			fog->surface[1] * backEnd.ori.axis[2][1] + fog->surface[2] * backEnd.ori.axis[2][2];
		parm.fogDepthVector[3] = -fog->surface[3] + DotProduct(backEnd.ori.origin, fog->surface);

		parm.eyeT = DotProduct(backEnd.ori.viewOrigin, parm.fogDepthVector) + parm.fogDepthVector[3];
	}
	else {
		parm.eyeT = 1.0f; // non-surface fog always has eye inside
	}

	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog
	if (parm.eyeT < 0) {
		parm.eyeOutside = qtrue;
	}
	else {
		parm.eyeOutside = qfalse;
	}

	parm.fogDistanceVector[3] += 1.0 / 512;
	parm.fogColor = fog->color;

	return &parm;
}

#ifdef USE_PMLIGHT
static void vk_set_light_params( vkUniform_t *uniform, const dlight_t *dl ) {
	float radius;

	if (!glConfig.deviceSupportsGamma && !vk.fboActive)
		VectorScale(dl->color, 2 * powf(r_intensity->value, r_gamma->value), uniform->lightColor);
	else
		VectorCopy(dl->color, uniform->lightColor);

	radius = dl->radius;

	// vertex data
	VectorCopy(backEnd.ori.viewOrigin, uniform->eyePos); uniform->eyePos[3] = 0.0f;
	VectorCopy(dl->transformed, uniform->lightPos); uniform->lightPos[3] = 0.0f;

	// fragment data
	uniform->lightColor[3] = 1.0f / Square(radius);

	if (dl->linear)
	{
		vec4_t ab;
		VectorSubtract(dl->transformed2, dl->transformed, ab);
		ab[3] = 1.0f / DotProduct(ab, ab);
		VectorCopy4(ab, uniform->lightVector);
	}
}
#endif

static void vk_set_fog_params( vkUniform_t *uniform, int *fogStage )
{
	if (tess.fogNum && tess.shader->fogPass) {
		const fogProgramParms_t *fp = RB_CalcFogProgramParms();
		// vertex data
		VectorCopy4(fp->fogDistanceVector, uniform->fogDistanceVector);
		VectorCopy4(fp->fogDepthVector, uniform->fogDepthVector);
		uniform->fogEyeT[0] = fp->eyeT;
		if (fp->eyeOutside) {
			uniform->fogEyeT[1] = 0.0; // fog eye out
		}
		else {
			uniform->fogEyeT[1] = 1.0; // fog eye in
		}
		// fragment data
		if ( backEnd.isGlowPass )
			VectorCopy4( colorBlack, uniform->fogColor );
		else
			VectorCopy4( fp->fogColor, uniform->fogColor );

		*fogStage = 1;
	}
	else {
		*fogStage = 0;
	}
}

/*
===================
RB_FogPass
Blends a fog texture on top of everything else
===================
*/
static vkUniform_t			uniform;
static vkUniformCamera_t	uniform_camera;
#ifdef USE_VBO_GHOUL2
static vkUniformData_t		uniform_data;
static vkUniformGhoul_t		uniform_ghoul;

mat3x4_t *vk_get_uniform_ghoul_bones( void ) {
	return uniform_ghoul.boneMatrices;
}
#endif

static void RB_FogPass( void ) {
#ifdef USE_VBO_GHOUL2
	const int sh = ( tess.vboIndex && tess.surfType == SF_MDX ) ? 1 : 0;
#else
	const int sh = 0;
#endif
	uint32_t pipeline = vk.std_pipeline.fog_pipelines[sh][tess.shader->fogPass - 1][tess.shader->cullType][tess.shader->polygonOffset];

#ifdef USE_FOG_ONLY
	int fog_stage;
	
	vk_bind_pipeline(pipeline);
	vk_set_fog_params(&uniform, &fog_stage);
	vk_push_uniform(&uniform);
	vk_update_descriptor(3, tr.fogImage->descriptor_set);
	vk_draw_geometry(DEPTH_RANGE_NORMAL, qtrue);
#else
	const fog_t *fog;
	int			i;

	fog = tr.world->fogs + tess.fogNum;

	for (i = 0; i < tess.numVertexes; i++) {
		*(int*)&tess.svars.colors[0][i] = fog->colorInt;
	}

	RB_CalcFogTexCoords((float*)tess.svars.texcoords[0]);
	tess.svars.texcoordPtr[0] = tess.svars.texcoords[0];
	vk_bind(tr.fogImage);

	vk_bind_pipeline(pipeline);
	vk_bind_geometry(TESS_ST0 | TESS_RGBA0);
	vk_draw_geometry(DEPTH_RANGE_NORMAL, qtrue);
#endif
}

void vk_bind( image_t *image ) {
	if (!image) {
		vk_debug("vk_bind: NULL image\n");
		image = tr.defaultImage;
	}

	image->frameUsed = tr.frameCount;

	vk_update_descriptor(vk.ctmu + VK_SAMPLER_LAYOUT_BEGIN, image->descriptor_set);
}

void R_BindAnimatedImage( const textureBundle_t *bundle ) {

	int64_t index;

	if ( bundle->isVideoMap ) {
		ri.CIN_RunCinematic( bundle->videoMapHandle );
		ri.CIN_UploadCinematic( bundle->videoMapHandle );
		return;
	}
	if ( bundle->isScreenMap ) {
		if ( !backEnd.screenMapDone ) {
			vk_bind( tr.blackImage );
		}
		else {

			vk_update_descriptor( vk.ctmu + VK_SAMPLER_LAYOUT_BEGIN, vk.screenMap.color_descriptor );
		}
		return;
	}

	if ( ( r_fullbright->value /*|| tr.refdef.doFullbright */ ) && bundle->isLightmap )
	{
		vk_bind( tr.whiteImage );
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		vk_bind(bundle->image[0]);
		return;
	}

	if ( backEnd.currentEntity->e.renderfx & RF_SETANIMINDEX )
	{
		index = backEnd.currentEntity->e.skinNum;
	}
	else
	{
		// it is necessary to do this messy calc to make sure animations line up
		// exactly with waveforms of the same frequency
		index = Q_ftol( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
		index >>= FUNCTABLE_SIZE2;

		if ( index < 0 ) {
			index = 0;	// may happen with shader time offsets
		}
	}

	if ( bundle->oneShotAnimMap )
	{
		if ( index >= bundle->numImageAnimations )
		{
			// stick on last frame
			index = bundle->numImageAnimations - 1;
		}
	}
	else
	{
		// loop
		index %= bundle->numImageAnimations;
	}

	vk_bind( bundle->image[index] );
}

void ComputeTexCoords( const int b, const textureBundle_t *bundle ) {
	int	i;
	int tm;
	vec2_t* src, * dst;

	if (!tess.numVertexes)
		return;

	src = dst = tess.svars.texcoords[b];

	//
	// generate the texture coordinates
	//
	switch (bundle->tcGen)
	{
	case TCGEN_IDENTITY:
		src = tess.texCoords00;
		break;
	case TCGEN_TEXTURE:
		src = tess.texCoords[0];
		break;
	case TCGEN_LIGHTMAP:
		src = tess.texCoords[1];
		break;
	case TCGEN_LIGHTMAP1:
		src = tess.texCoords[2];
		break;
	case TCGEN_LIGHTMAP2:
		src = tess.texCoords[3];
		break;
	case TCGEN_LIGHTMAP3:
		src = tess.texCoords[4];
		break;
	case TCGEN_VECTOR:
		for (i = 0; i < tess.numVertexes; i++) {
			dst[i][0] = DotProduct(tess.xyz[i], bundle->tcGenVectors[0]);
			dst[i][1] = DotProduct(tess.xyz[i], bundle->tcGenVectors[1]);
		}
		break;
	case TCGEN_FOG:
		RB_CalcFogTexCoords((float*)dst);
		break;
	case TCGEN_ENVIRONMENT_MAPPED:
		RB_CalcEnvironmentTexCoords((float*)dst);
		break;
	//case TCGEN_ENVIRONMENT_MAPPED_FP:
	//	RB_CalcEnvironmentTexCoordsFP((float*)dst, bundle->isScreenMap);
	//	break;
	case TCGEN_BAD:
		return;
	}

	//
	// alter texture coordinates
	//
	for (tm = 0; tm < bundle->numTexMods; tm++) {
		switch (bundle->texMods[tm].type)
		{
		case TMOD_NONE:
			tm = TR_MAX_TEXMODS; // break out of for loop
			break;

		case TMOD_TURBULENT:
			RB_CalcTurbulentTexCoords(&bundle->texMods[tm].wave, (float*)src, (float*)dst);
			src = dst;
			break;

		case TMOD_ENTITY_TRANSLATE:
			RB_CalcScrollTexCoords(backEnd.currentEntity->e.shaderTexCoord, (float*)src, (float*)dst);
			src = dst;
			break;

		case TMOD_SCROLL:
			RB_CalcScrollTexCoords(bundle->texMods[tm].translate, (float*)src, (float*)dst);
			src = dst;
			break;

		case TMOD_SCALE:
			RB_CalcScaleTexCoords(bundle->texMods[tm].translate, (float*)src, (float*)dst);
			src = dst;
			break;

		case TMOD_STRETCH:
			RB_CalcStretchTexCoords(&bundle->texMods[tm].wave, (float*)src, (float*)dst);
			src = dst;
			break;

		case TMOD_TRANSFORM:
			RB_CalcTransformTexCoords(&bundle->texMods[tm], (float*)src, (float*)dst);
			src = dst;
			break;

		case TMOD_ROTATE:
			RB_CalcRotateTexCoords(bundle->texMods[tm].translate[0], (float*)src, (float*)dst);
			src = dst;
			break;

		default:
			ri.Error(ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'", bundle->texMods[tm].type, tess.shader->name);
			break;
		}
	}

	/*if (r_mergeLightmaps->integer && bundle->isLightmap && bundle->tcGen != TCGEN_LIGHTMAP) {
		// adjust texture coordinates to map on proper lightmap
		for (i = 0; i < tess.numVertexes; i++) {
			dst[i][0] = (src[i][0] * tr.lightmapScale[0]) + tess.shader->lightmapOffset[0];
			dst[i][1] = (src[i][1] * tr.lightmapScale[1]) + tess.shader->lightmapOffset[1];
		}
		src = dst;
	}*/

	tess.svars.texcoordPtr[b] = src;
}

#ifdef USE_VBO_GHOUL2
#if 0 // skip ghoul2 vbo glsl in_colors for now
static void vk_set_attr_color( color4ub_t *dest, const qboolean skip ){
	uint32_t i;
	int numVerts;

	numVerts = ( tess.vboIndex && tess.surfType == SF_MDX ) ? 
		tess.mesh_ptr->numVertexes : tess.numVertexes;

	if ( skip ) {
		Com_Memset( dest, 0, numVerts * sizeof(color4ub_t) );
		return;
	}

	for ( i = 0; i < numVerts; i++ ) {
		dest[i][0] = tess.vertexColors[i][0];
		dest[i][1] = tess.vertexColors[i][1];
		dest[i][2] = tess.vertexColors[i][2];
		dest[i][3] = tess.vertexColors[i][3];
	}
}
#endif

static void vk_compute_colors( const int b, const shaderStage_t *pStage, int forceRGBGen ){	
	if ( backEnd.currentEntity->e.renderfx & RF_VOLUMETRIC ) 
		return;

	float *baseColor, *vertColor;
#ifdef USE_VBO_GHOUL2_RGBAGEN_CONSTS
	qboolean skipInColor = qtrue;
#endif

	int rgbGen = forceRGBGen;
	int alphaGen = pStage->bundle[b].alphaGen;

	baseColor = (float*)uniform_data.baseColor[b];
	vertColor = (float*)uniform_data.vertColor[b];

	baseColor[0] = baseColor[1] = baseColor[2] = baseColor[3] = 1.0f;  	
   	vertColor[0] = vertColor[1] = vertColor[2] = vertColor[3] = 0.0f;

	if ( !forceRGBGen )
		rgbGen = pStage->bundle[b].rgbGen;

	switch ( rgbGen) {
		case CGEN_IDENTITY_LIGHTING: 
			baseColor[0] = baseColor[1] = baseColor[2] = tr.identityLight;
			break;
		case CGEN_EXACT_VERTEX:
			baseColor[0] = baseColor[1] = baseColor[2] = baseColor[3] = 0.0f;
			vertColor[0] = vertColor[1] = vertColor[2] = vertColor[3] = 1.0f;
			break;
		case CGEN_CONST:
			baseColor[0] = pStage->bundle[b].constantColor[0] / 255.0f;
			baseColor[1] = pStage->bundle[b].constantColor[1] / 255.0f;
			baseColor[2] = pStage->bundle[b].constantColor[2] / 255.0f;
			baseColor[3] = pStage->bundle[b].constantColor[3] / 255.0f;
			break;
		case CGEN_VERTEX:
			baseColor[0] = baseColor[1] = baseColor[2] = baseColor[3] = 0.0f;
			vertColor[0] = vertColor[1] = vertColor[2] = tr.identityLight;
			vertColor[3] = 1.0f;
			break;
		case CGEN_ONE_MINUS_VERTEX:
			baseColor[0] = baseColor[1] = baseColor[2] = tr.identityLight;
			vertColor[0] = vertColor[1] = vertColor[2] = -tr.identityLight;
			break;
		case CGEN_FOG:
			{
				fog_t *fog = tr.world->fogs + tess.fogNum;
				VectorCopy4(fog->color, baseColor);
			}
			break;
		case CGEN_WAVEFORM:
			baseColor[0] = baseColor[1] = baseColor[2] = RB_CalcWaveColorSingle( &pStage->bundle[b].rgbWave );
			break;
		case CGEN_ENTITY:
		case CGEN_LIGHTING_DIFFUSE_ENTITY:
			if ( backEnd.currentEntity )
			{
				baseColor[0] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;

				vertColor[0] = vertColor[1] = vertColor[2] = tr.identityLight;
				vertColor[3] = 1.0f;

				if ( alphaGen == AGEN_IDENTITY && backEnd.currentEntity->e.shaderRGBA[3] == 255 )
					alphaGen = AGEN_SKIP;
			}
			break;
		case CGEN_ONE_MINUS_ENTITY:
			if ( backEnd.currentEntity )
			{
				baseColor[0] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			break;
		case CGEN_LIGHTMAPSTYLE:
			VectorScale4 (styleColors[pStage->lightmapStyle], 1.0f / 255.0f, baseColor);
			break;
		case CGEN_IDENTITY:
		case CGEN_LIGHTING_DIFFUSE:
		case CGEN_BAD:
			break;
		default:
			break;
	}

	switch ( alphaGen ) {
		case AGEN_SKIP:
			break;
		case AGEN_CONST:
			if ( rgbGen != CGEN_CONST ) {
				baseColor[3] = pStage->bundle[b].constantColor[3] / 255.0f;
				vertColor[3] = 0.0f;
			}
			break;
		case AGEN_WAVEFORM:
			baseColor[3] = RB_CalcWaveAlphaSingle( &pStage->bundle[b].alphaWave );
			vertColor[3] = 0.0f;
			break;
		case AGEN_ENTITY:
			if ( backEnd.currentEntity )
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;

			vertColor[3] = 0.0f;
			break;
		case AGEN_ONE_MINUS_ENTITY:
			if ( backEnd.currentEntity )
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;

			vertColor[3] = 0.0f;
			break;
		case AGEN_VERTEX:
			if ( rgbGen != CGEN_VERTEX ) {
				baseColor[3] = 0.0f;
				vertColor[3] = 1.0f;			
			}
			break;
		case AGEN_ONE_MINUS_VERTEX:
			baseColor[3] = 1.0f;
			vertColor[3] = -1.0f;
			break;
		case AGEN_IDENTITY:
		case AGEN_LIGHTING_SPECULAR:
		case AGEN_PORTAL:
			// done entirely in vertex program
			baseColor[3] = 1.0f;
			vertColor[3] = 0.0f;
			break;
		default:
			break;
	}

#ifdef USE_VBO_GHOUL2_RGBAGEN_CONSTS
	switch ( rgbGen) {
		case CGEN_EXACT_VERTEX:
		case CGEN_VERTEX:
		case CGEN_ONE_MINUS_VERTEX:
			skipInColor = qfalse; break;
	}
	switch ( alphaGen) {
		case AGEN_VERTEX:
		case AGEN_ONE_MINUS_VERTEX:
			skipInColor = qfalse; break;
	}

	// skip ghoul2 vbo glsl in_colors for now
	vk_set_attr_color( tess.svars.colors[b], skipInColor );
#endif

	if ( backEnd.currentEntity && backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA ) {
		baseColor[3] = backEnd.currentEntity->e.shaderRGBA[3] / 255.0f; 
		vertColor[3] = 0.0f;
	}

	// multiply color by overbrightbits if this isn't a blend
	if ( tr.overbrightBits 
	 && !( ( pStage->stateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_COLOR )
	 && !( ( pStage->stateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR )
	 && !( ( pStage->stateBits & GLS_DSTBLEND_BITS ) == GLS_DSTBLEND_SRC_COLOR )
	 && !( ( pStage->stateBits & GLS_DSTBLEND_BITS ) == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR ) )
	{
		float scale = 1 << tr.overbrightBits;

		baseColor[0] *= scale;
		baseColor[1] *= scale;
		baseColor[2] *= scale;
		vertColor[0] *= scale;
		vertColor[1] *= scale;
		vertColor[2] *= scale;
	}

	uniform_data.rgbGen[b] = (float)rgbGen;
	uniform_data.alphaGen[b] = (float)alphaGen;
}

static void vk_compute_deform( void ) {
	int		type = DEFORM_NONE;
	int		waveFunc = GF_NONE;
	float	*info;

	info = (float*)uniform_data.deformInfo;

	Com_Memset( info + 0, 0, sizeof(float) * 12 );

	if ( backEnd.currentEntity->e.renderfx & RF_DISINTEGRATE2 ) {
		info[8] = (float)DEFORM_DISINTEGRATION;
		return;
	}

	if ( tess.shader->numDeforms && !ShaderRequiresCPUDeforms( tess.shader ) ) {
		// only support the first one
		deformStage_t *ds = tess.shader->deforms[ 0 ];

		switch ( ds->deformation ) {
			case DEFORM_WAVE:
				type = DEFORM_WAVE;
				waveFunc = ds->deformationWave.func;

				info[0] = ds->deformationWave.base;
				info[1] = ds->deformationWave.amplitude;
				info[2] = ds->deformationWave.phase;
				info[3] = ds->deformationWave.frequency;
				info[4] = ds->deformationSpread;
				info[5] = 0.0f;
				info[6] = 0.0f;
				break;
			case DEFORM_BULGE:
				type = DEFORM_BULGE;

				info[0] = 0.0f;
				info[1] = ds->bulgeHeight; // amplitude
				info[2] = ds->bulgeWidth;  // phase
				info[3] = ds->bulgeSpeed;  // frequency
				info[4] = 0.0f;
				info[5] = 0.0f;
				info[6] = 0.0f;

				if ( ds->bulgeSpeed == 0.0f && ds->bulgeWidth == 0.0f )
					type = DEFORM_BULGE_UNIFORM;

				break;
			case DEFORM_MOVE:
				type = DEFORM_MOVE;
				waveFunc = ds->deformationWave.func;

				info[0] = ds->deformationWave.base;
				info[1] = ds->deformationWave.amplitude;
				info[2] = ds->deformationWave.phase;
				info[3] = ds->deformationWave.frequency;
				info[4] = ds->moveVector[0];
				info[5] = ds->moveVector[1];
				info[6] = ds->moveVector[2];
				break;
			case DEFORM_NORMALS:
				type = DEFORM_NORMALS;

				info[0] = 0.0f;
				info[1] = ds->deformationWave.amplitude; // amplitude
				info[2] = 0.0f;  // phase
				info[3] = ds->deformationWave.frequency;  // frequency
				info[4] = 0.0f;
				info[5] = 0.0f;
				info[6] = 0.0f;
				break;
			case DEFORM_PROJECTION_SHADOW:
				type = DEFORM_PROJECTION_SHADOW;

				info[0] = backEnd.ori.axis[0][2];
				info[1] = backEnd.ori.axis[1][2];
				info[2] = backEnd.ori.axis[2][2];
				info[3] = backEnd.ori.origin[2] - backEnd.currentEntity->e.shadowPlane;

				vec3_t lightDir;
				VectorCopy( backEnd.currentEntity->modelLightDir, lightDir );
				lightDir[2] = 0.0f;
				VectorNormalize( lightDir );
				VectorSet( lightDir, lightDir[0] * 0.3f, lightDir[1] * 0.3f, 1.0f );

				info[4] = lightDir[0];
				info[5] = lightDir[1];
				info[6] = lightDir[2];
				break;
			default:
				break;
		}
	}

	if ( type != DEFORM_NONE ) {
		info[7] = tess.shaderTime;
		info[8] = (float)type;
		info[9] = (float)waveFunc;	
	}
}

static void vk_compute_disintegration( int *forceRGBGen ){
	float	*info;

	if ( backEnd.currentEntity->e.renderfx & RF_DISINTEGRATE1 )
		*forceRGBGen = (int)CGEN_DISINTEGRATION_1;
	else
		*forceRGBGen = (int)CGEN_DISINTEGRATION_2;

	info = (float*)uniform_data.disintegrationInfo;

	info[0] = backEnd.currentEntity->e.oldorigin[0];
	info[1] = backEnd.currentEntity->e.oldorigin[1];
	info[2] = backEnd.currentEntity->e.oldorigin[2];
	info[3] = ( backEnd.refdef.time - backEnd.currentEntity->e.endTime ) * 0.045f;
	info[3] *= info[3];
}
#endif

#ifdef USE_PMLIGHT
void vk_lighting_pass( void )
{
	static uint32_t uniform_offset;
	static int fog_stage;
	uint32_t pipeline;
	const shaderStage_t *pStage;
	cullType_t cull;
	int abs_light;

	if (tess.shader->lightingStage < 0)
		return;

	pStage = tess.xstages[tess.shader->lightingStage];

	// we may need to update programs for fog transitions
	if (tess.dlightUpdateParams) {
		vk_set_fog_params(&uniform, &fog_stage);
		vk_set_light_params(&uniform, tess.light);

		uniform_offset = vk_push_uniform(&uniform);

		tess.dlightUpdateParams = qfalse;
	}

	if (uniform_offset == ~0)
		return; // no space left...

	cull = tess.shader->cullType;
	if (backEnd.viewParms.portalView == PV_MIRROR) {
		switch (cull) {
		case CT_FRONT_SIDED: cull = CT_BACK_SIDED; break;
		case CT_BACK_SIDED: cull = CT_FRONT_SIDED; break;
		default: break;
		}
	}

	abs_light = /* (pStage->stateBits & GLS_ATEST_BITS) && */ (cull == CT_TWO_SIDED) ? 1 : 0;

	if (fog_stage)
		vk_update_descriptor(3, tr.fogImage->descriptor_set);

	if (tess.light->linear)
		pipeline = vk.std_pipeline.dlight1_pipelines_x[cull][tess.shader->polygonOffset][fog_stage][abs_light];
	else
		pipeline = vk.std_pipeline.dlight_pipelines_x[cull][tess.shader->polygonOffset][fog_stage][abs_light];

	vk_select_texture(0);
	R_BindAnimatedImage(&pStage->bundle[tess.shader->lightingBundle]);

#ifdef USE_VBO
	if (tess.vboIndex == 0)
#endif
	{
		ComputeTexCoords(tess.shader->lightingBundle, &pStage->bundle[tess.shader->lightingBundle]);
	}
	
	vk_bind_pipeline(pipeline);
	vk_bind_index();
	vk_bind_lighting(tess.shader->lightingStage, tess.shader->lightingBundle);
	vk_draw_geometry(tess.depthRange, qtrue);
}
#endif // USE_PMLIGHT

void ForceAlpha(unsigned char *dstColors, int TR_ForceEntAlpha)
{
	int	i;

	dstColors += 3;

	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		*dstColors = TR_ForceEntAlpha;
	}
}

static qboolean vk_is_valid_pbr_surface( const qboolean hasPBR, const qboolean is_ghoul2_vbo, const qboolean is_mdv_vbo ) {
	if ( !vk.pbrActive || !hasPBR )
		return qfalse;
	
	if ( backEnd.isGlowPass )
		return qfalse;
			
	if ( backEnd.projection2D )
		return qfalse;

	if ( backEnd.viewParms.portalView == PV_MIRROR)
		return qfalse;

	if ( backEnd.currentEntity ){
		if ( backEnd.currentEntity != &tr.worldEntity && !is_ghoul2_vbo && !is_mdv_vbo )
			return qfalse;
	}
	
	return qtrue;
}

static ss_input ssInput;
void RB_StageIteratorGeneric( void )
{
	const shaderStage_t		*pStage;
	Vk_Pipeline_Def			def;
	uint32_t				stage = 0;
	uint32_t				pipeline;
	int						tess_flags, i;
	int						fog_stage = 0;
	qboolean				fogCollapse;
	qboolean				is_ghoul2_vbo;
	qboolean				is_mdv_vbo;
	qboolean				is_pbr_surface;

#ifdef USE_VBO
	if (tess.vboIndex != 0) {
		VBO_PrepareQueues();
		tess.vboStage = 0;
	} 
	else
#endif
	{
		RB_DeformTessGeometry();
	}

#ifdef USE_PMLIGHT
	if (tess.dlightPass) {
		vk_lighting_pass();
		return;
	}
#endif

	vk_bind_index();

	tess_flags = tess.shader->tessFlags;

	fogCollapse = qfalse;
	is_ghoul2_vbo = qfalse;
	is_mdv_vbo = qfalse;
	is_pbr_surface = qfalse;

#ifdef USE_FOG_COLLAPSE
	if ( tess.fogNum && tess.shader->fogPass && tess.shader->fogCollapse && r_drawfog->value == 2 ) {
		fogCollapse = qtrue;
	}
#endif

#if defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV) 
	if ( tess.vboIndex ) {
		is_ghoul2_vbo = (qboolean)( tess.surfType == SF_MDX );
		is_mdv_vbo = (qboolean)( tess.surfType == SF_VBO_MDVMESH );
	}

	if ( is_ghoul2_vbo || is_mdv_vbo ) {
		trRefEntity_t *refEntity = backEnd.currentEntity;
		orientationr_t ori;
		vec4_t tmp;

		Com_Memset( &uniform_data, 0, sizeof(uniform_data) );

		VectorScale( refEntity->ambientLight, 1.0f / 255.0, tmp );
		Com_Memcpy( &uniform_data.ambientLight, tmp, sizeof(vec4_t) );

		VectorScale( refEntity->directedLight, 1.0f / 255.0, tmp );
		Com_Memcpy( &uniform_data.directedLight, tmp, sizeof(vec4_t) );

		VectorCopy( refEntity->lightDir, tmp ); tmp[3] = 0.0f;
		Com_Memcpy( &uniform_data.lightDir, tmp, sizeof(vec4_t) );
		
		if ( refEntity == &tr.worldEntity ) {
			ori = backEnd.viewParms.world;
			Matrix16Identity( uniform_camera.modelMatrix );
		}else{
			R_RotateForEntity( refEntity, &backEnd.viewParms, &ori );
			Matrix16Copy(ori.modelMatrix, uniform_camera.modelMatrix );
		}

		Com_Memcpy( &uniform_camera.localViewOrigin, ori.viewOrigin, sizeof( vec3_t) );
		uniform_camera.localViewOrigin[3] = 0.0;

		if ( is_ghoul2_vbo )
			vk_push_uniform_ghoul2( &uniform_ghoul ); // bone matrices

		vk_compute_deform();	
	}
#endif

#ifdef USE_VK_PBR
	is_pbr_surface = vk_is_valid_pbr_surface( tess.shader->hasPBR, is_ghoul2_vbo, is_mdv_vbo );

	if ( is_pbr_surface && !is_ghoul2_vbo && !is_mdv_vbo ) {
		Com_Memcpy( &uniform_camera.modelMatrix, backEnd.ori.modelMatrix, sizeof(float) * 16 );
		Com_Memcpy( &uniform_camera.viewOrigin, backEnd.refdef.vieworg, sizeof( vec3_t) );
		uniform_camera.viewOrigin[3] = 0.0;
	}
#endif
	if ( is_pbr_surface || is_ghoul2_vbo || is_mdv_vbo )
		vk_push_uniform_camera( &uniform_camera );

	if ( fogCollapse ) {
		vk_set_fog_params( &uniform, &fog_stage );
		VectorCopy( backEnd.ori.viewOrigin, uniform.eyePos );
		vk_push_uniform( &uniform );
		vk_update_descriptor( 5, tr.fogImage->descriptor_set );
	}
	else {
		fog_stage = 0;
		if ( tess_flags & TESS_VPOS ) {
			VectorCopy( backEnd.ori.viewOrigin, uniform.eyePos );
			vk_push_uniform( &uniform );
			tess_flags &= ~TESS_VPOS;
		}
	}

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		int forceRGBGen = 0;

		pStage = tess.xstages[stage];

		if ( !pStage || !pStage->active )
			break;

#ifdef USE_VBO
		tess.vboStage = stage;
#endif

		// we check for surfacesprites AFTER drawing everything else
		if ( pStage->ss && pStage->ss->surfaceSpriteType )
			continue;

		// vertexLightmap isnt used rn
		if ( stage && r_lightmap->integer && !( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap ) )
			break;

		if ( backEnd.currentEntity ) {
			assert( backEnd.currentEntity->e.renderfx >= 0 );
#if defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV) 
			if ( is_ghoul2_vbo && backEnd.currentEntity->e.renderfx & ( RF_DISINTEGRATE1 | RF_DISINTEGRATE2 ) )
				vk_compute_disintegration( &forceRGBGen );

			//want to use RGBGen from ent
			else if ( backEnd.currentEntity->e.renderfx & RF_RGB_TINT )
				forceRGBGen = CGEN_ENTITY;
#else
			if ( backEnd.currentEntity->e.renderfx & RF_RGB_TINT )
				forceRGBGen = CGEN_ENTITY;
#endif
		}

		tess_flags |= pStage->tessFlags;

		for (i = 0; i < pStage->numTexBundles; i++) {
			if (pStage->bundle[i].image[0] != NULL) {
				vk_select_texture(i);
				R_BindAnimatedImage(&pStage->bundle[i]);
#if defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV) 
				if ( tess_flags & (TESS_RGBA0 << i) && ( is_ghoul2_vbo || is_mdv_vbo ) ) {
					vk_compute_colors( i, pStage, forceRGBGen );
					continue;
				}
#endif
				if (tess_flags & (TESS_ST0 << i))
					ComputeTexCoords(i, &pStage->bundle[i]);

				if (tess_flags & (TESS_RGBA0 << i))
					ComputeColors(i, tess.svars.colors[i], pStage, forceRGBGen);
			}
		}
	
		// reject this stage if it's not a glow stage but we are doing a glow pass.
		if ( backEnd.isGlowPass && !pStage->glow )
			continue;

		vk_select_texture( 0 );

		// for 2D flipped images
		if ( backEnd.projection2D ) {
			if ( pStage->vk_2d_pipeline == NULL ) {
				vk_get_pipeline_def(pStage->vk_pipeline[0], &def);

				// use an excisting pipeline with the same def or create a new one.
				def.face_culling = CT_TWO_SIDED;
				tess.xstages[stage]->vk_2d_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
			}

			pipeline = pStage->vk_2d_pipeline;
		}
		else if ( backEnd.currentEntity ) {
			if ( backEnd.viewParms.portalView == PV_MIRROR )
				vk_get_pipeline_def(pStage->vk_mirror_pipeline[fog_stage], &def);
			else{
#if defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV) 
				if( tess_flags & TESS_PBR && ( is_ghoul2_vbo || is_mdv_vbo ) )
					vk_get_pipeline_def(pStage->vk_pbr_pipeline[fog_stage], &def);
				else
#endif
					vk_get_pipeline_def(pStage->vk_pipeline[fog_stage], &def);
			}
			// we want to be able to rip a hole in the thing being disintegrated,
			// and by doing the depth-testing it avoids some kinds of artefacts, but will probably introduce others?
			if ( backEnd.currentEntity->e.renderfx & RF_DISINTEGRATE1 )
				def.state_bits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_C0;

			if( backEnd.currentEntity->e.renderfx & RF_FORCE_ENT_ALPHA ) {
				ForceAlpha( (unsigned char *) tess.svars.colors, backEnd.currentEntity->e.shaderRGBA[3] );
				
				def.state_bits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				// depth write, so faces through the model will be stomped over by nearer ones. this works because
				// we draw RF_FORCE_ENT_ALPHA stuff after everything else, including standard alpha surfs.
				if ( backEnd.currentEntity->e.renderfx & RF_ALPHA_DEPTH ) 
					def.state_bits |= GLS_DEPTHMASK_TRUE;	
			}
#if defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV) 
			if ( is_ghoul2_vbo ){
				def.vbo_ghoul2 = qtrue;

#ifdef USE_VBO_GHOUL2_RGBAGEN_CONSTS
				for ( i = 0; i < pStage->numTexBundles; i++ ) {
					if ( backEnd.currentEntity->e.renderfx & ( RF_DISINTEGRATE1 | RF_DISINTEGRATE2 ) ||
						 (int)uniform_data.rgbGen[i] ==		CGEN_LIGHTING_DIFFUSE || 
						 (int)uniform_data.alphaGen[i] ==	AGEN_LIGHTING_SPECULAR ||
						 (int)uniform_data.alphaGen[i] ==	AGEN_PORTAL )
					{
						def.rgbaGen[i] = 1;
					}
				}
#endif
			} 
			else if ( is_mdv_vbo ) {
				def.vbo_mdv = qtrue;
			}
#endif			
			pipeline = vk_find_pipeline_ext( 0, &def, qfalse );
		}
		else {
			if ( backEnd.viewParms.portalView == PV_MIRROR )
				pipeline = pStage->vk_mirror_pipeline[fog_stage];
			else
				pipeline = pStage->vk_pipeline[fog_stage];
		}

		if ( r_lightmap->integer && pStage->bundle[1].isLightmap ) {
			//vk_select_texture(0);
			vk_bind( tr.whiteImage ); // replace diffuse texture with a white one thus effectively render only lightmap
		}

		// move glow bundle to texture 0
		// does not work with multitextured dglow yet.
		if ( backEnd.isGlowPass && pStage->glow ){
			vk_get_pipeline_def( pStage->vk_pipeline[fog_stage], &def );
#if defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV) 		
			if ( is_ghoul2_vbo )
				def.vbo_ghoul2 = qtrue;
			else if ( is_mdv_vbo )
				def.vbo_mdv = qtrue;		

#endif	
			def.shader_type = TYPE_SINGLE_TEXTURE;
			pipeline = vk_find_pipeline_ext( 0, &def, qfalse );

			vk_select_texture( 0 );
			vk_bind( pStage->bundle[ ( pStage->numTexBundles - 1 ) ].image[0] );
			Com_Memcpy( tess.svars.colors[0], tess.svars.colors[( pStage->numTexBundles - 1 )], sizeof(tess.svars.colors[0]) );
		}

		// PBR
#ifdef USE_VK_PBR
		// todo:
		// use the bundles, and proper collapsing
		// - assign cubemap index per surface
		// - current situation will do for development purposes
		//
		if ( is_pbr_surface && pStage->vk_pbr_flags ) {
			vk_update_pbr_descriptor(6, vk.brdflut_image_descriptor);
				
			if ( pStage->vk_pbr_flags & PBR_HAS_NORMALMAP )
				vk_update_pbr_descriptor(7, pStage->normalMap->descriptor_set);

			if ( pStage->vk_pbr_flags & PBR_HAS_PHYSICALMAP )
				vk_update_pbr_descriptor(8, pStage->physicalMap->descriptor_set);

			if ( !tr.numCubemaps || backEnd.viewParms.targetCube != nullptr ) {
				vk_update_pbr_descriptor(9, tr.emptyCubemap->descriptor_set);
				//vk_update_pbr_descriptor(10, tr.emptyCubemap->descriptor_set);
			}
			// currently use the frist cubemap for every surface
			// use the first cubemap index, indexes are not assigned per surface yet
			else { 
				vk_update_pbr_descriptor(9, tr.cubemaps[0].prefiltered_image->descriptor_set);
				//vk_update_pbr_descriptor(10, tr.cubemaps[0].irradiance_image->descriptor_set);
			}

			if ( !is_ghoul2_vbo && !is_mdv_vbo )
				pipeline = pStage->vk_pbr_pipeline[fog_stage];
		}
#endif

#if defined(USE_VBO_GHOUL2) || defined(USE_VBO_MDV) 
		if ( is_ghoul2_vbo || is_mdv_vbo )
			vk_push_uniform_data( &uniform_data );
#endif
		vk_bind_pipeline( pipeline );
		vk_bind_geometry( tess_flags );
		vk_draw_geometry( tess.depthRange, qtrue );

		if ( pStage->depthFragment ) {
			if ( backEnd.viewParms.portalView == PV_MIRROR )
				pipeline = pStage->vk_mirror_pipeline_df;
			else
				pipeline = pStage->vk_pipeline_df;

			vk_bind_pipeline( pipeline );
			vk_draw_geometry( tess.depthRange, qtrue );
		}

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap ) )
			break;

		tess_flags = 0;
	}

	if (tess_flags) // fog-only shaders?
		vk_bind_geometry(tess_flags);

	// now do fog
	if (tr.world && r_drawfog->value && tess.fogNum && tess.shader->fogPass && !fogCollapse) {
		RB_FogPass();
	}

	// Now check for surfacesprites.
	if ( r_surfaceSprites->integer && !vk.vboWorldActive )
	{
		qboolean ssFound = qfalse;

		for (stage = 1; stage < tess.shader->numUnfoggedPasses; stage++)
		{
			if (tess.xstages[stage]->ss && tess.xstages[stage]->ss->surfaceSpriteType)
			{
				if (!ssFound) {
					// don't cringe, this is a temporary solution. but slow..
					// we are still reading from tess.xyz while also writing a group of surfacesprites to it.
					// which means the next group will read from garbaged surface data.
					// we duplicate the necessary tess data to ssInput and use that to read from.
					// yeah ..
					// surfacesprites currently don't work with vbo enabled.
					// need to look at the the methods from OpenJK repo

					ssInput.numIndexes = tess.numIndexes;
					ssInput.numVertexes = tess.numVertexes;

					memcpy(ssInput.indexes, tess.indexes, sizeof(tess.indexes));
					memcpy(ssInput.xyz, tess.xyz, sizeof(tess.xyz));
					memcpy(ssInput.normal, tess.normal, sizeof(tess.normal));
					memcpy(ssInput.vertexColors, tess.vertexColors, sizeof(tess.vertexColors));

					ssFound = qtrue;
				}

				// Draw the surfacesprite
				RB_DrawSurfaceSprites(tess.xstages[stage], &ssInput);
			}
		}
	}
}