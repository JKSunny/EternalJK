/*
Copyright (C) 2018 Christoph Schied
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

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

#include "path_tracer.h"	// contains global_ubo.h -> constants.h
#include "utils.glsl"

#include "global_textures.h"

#define VERTEX_READONLY 1
#include "vertex_buffer.h"
layout( set = 0, binding = BINDING_OFFSET_AS ) uniform accelerationStructureEXT topLevelAS;

#include "read_visbuf.glsl"
#include "compute/asvgf.glsl"
#include "brdf.glsl"
#include "water.glsl"

#define DESATURATE_ENVIRONMENT_MAP 1

/* RNG seeds contain 'X' and 'Y' values that are computed w/ a modulo BLUE_NOISE_RES,
 * so the shift values can be chosen to fit BLUE_NOISE_RES - 1
 * (see generate_rng_seed()) */
#define RNG_SEED_SHIFT_X        0u
#define RNG_SEED_SHIFT_Y        8u
#define RNG_SEED_SHIFT_ISODD    16u
#define RNG_SEED_SHIFT_FRAME    17u

#define RNG_PRIMARY_OFF_X			0
#define RNG_PRIMARY_OFF_Y			1
#define RNG_PRIMARY_APERTURE_X		2
#define RNG_PRIMARY_APERTURE_Y		3

#define RNG_NEE_LIGHT_SELECTION(bounce)   (4 + 0 + 9 * bounce)
#define RNG_NEE_TRI_X(bounce)             (4 + 1 + 9 * bounce)
#define RNG_NEE_TRI_Y(bounce)             (4 + 2 + 9 * bounce)
#define RNG_NEE_LIGHT_TYPE(bounce)        (4 + 3 + 9 * bounce)
#define RNG_BRDF_X(bounce)                (4 + 4 + 9 * bounce)
#define RNG_BRDF_Y(bounce)                (4 + 5 + 9 * bounce)
#define RNG_BRDF_FRESNEL(bounce)          (4 + 6 + 9 * bounce)
#define RNG_SUNLIGHT_X(bounce)			  (4 + 7 + 9 * bounce)
#define RNG_SUNLIGHT_Y(bounce)			  (4 + 8 + 9 * bounce)

#define BOUNCE_SPECULAR 1

#define MAX_OUTPUT_VALUE 1000

#define PAYLOAD_SHADOW 0
#define PAYLOAD_BRDF 1

layout(location = PAYLOAD_SHADOW) rayPayloadEXT RayPayloadShadow ray_payload_shadow;
layout(location = PAYLOAD_BRDF) rayPayloadEXT RayPayload ray_payload_brdf;

uint rng_seed;

struct Ray {
	vec3 origin, direction;
	float t_min, t_max;
};

vec3
env_map( vec3 direction, bool remove_sun )
{
	direction = ( global_ubo.environment_rotation_matrix * vec4(direction, 0) ).xyz;

    vec3 envmap = vec3( 0 );
    if ( global_ubo.environment_type == ENVIRONMENT_DYNAMIC )
    {
	    envmap = textureLod(TEX_PHYSICAL_SKY, direction.xzy, 0).rgb;

	    if ( remove_sun )
	    {
			// roughly remove the sun from the env map
			envmap = min( envmap, vec3( ( 1 - dot( direction, global_ubo.sun_direction_envmap ) ) * 200 ) );
		}
	}
    else if ( global_ubo.environment_type == ENVIRONMENT_STATIC )
    {
        envmap = textureLod( TEX_ENVMAP, direction.xzy, 0 ).rgb;
#if DESATURATE_ENVIRONMENT_MAP
        float avg = ( envmap.x + envmap.y + envmap.z ) / 3.0;
        envmap = mix( envmap, avg.xxx, 0.1 ) * 0.5;
#endif
    }

	return envmap;
}

#include "light_lists.h"

ivec2 get_image_position()
{
	ivec2 pos;
	bool is_even_checkerboard = gl_LaunchIDEXT.z == 0;

	if ( global_ubo.pt_swap_checkerboard != 0 )
		is_even_checkerboard = !is_even_checkerboard;

	if ( is_even_checkerboard )
		pos.x = int(gl_LaunchIDEXT.x * 2) + int(gl_LaunchIDEXT.y & 1);
	else
		pos.x = int(gl_LaunchIDEXT.x * 2 + 1) - int(gl_LaunchIDEXT.y & 1);

	pos.y = int(gl_LaunchIDEXT.y);
	return pos;
}

