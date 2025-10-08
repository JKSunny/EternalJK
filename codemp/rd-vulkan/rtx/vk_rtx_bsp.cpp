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
#include "conversion.h"

// uncomment the define to visualize polygonal lights by rednering debug triangles 
// the value represents the offset along the light’s normal direction
// #define DEBUG_POLY_LIGHTS 3.0f

static void vk_bind_storage_buffer( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkBuffer buffer )
{
	const uint32_t count = 1;

	assert( buffer != NULL );

	vk_rtx_add_descriptor_buffer( descriptor, count, binding, stage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
	vk_rtx_set_descriptor_update_size( descriptor, binding, stage, count );
	vk_rtx_bind_descriptor_buffer( descriptor, binding, stage, buffer );
}

static void vk_bind_primitive_buffer( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage )
{
	const uint32_t count = 7;

	vk_rtx_add_descriptor_buffer( descriptor, count, binding, stage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
	vk_rtx_set_descriptor_update_size( descriptor, binding, stage, count );

	// world
	vk_rtx_bind_descriptor_buffer_element(descriptor, binding, stage, VERTEX_BUFFER_WORLD,				tr.world->geometry.world_static.buffer[0].buffer);
	vk_rtx_bind_descriptor_buffer_element(descriptor, binding, stage, VERTEX_BUFFER_SKY,				tr.world->geometry.sky_static.buffer[0].buffer);
	vk_rtx_bind_descriptor_buffer_element(descriptor, binding, stage, VERTEX_BUFFER_WORLD_D_MATERIAL,	tr.world->geometry.world_dynamic_material.buffer[0].buffer);
	vk_rtx_bind_descriptor_buffer_element(descriptor, binding, stage, VERTEX_BUFFER_WORLD_D_GEOMETRY,	tr.world->geometry.world_dynamic_geometry.buffer[0].buffer);
#ifdef DEBUG_POLY_LIGHTS
	vk_rtx_bind_descriptor_buffer_element(descriptor, binding, stage, VERTEX_BUFFER_DEBUG_LIGHT_POLYS,	tr.world->geometry.debug_light_polys.buffer[0].buffer);
#endif
	vk_rtx_bind_descriptor_buffer_element(descriptor, binding, stage, VERTEX_BUFFER_SUB_MODELS,			tr.world->geometry.world_submodels.buffer[0].buffer);
	
	// instanced
	vk_rtx_bind_descriptor_buffer_element(descriptor, binding, stage, VERTEX_BUFFER_INSTANCED,			vk.buf_primitive_instanced.buffer );
}

static void vk_bind_uniform_buffer( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkBuffer buffer )
{
	const uint32_t count = 1;

	assert( buffer != NULL );

	vk_rtx_add_descriptor_buffer( descriptor, count, binding, stage, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER );
	vk_rtx_set_descriptor_update_size( descriptor, binding, stage, count );
	vk_rtx_bind_descriptor_buffer( descriptor, binding, stage, buffer );
}

static void vk_bind_storage_image( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkImageView view )
{
	vk_rtx_add_descriptor_image( descriptor, binding, stage );
	vk_rtx_bind_descriptor_image_sampler( descriptor, binding, stage, VK_NULL_HANDLE, view, 0 );
}

static void vk_bind_sampler( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkSampler sampler, VkImageView view, VkImageLayout layout )
{
	const uint32_t count = 1;

	vk_rtx_add_descriptor_sampler( descriptor, binding, stage, count, layout ); 
	vk_rtx_set_descriptor_update_size( descriptor, binding, stage, count );
	vk_rtx_bind_descriptor_image_sampler( descriptor, binding, stage, sampler, view, 0 );
}

static void vk_create_vertex_buffer_descriptor( world_t& worldData, uint32_t index, uint32_t prev_index ) 
{	
	vkdescriptor_t *descriptor = vk_rtx_init_descriptor( & vk.desc_set_vertex_buffer[index] );

	vk_bind_primitive_buffer( descriptor, PRIMITIVE_BUFFER_BINDING_IDX,				VK_SHADER_STAGE_ALL );
	vk_bind_storage_buffer( descriptor, POSITION_BUFFER_BINDING_IDX,				VK_SHADER_STAGE_ALL, vk.buf_positions_instanced.buffer );

	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_READBACK_BUFFER,				VK_SHADER_STAGE_ALL, vk.buf_readback.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_DYNAMIC_VERTEX,				VK_SHADER_STAGE_ALL, vk.buf_readback.buffer );	// can be reomved
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_LIGHT_BUFFER,				VK_SHADER_STAGE_ALL, vk.buf_light.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_TONEMAP_BUFFER,				VK_SHADER_STAGE_ALL, vk.buf_tonemap.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_SUN_COLOR_BUFFER,			VK_SHADER_STAGE_ALL, vk.buf_sun_color.buffer );
	vk_bind_uniform_buffer( descriptor, BINDING_OFFSET_SUN_COLOR_UBO,				VK_SHADER_STAGE_ALL, vk.buf_sun_color.buffer );

	// light stats
	{
		uint32_t i, light_stats_index;
		light_stats_index = vk_rtx_add_descriptor_buffer( descriptor, NUM_LIGHT_STATS_BUFFERS, BINDING_OFFSET_LIGHT_STATS_BUFFER, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
		vk_rtx_set_descriptor_update_size( descriptor, BINDING_OFFSET_LIGHT_STATS_BUFFER, VK_SHADER_STAGE_ALL, NUM_LIGHT_STATS_BUFFERS );

		for (i = 0; i < NUM_LIGHT_STATS_BUFFERS; i++)
		{
			assert( vk.buf_light_stats[i].buffer != NULL );

			descriptor->data[light_stats_index].buffer[i] = { vk.buf_light_stats[i].buffer, 0, vk.buf_light_stats[i].size };
		}
	}

	// light count history
	{
		uint32_t i, light_count_history_index;
		light_count_history_index = vk_rtx_add_descriptor_buffer( descriptor, LIGHT_COUNT_HISTORY, BINDING_LIGHT_COUNTS_HISTORY_BUFFER, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
		vk_rtx_set_descriptor_update_size( descriptor, BINDING_LIGHT_COUNTS_HISTORY_BUFFER, VK_SHADER_STAGE_ALL, LIGHT_COUNT_HISTORY );

		for ( i = 0; i < LIGHT_COUNT_HISTORY; i++ ) 
		{
			assert( vk.buf_light_counts_history[i].buffer != NULL );

			descriptor->data[light_count_history_index].buffer[i] = { 
				vk.buf_light_counts_history[i].buffer, 
				0, 
				vk.buf_light_counts_history[i].size 
			};
		}
	}

	vk_rtx_create_descriptor( descriptor );
	vk_rtx_update_descriptor( descriptor );
}

static void vk_create_rt_descriptor( uint32_t index, uint32_t prev_index ) 
{
	vkdescriptor_t *descriptor = vk_rtx_init_descriptor( &vk.rt_descriptor_set[index] );

	vk_rtx_add_descriptor_as( descriptor, RAY_GEN_DESCRIPTOR_SET_IDX, VK_SHADER_STAGE_RAYGEN_BIT_KHR );
	vk_rtx_bind_descriptor_as( descriptor, RAY_GEN_DESCRIPTOR_SET_IDX, VK_SHADER_STAGE_RAYGEN_BIT_KHR, &vk.tlas_geometry[index].accel );

	vk_rtx_create_descriptor( descriptor );
	vk_rtx_update_descriptor( descriptor );
}

void vk_rtx_create_rt_descriptors( world_t& worldData )
{
	uint32_t i, prev_index;

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		prev_index = (i + (vk.swapchain_image_count - 1)) % vk.swapchain_image_count;

		vk_create_rt_descriptor( i, prev_index );
		vk_create_vertex_buffer_descriptor( worldData, i, prev_index );
	}
}

void vk_rtx_destroy_rt_descriptors( void )
{
	uint32_t i;

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		vk_rtx_destroy_descriptor( &vk.rt_descriptor_set[i] );
		vk_rtx_destroy_descriptor( &vk.desc_set_vertex_buffer[i] );
	}
}

static void vk_rtx_create_primary_rays_resources( world_t& worldData ) 
{
	//vkpt_physical_sky_initialize();

	vk_rtx_create_rt_descriptors( worldData);

	vk_rtx_create_shader_modules();
	vk_rtx_create_rt_pipelines();

	vk_load_final_blit_shader();
	vk_rtx_create_final_blit_pipeline();

	vk_rtx_create_compute_pipelines();
}

void vk_rtx_destroy_primary_rays_resources( void ) 
{
	//vkpt_physical_sky_destroy();

	vk_rtx_destroy_compute_pipelines();
	vk_rtx_destroy_rt_pipelines();

	vk_rtx_destroy_rt_descriptors();

	vk_rtx_destroy_accel_all();

	vk_rtx_clear_material_list();
}

//
// bsp loading
//

#define PlaneDiff(v,p)   (DotProduct(v,(p)->normal)-(p)->dist)

typedef struct {
    vec3_t position;  // 3 floats: x, y, z
    vec2_t st;        // 2 floats: texture coords u, v
} tri_vertex_t;

typedef struct {
	float x, y;

} point2_t;
#define MAX_POLY_VERTS 32
typedef struct {
	point2_t v[MAX_POLY_VERTS];
	int len;
} rt_poly_t;

static inline float
dot2(point2_t* a, point2_t* b)
{
	return a->x * b->x + a->y * b->y;
}

static inline float
cross2(point2_t* a, point2_t* b)
{
	return a->x * b->y - a->y * b->x;
}

static inline point2_t
vsub(point2_t* a, point2_t* b)
{
	point2_t res;
	res.x = a->x - b->x;
	res.y = a->y - b->y;
	return res;
}

/* tells if vec c lies on the left side of directed edge a->b
 * 1 if left, -1 if right, 0 if colinear
 */
static int
left_of(point2_t* a, point2_t* b, point2_t* c)
{
	float x;
	point2_t tmp1 = vsub(b, a);
	point2_t tmp2 = vsub(c, b);
	x = cross2(&tmp1, &tmp2);
	return x < 0 ? -1 : x > 0;
}

static int
line_sect(point2_t* x0, point2_t* x1, point2_t* y0, point2_t* y1, point2_t* res)
{
	point2_t dx, dy, d;
	dx = vsub(x1, x0);
	dy = vsub(y1, y0);
	d = vsub(x0, y0);
	/* x0 + a dx = y0 + b dy ->
	   x0 X dx = y0 X dx + b dy X dx ->
	   b = (x0 - y0) X dx / (dy X dx) */
	float dyx = cross2(&dy, &dx);
	if (!dyx) return 0;
	dyx = cross2(&d, &dx) / dyx;
	if (dyx <= 0 || dyx >= 1) return 0;

	res->x = y0->x + dyx * dy.x;
	res->y = y0->y + dyx * dy.y;
	return 1;
}

static void
poly_append(rt_poly_t* p, point2_t* v)
{
	assert(p->len < MAX_POLY_VERTS - 1);
	p->v[p->len++] = *v;
}

static int
poly_winding(rt_poly_t* p)
{
	return left_of(p->v, p->v + 1, p->v + 2);
}

