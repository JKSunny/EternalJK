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

layout(set = 0, binding = 1)
uniform textureBuffer particle_color_buffer;

layout(set = 0, binding = 2)
uniform textureBuffer beam_color_buffer;

layout(set = 0, binding = 3)
uniform utextureBuffer sprite_texure_buffer;

layout(set = 0, binding = 4)
uniform utextureBuffer beam_info_buffer;

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

bool pt_logic_masked(int primitiveID, int instanceID, int geometryIndex, uint instanceCustomIndex, vec2 bary)
{
	int model_index;
	uint prim_offset;
	get_model_index_and_prim_offset(instanceID, geometryIndex, model_index, prim_offset);

	uint prim = primitiveID + prim_offset;
	uint buffer_idx = instanceCustomIndex;

	Triangle triangle = load_and_transform_triangle(model_index, buffer_idx, prim);

	MaterialInfo minfo = get_material_info(triangle.material_id);

	if (minfo.base_texture == 0)
		return true;

	if (minfo.alpha_test_func == 0u)
		return true;

	vec2 tex_coord = triangle.tex_coords0 * vec3(1.0 - bary.x - bary.y, bary.x, bary.y);
#if 0
	perturb_tex_coord(triangle.material_id, global_ubo.time, tex_coord);	
	vec4 mask_value = global_textureLod(minfo.base_texture, tex_coord, /* mip_level = */ 0);
	return mask_value.a >= 0.5;
#endif
	vec4 texel = global_textureLod(minfo.base_texture, tex_coord, 0);

	if (minfo.discard_mode == 1u)
	{
		if (texel.a == 0.0)
			return true;
	}
	else if (minfo.discard_mode == 2u)
	{
		if (dot(texel.rgb, texel.rgb) == 0.0)
			return true;
	}

	switch (minfo.alpha_test_func)
	{
		// GLS_ATEST_GT_0
		case 1u:
			return texel.a > 0.0;

		// GLS_ATEST_LT_80
		case 2u:
			return texel.a < minfo.alpha_test_value;

		// GLS_ATEST_GE_80 / GLS_ATEST_GE_C0
		case 3u:
			return texel.a >= minfo.alpha_test_value;
	}

	return true;

}

vec4 unpack_rgba8(uint p)
{
    return vec4(
        float((p >>  0) & 255u),
        float((p >>  8) & 255u),
        float((p >> 16) & 255u),
        float((p >> 24) & 255u)
    ) * (1.0 / 255.0);
}

vec4 pt_logic_sprite(int primitiveID, vec2 bary)
{
	const vec3 barycentric = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);
	vec4 shaderRGBA = vec4(0.0);

	vec2 uv;
	if((primitiveID & 1) == 0)
		uv = vec2(1.0, 0.0) * barycentric.x + vec2(0.0, 0.0) * barycentric.y + vec2(0.0, 1.0) * barycentric.z;
	else
		uv = vec2(0.0, 1.0) * barycentric.x + vec2(1.0, 1.0) * barycentric.y + vec2(1.0, 0.0) * barycentric.z;

	const int sprite_index = primitiveID / 2;

	uvec4 info = texelFetch(sprite_texure_buffer, sprite_index);

	MaterialInfo minfo = get_material_info( info.x );

	if (minfo.base_texture == 0)
		return vec4(0.0);

	shaderRGBA = unpack_rgba8(info.y);

	vec4 color = global_textureLod(minfo.base_texture, uv, 0);
	color.rgb *= shaderRGBA.rgb;
	if (shaderRGBA.a > 0.0) {
		color.rgb *= shaderRGBA.a;
		color.a *= shaderRGBA.a;
	}
	
	if (minfo.discard_mode == 1u)
	{
		if (color.a == 0.0)
			return vec4(0.0);
	}
	
	if (minfo.discard_mode == 2u)
	{
		if (dot(color.rgb, color.rgb) == 0.0)
			return vec4(0.0);

		color.a = 1e-6;
	}else {
		color.rgb *= color.a;
	}

	bool pass = true;

	switch (minfo.alpha_test_func)
	{
		case 1u: pass = color.a > 0.0; break;
		case 2u: pass = color.a < minfo.alpha_test_value; break;
		case 3u: pass = color.a >= minfo.alpha_test_value; break;
	}

	if (!pass)
		return vec4(0.0);
#if 0
	float lum = luminance(color.rgb);
	if(lum > 0)
	{
		float lum2 = pow(lum, 2.2);
		color.rgb = color.rgb * (lum2 / lum) * color.a * alpha;
	
		color.rgb *= global_ubo.prev_adapted_luminance * 2000;
	}
#endif

	return color;
}