ivec2 get_image_size()
{
	return ivec2(global_ubo.width, global_ubo.height);
}

bool
is_dynamic_instance(RayPayload pay_load)
{
	return (pay_load.instance_prim & INSTANCE_DYNAMIC_FLAG) > 0;
}

uint
get_primitive(RayPayload pay_load)
{
	return pay_load.instance_prim & PRIM_ID_MASK;
}

bool
found_intersection(in RayPayload ray_payload_brdf)
{
	return ray_payload_brdf.instanceID != ~0u;
}

Triangle get_hit_triangle( in RayPayload ray_payload_brdf )
{
	uint prim = get_primitive(ray_payload_brdf);

	return is_dynamic_instance(ray_payload_brdf)
		?  get_instanced_triangle(prim)
		:  get_bsp_triangle( ray_payload_brdf.instanceID, prim);
}

vec3
get_hit_barycentric( RayPayload ray_payload_brdf )
{
	vec3 bary;
	bary.yz = ray_payload_brdf.barycentric;
	bary.x  = 1.0 - bary.y - bary.z;
	return bary;
}

float
get_rng(uint idx)
{
	uvec3 p = uvec3(rng_seed >> RNG_SEED_SHIFT_X, rng_seed >> RNG_SEED_SHIFT_Y, rng_seed >> RNG_SEED_SHIFT_ISODD);
	p.z = (p.z >> 1) + (p.z & 1);
	p.z = (p.z + idx);
	p &= uvec3(BLUE_NOISE_RES - 1, BLUE_NOISE_RES - 1, NUM_BLUE_NOISE_TEX - 1);

	return min(texelFetch(TEX_BLUE_NOISE, ivec3(p), 0).r, 0.9999999999999);
	//return fract(vec2(get_rng_uint(idx)) / vec2(0xffffffffu));
}

// material flags
bool
is_mirror( in uint material )
{
	return (material & MATERIAL_FLAG_MIRROR) != 0;
}
bool
isSeeThrough( in uint material) 
{
	return (material & MATERIAL_FLAG_SEE_THROUGH) != 0;
}
bool
isSeeThroughAdd( in uint material) 
{
	return (material & MATERIAL_FLAG_SEE_THROUGH_ADD) != 0;
}
bool
isSeeThroughNoAlpha( in uint material) 
{
	return (material & MATERIAL_FLAG_SEE_THROUGH_NO_ALPHA) != 0;
}

// material kinds
bool
is_water( in uint material )
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_WATER;
}

bool
is_slime( in uint material )
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_SLIME;
}

bool
is_lava( in uint material )
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_LAVA;
}

bool
is_glass( in uint material )
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_GLASS;
}

bool
is_transparent( in uint material )
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_TRANSPARENT;
}

bool
is_chrome( in uint material )
{
	uint kind = material & MATERIAL_KIND_MASK;
	return kind == MATERIAL_KIND_CHROME || kind == MATERIAL_KIND_CHROME_MODEL;
}

bool
is_sky( in uint material )
{
	uint kind = material & MATERIAL_KIND_MASK;
	return kind == MATERIAL_KIND_SKY;
}

bool
is_screen( in uint material )
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_SCREEN;
}

bool
is_camera( in uint material )
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_CAMERA;
}

vec3
correct_emissive(uint material_id, vec3 emissive)
{
	return max(vec3(0), emissive.rgb + vec3(EMISSIVE_TRANSFORM_BIAS));
}

#ifndef NO_TRACE_EXT
void trace_ray( Ray ray, bool cull_back_faces, uint cullMask )
{
	uint rayFlags = 0; // gl_RayFlagsNoOpaqueEXT

	if ( !cull_back_faces )	// reversed?
		rayFlags |= gl_RayFlagsCullBackFacingTrianglesEXT;

	ray_payload_brdf.transparency = vec4(0);
	ray_payload_brdf.hit_distance = 0;
	ray_payload_brdf.max_transparent_distance = 0;

	traceRayEXT( topLevelAS, rayFlags, cullMask,
			SBT_RCHIT_OPAQUE /*sbtRecordOffset*/, 0 /*sbtRecordStride*/, SBT_RMISS_PATH_TRACER /*missIndex*/,
			ray.origin, ray.t_min, ray.direction, ray.t_max, PAYLOAD_BRDF);
}