static void
poly_edge_clip(rt_poly_t* sub, point2_t* x0, point2_t* x1, int left, rt_poly_t* res)
{
	int i, side0, side1;
	point2_t tmp;
	point2_t* v0 = sub->v + sub->len - 1;
	point2_t* v1;
	res->len = 0;

	side0 = left_of(x0, x1, v0);
	if (side0 != -left) poly_append(res, v0);

	for (i = 0; i < sub->len; i++) {
		v1 = sub->v + i;
		side1 = left_of(x0, x1, v1);
		if (side0 + side1 == 0 && side0)
			/* last point and current straddle the edge */
			if (line_sect(x0, x1, v0, v1, &tmp))
				poly_append(res, &tmp);
		if (i == sub->len - 1) break;
		if (side1 != -left) poly_append(res, v1);
		v0 = v1;
		side0 = side1;
	}
}

static inline vec_t PlaneDiffFast( vec3_t v, cplane_t *p )
{
    // fast axial cases
    if ( p->type < 3 )
        return v[p->type] - p->dist;

    // slow generic case
    return PlaneDiff( v, p );
}

// similar to R_PointInLeaf
mnode_t *BSP_PointLeaf( mnode_t *node, vec3_t p )
{
    float d;

    while ( node->plane ) 
	{
		//if ( node->contents != -1 )
		//	break;
        d = PlaneDiffFast( p, node->plane );
        if ( d < 0 )
            node = node->children[1];
        else
            node = node->children[0];
    }

    return node;

}

static void merge_pvs_rows( world_t* world, char* src, char* dst )
{
	for (int i = 0; i < world->clusterBytes; i++)
	{
		dst[i] |= src[i];
	}
}

int vk_get_surface_cluster( world_t &worldData, surfaceType_t *surf ) 
{
	int i, j;

	for ( i = 0; i < worldData.numnodes; i++) 
	{
		mnode_t* node = &worldData.nodes[i];

		if ( node->contents == -1 ) 
			continue;

		msurface_t** mark = node->firstmarksurface;

		for ( j = 0; j < node->nummarksurfaces; j++ )
		{
			if ( mark[j]->data == surf ) 
				return node->cluster;
		}
	}

	return -1;
}

