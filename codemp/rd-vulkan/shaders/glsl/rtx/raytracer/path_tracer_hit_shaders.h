/*
Copyright (C) 2020-2021, NVIDIA CORPORATION. All rights reserved.
Copyright (C) 2021 Frank Richter

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define VERTEX_BUFFER_FIRST_MODEL 5

void get_model_index_and_prim_offset(int instanceID, int geometryIndex, out int model_index, out uint prim_offset, out uint type /* deprecated soon */ )
{
	
	// type deprecated soon!
	//type = instance_buffer.tlas_instance_type[instanceID]; // type of vtx/idx buffer to look for opaque/material/data

	model_index = instance_buffer.tlas_instance_model_indices[instanceID];
	if (model_index >= 0)
	{
		model_index += geometryIndex;
		prim_offset = instance_buffer.model_instances[model_index].render_prim_offset;
	}
	else
	{
		prim_offset = instance_buffer.tlas_instance_prim_offsets[instanceID];
	}
}

void pt_logic_rchit(inout RayPayloadGeometry ray_payload, int primitiveID, int instanceID, int geometryIndex, uint instanceCustomIndex, float hitT, vec2 bary)
{
	int model_index;
	uint prim_offset;
	uint type;

	get_model_index_and_prim_offset(instanceID, geometryIndex, model_index, prim_offset, type);

	ray_payload.barycentric = bary.xy;
	//ray_payload.primitive_id = primitiveID + prim_offset;
	ray_payload.primitive_id = primitiveID + prim_offset;
	ray_payload.buffer_and_instance_idx = (int(instanceCustomIndex) & 0xffff)
	                                    | (model_index << 16);
	ray_payload.hit_distance = hitT;
	ray_payload.type = type;
}