Ray get_shadow_ray( vec3 p1, vec3 p2, float tmin )
{
	vec3 l = p2 - p1;
	float dist = length(l);
	l /= dist;

	Ray ray;
	ray.origin = p1 + l * tmin;
	ray.t_min = 0;
	ray.t_max = dist - tmin - 0.01;
	ray.direction = l;

	return ray;
}

float
trace_shadow_ray( Ray ray, int cull_mask )
{
	const uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;

	ray_payload_shadow.missed = 0;

	traceRayEXT( topLevelAS, rayFlags, cull_mask,
			SBT_RCHIT_EMPTY /*sbtRecordOffset*/, 0 /*sbtRecordStride*/, SBT_RMISS_SHADOW /*missIndex*/,
			ray.origin, ray.t_min, ray.direction, ray.t_max, PAYLOAD_SHADOW);

	return float( ray_payload_shadow.missed );
}


vec3
trace_caustic_ray( Ray ray, int surface_medium )
{
	ray_payload_brdf.hit_distance = -1;

#if 1
	traceRayEXT(topLevelAS, gl_RayFlagsCullBackFacingTrianglesEXT, RAY_FIRST_PERSON_OPAQUE_VISIBLE,
		SBT_RCHIT_OPAQUE, 0, SBT_RMISS_PATH_TRACER,
			ray.origin, ray.t_min, ray.direction, ray.t_max, PAYLOAD_BRDF);
#else
	traceRayEXT(topLevelAS, gl_RayFlagsCullBackFacingTrianglesEXT, AS_FLAG_TRANSPARENT,
		SBT_RCHIT_OPAQUE, 0, SBT_RMISS_PATH_TRACER,
			ray.origin, ray.t_min, ray.direction, ray.t_max, PAYLOAD_BRDF);
#endif

	float extinction_distance = ray.t_max - ray.t_min;
	vec3 throughput = vec3(1);

	if(found_intersection(ray_payload_brdf))
	{
		Triangle triangle = get_hit_triangle(ray_payload_brdf);	

		vec3 geo_normal = triangle.normals[0];
		bool is_vertical = abs(geo_normal.z) < 0.1;

#if 0
		if((is_water(triangle.material_id) || is_slime(triangle.material_id)) && !is_vertical)
		{
			vec3 position = ray.origin + ray.direction * ray_payload_brdf.hit_distance;
			vec3 w = get_water_normal(triangle.material_id, geo_normal, triangle.tangents, position, true);

			float caustic = clamp((1 - pow(clamp(1 - length(w.xz), 0, 1), 2)) * 100, 0, 8);
			caustic = mix(1, caustic, clamp(ray_payload_brdf.hit_distance * 0.02, 0, 1));
			throughput = vec3(caustic);

			if(surface_medium != MEDIUM_NONE)
			{
				extinction_distance = ray_payload_brdf.hit_distance;
			}
			else
			{
				if(is_water(triangle.material_id))
					surface_medium = MEDIUM_WATER;
				else
					surface_medium = MEDIUM_SLIME;

				extinction_distance = max(0, ray.t_max - ray_payload_brdf.hit_distance);
			}
		}
		else 
#endif			
		if(is_glass(triangle.material_id) || is_water(triangle.material_id) && is_vertical)
		{
			vec3 bary = get_hit_barycentric(ray_payload_brdf);
			vec2 tex_coord = triangle.tex_coords0 * bary;

			MaterialInfo minfo = get_material_info(triangle.material_id);

	    	vec3 base_color = vec3(minfo.base_factor);
	    	if (minfo.base_texture > 0)
	    		base_color *= global_textureLod(minfo.base_texture, tex_coord, 2).rgb;
	    	base_color = clamp(base_color, vec3(0), vec3(1));

			throughput = base_color;
		}
#if 0
		else
		{
			throughput = vec3(clamp(1.0 - triangle.alpha, 0.0, 1.0));
		}
#endif
	}

	//return vec3(caustic);
	return extinction(surface_medium, extinction_distance) * throughput;
}
#endif

vec3 rgbToNormal(vec3 rgb, out float len)
{
    vec3 n = vec3(rgb.xy * 2 - 1, rgb.z);

    len = length(n);
    return len > 0 ? n / len : vec3(0);
}