#define FOREACH_BIT_BEGIN(SET,ROWSIZE,VAR) \
	for (int _byte_idx = 0; _byte_idx < ROWSIZE; _byte_idx++) { \
	if (SET[_byte_idx]) { \
		for (int _bit_idx = 0; _bit_idx < 8; _bit_idx++) { \
			if (SET[_byte_idx] & (1 << _bit_idx)) { \
				int VAR = (_byte_idx << 3) | _bit_idx;

#define FOREACH_BIT_END  } } } }

static void build_pvs2( world_t *world )
{
	size_t matrix_size = world->clusterBytes * world->numClusters;

	world->vis2 = (const byte*)Z_Malloc(matrix_size, qtrue);

	for (int cluster = 0; cluster < world->numClusters; cluster++)
	{
		char* pvs = (char*)world->vis + cluster * world->clusterBytes;
		char* dest_pvs = (char*)world->vis2 + cluster * world->clusterBytes;
		memcpy(dest_pvs, pvs, world->clusterBytes);

		FOREACH_BIT_BEGIN(pvs, world->clusterBytes, vis_cluster)
			char* pvs2 = (char*)world->vis + vis_cluster * world->clusterBytes;
			merge_pvs_rows(world, pvs2, dest_pvs);
		FOREACH_BIT_END
	}
}

// Computes a point at a small distance above the center of the triangle.
// Returns qfalse if the triangle is degenerate, qtrue otherwise.
void get_triangle_norm( const float* positions, float* normal )
{
	const float* v0 = positions + 0;
	const float* v1 = positions + 3;
	const float* v2 = positions + 6;


	// Compute the normal

	vec3_t e1, e2;
	VectorSubtract(v1, v0, e1);
	VectorSubtract(v2, v0, e2);
	CrossProduct(e1, e2, normal);
	VectorNormalize(normal);
}


#if 0
void vk_compute_cluster_aabbs( world_t &worldData, mnode_t* node, int numClusters ) 
{
	if (node->contents == -1) {
		vk_compute_cluster_aabbs( worldData, node->children[0], numClusters );
		vk_compute_cluster_aabbs( worldData, node->children[1], numClusters );
		return;
	}

	if ( node->cluster < 0 || node->cluster >= numClusters )
		return;

	aabb_t *aabb = worldData.cluster_aabbs + node->cluster;

	aabb->mins[0] = MIN(aabb->mins[0], node->mins[0]);
	aabb->mins[1] = MIN(aabb->mins[1], node->mins[1]);
	aabb->mins[2] = MIN(aabb->mins[2], node->mins[2]);

	aabb->maxs[0] = MAX(aabb->maxs[0], node->maxs[0]);
	aabb->maxs[1] = MAX(aabb->maxs[1], node->maxs[1]);
	aabb->maxs[2] = MAX(aabb->maxs[2], node->maxs[2]);

}
#else
static void
vk_compute_cluster_aabbs( world_t &worldData )
{
	worldData.cluster_aabbs	= (aabb_t*)calloc(worldData.numClusters, sizeof(aabb_t));
	for ( int i = 0; i < worldData.numClusters; i++ ) 
	{
		VectorSet(worldData.cluster_aabbs[i].mins, FLT_MAX, FLT_MAX, FLT_MAX );
		VectorSet(worldData.cluster_aabbs[i].maxs, -FLT_MAX, -FLT_MAX, -FLT_MAX );
	}

	for (uint32_t prim_idx = 0; prim_idx < worldData.geometry.world_static.geom_opaque.prim_counts[0]; prim_idx++)
	{
		int c = worldData.geometry.world_static.primitives[prim_idx].cluster;

		if(c < 0 || c >= worldData.numClusters)
			continue;

		aabb_t* aabb = worldData.cluster_aabbs + c;
		
		const VboPrimitive* prim = worldData.geometry.world_static.primitives + prim_idx;

		for (int i = 0; i < 3; i++)
		{
			const float* position;
			switch(i)
			{
			case 0:  position = prim->pos0; break;
			case 1:  position = prim->pos1; break;
			default: position = prim->pos2; break;
			}

			aabb->mins[0] = MIN(aabb->mins[0], position[0]);
			aabb->mins[1] = MIN(aabb->mins[1], position[1]);
			aabb->mins[2] = MIN(aabb->mins[2], position[2]);

			aabb->maxs[0] = MAX(aabb->maxs[0], position[0]);
			aabb->maxs[1] = MAX(aabb->maxs[1], position[1]);
			aabb->maxs[2] = MAX(aabb->maxs[2], position[2]);
		}
	}
}
#endif

void append_aabb( const VboPrimitive* primitives, uint32_t numprims, float* aabb_min, float* aabb_max )
{
	for ( uint32_t prim_idx = 0; prim_idx < numprims; prim_idx++ )
	{
		const VboPrimitive* prim = primitives + prim_idx;

		for ( uint32_t vert_idx = 0; vert_idx < 3; vert_idx++ )
		{
			const float* position;
			switch (vert_idx)
			{
			case 0:  position = prim->pos0; break;
			case 1:  position = prim->pos1; break;
			default: position = prim->pos2; break;
			}

			aabb_min[0] = MIN(aabb_min[0], position[0]);
			aabb_min[1] = MIN(aabb_min[1], position[1]);
			aabb_min[2] = MIN(aabb_min[2], position[2]);

			aabb_max[0] = MAX(aabb_max[0], position[0]);
			aabb_max[1] = MAX(aabb_max[1], position[1]);
			aabb_max[2] = MAX(aabb_max[2], position[2]);
		}
	}
}

void compute_aabb( const VboPrimitive* primitives, uint32_t numprims, float* aabb_min, float* aabb_max )
{
	VectorSet( aabb_min, FLT_MAX, FLT_MAX, FLT_MAX );
	VectorSet( aabb_max, -FLT_MAX, -FLT_MAX, -FLT_MAX );

	append_aabb( primitives, numprims, aabb_min, aabb_max );
}

#if 0
static void
remove_collinear_edges(float* positions, float* tex_coords, int* num_vertices)
{
	int num_vertices_local = *num_vertices;

	for (int i = 1; i < num_vertices_local;)
	{
		float* p0 = positions + (i - 1) * 3;
		float* p1 = positions + (i % num_vertices_local) * 3;
		float* p2 = positions + ((i + 1) % num_vertices_local) * 3;

		vec3_t e1, e2;
		VectorSubtract(p1, p0, e1);
		VectorSubtract(p2, p1, e2);
		float l1 = VectorLength(e1);
		float l2 = VectorLength(e2);

		qboolean remove = qfalse;
		if (l1 == 0)
		{
			remove = qtrue;
		}
		else if (l2 > 0)
		{
			VectorScale(e1, 1.f / l1, e1);
			VectorScale(e2, 1.f / l2, e2);

			float dot = DotProduct(e1, e2);
			if (dot > 0.999f)
				remove = qtrue;
		}

		if (remove)
		{
			if (num_vertices_local - i >= 1)
			{
				memcpy(p1, p2, (num_vertices_local - i - 1) * 3 * sizeof(float));

				if (tex_coords)
				{
					float* t1 = tex_coords + (i % num_vertices_local) * 2;
					float* t2 = tex_coords + ((i + 1) % num_vertices_local) * 2;
					memcpy(t1, t2, (num_vertices_local - i - 1) * 2 * sizeof(float));
				}
			}

			num_vertices_local--;
		}
		else
		{
			i++;
		}
	}

	*num_vertices = num_vertices_local;
}
#endif

static int belongs_to_model( world_t &worldData, msurface_t *surf )
{
	for (int i = 0; i < worldData.num_bmodels; i++) 
	{
		if ( i == 0 )
			continue;	// 0 = world ?

		if (surf >= worldData.bmodels[i].firstSurface
		&& surf < worldData.bmodels[i].firstSurface + worldData.bmodels[i].numSurfaces)
			return 1;
	}
	return 0;
}

// Computes a point at a small distance above the center of the triangle.
// Returns qfalse if the triangle is degenerate, qtrue otherwise.
qboolean 
get_triangle_off_center(const float* positions, float* center, float* anti_center, float offset )
{
	const float* v0 = positions + 0;
	const float* v1 = positions + 3;
	const float* v2 = positions + 6;

	// Compute the triangle center

	VectorCopy(v0, center);
	VectorAdd(center, v1, center);
	VectorAdd(center, v2, center);
	VectorScale(center, 1.f / 3.f, center);

	// Compute the normal

	vec3_t e1, e2, normal;
	VectorSubtract(v1, v0, e1);
	VectorSubtract(v2, v0, e2);
	CrossProduct(e1, e2, normal);
	float length = VectorNormalize(normal);

	// Offset the center by one normal to make sure that the point is
	// inside a BSP leaf and not on a boundary plane.
	VectorScale(normal, offset, normal);
	VectorAdd(center, normal, center);

	if (anti_center)
	{
		VectorMA(center, -2.f, normal, anti_center);
	}

	return (qboolean)(length > 0.f);
}

static bool
get_surf_plane_equation( float* plane, tri_vertex_t *vertices, int *indices, int numIndices, int numVertices )
{
	// Go over multiple planes defined by different edge pairs of the surface.
	// Some of the edges may be collinear or almost-collinear to each other,
	// so we can't just take the first pair of edges.
	// We can't even take the first pair of edges with nonzero cross product
	// because of numerical issues - they may be almost-collinear.
	// So this function finds the plane equation from the triangle with the
	// largest area, i.e. the longest cross product.
	int i0, i1, i2;
	float *v0, *v1, *v2;

	float maxlen = 0.f;
	for ( int  i = 0; i < numIndices; i += 3 )
	{
		// mostly likely incorrect,dot it later
		i0 = indices[i + 0];
		i1 = indices[i + 1];
		i2 = indices[i + 2];

		if ( i0 >= numVertices || i1 >= numVertices || i2 >= numVertices )
			continue;

		//float* v0 = surf->points[i0];
		//float* v1 = surf->points[i1];
		//float* v2 = surf->points[i2];
		v0 = vertices[i0].position;
		v1 = vertices[i1].position;
		v2 = vertices[i2].position;


		vec3_t e0, e1;
		VectorSubtract(v1, v0, e0);
		//VectorSubtract(v2, v1, e1);
		VectorSubtract(v2, v0, e1);
		vec3_t normal;
		CrossProduct(e0, e1, normal);
		float len = VectorLength(normal);
		if (len > maxlen)
		{
			VectorScale(normal, 1.0f / len, plane);
			plane[3] = -DotProduct(plane, v0);
			maxlen = len;
		}
	}
	return (maxlen > 0.f);
}

static void
clip_polygon(rt_poly_t* input, rt_poly_t* clipper, rt_poly_t* output)
{
	int i;
	rt_poly_t p1_m, p2_m;
	rt_poly_t* p1 = &p1_m;
	rt_poly_t* p2 = &p2_m;
	rt_poly_t* tmp;

	int dir = poly_winding(clipper);
	poly_edge_clip(input, clipper->v + clipper->len - 1, clipper->v, dir, p2);
	for (i = 0; i < clipper->len - 1; i++) {
		tmp = p2; p2 = p1; p1 = tmp;
		if (p1->len == 0) {
			p2->len = 0;
			break;
		}
		poly_edge_clip(p1, clipper->v + i, clipper->v + i + 1, dir, p2);
	}

	output->len = p2->len;
	if (output->len)
		memcpy(output->v, p2->v, p2->len * sizeof(point2_t));
}

static light_poly_t*
append_light_poly( int *num_lights, int *allocated, light_poly_t **lights )
{
	if (*num_lights == *allocated)
	{
		*allocated = MAX(*allocated * 2, 128);

		// fail ?
		*lights = (light_poly_t*)realloc(*lights, *allocated * sizeof(light_poly_t));
	}
	return *lights + (*num_lights)++;
}

static void get_surface_light_contribution( int surfaceLight, light_poly_t *light )
{
	if ( surfaceLight <= 0 || !light )
		return;

	vec3_t v0, v1, v2;
	VectorCopy( light->positions + 0, v0 );
	VectorCopy( light->positions + 3, v1 );
	VectorCopy( light->positions + 6, v2 );

	float scale = 1.5f;

	vec3_t e0, e1, cross;
	VectorSubtract( v1, v0, e0 );
	VectorSubtract( v2, v0, e1 );
	CrossProduct( e0, e1, cross );

	float area = 0.5f * VectorLength( cross );

	// optional: normalize area to avoid huge surfaces dominating
	const float max_expected_area = 2048.0f;
	float normalized_area = fminf( area / max_expected_area, 1.0f );

	// compress surfacelight values using logarithmic scale
	float surfacelight_scale = log2f((float)surfaceLight + 1.0f); // log2(1 + x) to avoid log(0)

	light->emissive_factor += surfacelight_scale * normalized_area * scale;
	light->emissive_factor = MIN( light->emissive_factor, 4.0f );
}

static void collect_one_light_poly_entire_texture(  world_t &worldData, rtx_material_t *material, int model_idx, 
													const vec3_t light_color, float emissive_factor, int light_style,
													int* num_lights, int* allocated_lights, light_poly_t** lights,
													tri_vertex_t *vertices, int *indices, int numIndices, int numVertices )
{
	float *v0, *v1, *v2;
	int i, i0, i1, i2;
	vec3_t e0, e1, normal;

	for ( i = 0; i < numIndices; i += 3 )
	{
		i0 = indices[i + 0];
		i1 = indices[i + 1];
		i2 = indices[i + 2];

		if ( i0 >= numVertices || i1 >= numVertices || i2 >= numVertices )
			continue;

		v0 = vertices[i0].position;
		v1 = vertices[i1].position;
		v2 = vertices[i2].position;

		light_poly_t light;
		VectorCopy( v0, light.positions + 0 );
		VectorCopy( v2, light.positions + 3 );
		VectorCopy( v1, light.positions + 6 );

		// add offset
		VectorSubtract( v1, v0, e0 );
		VectorSubtract( v2, v0, e1 );
		CrossProduct( e0, e1, normal );
		VectorNormalize( normal );
		const float offset = -0.05f; 
		for ( int j = 0; j < 3; j++ ) 
		{
			VectorMA(light.positions + j * 3, offset, normal, light.positions + j * 3);
		}

		VectorScale( light_color, emissive_factor, light.color );

		light.material = material;
		light.style = light_style;
		light.type = LIGHT_POLYGON;
		light.emissive_factor = emissive_factor;

		if ( material->surface_light > 0 )
			get_surface_light_contribution( material->surface_light, &light );

		if ( !get_triangle_off_center( light.positions, light.off_center, NULL, 1.f ) )
			continue;

		if ( model_idx >= 0 )
			light.cluster = -1;
		else
			light.cluster = BSP_PointLeaf( worldData.nodes, light.off_center )->cluster;

		if ( model_idx >= 0 || light.cluster >= 0 )
		{
			light_poly_t *list_light = append_light_poly( num_lights, allocated_lights, lights );
			memcpy(list_light, &light, sizeof(light_poly_t));
		}
	}
}

static qboolean vk_rtx_compute_bary_weights( const point2_t p, const vec2_t a, const vec2_t b, const vec2_t c, vec3_t weights )
{
	float v0[2], v1[2], v2[2];
	float d00, d01, d11, d20, d21;
	float denom, inv_denom;
	float u, v, w;

	// edge vectors
	v0[0] = b[0] - a[0]; v0[1] = b[1] - a[1];
	v1[0] = c[0] - a[0]; v1[1] = c[1] - a[1];
	v2[0] = p.x - a[0];  v2[1] = p.y - a[1];

	d00 = v0[0] * v0[0] + v0[1] * v0[1];
	d01 = v0[0] * v1[0] + v0[1] * v1[1];
	d11 = v1[0] * v1[0] + v1[1] * v1[1];
	d20 = v2[0] * v0[0] + v2[1] * v0[1];
	d21 = v2[0] * v1[0] + v2[1] * v1[1];

	// return false when barycentric coordinates is degnerate (zero area)
	denom = d00 * d11 - d01 * d01;
	if ( fabsf( denom ) < 1e-8f )
		return qfalse;

	inv_denom = 1.0f / denom;
	v = (d11 * d20 - d01 * d21) * inv_denom;
	w = (d00 * d21 - d01 * d20) * inv_denom;
	u = 1.0f - v - w;

	weights[0] = u;
	weights[1] = v;
	weights[2] = w;

	return qtrue;
}

static void collect_one_light_poly( world_t &worldData, rtx_material_t *material, int model_idx,
									const vec2_t min_light_texcoord, const vec2_t max_light_texcoord,
									const vec3_t light_color, float emissive_factor, int light_style,
									int *num_lights, int *allocated_lights, light_poly_t **lights,
									tri_vertex_t *vertices, int *indices, int numIndices, int numVertices )
{
	float *v0, *v1, *v2;
	int i, j, k, i0, i1, i2, axis;
	int tile_u, tile_v, min_tile_u, max_tile_u, min_tile_v, max_tile_v;
	vec2_t uv0, uv1, uv2, tri_min_uv, tri_max_uv;
	vec2_t min_tc, max_tc;
	vec3_t e0, e1, normal, weights;
	vec3_t tri_positions[3], instance_positions[MAX_POLY_VERTS];
	rt_poly_t tri_uv_poly, clipper, clipped;
	
	for ( i = 0; i < numIndices; i += 3 ) 
	{
		i0 = indices[i + 0];
		i1 = indices[i + 1];
		i2 = indices[i + 2];

		if ( i0 >= numVertices || i1 >= numVertices || i2 >= numVertices )
			continue;

		v0 = vertices[i0].position;
		v1 = vertices[i1].position;
		v2 = vertices[i2].position;

		VectorSet2( uv0, vertices[i0].st[0], vertices[i0].st[1] );
		VectorSet2( uv1, vertices[i1].st[0], vertices[i1].st[1] );
		VectorSet2( uv2, vertices[i2].st[0], vertices[i2].st[1] );

		// get UV aabb
		VectorSet2( tri_min_uv, 
			fminf( fminf( uv0[0], uv1[0] ), uv2[0] ),
			fminf( fminf( uv0[1], uv1[1] ), uv2[1] )
		);
		VectorSet2( tri_max_uv, 
			fmaxf( fmaxf( uv0[0], uv1[0] ), uv2[0] ),
			fmaxf( fmaxf( uv0[1], uv1[1] ), uv2[1] )
		);

		min_tile_u = (int)floorf( tri_min_uv[0] );
		max_tile_u = (int)ceilf( tri_max_uv[0] );
		min_tile_v = (int)floorf( tri_min_uv[1] );
		max_tile_v = (int)ceilf( tri_max_uv[1] );

		tri_uv_poly.len = 3;
		tri_uv_poly.v[0] = { uv0[0], uv0[1] };
		tri_uv_poly.v[1] = { uv1[0], uv1[1] };
		tri_uv_poly.v[2] = { uv2[0], uv2[1] };

		// world-space positions
		VectorCopy( v0, tri_positions[0] );
		VectorCopy( v1, tri_positions[1] );
		VectorCopy( v2, tri_positions[2] );

		for ( tile_u = min_tile_u; tile_u < max_tile_u; tile_u++ ) 
		{
			for ( tile_v = min_tile_v; tile_v < max_tile_v; tile_v++ ) 
			{
				// glow bounds for this tile
				VectorSet2( min_tc, tile_u + min_light_texcoord[0], tile_v + min_light_texcoord[1] );
				VectorSet2( max_tc, tile_u + max_light_texcoord[0], tile_v + max_light_texcoord[1] );

				clipper.len = 4;
				clipper.v[0] = { min_tc[0], min_tc[1] };
				clipper.v[1] = { max_tc[0], min_tc[1] };
				clipper.v[2] = { max_tc[0], max_tc[1] };
				clipper.v[3] = { min_tc[0], max_tc[1] };

				clip_polygon( &tri_uv_poly, &clipper, &clipped );
				if (clipped.len < 3)
					continue;

				// reproject each clipped vertex from UV to world-space using barycentric interpolation
				for (  j = 0; j < clipped.len; j++ ) {
					Com_Memset( instance_positions[j], 0, sizeof(vec3_t) );

					if ( !vk_rtx_compute_bary_weights( clipped.v[j], uv0, uv1, uv2, weights ) )
						continue;

					for ( k = 0; k < 3; k++ ) 
					{
						for ( axis = 0; axis < 3; axis++ ) 
						{
							instance_positions[j][axis] += tri_positions[k][axis] * weights[k];
						}
					}
				}

				// triangulate (fan)
				for ( j = 1; j < clipped.len - 1; j++ ) 
				{
					light_poly_t *light = append_light_poly( num_lights, allocated_lights, lights );
					light->material = material;
					light->style = light_style;
					light->type = LIGHT_POLYGON;
					light->emissive_factor = emissive_factor;


					VectorCopy( instance_positions[0],		light->positions + 0 );
					VectorCopy( instance_positions[j + 1],	light->positions + 3 );
					VectorCopy( instance_positions[j],		light->positions + 6 );
					VectorScale( light_color, emissive_factor, light->color );

					// add offset
					VectorSubtract( light->positions + 3, light->positions + 0, e0);
					VectorSubtract( light->positions + 6, light->positions + 0, e1);
					CrossProduct( e0, e1, normal );
					VectorNormalize( normal );
					const float offset = 0.05f; 
					for ( k = 0; k < 3; k++ )
					{
						VectorMA( light->positions + k * 3, offset, normal, light->positions + k * 3 );
					}

					if ( material->surface_light > 0 )
						get_surface_light_contribution( material->surface_light, light );

					get_triangle_off_center( light->positions, light->off_center, NULL, 1.f  );
	
					if ( model_idx < 0 )
					{
						// Find the cluster for this triangle
						light->cluster = BSP_PointLeaf(worldData.nodes, light->off_center)->cluster;

						if (light->cluster < 0)
						{
							// Cluster not found - which happens sometimes.
							// The lighting system can't work with lights that have no cluster, so remove the triangle.
							(*num_lights)--;
						}
					}
					else
					{
						// It's a model: cluster will be determined after model instantiation.
						light->cluster = -1;
					}
				}
			}
		}
	}
}

static bool
collect_frames_emissive_info( shader_t *shader, rtx_material_t *material, bool *entire_texture_emissive, vec2_t min_light_texcoord, vec2_t max_light_texcoord, vec3_t light_color )
{
	*entire_texture_emissive = false;
	min_light_texcoord[0] = min_light_texcoord[1] = 1.0f;
	max_light_texcoord[0] = max_light_texcoord[1] = 0.0f;

	bool any_emissive_valid = false;

	if ( material == NULL )
		return false;

	uint32_t emissive_image_index = material->emissive;

	if ( !emissive_image_index )
		return false;

	const image_t *image = tr.images[emissive_image_index];

	if ( !image )
		return false;

	if ( !any_emissive_valid )
	{
		// emissive light color of first frame
		memcpy(light_color, image->light_color, sizeof(vec3_t));
	}
	any_emissive_valid = true;

	*entire_texture_emissive |= image->entire_texture_emissive;
	min_light_texcoord[0] = MIN(min_light_texcoord[0], image->min_light_texcoord[0]);
	min_light_texcoord[1] = MIN(min_light_texcoord[1], image->min_light_texcoord[1]);
	max_light_texcoord[0] = MAX(max_light_texcoord[0], image->max_light_texcoord[0]);
	max_light_texcoord[1] = MAX(max_light_texcoord[1], image->max_light_texcoord[1]);

	return any_emissive_valid;
}

static float compute_emissive( rtx_material_t *material )
{
	if ( material == NULL )
		return 1.f;

	return (float)material->emissive_factor;
}

static void collect_light_polys_from_triangles( 
	world_t &worldData, int model_idx, int* num_lights, int* allocated_lights, light_poly_t** lights,  shader_t *shader,
	tri_vertex_t *vertices, int *indices, int numIndices, int numVertices
	)
{
		rtx_material_t *material = vk_rtx_get_material( shader->index );

		if ( material == NULL )
			return;

		bool entire_texture_emissive;
		vec2_t min_light_texcoord;
		vec2_t max_light_texcoord;
		vec3_t light_color;

		if ( !collect_frames_emissive_info( shader, material, &entire_texture_emissive, min_light_texcoord, max_light_texcoord, light_color ) )
		{
			// This algorithm relies on information from the emissive texture,
			// specifically the extents of the emissive pixels in that texture.
			// Ignore surfaces that don't have an emissive texture attached.
			return;
		}

		float emissive_factor = compute_emissive( material );
		if ( emissive_factor == 0 )
			return;

		int light_style = 0;
		
		int light_ctr = *num_lights;	// debug

		if ( entire_texture_emissive )
		{
			collect_one_light_poly_entire_texture(  worldData, material, model_idx, light_color, emissive_factor, light_style,
													num_lights, allocated_lights, lights, 
													vertices, indices, numIndices, numVertices );

			int num_light_polys = (*num_lights - light_ctr);
			if ( num_light_polys > 128 )
				Com_Printf( "verts: %d - light-polys: %d ", numVertices, num_light_polys);
			return;
		}

		vec4_t plane;
		if ( !get_surf_plane_equation( plane, vertices, indices, numIndices, numVertices ) )
		{
			// It's possible that some polygons in the game are degenerate, ignore these.
			return;
		}

		collect_one_light_poly( worldData, material, model_idx,
							   min_light_texcoord, max_light_texcoord,
							   light_color, emissive_factor, light_style,
							   num_lights, allocated_lights, lights,
							   vertices, indices, numIndices, numVertices );

		int num_light_polys = (*num_lights - light_ctr);
		if ( num_light_polys > 128 )
			Com_Printf( "verts: %d - light-polys: %d ", numVertices, num_light_polys);
}

static void collect_face_light_polys( srfSurfaceFace_t *surf, shader_t *shader,
	world_t &worldData, int model_idx, int* num_lights, int* allocated_lights, light_poly_t** lights )
{
	int *indices = (int *)((byte *)surf + surf->ofsIndices);

	tri_vertex_t *vertices = (tri_vertex_t*)malloc(surf->numPoints * sizeof(tri_vertex_t));
	for (int i = 0; i < surf->numPoints; i++) {
		VectorCopy( surf->points[i] + 0, vertices[i].position); // copy vec3 position
		VectorCopy2( surf->points[i] + 6, vertices[i].st);      // copy vec2 tex coords
	}

	collect_light_polys_from_triangles( 
		worldData, model_idx, num_lights, allocated_lights, lights, 
		shader, vertices, indices, surf->numIndices, surf->numPoints 
	);
}

static void collect_tri_light_polys( srfTriangles_t *tri, shader_t *shader,
	world_t &worldData, int model_idx, int* num_lights, int* allocated_lights, light_poly_t** lights )
{
	int *indices = tri->indexes;

	tri_vertex_t *vertices = (tri_vertex_t*)malloc(tri->numVerts  * sizeof(tri_vertex_t));
	for (int i = 0; i < tri->numVerts; i++) {
		VectorCopy( tri->verts[i].xyz, vertices[i].position); // copy vec3 position
		VectorCopy2( tri->verts[i].st + 6, vertices[i].st);      // copy vec2 tex coords
	}

	collect_light_polys_from_triangles( 
		worldData, model_idx, num_lights, allocated_lights, lights, 
		shader, vertices, indices, tri->numIndexes, tri->numVerts 
	);
}

static void collect_grid_light_polys( srfGridMesh_t *cv, shader_t *shader,
    world_t &worldData, int model_idx, int* num_lights, int* allocated_lights, light_poly_t** lights )
{
    int widthTable[MAX_GRID_SIZE];
    int heightTable[MAX_GRID_SIZE];
    float lodError;
    int lodWidth = 0, lodHeight = 0;

#if 0
	// determine the allowable discrepance
    lodError = LodErrorForVolume(cv->lodOrigin, cv->lodRadius);
#else
	lodError = r_lodCurveError->value;
#endif

	// determine which rows and columns of the subdivision
	// we are actually going to use
    widthTable[0] = 0;
    lodWidth = 1;
    for (int i = 1; i < cv->width - 1; i++) {
        if (cv->widthLodError[i] <= lodError) {
            widthTable[lodWidth++] = i;
        }
    }
    widthTable[lodWidth++] = cv->width - 1;

    heightTable[0] = 0;
    lodHeight = 1;
    for (int i = 1; i < cv->height - 1; i++) {
        if (cv->heightLodError[i] <= lodError) {
            heightTable[lodHeight++] = i;
        }
    }
    heightTable[lodHeight++] = cv->height - 1;

    const int numVerts = lodWidth * lodHeight;
    const int numIndices = (lodWidth - 1) * (lodHeight - 1) * 6;

    // fill vertices 
    tri_vertex_t *vertices = (tri_vertex_t*)malloc(numVerts * sizeof(tri_vertex_t));
    int vertIndex = 0;

    for (int h = 0; h < lodHeight; ++h) {
        for (int w = 0; w < lodWidth; ++w) {
            int gridIndex = heightTable[h] * cv->width + widthTable[w];
            srfVert_t *dv = &cv->verts[gridIndex];

            VectorCopy(dv->xyz, vertices[vertIndex].position);
            VectorCopy2(dv->st, vertices[vertIndex].st); 
            vertIndex++;
        }
    }

    // fill indices
    int *indices = (int *)malloc(numIndices * sizeof(int));
    int idx = 0;
    for (int h = 0; h < lodHeight - 1; ++h) {
        for (int w = 0; w < lodWidth - 1; ++w) {
            int v0 = (h + 0) * lodWidth + (w + 0);
            int v1 = (h + 0) * lodWidth + (w + 1);
            int v2 = (h + 1) * lodWidth + (w + 0);
            int v3 = (h + 1) * lodWidth + (w + 1);

            indices[idx++] = v0;
            indices[idx++] = v2;
            indices[idx++] = v1;

            indices[idx++] = v1;
            indices[idx++] = v2;
            indices[idx++] = v3;
        }
    }

    collect_light_polys_from_triangles(
        worldData, model_idx, num_lights, allocated_lights, lights,
        shader, vertices, indices, numIndices, numVerts
    );
}

static void collect_light_polys( world_t &worldData, int model_idx, int* num_lights, int* allocated_lights, light_poly_t** lights )
{
	msurface_t *surf, *surfaces;
	uint32_t i, num_surfaces;
	srfSurfaceFace_t *face;
	srfTriangles_t *tris;
	srfGridMesh_t *grid;

	surfaces		= model_idx < 0 ? worldData.surfaces : worldData.bmodels[model_idx].firstSurface;
	num_surfaces	= model_idx < 0 ? worldData.numsurfaces : worldData.bmodels[model_idx].numSurfaces;

	for ( i = 0; i < num_surfaces; i++ ) 
	{
		surf = surfaces + i;

		// Don't create light polys from SKY surfaces, those are handled separately.
		// Sometimes, textures with a light fixture are used on sky polys (like in rlava1),
		// and that leads to subdivision of those sky polys into a large number of lights.
		if ( RB_IsSky(surf->shader) )
			continue;

		if ( model_idx < 0 && belongs_to_model( worldData, surf ) )
			continue;

		face = (srfSurfaceFace_t *)surf->data;
		if ( face->surfaceType == SF_FACE ) {
			collect_face_light_polys(  face, surf->shader, worldData, model_idx, num_lights, allocated_lights, lights );
			continue;
		}

		tris = (srfTriangles_t *)surf->data;
		if ( tris->surfaceType == SF_TRIANGLES) {
			collect_tri_light_polys( tris, surf->shader , worldData, model_idx, num_lights, allocated_lights, lights );
			continue;
		}

		grid = (srfGridMesh_t *)surf->data;
		if ( grid->surfaceType == SF_GRID ) { 
			collect_grid_light_polys(  grid, surf->shader, worldData, model_idx, num_lights, allocated_lights, lights );
			continue;
		}
	}
}

static void
get_aabb_corner(const aabb_t* aabb, int corner_idx, vec3_t corner)
{
	corner[0] = (corner_idx & 1) ? aabb->maxs[0] : aabb->mins[0];
	corner[1] = (corner_idx & 2) ? aabb->maxs[1] : aabb->mins[1];
	corner[2] = (corner_idx & 4) ? aabb->maxs[2] : aabb->mins[2];
}

static bool
light_affects_cluster(light_poly_t* light, const aabb_t* aabb)
{
	// Empty cluster, nothing is visible
	if (aabb->mins[0] > aabb->maxs[0])
		return false;

	const float* v0 = light->positions + 0;
	const float* v1 = light->positions + 3;
	const float* v2 = light->positions + 6;
	
	// Get the light plane equation
	vec3_t e1, e2, normal;
	VectorSubtract(v1, v0, e1);
	VectorSubtract(v2, v0, e2);
	CrossProduct(e1, e2, normal);
	VectorNormalize(normal);
	
	float plane_distance = -DotProduct(normal, v0);

	bool all_culled = true;

	// If all 8 corners of the cluster's AABB are behind the light, it's definitely invisible
	for (int corner_idx = 0; corner_idx < 8; corner_idx++)
	{
		vec3_t corner;
		get_aabb_corner(aabb, corner_idx, corner);

		float side = DotProduct(normal, corner) + plane_distance;
		if (side > 0)
			all_culled = false;
	}

	if (all_culled)
	{
		return false;
	}

	return true;
}

static void collect_cluster_lights( world_t &worldData ) 
{
#define MAX_LIGHTS_PER_CLUSTER 1024
	int *cluster_lights = (int *)Z_Malloc( MAX_LIGHTS_PER_CLUSTER * worldData.numClusters * sizeof(int), TAG_GENERAL, qfalse );
	int *cluster_light_counts = (int *)Z_Malloc( worldData.numClusters * sizeof(int), TAG_GENERAL, qtrue );

	// Construct an array of visible lights for each cluster.
	// The array is in `cluster_lights`, with MAX_LIGHTS_PER_CLUSTER stride.

	for ( int nlight = 0; nlight < worldData.num_light_polys; nlight++ )
	{
		light_poly_t *light = worldData.light_polys + nlight;

		if ( light->cluster < 0 )
			continue;

		const byte *pvs = (const byte*)BSP_GetPvs( &worldData, light->cluster);

		FOREACH_BIT_BEGIN( pvs, worldData.clusterBytes, other_cluster )
			aabb_t *cluster_aabb = worldData.cluster_aabbs + other_cluster;
			if (light_affects_cluster(light, cluster_aabb))
			{
				int *num_cluster_lights = cluster_light_counts + other_cluster;
				if ( *num_cluster_lights < MAX_LIGHTS_PER_CLUSTER )
				{
					cluster_lights[other_cluster * MAX_LIGHTS_PER_CLUSTER + *num_cluster_lights] = nlight;
					(*num_cluster_lights)++;
				}
			}
		FOREACH_BIT_END
	}

	// Count the total number of cluster <-> light relations to allocate memory
	worldData.num_cluster_lights = 0;
	for (int cluster = 0; cluster < worldData.numClusters; cluster++)
	{
		worldData.num_cluster_lights += cluster_light_counts[cluster];
	}

	worldData.cluster_lights = (int*)Z_Malloc(worldData.num_cluster_lights * sizeof(int), TAG_GENERAL, qtrue );
	worldData.cluster_light_offsets = (int*)Z_Malloc((worldData.numClusters + 1) * sizeof(int), TAG_GENERAL, qtrue );

	Com_Printf( S_COLOR_MAGENTA "\n\n\nTotal interactions: %d,\n\n", worldData.num_cluster_lights );

	// Compact the previously constructed array into worldData.cluster_lights
	int list_offset = 0;
	for ( int cluster = 0; cluster < worldData.numClusters; cluster++ )
	{
		assert(list_offset >= 0);
		worldData.cluster_light_offsets[cluster] = list_offset;
		int count = cluster_light_counts[cluster];
		memcpy(
			worldData.cluster_lights + list_offset, 
			cluster_lights + MAX_LIGHTS_PER_CLUSTER * cluster, 
			count * sizeof(int));
		list_offset += count;
	}
	worldData.cluster_light_offsets[worldData.numClusters] = list_offset;

	Z_Free(cluster_lights);
	Z_Free(cluster_light_counts);
#undef MAX_LIGHTS_PER_CLUSTER
}

static int filter_opaque( shader_t *shader )
{
	if ( RB_IsTransparent( shader ) ) 
		return 0;

	return 1;
}

static int filter_transparent( shader_t *shader )
{
	if ( !RB_IsTransparent( shader ) ) 
		return 0;

	return 1;
}

static int filter_static( shader_t *shader )
{
	qboolean as_dynamic = RB_IsDynamicGeometry( shader );
	qboolean as_dynamic_data = RB_IsDynamicMaterial( shader );
	qboolean is_sky = RB_IsSky(shader);

	if ( !as_dynamic && !as_dynamic_data && !is_sky )
		return 1;

	return 0;
}

static int filter_dynamic_geometry(shader_t *shader)
{
    qboolean is_geom_dynamic = RB_IsDynamicGeometry(shader);
    qboolean is_mat_dynamic = RB_IsDynamicMaterial(shader);
    qboolean is_sky = RB_IsSky(shader);

    if (is_geom_dynamic && !is_mat_dynamic)
        return 1;

    return 0;
}

static int filter_dynamic_material( shader_t *shader )
{
    qboolean is_geom_dynamic = RB_IsDynamicGeometry(shader);
    qboolean is_mat_dynamic = RB_IsDynamicMaterial(shader);
    qboolean is_sky = RB_IsSky(shader);

    if (!is_geom_dynamic && is_mat_dynamic)
        return 1;

    return 0;
}

static int filter_sky( shader_t *shader )
{
	qboolean is_sky = RB_IsSky(shader);

	if ( !is_sky )
		return 0;

	return 1;
}

static int skip_invalid_surface( shader_t *shader )
{
	qboolean is_sky = RB_IsSky(shader);

	if ( is_sky )
		return 0;

	if ( shader->surfaceFlags == SURF_NODRAW /*|| shader->surfaceFlags == SURF_NONE*///SURF_SKIP
		|| shader->stages[0] == NULL 
		|| !shader->stages[0]->active )
		return 1;

	return 0;
}

static void vk_rtx_estimate_geometry_recursive( world_t &worldData, mnode_t *node, vk_geometry_data_t *geom, int (*filter)(shader_t*) )
{
	uint32_t	i, j;
	msurface_t	*surf, **mark;

	if ( node->contents == -1 ) 
	{
		for ( i = 0; i < 2; i++ ) 
		{
			vk_rtx_estimate_geometry_recursive( worldData, node->children[i], geom, filter );
		}
	}

	mark = node->firstmarksurface;

	for ( j = 0; j < node->nummarksurfaces; j++ ) 
	{
		surf = mark[j];

		shader_t* shader = tr.shaders[surf->shader->index];

		if ( !filter( shader ) )
			continue;

		tess.shader = shader;
		tess.allowVBO = qfalse;

		tess.numVertexes = tess.numIndexes = 0;
		
		// process surface
		rb_surfaceTable[*surf->data](surf->data);	

		const uint32_t num_clusters = (tess.numIndexes / 3);

		geom->host.surf_count++;
		geom->num_primitives_allocated += tess.numIndexes / 3;
	}
}

static void vk_rtx_estimate_geometry( world_t &worldData, mnode_t *node, vk_geometry_data_t *geom, int (*filter)(shader_t*) )
{ 
	// should already be done in vk_rtx_reset_world_geometry() with memset
	geom->host.surf_count = 0;
	geom->host.surf_offset = 0;

	geom->num_primitives_allocated = 0;
	geom->num_primitives = 0;

	vk_rtx_estimate_geometry_recursive( worldData, node, geom, filter );

	geom->primitives	= (VboPrimitive*)calloc( MAX( 1, (geom->num_primitives_allocated )),  sizeof(VboPrimitive) );
	Com_Memset( geom->primitives, 0, MAX( 1, (geom->num_primitives_allocated )) * sizeof(VboPrimitive) );

	// ~sunny, might be usfull to keep for dynamic surfaces
	const uint32_t surf_count_allocated = MAX( 1, (geom->host.surf_count ));
	geom->host.dynamic_surfs = (vk_geometry_dynamic_surf_t*)calloc( surf_count_allocated, sizeof(vk_geometry_dynamic_surf_t) );
	Com_Memset( geom->host.dynamic_surfs, 0, surf_count_allocated * sizeof(vk_geometry_dynamic_surf_t) );
}

static void vk_rtx_estimate_bmodels( world_t& worldData, vk_geometry_data_t *geom )
{
	geom->host.surf_count = 0;
	geom->host.surf_offset = 0;

	geom->num_primitives_allocated = 0;
	geom->num_primitives = 0;

	for ( int model = 0; model < worldData.num_bmodels; model++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[model];
		msurface_t	*surf;

		for (int i = 0; i < bmodel->numSurfaces; i++) 
		{
			surf = bmodel->firstSurface + i;

			shader_t* shader = tr.shaders[surf->shader->index];

			tess.numVertexes = tess.numIndexes = 0;
			tess.shader = shader;
			tess.allowVBO = qfalse;
			rb_surfaceTable[*surf->data](surf->data);

			geom->num_primitives += tess.numIndexes / 3;

			tess.numVertexes = tess.numIndexes = 0;
		}
	}

	geom->primitives	= (VboPrimitive*)calloc( MAX( 1, (geom->num_primitives )),  sizeof(VboPrimitive) );
	Com_Memset( geom->primitives, 0, MAX( 1, (geom->num_primitives )) * sizeof(VboPrimitive) );

	/*if ( geom->dynamic_flags )
	{
		geom->host.dynamic_surfs = (vk_geometry_dynamic_surf_t*)calloc( geom->host.surf_count, sizeof(vk_geometry_dynamic_surf_t) );
	}*/
}

#define clamp(a,b,c)    ((a)<(b)?(a)=(b):(a)>(c)?(a)=(c):(a))

uint32_t
encode_normal(const vec3_t normal)
{
	float invL1Norm = 1.0f / (fabsf(normal[0]) + fabsf(normal[1]) + fabsf(normal[2]));

	vec2_t p = { normal[0] * invL1Norm, normal[1] * invL1Norm };
	vec2_t pp = { p[0], p[1] };

	if (normal[2] < 0.f)
	{
		pp[0] = (1.f - fabsf(p[1])) * ((p[0] >= 0.f) ? 1.f : -1.f);
		pp[1] = (1.f - fabsf(p[0])) * ((p[1] >= 0.f) ? 1.f : -1.f);
	}

	pp[0] = pp[0] * 0.5f + 0.5f;
	pp[1] = pp[1] * 0.5f + 0.5f;

	clamp(pp[0], 0.f, 1.f);
	clamp(pp[1], 0.f, 1.f);

	uint32_t ux = (uint32_t)(pp[0] * 0xffffu);
	uint32_t uy = (uint32_t)(pp[1] * 0xffffu);

	return ux | (uy << 16);
}

// create_poly(worldData, surf, material_id, geom, surface_prims);
static uint32_t create_poly( vk_geometry_data_t *geom, rtx_material_t *material, uint32_t material_id, VboPrimitive *primitives_out )
{
	int i0, i1, i2;
	const shaderStage_t *pStage;

	const float emissive_factor = compute_emissive(material);

	float alpha = 1.f;
	//if (MAT_IsKind(material_id, MATERIAL_KIND_TRANSPARENT))
	//	alpha = (texinfo->c.flags & SURF_TRANS33) ? 0.33f : (texinfo->c.flags & SURF_TRANS66) ? 0.66f : 1.0f;

	const uint32_t emissive_and_alpha = floatToHalf(emissive_factor) | (floatToHalf(alpha) << 16);

	int numTris = tess.numIndexes / 3;

	VboPrimitive *base_primitive = primitives_out;

	for ( uint32_t i = 0; i < numTris; ++i ) 
	{
		memset(primitives_out, 0, sizeof(VboPrimitive));

		uint32_t idx_base = i * 3;

		i0 = tess.indexes[idx_base + 0];
		i1 = tess.indexes[idx_base + 1];
		i2 = tess.indexes[idx_base + 2];

		VectorCopy( tess.xyz[i0], primitives_out->pos0 );
		VectorCopy( tess.xyz[i1], primitives_out->pos1 );
		VectorCopy( tess.xyz[i2], primitives_out->pos2 );

		primitives_out->normals[0] = encode_normal( tess.normal[i0] );
		primitives_out->normals[1] = encode_normal( tess.normal[i1] );
		primitives_out->normals[2] = encode_normal( tess.normal[i2] );

		primitives_out->tangents[0] = encode_normal( tess.qtangent[i0] );
		primitives_out->tangents[1] = encode_normal( tess.qtangent[i1] );
		primitives_out->tangents[2] = encode_normal( tess.qtangent[i2] );

		primitives_out->color0[0] = tess.svars.colors[0][i0][0] | tess.svars.colors[0][i0][1] << 8 | tess.svars.colors[0][i0][2] << 16 | tess.svars.colors[0][i0][3] << 24;
		primitives_out->color1[0] = tess.svars.colors[0][i1][0] | tess.svars.colors[0][i1][1] << 8 | tess.svars.colors[0][i1][2] << 16 | tess.svars.colors[0][i1][3] << 24;
		primitives_out->color2[0] = tess.svars.colors[0][i2][0] | tess.svars.colors[0][i2][1] << 8 | tess.svars.colors[0][i2][2] << 16 | tess.svars.colors[0][i2][3] << 24;

		primitives_out->material_id = material_id; //(material_flags & ~MATERIAL_INDEX_MASK) | (material_index & MATERIAL_INDEX_MASK);
		primitives_out->emissive_and_alpha = emissive_and_alpha;
		primitives_out->instance = 0;

		++primitives_out;
	}

	for ( uint32_t stage = 0; stage < MAX_RTX_STAGES; stage++ ) 
	{
		pStage = tess.shader->stages[stage];

		if ( !pStage || !pStage->active )
			break;

		//
		// only compute bundle 0 for now
		//
		if ( pStage->tessFlags & TESS_RGBA0 )
			ComputeColors( 0, tess.svars.colors[0], pStage, 0 );

		if ( pStage->tessFlags & TESS_ST0 )
			ComputeTexCoords( 0, &pStage->bundle[0] );

		primitives_out = base_primitive;
		for ( uint32_t prim = 0; prim < numTris; ++prim ) 
		{
			uint32_t idx_base = prim * 3;

			i0 = tess.indexes[idx_base + 0];
			i1 = tess.indexes[idx_base + 1];
			i2 = tess.indexes[idx_base + 2];

			if ( pStage->tessFlags & TESS_ST0 )
			{
				primitives_out->uv0[stage] = floatToHalf(tess.svars.texcoordPtr[0][i0][0]) | (floatToHalf(tess.svars.texcoordPtr[0][i0][1]) << 16);
				primitives_out->uv1[stage] = floatToHalf(tess.svars.texcoordPtr[0][i1][0]) | (floatToHalf(tess.svars.texcoordPtr[0][i1][1]) << 16);
				primitives_out->uv2[stage] = floatToHalf(tess.svars.texcoordPtr[0][i2][0]) | (floatToHalf(tess.svars.texcoordPtr[0][i2][1]) << 16);
			}

			if ( pStage->tessFlags & TESS_RGBA0 ) 
			{
				primitives_out->color0[stage] = tess.svars.colors[0][i0][0] | tess.svars.colors[0][i0][1] << 8 | tess.svars.colors[0][i0][2] << 16 | tess.svars.colors[0][i0][3] << 24;
				primitives_out->color1[stage] = tess.svars.colors[0][i1][0] | tess.svars.colors[0][i1][1] << 8 | tess.svars.colors[0][i1][2] << 16 | tess.svars.colors[0][i1][3] << 24;
				primitives_out->color2[stage] = tess.svars.colors[0][i2][0] | tess.svars.colors[0][i2][1] << 8 | tess.svars.colors[0][i2][2] << 16 | tess.svars.colors[0][i2][3] << 24;
			}

			++primitives_out;
		}
	}

	return numTris;
}

static void vk_rtx_collect_surfaces( uint32_t *prim_ctr, vk_geometry_data_t *geom, int type, world_t &worldData, mnode_t *node, int model_idx,
	int (*filter)(shader_t*), int (*filter_visibiliy)(shader_t*) )
{
	uint32_t	i, j;
	msurface_t	*surf, **mark;

	if ( node->contents == -1 ) 
	{
		for ( i = 0; i < 2; i++ ) 
		{
			vk_rtx_collect_surfaces( prim_ctr, geom, type, worldData, node->children[i], model_idx, filter, filter_visibiliy );
		}
	}

	vk_geometry_data_accel_t *accel = &geom->accel[type];
	
	mark = node->firstmarksurface;

	for ( j = 0; j < node->nummarksurfaces; j++ ) 
	{
		surf = mark[j];
		surf->notBrush = qtrue;

		if ( model_idx < 0 && belongs_to_model( worldData, surf ) )
			continue;

		shader_t* shader = tr.shaders[surf->shader->index];

		if ( surf->added || surf->skip )	
			continue;

		if ( !filter_visibiliy( shader ) )
			continue;

		rtx_material_t *mat = vk_rtx_shader_to_material( shader );
		uint32_t material_id = mat ? mat->flags : 0;

		if ( RB_IsSky(shader) )
		{
			if ( !filter( shader ) )
				continue;

			material_id = MAT_SetKind(material_id, MATERIAL_KIND_SKY);

			// ~sunny, force for now
			if ( (material_id & MATERIAL_FLAG_LIGHT) == 0 )
			{
				material_id |= MATERIAL_FLAG_LIGHT;
			}
		}
		else 
		{
			{	// ~sunny, debug
				if ( strstr( shader->name, "glass" ) )
					material_id = MAT_SetKind(material_id, MATERIAL_KIND_GLASS);

				if ( strstr( shader->name, "botton") || strstr( shader->name, "door02") ) 
					material_id = MAT_SetKind(material_id, MATERIAL_KIND_GLASS);

				//if ( shader->sort == SS_PORTAL && strstr( shader->name, "mirror" ) != NULL ) 
				//	material_id |= MATERIAL_FLAG_MIRROR;
			}

			if ( !filter( shader ) )
				continue;
		}

		if ( skip_invalid_surface( shader ) || *surf->data == SF_BAD || *surf->data == SF_SKIP) 
		{
			surf->skip = qtrue;
			continue;
		}

		tess.numVertexes = tess.numIndexes = 0;
		tess.shader = shader;
		tess.allowVBO = qfalse;

		rb_surfaceTable[*surf->data](surf->data);	// process surface

		if ( tess.numIndexes == 0 ) 
			continue;

		if ( geom->dynamic_flags )
		{
			vk_geometry_dynamic_surf_t *dsurf = geom->host.dynamic_surfs + geom->host.surf_offset;

			dsurf->surf = surf;
			dsurf->shader = shader;
			dsurf->cluster = node->cluster;
			dsurf->fogIndex = 0;

			geom->host.surf_offset++;
			accel->surf_count++;
		}





		VboPrimitive* surface_prims = geom->primitives + *prim_ctr;
		uint32_t prims_in_surface = create_poly( geom, mat, material_id, surface_prims );

		for (uint32_t k = 0; k < prims_in_surface; ++k) {
			//if (model_idx < 0) world, sub bmodels have sep collector
			{
				float positions[9];
				VectorCopy(surface_prims[k].pos0, positions + 0);
				VectorCopy(surface_prims[k].pos1, positions + 3);
				VectorCopy(surface_prims[k].pos2, positions + 6);

				vec3_t center, anti_center;
				get_triangle_off_center(positions, center, anti_center, 0.01f);

				//int cluster = BSP_PointLeaf(worldData.nodes, center)->cluster;
				int cluster = (model_idx < 0) ? node->cluster : -1;

				if (cluster < 0) {
					get_triangle_off_center(positions, center, anti_center, 1.f);
					cluster = BSP_PointLeaf(worldData.nodes, center)->cluster;
				}

				surface_prims[k].cluster = cluster;
			}

			//else{ // sub bmodels
			//	surface_prims[k].cluster = -1;
			//}
		}

		tess.numVertexes = tess.numIndexes = 0;	// ~sunny, redundant?

		surf->added = qtrue;

		*prim_ctr += prims_in_surface;
	}	
}

static void vk_rtx_collect_bmodel_surfaces( uint32_t *prim_ctr, world_t &worldData, vk_geometry_data_t *geom, int type, bmodel_t *bmodel )
{
	msurface_t	*surf;

	for ( int i = 0; i < bmodel->numSurfaces; i++ ) 
	{
		surf = bmodel->firstSurface + i;

		shader_t* shader = tr.shaders[surf->shader->index];

		if ( surf->added || surf->skip )	
			continue;

		if ( skip_invalid_surface( shader ) || *surf->data == SF_BAD || *surf->data == SF_SKIP)
		{
			surf->skip = qtrue;
			continue;
		}

		tess.numVertexes = tess.numIndexes = 0;
		tess.shader = shader;
		tess.allowVBO = qfalse;
		rb_surfaceTable[*surf->data](surf->data);

		// ~sunny, rework this
		rtx_material_t *mat = vk_rtx_shader_to_material( tess.shader );
		uint32_t material_id = (mat != NULL) ? mat->flags : 0U;

		VboPrimitive* surface_prims = geom->primitives + *prim_ctr;
		uint32_t prims_in_surface = create_poly( geom, mat, material_id, surface_prims );

		for (uint32_t k = 0; k < prims_in_surface; ++k) {
			//if (model_idx < 0) world, sub bmodels have sep collector
			{
				float positions[9];
				VectorCopy(surface_prims[k].pos0, positions + 0);
				VectorCopy(surface_prims[k].pos1, positions + 3);
				VectorCopy(surface_prims[k].pos2, positions + 6);

				vec3_t center, anti_center;
				get_triangle_off_center(positions, center, anti_center, 0.01f);

				int cluster = BSP_PointLeaf(worldData.nodes, center)->cluster;
				//int cluster = (model_idx < 0) ? node->cluster : -1;

				if (cluster < 0) {
					get_triangle_off_center(positions, center, anti_center, 1.f);
					cluster = BSP_PointLeaf(worldData.nodes, center)->cluster;
				}

				surface_prims[k].cluster = cluster;
			}

			//else{ // sub bmodels
			//	surface_prims[k].cluster = -1;
			//}
		}

		tess.numVertexes = tess.numIndexes = 0;

		*prim_ctr += prims_in_surface;
	}
}

// ~sunny, on list to deprecate
void vk_rtx_update_dynamic_geometry( VkCommandBuffer cmd_buf, vk_geometry_data_t *geom ) 
{
#if 0
	uint32_t i, type;

	if ( !geom->allow_update || !geom->dynamic_flags )
		return;

	const int frame_idx = vk.frame_counter & 1;

	uint32_t idx_count = 0;
	uint32_t xyz_count = 0;
	uint32_t cluster_count = 0;
	uint32_t num_primitives = 0;

	// reset
	geom->host.offset_primitives = 0;

	for ( type = 0; type < BLAS_TYPE_COUNT; ++type ) 
	{
		if ( !(geom->blas_type_flags & (1 << type) ) )
			continue;
	
		vk_geometry_data_accel_t *accel = &geom->accel[type];
		msurface_t *surf;

		float originalTime = backEnd.refdef.floatTime;

		accel->idx_count		= 0;						// reset
		accel->xyz_count		= 0;
		accel->cluster_count	= 0;
		accel->idx_offset		= geom->host.idx_offset;	// vk_rtx_set_geomertry_accel_offsets()
		accel->xyz_offset		= geom->host.xyz_offset;
		accel->cluster_offset	= geom->host.cluster_offset;

		accel->num_primitives		= 0;
		accel->offset_primitives	= geom->host.offset_primitives;

		for ( i = 0; i < accel->surf_count; i++ )
		{
			vk_geometry_dynamic_surf_t *dsurf = geom->host.dynamic_surfs + accel->surf_offset + i;

			tess.numVertexes = tess.numIndexes = 0;
			tess.shader = (shader_t*)dsurf->shader;
			tess.fogNum = dsurf->fogIndex;

			backEnd.refdef.floatTime = originalTime;
			tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

			tess.allowVBO = qfalse;
			surf = (msurface_t*)dsurf->surf;
			rb_surfaceTable[*surf->data](surf->data);

			if ( geom->dynamic_flags & BLAS_DYNAMIC_FLAG_ALL )
				RB_DeformTessGeometry();

			const uint32_t num_clusters = (tess.numIndexes / 3);

			assert( geom->host.xyz_offset + tess.numVertexes <= geom->host.xyz_count );
			assert( geom->host.idx_offset + tess.numIndexes <= geom->host.idx_count );
			assert( geom->host.cluster_offset + num_clusters <= geom->host.cluster_count );

			if ( geom->dynamic_flags & BLAS_DYNAMIC_FLAG_IDX )
				vk_rtx_bind_indicies( geom->host.idx + geom->host.idx_offset, accel->xyz_count );

			if ( geom->dynamic_flags & BLAS_DYNAMIC_FLAG_XYZ )
				vk_rtx_bind_vertices( geom->host.xyz + geom->host.xyz_offset, dsurf->cluster );

			vk_rtx_bind_cluster( geom->host.cluster + geom->host.cluster_offset, num_clusters, dsurf->cluster );

			geom->host.idx_offset += tess.numIndexes;
			geom->host.xyz_offset += tess.numVertexes;
			geom->host.cluster_offset += num_clusters;

			accel->idx_count += tess.numIndexes;
			accel->xyz_count += tess.numVertexes;
			accel->cluster_count += num_clusters;
			accel->num_primitives += tess.numIndexes / 3;

			tess.numVertexes = tess.numIndexes = 0;
		}

		idx_count += accel->idx_count;
		xyz_count += accel->xyz_count;
		cluster_count += accel->cluster_count;
		num_primitives += accel->num_primitives;

		backEnd.refdef.floatTime = originalTime;
	}

	if ( idx_count <= 0 )
		return;

	const uint32_t offset = 0 ;

	if ( geom->dynamic_flags & BLAS_DYNAMIC_FLAG_IDX )
		vk_rtx_upload_buffer_data_offset( &geom->idx[frame_idx], 0, idx_count * sizeof(uint32_t), (const byte*)geom->host.idx + offset * sizeof(uint32_t) );
	
	if ( geom->dynamic_flags & BLAS_DYNAMIC_FLAG_XYZ )
		vk_rtx_upload_buffer_data_offset( &geom->xyz[frame_idx], 0, xyz_count * sizeof(VertexBuffer), (const byte*)geom->host.xyz + offset * sizeof(VertexBuffer) );

	vk_rtx_upload_buffer_data_offset( &geom->cluster[frame_idx], 0, cluster_count * sizeof(uint32_t), (const byte*)geom->host.cluster + offset * sizeof(uint32_t) );

	for ( type = 0; type < BLAS_TYPE_COUNT; ++type ) 
	{
		if ( !(geom->blas_type_flags & (1 << type) ) )
			continue;
	
		vk_geometry_data_accel_t *accel = &geom->accel[type];
		vk_blas_t *blas = &accel->blas[frame_idx];

		vk_rtx_update_blas( cmd_buf, geom, accel, blas, blas );
	}
#endif
}

static void vk_rtx_reset_world_geometry( vk_geometry_data_t *geom )
{
	uint32_t i, j;

	if ( geom == NULL )
		return;

	// host
	if ( geom->host.dynamic_surfs )
		free( geom->host.dynamic_surfs );

	if ( geom->primitives )
		free( geom->primitives );

	Com_Memset( &geom->host, 0, sizeof(vk_geometry_host_t) );
	
	// device
	uint32_t num_instances = geom->dynamic_flags ? NUM_COMMAND_BUFFERS : 1; // ~sunny, keep this for now

	for ( i = 0; i < num_instances; i++ )
	{
		vk_rtx_buffer_destroy( &geom->buffer[i] );
	}

	Com_Memset( geom, 0, sizeof(geom) );
}

void vk_rtx_reset_world_geometries( world_t *world ) 
{
	free( world->cluster_aabbs );
	world->cluster_aabbs = NULL;

	Com_Memset( &world->world_aabb, 0, sizeof(aabb_t) );

	vk_rtx_reset_world_geometry( &world->geometry.sky_static );
	vk_rtx_reset_world_geometry( &world->geometry.world_static );
	vk_rtx_reset_world_geometry( &world->geometry.world_dynamic_material );
	vk_rtx_reset_world_geometry( &world->geometry.world_dynamic_geometry );
	vk_rtx_reset_world_geometry( &world->geometry.world_submodels );
#ifdef DEBUG_POLY_LIGHTS 
	vk_rtx_reset_world_geometry( &world->geometry.debug_light_polys );
#endif
	Com_Memset( &world->geometry, 0, sizeof(vkgeometry_t) );
}

static void vk_rtx_init_geometry( vk_geometry_data_t *geom, uint32_t blas_type_flags, uint32_t dynamic_flags, qboolean fast_build, qboolean allow_update )
{
	geom->blas_type_flags	= blas_type_flags;	// ~sunny, unused?
	geom->dynamic_flags		= dynamic_flags;	// ~sunny, unused?
	geom->fast_build		= fast_build;		// ~sunny, unused?
	geom->allow_update		= allow_update;		// ~sunny, unused?

	vkpt_init_model_geometry( &geom->geom_opaque,		1	);
	vkpt_init_model_geometry( &geom->geom_transparent,	1	);
	vkpt_init_model_geometry( &geom->geom_masked,		1	);
}

static void vk_rtx_set_geomertry_accel_offsets( vk_geometry_data_t *geom, int type )
{
	vk_geometry_data_accel_t *accel = &geom->accel[type];

	accel->surf_offset			= geom->host.surf_offset;
}

// ~sunny, needs rework
static void vk_rtx_inject_light_poly_debug( vk_geometry_data_t *geom, world_t& worldData, float offset )
{
#if 0
	vk_geometry_data_accel_t *accel = &geom->accel[BLAS_TYPE_OPAQUE];

	for ( int nlight = 0; nlight < worldData.num_light_polys; nlight++ )
	{
		tess.numVertexes = tess.numIndexes = 0;
		tess.shader->sort	= 10;

		light_poly_t *light = worldData.light_polys + nlight;

		vec3_t a = { light->positions[0], light->positions[1], light->positions[2] };
		vec3_t b = { light->positions[3], light->positions[4], light->positions[5] };
		vec3_t c = { light->positions[6], light->positions[7], light->positions[8] };

		if ( offset > .0f )
		{
			// Get the light plane equation
			vec3_t e0, e1, normal;
			VectorSubtract(b, a, e0);
			VectorSubtract(c, a, e1);
			CrossProduct(e0, e1, normal);
			VectorNormalize(normal);

			for ( int j = 0; j < 3; j++ ) {
				VectorMA(a, offset, normal, a);
				VectorMA(b, offset, normal, b);
				VectorMA(c, offset, normal, c);
			}
		}

		int cluster = BSP_PointLeaf( worldData.nodes, a)->cluster;

		if ( cluster > 0 )
		{
			tess.shader = tr.whiteShader;
		}
		else {
			tess.shader	= tr.whiteShader;
		}

		color4ub_t color = { 255, 255, 0, 255 };
		RB_AddTriangle( a, b, c, color );

		const uint32_t num_clusters = (tess.numIndexes / 3);

		assert( geom->host.xyz_offset + tess.numVertexes <= geom->host.xyz_count );
		assert( geom->host.idx_offset + tess.numIndexes <= geom->host.idx_count );
		assert( geom->host.cluster_offset + num_clusters <= geom->host.cluster_count );

		vk_rtx_bind_indicies( geom->host.idx + geom->host.idx_offset, accel->xyz_count );
		vk_rtx_bind_vertices( geom->host.xyz + geom->host.xyz_offset, cluster );
		vk_rtx_bind_cluster( geom->host.cluster + geom->host.cluster_offset, num_clusters, cluster );

		geom->host.cluster_offset += num_clusters;
		geom->host.idx_offset += tess.numIndexes;
		geom->host.xyz_offset += tess.numVertexes;

		accel->idx_count += tess.numIndexes;
		accel->xyz_count += tess.numVertexes;

		accel->cluster_count += num_clusters;
	}
#endif
}

// ~sunny, re-check this
static void
mark_clusters_with_sky( const vk_geometry_data_t* geom, uint8_t* clusters_with_sky)
{
	uint32_t i;

	for (uint32_t prim_idx = 0; prim_idx < geom->geom_opaque.prim_counts[0]; prim_idx++)
	{
		uint32_t prim = geom->geom_opaque.prim_offsets[0] + prim_idx;

		int cluster = geom->primitives[prim].cluster;

		if ( cluster < VIS_MAX_BYTES )
			clusters_with_sky[cluster >> 3] |= (1 << (cluster & 7));
	}
}

static void
compute_sky_visibility( world_t &worldData )
{
	memset(worldData.sky_visibility, 0, VIS_MAX_BYTES);

	vk_geometry_data_t *sky_static = &worldData.geometry.sky_static;

	if ( sky_static->num_primitives == 0 )
		return; 

	uint32_t numclusters = worldData.numClusters;

	uint8_t clusters_with_sky[VIS_MAX_BYTES] = { 0 };

	mark_clusters_with_sky( sky_static, clusters_with_sky );

	for (uint32_t cluster = 0; cluster < numclusters; cluster++)
	{
		if (clusters_with_sky[cluster >> 3] & (1 << (cluster & 7)))
		{
			byte* mask = BSP_GetPvs( &worldData, (int)cluster);

			for ( int i = 0; i < worldData.clusterBytes; i++ )
				worldData.sky_visibility[i] |= mask[i];
		}
	}
}

#ifdef DEBUG_POLY_LIGHTS 
static void vk_rtx_build_debug_light_poly_mesh( world_t &worldData, vk_geometry_data_t *geom )
{
	geom->host.surf_count = 0;
	geom->host.surf_offset = 0;

	geom->primitives = (VboPrimitive*)calloc( MAX( 1, (geom->num_primitives_allocated )),  sizeof(VboPrimitive) );
	Com_Memset( geom->primitives, 0, MAX( 1, (geom->num_primitives_allocated)) * sizeof(VboPrimitive) );

	VboPrimitive *primitives_out = geom->primitives + 0;

	rtx_material_t *mat = vk_rtx_shader_to_material( tr.shaders[0] );

	for ( int nlight = 0; nlight < geom->num_primitives; nlight++ )
	{
		light_poly_t *light = worldData.light_polys + nlight;

		memset(primitives_out, 0, sizeof(VboPrimitive));

		vec3_t a { light->positions[0], light->positions[1], light->positions[2] };
		vec3_t b { light->positions[3], light->positions[4], light->positions[5] };
		vec3_t c { light->positions[6], light->positions[7], light->positions[8] };

		if ( DEBUG_POLY_LIGHTS > .0f )
		{
			// Get the light plane equation
			vec3_t e0, e1, normal;
			VectorSubtract(b, a, e0);
			VectorSubtract(c, a, e1);
			CrossProduct(e0, e1, normal);
			VectorNormalize(normal);

			for ( int j = 0; j < 3; j++ ) {
				VectorMA(a, DEBUG_POLY_LIGHTS, normal, a);
				VectorMA(b, DEBUG_POLY_LIGHTS, normal, b);
				VectorMA(c, DEBUG_POLY_LIGHTS, normal, c);
			}

			primitives_out->normals[0] = encode_normal( normal );
			primitives_out->normals[1] = encode_normal( normal );
			primitives_out->normals[2] = encode_normal( normal );
		}

		int cluster = BSP_PointLeaf( worldData.nodes, a)->cluster;

		VectorCopy( a, primitives_out->pos0 );
		VectorCopy( b, primitives_out->pos1 );
		VectorCopy( c, primitives_out->pos2 );

		float alpha = 1.f;
		float emissive = 0.f;

		primitives_out->material_id = mat->flags;
		primitives_out->emissive_and_alpha = floatToHalf(emissive) | (floatToHalf(alpha) << 16);
		primitives_out->instance = 0;

		++primitives_out;
	}
}
#endif

void R_PreparePT( world_t &worldData ) 
{
	if ( !vk.rtxActive )
		return;

	uint32_t i, prim_ctr, first_prim;

	// polygonal lights
	worldData.num_light_polys = 0;
	worldData.allocated_light_polys = 0;
	worldData.light_polys = NULL;

	// reset acceleration structures, redundant
	vk_rtx_reset_world_geometries( tr.world );

	vk_geometry_data_t *sky_static				= &worldData.geometry.sky_static;
	vk_geometry_data_t *world_static			= &worldData.geometry.world_static;
	vk_geometry_data_t *world_dynamic_material	= &worldData.geometry.world_dynamic_material;
	vk_geometry_data_t *world_dynamic_geometry	= &worldData.geometry.world_dynamic_geometry;
	vk_geometry_data_t *world_submodels			= &worldData.geometry.world_submodels;
#ifdef DEBUG_POLY_LIGHTS 
	vk_geometry_data_t *debug_light_polys		= &worldData.geometry.debug_light_polys;
#endif

	// initialize .. 								type (unused)			dynamic_flags(unused)		fast_build (unused)  allow_update (unused)	
	vk_rtx_init_geometry( sky_static,				BLAS_TYPE_FLAG_OPAQUE,	BLAS_DYNAMIC_FLAG_NONE,		qtrue,				 qfalse	);
	vk_rtx_init_geometry( world_static,				BLAS_TYPE_FLAG_ALL,		BLAS_DYNAMIC_FLAG_NONE,		qtrue,				 qfalse	);
	vk_rtx_init_geometry( world_dynamic_material,	BLAS_TYPE_FLAG_ALL,		BLAS_DYNAMIC_FLAG_ALL,		qfalse,				 qtrue	);
	vk_rtx_init_geometry( world_dynamic_geometry,	BLAS_TYPE_FLAG_ALL,		BLAS_DYNAMIC_FLAG_ALL,		qfalse,				 qtrue	);
	vk_rtx_init_geometry( world_submodels,			BLAS_TYPE_FLAG_OPAQUE,	BLAS_DYNAMIC_FLAG_NONE,		qtrue,				 qfalse	);
#ifdef DEBUG_POLY_LIGHTS 
	vk_rtx_init_geometry( debug_light_polys,		BLAS_TYPE_FLAG_OPAQUE,	BLAS_DYNAMIC_FLAG_NONE,		qtrue,				 qfalse	);
#endif

	// esitmate size of world geometries
	vk_rtx_estimate_geometry( worldData, worldData.nodes, sky_static,				filter_sky );
	vk_rtx_estimate_geometry( worldData, worldData.nodes, world_static,				filter_static );
	vk_rtx_estimate_geometry( worldData, worldData.nodes, world_dynamic_material,	filter_dynamic_material );
	vk_rtx_estimate_geometry( worldData, worldData.nodes, world_dynamic_geometry,	filter_dynamic_geometry );
	vk_rtx_estimate_bmodels( worldData, world_submodels );

	// Q2RTX, collects these into one buffer, 
	// here use separate buffers, for now (idea for dynamic shaders)

	// sky
	prim_ctr = 0;
	first_prim = prim_ctr;
	vk_rtx_collect_surfaces( &prim_ctr, sky_static, BLAS_TYPE_OPAQUE, worldData, worldData.nodes, -1, filter_sky, filter_opaque );
		vkpt_append_model_geometry(&sky_static->geom_opaque, prim_ctr - first_prim, first_prim, "sky opqaue");
	sky_static->num_primitives = prim_ctr;

	// opaque
	prim_ctr = 0;
	first_prim = prim_ctr;
	vk_rtx_collect_surfaces( &prim_ctr, world_static, BLAS_TYPE_OPAQUE, worldData, worldData.nodes, -1, filter_static, filter_opaque );
		vkpt_append_model_geometry(&world_static->geom_opaque, prim_ctr - first_prim, first_prim, "bsp static opaque");
	vk_rtx_set_geomertry_accel_offsets( world_static, BLAS_TYPE_TRANSPARENT );
	first_prim = prim_ctr;
	vk_rtx_collect_surfaces( &prim_ctr, world_static, BLAS_TYPE_TRANSPARENT, worldData, worldData.nodes, -1, filter_static, filter_transparent );
		vkpt_append_model_geometry(&world_static->geom_transparent, prim_ctr - first_prim, first_prim, "bsp static transparent");
	world_static->num_primitives = prim_ctr;


	// dynamic material
	prim_ctr = 0;
	first_prim = prim_ctr;
	vk_rtx_collect_surfaces( &prim_ctr, world_dynamic_material, BLAS_TYPE_OPAQUE, worldData, worldData.nodes, -1, filter_dynamic_material, filter_opaque );
		vkpt_append_model_geometry(&world_dynamic_material->geom_opaque, prim_ctr - first_prim, first_prim, "bsp d material opaque");
	vk_rtx_set_geomertry_accel_offsets( world_dynamic_material, BLAS_TYPE_TRANSPARENT );
	first_prim = prim_ctr;
	vk_rtx_collect_surfaces( &prim_ctr, world_dynamic_material, BLAS_TYPE_TRANSPARENT, worldData, worldData.nodes, -1, filter_dynamic_material, filter_transparent );
		vkpt_append_model_geometry(&world_dynamic_material->geom_transparent, prim_ctr - first_prim, first_prim, "bsp d material transparent");
	world_dynamic_material->num_primitives = prim_ctr;


	// dynamic geometry
	prim_ctr = 0;
	first_prim = prim_ctr;
	vk_rtx_collect_surfaces( &prim_ctr, world_dynamic_geometry, BLAS_TYPE_OPAQUE, worldData, worldData.nodes, -1, filter_dynamic_geometry, filter_opaque );
		vkpt_append_model_geometry(&world_dynamic_geometry->geom_opaque, prim_ctr - first_prim, first_prim, "bsp d geometry opaque");
	vk_rtx_set_geomertry_accel_offsets( world_dynamic_geometry, BLAS_TYPE_TRANSPARENT );
	first_prim = prim_ctr;
	vk_rtx_collect_surfaces( &prim_ctr, world_dynamic_geometry, BLAS_TYPE_TRANSPARENT, worldData, worldData.nodes, -1, filter_dynamic_geometry, filter_transparent );
		vkpt_append_model_geometry(&world_dynamic_geometry->geom_transparent, prim_ctr - first_prim, first_prim, "bsp d geomatry transparent");
	world_dynamic_geometry->num_primitives = prim_ctr;
	

	// sub brush models
	prim_ctr = 0;
	for ( i = 0; i < worldData.num_bmodels; i++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[i];
		first_prim = prim_ctr;
		vk_rtx_collect_bmodel_surfaces( &prim_ctr, worldData, world_submodels, BLAS_TYPE_OPAQUE, bmodel );
		vkpt_init_model_geometry(&bmodel->geometry, 1);
		vkpt_append_model_geometry( &bmodel->geometry, prim_ctr - first_prim, first_prim, "bsp_model");
	}
	world_submodels->num_primitives = prim_ctr;

	build_pvs2( &worldData );

	for ( i = 0; i < worldData.num_bmodels; i++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[i];

		// ~sunny, recheck this
		compute_aabb( world_submodels->primitives + bmodel->geometry.prim_offsets[0], bmodel->geometry.prim_counts[0], bmodel->aabb_min, bmodel->aabb_max);
		VectorAdd( bmodel->aabb_min, bmodel->aabb_max, bmodel->center );
		VectorScale( bmodel->center, 0.5f, bmodel->center );
	}

	compute_aabb( world_static->primitives + world_static->geom_opaque.prim_offsets[0],			world_static->geom_opaque.prim_counts[0],		worldData.world_aabb.mins, worldData.world_aabb.maxs );
	append_aabb(  world_static->primitives + world_static->geom_transparent.prim_offsets[0],	world_static->geom_transparent.prim_counts[0],	worldData.world_aabb.mins, worldData.world_aabb.maxs );
	append_aabb(  world_static->primitives + world_static->geom_masked.prim_offsets[0],			world_static->geom_masked.prim_counts[0],		worldData.world_aabb.mins, worldData.world_aabb.maxs );

	vec3_t margin = { 1.f, 1.f, 1.f };
	VectorSubtract( worldData.world_aabb.mins, margin, worldData.world_aabb.mins );
	VectorAdd( worldData.world_aabb.maxs, margin, worldData.world_aabb.maxs );

	vk_compute_cluster_aabbs( worldData );

	collect_light_polys( worldData, -1, &worldData.num_light_polys, &worldData.allocated_light_polys, &worldData.light_polys );

#ifdef DEBUG_POLY_LIGHTS
	debug_light_polys->num_primitives = worldData.num_light_polys;
	debug_light_polys->num_primitives_allocated = worldData.num_light_polys;
	vk_rtx_build_debug_light_poly_mesh( worldData, debug_light_polys );
	vkpt_append_model_geometry( &debug_light_polys->geom_opaque, worldData.num_light_polys, 0, "bsp_model");
#endif

	for ( i = 0; i < worldData.num_bmodels; i++ ) 
	{
		bmodel_t *bmodel = &worldData.bmodels[i];

		bmodel->num_light_polys = 0;
		bmodel->allocated_light_polys = 0;
		bmodel->light_polys = NULL;

		collect_light_polys( worldData, i, &bmodel->num_light_polys, &bmodel->allocated_light_polys, &bmodel->light_polys );
	
		bmodel->transparent = false; // is_model_transparent(wm, model);
		bmodel->masked = false; // is_model_masked(wm, model);
		Com_Printf("num light polys : %d", bmodel->num_light_polys );
	}

#ifdef DEBUG_POLY_LIGHTS
	vk_rtx_inject_light_poly_debug( world_static, worldData, DEBUG_POLY_LIGHTS );
#endif

	collect_cluster_lights( worldData );

	compute_sky_visibility( worldData );

	vkpt_light_buffers_create( worldData  );

	// create buffers and upload and create blas
	vkpt_vertex_buffer_upload_bsp_mesh( worldData );

	vk_rtx_prepare_envmap( worldData );

	vkpt_physical_sky_initialize();

	vk_rtx_create_primary_rays_resources( worldData );
}