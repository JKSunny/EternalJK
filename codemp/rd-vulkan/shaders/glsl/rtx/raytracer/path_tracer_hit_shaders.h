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

void get_model_index_and_prim_offset(int instanceID, int geometryIndex, out int model_index, out uint prim_offset )
{
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

	get_model_index_and_prim_offset(instanceID, geometryIndex, model_index, prim_offset);

	ray_payload.barycentric = bary.xy;
	//ray_payload.primitive_id = primitiveID;
	ray_payload.primitive_id = primitiveID + prim_offset;
	ray_payload.buffer_and_instance_idx = (int(instanceCustomIndex) & 0xffff)
	                                    | (model_index << 16);
	ray_payload.hit_distance = hitT;
}

#ifndef USE_SIMPLE
bool pt_logic_masked(int primitiveID, int instanceID, int geometryIndex, uint instanceCustomIndex, vec2 bary)
{
	int model_index;
	uint prim_offset;
	get_model_index_and_prim_offset(instanceID, geometryIndex, model_index, prim_offset);

	uint prim = primitiveID + prim_offset;
	uint buffer_idx = instanceCustomIndex;

	Triangle triangle = load_and_transform_triangle(model_index, buffer_idx, prim);

	MaterialInfo minfo = get_material_info(triangle.material_id);

	if (minfo.mask_texture == 0)
		return true;
	
	vec2 tex_coord = triangle.tex_coords0 * vec3(1.0 - bary.x - bary.y, bary.x, bary.y);

	//perturb_tex_coord(triangle.material_id, global_ubo.time, tex_coord);	

	vec4 mask_value = global_textureLod(minfo.mask_texture, tex_coord, /* mip_level = */ 0);

	return mask_value.x >= 0.5;
}
#endif