float
AdjustRoughnessToksvig(float roughness, float normalMapLen, float mip_level)
{
	float effect = global_ubo.pt_toksvig * clamp(mip_level, 0, 1);
    float shininess = RoughnessSquareToSpecPower(roughness) * effect; // not squaring the roughness here - looks better this way
    float ft = normalMapLen / mix(shininess, 1.0f, normalMapLen);
    ft = max(ft, 0.01f);
    return SpecPowerToRoughnessSquare(ft * shininess / effect);
}


#ifndef NO_TRACE_EXT
float
get_specular_sampled_lighting_weight(float roughness, vec3 N, vec3 V, vec3 L, float pdfw)
{
    float ggxVndfPdf = ImportanceSampleGGX_VNDF_PDF(max(roughness, 0.01), N, V, L);

    // Balance heuristic assuming one sample from each strategy: light sampling and BRDF sampling
    return clamp(pdfw / (pdfw + ggxVndfPdf), 0, 1);
}

void
get_direct_illumination(
	vec3 position, 
	vec3 normal, 
	vec3 geo_normal, 
	uint cluster_idx, 
	uint material_id,
	int shadow_cull_mask, 
	vec3 view_direction, 
	vec3 albedo,
	vec3 base_reflectivity,
	float specular_factor,
	float roughness, 
	int surface_medium, 
	bool enable_caustics, 
	float direct_specular_weight, 
	bool enable_polygonal,
	bool enable_dynamic,
	bool is_gradient, 
	int bounce,
	out vec3 diffuse,
	out vec3 specular)
{
	diffuse = vec3(0);
	specular = vec3(0);

	vec3 pos_on_light_polygonal;
	vec3 pos_on_light_spherical;

	vec3 contrib_polygonal = vec3(0);
	vec3 contrib_spherical = vec3(0);

	float alpha = square(roughness);
	float phong_exp = RoughnessSquareToSpecPower(alpha);
	float phong_scale = min(100, 1 / (M_PI * square(alpha)));
	float phong_weight = clamp(specular_factor *luminance(base_reflectivity) / (luminance(base_reflectivity) + luminance(albedo)), 0, 0.9);

	int polygonal_light_index = -1;
	float polygonal_light_pdfw = 0;
	bool polygonal_light_is_sky = false;

	vec3 rng = vec3(
		get_rng(RNG_NEE_LIGHT_SELECTION(bounce)),
		get_rng(RNG_NEE_TRI_X(bounce)),
		get_rng(RNG_NEE_TRI_Y(bounce)));

	/* polygonal light illumination */
	if(enable_polygonal) 
	{
		sample_polygonal_lights(
			cluster_idx,
			position, 
			normal, 
			geo_normal, 
			view_direction, 
			phong_exp, 
			phong_scale,
			phong_weight, 
			is_gradient, 
			pos_on_light_polygonal, 
			contrib_polygonal,
			polygonal_light_index,
			polygonal_light_pdfw,
			polygonal_light_is_sky,
			rng);
	}

	bool is_polygonal = true;
	float vis = 1;

	/* dynamic light illumination */
	if(enable_dynamic) 
	{
		// Limit the solid angle of sphere lights for indirect lighting 
		// in order to kill some fireflies in locations with many sphere lights.
		// Example: green wall-lamp corridor in the "train" map.
		float max_solid_angle = (bounce == 0) ? 2 * M_PI : 0.02;
#if 0	
		sample_dynamic_lights(
			position,
			normal,
			geo_normal,
			max_solid_angle,
			pos_on_light_spherical,
			contrib_spherical,
			rng);
#endif
	}

	float spec_polygonal = phong(normal, normalize(pos_on_light_polygonal - position), view_direction, phong_exp) * phong_scale;
	float spec_spherical = phong(normal, normalize(pos_on_light_spherical - position), view_direction, phong_exp) * phong_scale;

	float l_polygonal  = luminance(abs(contrib_polygonal)) * mix(1, spec_polygonal, phong_weight);
	float l_spherical = luminance(abs(contrib_spherical)) * mix(1, spec_spherical, phong_weight);
	float l_sum = l_polygonal + l_spherical;

	bool null_light = (l_sum == 0);

	float w = null_light ? 0.5 : l_polygonal / (l_polygonal + l_spherical);

	float rng2 = get_rng(RNG_NEE_LIGHT_TYPE(bounce));
	is_polygonal = (rng2 < w);
	vis = is_polygonal ? (1 / w) : (1 / (1 - w));
	vec3 pos_on_light = null_light ? position : (is_polygonal ? pos_on_light_polygonal : pos_on_light_spherical);
	vec3 contrib = is_polygonal ? contrib_polygonal : contrib_spherical;

	Ray shadow_ray = get_shadow_ray(position - view_direction * 0.01, pos_on_light, 0);
	
	vis *= trace_shadow_ray(shadow_ray, null_light ? 0 : shadow_cull_mask);
#ifdef ENABLE_SHADOW_CAUSTICS
	if(enable_caustics)
	{
		contrib *= trace_caustic_ray(shadow_ray, surface_medium);
	}
#endif

	/* 
		Accumulate light shadowing statistics to guide importance sampling on the next frame.
		Inspired by paper called "Adaptive Shadow Testing for Ray Tracing" by G. Ward, EUROGRAPHICS 1994.

		The algorithm counts the shadowed and unshadowed rays towards each light, per cluster,
		per surface orientation in each cluster. Orientation helps improve accuracy in cases 
		when a single cluster has different parts which have the same light mostly shadowed and 
		mostly unshadowed.

		On the next frame, the light CDF is built using the counts from this frame, or the frame
		before that in case of gradient rays. See light_lists.h for more info.

		Only applies to polygonal polygon lights (i.e. no model or beam lights) because the spherical
		polygon lights do not have polygonal indices, and it would be difficult to map them 
		between frames.
	*/
	if(global_ubo.pt_light_stats != 0 
		&& is_polygonal 
		&& !null_light
		&& polygonal_light_index >= 0 
		&& polygonal_light_index < global_ubo.num_static_lights)
	{
		uint addr = get_light_stats_addr(cluster_idx, polygonal_light_index, get_primary_direction(normal));

		// Offset 0 is unshadowed rays,
		// Offset 1 is shadowed rays
		if(vis == 0) addr += 1;

		// Increment the ray counter
		atomicAdd(light_stats_bufers[global_ubo.current_frame_idx % NUM_LIGHT_STATS_BUFFERS].stats[addr], 1);
	}

	if(null_light)
		return;

	vec3 radiance = vis * contrib;

	vec3 L = pos_on_light - position;
	L = normalize(L);

	if(is_polygonal && direct_specular_weight > 0 && polygonal_light_is_sky && global_ubo.pt_specular_mis != 0)
	{
		// MIS with direct specular and indirect specular.
		// Only applied to sky lights, for two reasons:
		//  1) Non-sky lights are trimmed to match the light texture, and indirect rays don't see that;
		//  2) Non-sky lights are usually away from walls, so the direct sampling issue is not as pronounced.

		direct_specular_weight *= get_specular_sampled_lighting_weight(roughness,
			normal, -view_direction, L, polygonal_light_pdfw);
	}

	vec3 F = vec3(0);

	if(vis > 0 && direct_specular_weight > 0)
	{
		vec3 specular_brdf = GGX_times_NdotL(view_direction, normalize(pos_on_light - position),
			normal, roughness, base_reflectivity, 0.0, specular_factor, F);
		specular = radiance * specular_brdf * direct_specular_weight;
	}

	float NdotL = max(0, dot(normal, L));

	float diffuse_brdf = NdotL / M_PI;
	diffuse = radiance * diffuse_brdf * (vec3(1.0) - F);
}

