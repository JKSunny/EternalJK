#include "tr_local.h"
#include <vector>

static void vk_destroy_lightgrid_ssbo( vk_storage_buffer_t *buffer )
{
	if ( buffer->memory && buffer->buffer_ptr )
		qvkUnmapMemory( vk.device, buffer->memory );

	if ( buffer->buffer )
		VK_DESTROY_BUFFER( vk.device, buffer->buffer );

	if ( buffer->memory )
		VK_FREE_MEMORY( vk.device, buffer->memory );

	if ( buffer->descriptor )
		qvkFreeDescriptorSets(vk.device, vk.descriptor_pool, 1, &buffer->descriptor);

	Com_Memset( buffer, 0, sizeof( *buffer ) );
}

void vk_create_lightgrid_ssbo_descriptor( vk_storage_buffer_t *buffer )
{
	VkDescriptorSetAllocateInfo alloc;
	VkDescriptorBufferInfo info;
	VkWriteDescriptorSet desc;

    // patches
	alloc.sType                 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc.pNext                 = NULL;
	alloc.descriptorPool        = vk.descriptor_pool;
	alloc.descriptorSetCount    = 1;
	alloc.pSetLayouts           = &vk.set_layout_storage_static;
	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &buffer->descriptor ) );

	info.buffer = vk.lightgrid.ssbo.buffer;
	info.offset = 0;
	info.range	= sizeof(vkLightGridSample_t) * vk.lightgrid.numSamples; // VK_WHOLE_SIZE;

	desc.sType                  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.pNext                  = NULL;
	desc.dstSet                 = buffer->descriptor;
	desc.dstBinding             = 0;
	desc.dstArrayElement        = 0;
	desc.descriptorCount        = 1;
	desc.descriptorType         = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc.pImageInfo             = NULL;
	desc.pBufferInfo            = &info;
	desc.pTexelBufferView       = NULL;
	qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );
	VK_SET_OBJECT_NAME( buffer->descriptor, va("world lightgrid"), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT );
}

static uint32_t vk_add_lightgrid_indirect_cmd( void )
{
	uint32_t offset;
	VkDrawIndirectCommand *cmd = vk_reserve_draw_indirect( 1, &offset );
    cmd[0].vertexCount = 36;
    cmd[0].instanceCount = vk.lightgrid.numSamples;
    cmd[0].firstVertex = 0;
    cmd[0].firstInstance = 0;

	return offset;
}

void vk_render_lightgrid( void )
{
    if ( !tr.world || !vk.lightgrid.numSamples || vk.renderPassIndex )
        return;

    uint32_t i, indirect_offset;
    vkUniform_t uniform;

	indirect_offset = vk_add_lightgrid_indirect_cmd();

	// re-use these bits
	VectorCopy( vk.lightgrid.origin, uniform.eyePos );
	VectorCopy( vk.lightgrid.size, uniform.lightPos );
	uniform.lightColor[0] = vk.lightgrid.bounds[0];
	uniform.lightColor[1] = vk.lightgrid.bounds[1];
	uniform.lightColor[2] = vk.lightgrid.bounds[2];

	float tmp[16];

	VkPipeline vkpipe;
	vkpipe = vk_gen_pipeline(vk.lightgrid.pipeline);
    qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipe );

	Com_Memcpy(tmp, vk_world.modelview_transform, 64);
	Com_Memcpy(vk_world.modelview_transform, backEnd.viewParms.world.modelViewMatrix, 64);
	vk_update_mvp(NULL);

    uint32_t offsets[VK_DESC_UNIFORM_COUNT], offset_count;
    offset_count = 0;
	offsets[offset_count++] = vk_push_uniform( &uniform );
	offsets[offset_count++] = vk.cmd->camera_ubo_offset;
	offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_LIGHT_BINDING];
	offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_ENTITY_BINDING];
	offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_BONES_BINDING];
	offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_FOGS_BINDING];
	offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_GLOBAL_BINDING];

    const VkDescriptorSet sets[2] = {
		vk.cmd->uniform_descriptor,
		vk.lightgrid.ssbo.descriptor
	};

    qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.lightgrid.pipeline_layout, 0, ARRAY_LEN(sets), sets, offset_count, offsets );
	qvkCmdDrawIndirect( vk.cmd->command_buffer, vk.cmd->indirect_buffer, indirect_offset, 1, sizeof(VkDrawIndirectCommand) );

	Com_Memcpy(vk_world.modelview_transform, tmp, 64);
	vk_update_mvp(NULL);

	if ( vk.cmd->last_pipeline != VK_NULL_HANDLE )
	{
		// restore last pipeline
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.cmd->last_pipeline );

		uint32_t offsets[VK_DESC_UNIFORM_COUNT], offset_count;

		// restore clobbered descriptor sets
		for ( i = 0; i < 2; i++ ) {
			if ( vk.cmd->descriptor_set.current[i] != VK_NULL_HANDLE ) {
				if ( /*i == VK_DESC_STORAGE ||*/ i == VK_DESC_UNIFORM ) {
					offset_count = 0;

					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_MAIN_BINDING];

					// not required for dot storage flare test, chances are slim thats the previous pipeline.
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_CAMERA_BINDING];
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_LIGHT_BINDING];
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_ENTITY_BINDING];
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_BONES_BINDING];
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_FOGS_BINDING];
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_GLOBAL_BINDING];

					qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, i, 1, &vk.cmd->descriptor_set.current[i], offset_count, offsets );
				}
				else
					qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, i, 1, &vk.cmd->descriptor_set.current[i], 0, NULL );
			}
		}
	}
}

