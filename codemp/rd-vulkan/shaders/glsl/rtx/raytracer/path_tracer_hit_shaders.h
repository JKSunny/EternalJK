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

	if (minfo.blend_mode == RTX_BLEND_ALPHA)
	{
		if (texel.a == 0.0)
			return true;
	}
	else if (minfo.blend_mode == RTX_BLEND_ADDITIVE)
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

TransparencyHit make_empty_hit()
{
    return TransparencyHit(vec4(0.0), RTX_BLEND_SKIP);
}

TransparencyHit pt_logic_sprite(int primitiveID, vec2 bary)
{
    const vec3 barycentric = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);

    vec2 uv;
    if ((primitiveID & 1) == 0)
        uv = vec2(1.0, 0.0) * barycentric.x + vec2(0.0, 0.0) * barycentric.y + vec2(0.0, 1.0) * barycentric.z;
    else
        uv = vec2(0.0, 1.0) * barycentric.x + vec2(1.0, 1.0) * barycentric.y + vec2(1.0, 0.0) * barycentric.z;

    const int sprite_index = primitiveID / 2;

    uvec4 info = texelFetch(sprite_texure_buffer, sprite_index);
    MaterialInfo minfo = get_material_info(info.x);

    if (minfo.base_texture == 0)
        return make_empty_hit();

    vec4 shaderRGBA = unpack_rgba8(info.y);

    vec4 color = global_textureLod(minfo.base_texture, uv, 0);

    // entity tint
    color.rgb *= shaderRGBA.rgb;

    if (shaderRGBA.a > 0.0)
    {
        color.rgb *= shaderRGBA.a;
        color.a   *= shaderRGBA.a;
    }

    // alpha test uses texture alpha (unchanged)
    bool pass = true;
    switch (minfo.alpha_test_func)
    {
        case 1u: pass = color.a > 0.0; break;
        case 2u: pass = color.a < minfo.alpha_test_value; break;
        case 3u: pass = color.a >= minfo.alpha_test_value; break;
    }

    if (!pass)
        return make_empty_hit();

    switch (minfo.blend_mode)
    {
        case RTX_BLEND_ALPHA:
        {
            if (color.a <= 0.0)
                return make_empty_hit();

            // correct premultiplied alpha
            color.rgb *= color.a;
            break;
        }

        case RTX_BLEND_ADDITIVE:
        {
            vec3 c = color.rgb;

            float lum = luminance(c);
            if (lum > 0.0)
            {
                float lum2 = pow(lum, 2.2);

                // reshape energy distribution (preserves hue but adjusts intensity curve)
                c *= (lum2 / lum);

                // entity/texture alpha already folded into color.rgb earlier
                c *= global_ubo.prev_adapted_luminance * 2000.0;
            }

            color.rgb = c;

            // additive has no coverage
            color.a = 0.0;

            break;
        }
    }

    return TransparencyHit(color, minfo.blend_mode);
}