void
get_sunlight(
	uint cluster_idx, 
	uint material_id,
	vec3 position, 
	vec3 normal, 
	vec3 geo_normal, 
	vec3 view_direction, 
	vec3 base_reflectivity,
	float specular_factor,
	float roughness, 
	int surface_medium, 
	bool enable_caustics, 
	out vec3 diffuse, 
	out vec3 specular, 
	int shadow_cull_mask)
{
	diffuse = vec3(0);
	specular = vec3(0);

	if(global_ubo.sun_visible == 0)
		return;

#if 0
	bool visible = (cluster_idx == ~0u) || (get_sky_visibility(cluster_idx >> 5) & (1 << (cluster_idx & 31))) != 0;

	if(!visible)
		return;
#else
	bool visible = true;
#endif

	vec2 rng3 = vec2(get_rng(RNG_SUNLIGHT_X(0)), get_rng(RNG_SUNLIGHT_Y(0)));
	vec2 disk = sample_disk(rng3);
	disk.xy *= global_ubo.sun_tan_half_angle;

	vec3 direction = normalize(global_ubo.sun_direction + global_ubo.sun_tangent * disk.x + global_ubo.sun_bitangent * disk.y);

	float NdotL = dot(direction, normal);
	float GNdotL = dot(direction, geo_normal);

	if(NdotL <= 0 || GNdotL <= 0)
		return;

	Ray shadow_ray = get_shadow_ray(position - view_direction * 0.01, position + direction * 10000, 0);
 
	float vis = trace_shadow_ray(shadow_ray, shadow_cull_mask);

	if(vis == 0)
		return;

#ifdef ENABLE_SUN_SHAPE
	// Fetch the sun color from the environment map. 
	// This allows us to get properly shaped shadows from the sun that is partially occluded
	// by clouds or landscape.

	vec3 envmap_direction = (global_ubo.environment_rotation_matrix * vec4(direction, 0)).xyz;
	
    vec3 envmap = textureLod(TEX_PHYSICAL_SKY, envmap_direction.xzy, 0).rgb;

    vec3 radiance = (global_ubo.sun_solid_angle * global_ubo.pt_env_scale) * envmap;
#else
    // Fetch the average sun color from the resolved UBO - it's faster.

    vec3 radiance = global_ubo.sun_color;
#endif

#ifdef ENABLE_SHADOW_CAUSTICS
	if(enable_caustics)
	{
    	radiance *= trace_caustic_ray(shadow_ray, surface_medium);
	}
#endif

	vec3 F = vec3(0);

    if(global_ubo.pt_sun_specular > 0)
    {
		float NoH_offset = 0.5 * square(global_ubo.sun_tan_half_angle);
		vec3 specular_brdf = GGX_times_NdotL(view_direction, global_ubo.sun_direction,
			normal,roughness, base_reflectivity, NoH_offset, specular_factor, F);
    	specular = radiance * specular_brdf;
	}

	float diffuse_brdf = NdotL / M_PI;
	diffuse = radiance * diffuse_brdf * (vec3(1.0) - F);
}
#endif