void vk_build_gpu_lightgrid( const world_t &worldData, int index )
{
    vk.lightgrid.numSamples = 0;

	if ( index )
		return;

    if ( !worldData.numGridArrayElements || !worldData.lightGridArray || !worldData.lightGridData )
        return;

    vk.lightgrid.numSamples = worldData.numGridArrayElements;

    std::vector<vkLightGridSample_t> samples( vk.lightgrid.numSamples );

    for ( uint32_t i = 0; i < vk.lightgrid.numSamples; i++ )
    {
        const mgrid_t *data = worldData.lightGridData + worldData.lightGridArray[i];
        vkLightGridSample_t &sample = samples[i];

        Com_Memset( &sample, 0, sizeof( sample ) );

        if ( data->styles[0] == LS_LSNONE )
            continue;

        vec3_t direction;
        VectorClear( direction );

        for ( uint32_t j = 0; j < MAXLIGHTMAPS; j++ )
        {
            if ( data->styles[j] == LS_LSNONE )
                break;

            const byte style = data->styles[j];

            sample.ambientLight[0] += data->ambientLight[j][0] * styleColors[style][0] / 255.0f;
            sample.ambientLight[1] += data->ambientLight[j][1] * styleColors[style][1] / 255.0f;
            sample.ambientLight[2] += data->ambientLight[j][2] * styleColors[style][2] / 255.0f;

            sample.directedLight[0] += data->directLight[j][0] * styleColors[style][0] / 255.0f;
            sample.directedLight[1] += data->directLight[j][1] * styleColors[style][1] / 255.0f;
            sample.directedLight[2] += data->directLight[j][2] * styleColors[style][2] / 255.0f;
        }

        int lat = data->latLong[1] * (FUNCTABLE_SIZE / 256);
        int lng = data->latLong[0] * (FUNCTABLE_SIZE / 256);

        sample.lightDir[0] = tr.sinTable[(lat + FUNCTABLE_SIZE / 4) & FUNCTABLE_MASK] * tr.sinTable[lng];
        sample.lightDir[1] = tr.sinTable[lat] * tr.sinTable[lng];
        sample.lightDir[2] = tr.sinTable[(lng + FUNCTABLE_SIZE / 4) & FUNCTABLE_MASK];
        sample.lightDir[3] = 0.0f;
    }

    VectorCopy( worldData.lightGridOrigin, vk.lightgrid.origin );
    VectorCopy( worldData.lightGridSize, vk.lightgrid.size );
    Com_Memcpy( vk.lightgrid.bounds, worldData.lightGridBounds, sizeof(int) * 3 );

	vk_destroy_lightgrid_ssbo( &vk.lightgrid.ssbo );

    const VkDeviceSize size = sizeof( vkLightGridSample_t ) * vk.lightgrid.numSamples;
    vk_create_storage_buffer( &vk.lightgrid.ssbo, size, samples.data(), "lightgrid" );

	vk_create_lightgrid_ssbo_descriptor( &vk.lightgrid.ssbo );
}

void vk_clean_lightgrid( void )
{
	vk_destroy_lightgrid_ssbo( &vk.lightgrid.ssbo );
	vk.lightgrid.numSamples = 0;
}