vec3 clamp_output(vec3 c)
{
	if(any(isnan(c)) || any(isinf(c)))
		return vec3(0);
	else 
		return clamp(c, vec3(0), vec3(MAX_OUTPUT_VALUE));
}

vec3
sample_emissive_texture(uint material_id, MaterialInfo minfo, vec2 tex_coord, vec2 tex_coord_x, vec2 tex_coord_y, float mip_level)
{
	if (minfo.emissive_texture != 0)
    {
        vec4 image3;
	    if (mip_level >= 0)
	        image3 = global_textureLod(minfo.emissive_texture, tex_coord, mip_level);
	    else
	        image3 = global_textureGrad(minfo.emissive_texture, tex_coord, tex_coord_x, tex_coord_y);

    	vec3 corrected = correct_emissive(material_id, image3.rgb);

	    return corrected * minfo.emissive_factor;
	}

	return vec3(0);
}

#if 0 // unused
vec3 get_emissive_shell(uint material_id, uint shell)
{
	vec3 c = vec3(0);

	if((shell & SHELL_MASK) != 0)
	{ 
		if ((shell & SHELL_HALF_DAM) != 0)
		{
			c.r = 0.56f;
			c.g = 0.59f;
			c.b = 0.45f;
		}
		if ((shell & SHELL_DOUBLE) != 0)
		{
			c.r = 0.9f;
			c.g = 0.7f;
		}
		if ((shell & SHELL_LITE_GREEN) != 0)
		{
			c.r = 0.7f;
			c.g = 1.0f;
			c.b = 0.7f;
		}
	    if((shell & SHELL_RED) != 0) c.r += 1;
	    if((shell & SHELL_GREEN) != 0) c.g += 1;
	    if((shell & SHELL_BLUE) != 0) c.b += 1;

	    if((material_id & MATERIAL_FLAG_WEAPON) != 0) c *= 0.2;
	}

	if(tonemap_buffer.adapted_luminance > 0)
			c.rgb *= tonemap_buffer.adapted_luminance * 100;

    return c;
}
#endif

bool get_is_gradient(ivec2 ipos)
{
	if(global_ubo.flt_enable != 0)
	{
		uint u = texelFetch(TEX_ASVGF_GRAD_SMPL_POS_A, ipos / GRAD_DWN, 0).r;

		ivec2 grad_strata_pos = ivec2(
				u >> (STRATUM_OFFSET_SHIFT * 0),
				u >> (STRATUM_OFFSET_SHIFT * 1)) & STRATUM_OFFSET_MASK;

		return (u > 0 && all(equal(grad_strata_pos, ipos % GRAD_DWN)));
	}
	
	return false;
}


void get_material( 
	Triangle triangle,
	vec3 bary,
	vec2 tex_coord[4],
	vec2 tex_coord_x[4],
	vec2 tex_coord_y[4],
	float mip_level,
	vec3 geo_normal,
	out vec3 geo_tangent,
	out vec3 base_color,
	out vec3 normal,
	out float metallic,
	out float roughness,
	out vec3 emissive,
	out float specular_factor) 
{
	MaterialInfo minfo = get_material_info( triangle.material_id );
	//TextureData	minfo_additional = unpackTextureData( triangle.tex0 );

	float effective_mip = mip_level;
	float normalMapLen;

	if ( minfo.normals_texture != 0 ) 
	{
		vec3 n;

		if ( mip_level >= 0 )
			n = global_textureLod( minfo.normals_texture, tex_coord[0], mip_level ).agb;
		else 
			n = global_textureGrad( minfo.normals_texture, tex_coord[0], tex_coord_x[0], tex_coord_y[0] ).agb;

		vec3 local_normal = rgbToNormal(n, normalMapLen);

		n -= vec3(0.5);

		//n.xy *= u_NormalScale.xy;
		n.xy *= 1.0;
		n.z = sqrt(clamp((0.25 - n.x * n.x) - n.y * n.y, 0.0, 1.0));
		
		geo_tangent = normalize(triangle.tangents * bary);
#if 0
		// sunny, binormal is invalid here, we could add it to the triangle struct
		// and apply skin_matrix in instance_geomtery.comp
		// or, calculate bitangent from normal and tangent ..
		n = n.x * geo_tangent + n.y * geo_binormal + n.z * geo_normal;
#else
		vec3 bitangent = cross(geo_normal, geo_tangent);
		n = geo_tangent * n.x + bitangent * n.y + geo_normal * n.z;
#endif   
		
#if 0
		float bump_scale = global_ubo.pt_bump_scale; //  * minfo.bump_scale;
		if(is_glass(triangle.material_id))
        	bump_scale *= 0.2;

		normal = normalize(mix(geo_normal, normal, bump_scale));
#else
		normal = normalize(n);

    	if (effective_mip < 0)
    	{
        	ivec2 texSize = global_textureSize(minfo.normals_texture, 0);
        	vec2 tx = tex_coord_x[0] * texSize;
        	vec2 ty = tex_coord_y[0] * texSize;
        	float d = max(dot(tx, tx), dot(ty, ty));
        	effective_mip = 0.5 * log2(d);
        }
#endif
	}
	else 
	{
		normal = geo_normal;
	}

	metallic = 0;
    roughness = 1;

	vec4 image1 = vec4(1);

	// no multi-stage support yet
    if (minfo.base_texture != 0)
    {
		if (mip_level >= 0)
		    image1 = global_textureLod(minfo.base_texture, tex_coord[0], mip_level);
		else
		    image1 = global_textureGrad(minfo.base_texture, tex_coord[0], tex_coord_x[0], tex_coord_y[0]);
	}

	base_color = image1.rgb * minfo.base_factor;
	base_color = clamp(base_color, vec3(0), vec3(1));

	float spec_mix_bias = 0.08;
	float avgrgb = 0.8;
	float AO = 1.0;


	if ( minfo.physical_texture != 0 ) 
	{
		vec4 ORMS;

		if ( mip_level >= 0 )
			ORMS = global_textureLod( minfo.physical_texture, tex_coord[0], mip_level );
		else 
			ORMS = global_textureGrad( minfo.physical_texture, tex_coord[0], tex_coord_x[0], tex_coord_y[0] );

		//
		// specularScale <rgb> <gloss>
		// or specularScale <r> <g> <b>
		// or specularScale <r> <g> <b> <gloss>
		// or specularScale <metalness> <specular> <unused> <gloss> when metal roughness workflow is used
		//
#if 0	
		ORMS.xyzw *= minfo.specular_scale.zwxy;
		metallic = ORMS.z;
		avgrgb = (base_color.r + base_color.g + base_color.b) / 3;
		//specular.rgb = mix( vec3( 0.08 ) * ORMS.w, base_color.rgb, ORMS.z );
		spec_mix_bias *= ORMS.w;
		base_color.rgb *= vec3( 1.0 - ORMS.z );	

		//roughness = mix( 0.01, 1.0, ORMS.y );
		roughness = clamp(ORMS.y, 0, 1);
		AO = min( ORMS.x, AO );
#else
		avgrgb = (base_color.r + base_color.g + base_color.b) / 3;
		spec_mix_bias *= ORMS.w;
		metallic = clamp( ORMS.z * minfo.specular_scale.x , 0, 1);
		roughness = ORMS.y * minfo.specular_scale.w;
		roughness = clamp(roughness, 0, 1);
		AO = min( ORMS.x, AO ) * minfo.specular_scale.z;
		//base_color.rgb *= vec3( 1.0 - ORMS.z );	
#endif
	}

    bool is_mirror = (roughness < MAX_MIRROR_ROUGHNESS) && (is_chrome(triangle.material_id) || is_screen(triangle.material_id));

    if (normalMapLen > 0 && global_ubo.pt_toksvig > 0 && effective_mip > 0 && !is_mirror)
    {
        roughness = AdjustRoughnessToksvig(roughness, normalMapLen, effective_mip);
    }

    if ( global_ubo.pt_roughness_override >= 0 ) roughness = global_ubo.pt_roughness_override;
    if ( global_ubo.pt_metallic_override >= 0 ) metallic = global_ubo.pt_metallic_override;

    // The specular factor parameter should only affect dielectrics, so make it 1.0 for metals
#if 0
	float specular_factor = 1.0;
    specular_factor = mix(specular_factor, 1.0, metallic);
#else
	specular_factor = mix(minfo.specular_scale.y, 1.0, metallic);
	//specular = mix(0.08, 1.0, metallic) * minfo.specular_scale.y;
#endif

	base_color.rgb = AO * base_color.rgb;

	emissive = sample_emissive_texture( triangle.material_id, minfo, tex_coord[0], tex_coord_x[0], tex_coord_y[0], mip_level );

	//emissive += get_emissive_shell(triangle.material_id, triangle.shell) * base_color * (1 - metallic * 0.9);
}

// Anisotropic texture sampling algorithm from 
// "Improved Shader and Texture Level of Detail Using Ray Cones"
// by T. Akenine-Moller et al., JCGT Vol. 10, No. 1, 2021.
// See section 5. Anisotropic Lookups.
void compute_anisotropic_texture_gradients(
	vec3 intersection,
	vec3 normal,
	vec3 ray_direction,
	float cone_radius,
	mat3 positions,
	mat3x2 tex_coords,
	vec2 tex_coords_at_intersection[4],
	out vec2 texGradient1[4],
	out vec2 texGradient2[4],
	out float fwidth_depth)
{
	// Compute ellipse axes.
	vec3 a1 = ray_direction - dot(normal, ray_direction) * normal;
	vec3 p1 = a1 - dot(ray_direction, a1) * ray_direction;
	a1 *= cone_radius / max(0.0001, length(p1));

	vec3 a2 = cross(normal, a1);
	vec3 p2 = a2 - dot(ray_direction, a2) * ray_direction;
	a2 *= cone_radius / max(0.0001, length(p2));

	// Compute texture coordinate gradients.
	vec3 eP, delta = intersection - positions[0];
	vec3 e1 = positions[1] - positions[0];
	vec3 e2 = positions[2] - positions[0];
	float inv_tri_area = 1.0 / dot(normal, cross(e1, e2));

	eP = delta + a1;
	float u1 = dot(normal, cross(eP, e2)) * inv_tri_area;
	float v1 = dot(normal, cross(e1, eP)) * inv_tri_area;
	texGradient1[0] = (1.0-u1-v1) * tex_coords[0] + u1 * tex_coords[1] +
		v1 * tex_coords[2] - tex_coords_at_intersection[0];

	eP = delta + a2;
	float u2 = dot(normal, cross(eP, e2)) * inv_tri_area;
	float v2 = dot(normal, cross(e1, eP)) * inv_tri_area;
	texGradient2[0] = (1.0-u2-v2) * tex_coords[0] + u2 * tex_coords[1] +
		v2 * tex_coords[2] - tex_coords_at_intersection[0];

	fwidth_depth = 1.0 / max(0.1, abs(dot(a1, ray_direction)) + abs(dot(a2, ray_direction)));
}
