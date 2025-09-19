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

#define PlaneDiff(v,p)   (DotProduct(v,(p)->normal)-(p)->dist)

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

#define Q_IsBitSet(data, bit)   (((data)[(bit) >> 3] & (1 << ((bit) & 7))) != 0)
#define Q_SetBit(data, bit)     ((data)[(bit) >> 3] |= (1 << ((bit) & 7)))
#define Q_ClearBit(data, bit)   ((data)[(bit) >> 3] &= ~(1 << ((bit) & 7)))

static void merge_pvs_rows( world_t* world, char* src, char* dst )
{
	for (int i = 0; i < world->clusterBytes; i++)
	{
		dst[i] |= src[i];
	}
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

void vk_compute_cluster_aabbs( mnode_t* node, int numClusters ) 
{
	if (node->contents == -1) {
		vk_compute_cluster_aabbs(node->children[0], numClusters);
		vk_compute_cluster_aabbs(node->children[1], numClusters);
		return;
	}

	if ( node->cluster < 0 || node->cluster >= numClusters )
		return;

	aabb_t *aabb = vk.cluster_aabbs + node->cluster;

	aabb->mins[0] = MIN(aabb->mins[0], node->mins[0]);
	aabb->mins[1] = MIN(aabb->mins[1], node->mins[1]);
	aabb->mins[2] = MIN(aabb->mins[2], node->mins[2]);

	aabb->maxs[0] = MAX(aabb->maxs[0], node->maxs[0]);
	aabb->maxs[1] = MAX(aabb->maxs[1], node->maxs[1]);
	aabb->maxs[2] = MAX(aabb->maxs[2], node->maxs[2]);

}

static void RB_UploadCluster( vkbuffer_t *buffer, uint32_t offset, int cluster ) 
{
	uint32_t i;
	const int num_clusters = tess.numIndexes / 3;

	uint32_t *clusterData = (uint32_t*)calloc( num_clusters, sizeof(uint32_t) );

	for ( i = 0; i < num_clusters; i++ ) 
	{
		clusterData[i] = cluster;
	}

	vk_rtx_upload_buffer_data_offset( buffer, offset * sizeof(uint32_t), num_clusters * sizeof(uint32_t), (const byte*)clusterData );
	free(clusterData);
}

static void vk_bind_storage_buffer( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkBuffer buffer )
{
	const uint32_t count = 1;

	assert( buffer != NULL );

	vk_rtx_add_descriptor_buffer( descriptor, count, binding, stage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
	vk_rtx_set_descriptor_update_size( descriptor, binding, stage, count );
	vk_rtx_bind_descriptor_buffer( descriptor, binding, stage, buffer );
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

static void vk_create_rt_descriptor( uint32_t index, uint32_t prev_index ) 
{
	vkdescriptor_t *descriptor = &vk.rt_descriptor_set[index];

	vk_rtx_add_descriptor_as( descriptor, RAY_GEN_DESCRIPTOR_SET_IDX, VK_SHADER_STAGE_RAYGEN_BIT_KHR );
	vk_rtx_bind_descriptor_as( descriptor, RAY_GEN_DESCRIPTOR_SET_IDX, VK_SHADER_STAGE_RAYGEN_BIT_KHR, &vk.tlas_geometry[index].accel );

	vk_rtx_create_descriptor( descriptor );
	vk_rtx_update_descriptor( descriptor );
}

static void vk_create_vertex_buffer_descriptor( world_t& worldData, uint32_t index, uint32_t prev_index ) 
{	
	vkdescriptor_t *descriptor = &vk.desc_set_vertex_buffer[index] ;

	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_SKY,						VK_SHADER_STAGE_ALL, worldData.geometry.sky_static.xyz[0].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_SKY,						VK_SHADER_STAGE_ALL, worldData.geometry.sky_static.idx[0].buffer );


	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_STATIC,			VK_SHADER_STAGE_ALL, worldData.geometry.world_static.xyz[0].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_STATIC,			VK_SHADER_STAGE_ALL, worldData.geometry.world_static.idx[0].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA,		VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_material.xyz[index].buffer );	// index
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA,		VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_material.idx[index].buffer );	// index
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS,		VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_geometry.xyz[index].buffer );	// index
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS,		VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_geometry.idx[index].buffer );	// index

	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_SKY,					VK_SHADER_STAGE_ALL, worldData.geometry.sky_static.cluster[0].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_WORLD_STATIC,		VK_SHADER_STAGE_ALL, worldData.geometry.world_static.cluster[0].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_DATA,	VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_material.cluster[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_AS,	VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_geometry.cluster[index].buffer );

	// previous
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA_PREV, VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_material.xyz[prev_index].buffer ); // prev_index
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA_PREV, VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_material.idx[prev_index].buffer ); // prev_index
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS_PREV,	VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_geometry.xyz[prev_index].buffer ); // prev_index
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS_PREV,	VK_SHADER_STAGE_ALL, worldData.geometry.world_dynamic_geometry.idx[prev_index].buffer ); // prev_index

	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_READBACK_BUFFER,				VK_SHADER_STAGE_ALL, vk.buf_readback.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_DYNAMIC_VERTEX,				VK_SHADER_STAGE_ALL, vk.model_instance.buffer_vertex.buffer );
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

static void vk_create_primary_rays_pipelines( world_t& worldData ) 
{
	uint32_t i, prev_index;

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		prev_index = (i + (vk.swapchain_image_count - 1)) % vk.swapchain_image_count;

		vk_create_rt_descriptor( i, prev_index );
		vk_create_vertex_buffer_descriptor( worldData, i, prev_index );
	}

	vk_rtx_create_shader_modules();
	vk_rtx_create_rt_pipelines();
	vk_rtx_create_compute_pipelines();
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

// Computes a point at a small distance above the center of the triangle.
// Returns qfalse if the triangle is degenerate, qtrue otherwise.
qboolean 
get_triangle_off_center(const float* positions, float* center, float* anti_center)
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

	VectorAdd(center, normal, center);

	if (anti_center)
	{
		VectorMA(center, -2.f, normal, anti_center);
	}

	return (qboolean)(length > 0.f);
}

static bool
get_surf_plane_equation(srfSurfaceFace_t *surf , float* plane )
{
	// Go over multiple planes defined by different edge pairs of the surface.
	// Some of the edges may be collinear or almost-collinear to each other,
	// so we can't just take the first pair of edges.
	// We can't even take the first pair of edges with nonzero cross product
	// because of numerical issues - they may be almost-collinear.
	// So this function finds the plane equation from the triangle with the
	// largest area, i.e. the longest cross product.
	int i0, i1, i2;
	int *indices = ( (int*)( (byte*)surf + surf->ofsIndices ) );

	float maxlen = 0.f;
	for ( int  i = 0; i < surf->numIndices; i += 3 )
	{
		// mostly likely incorrect,dot it later
		i0 = indices[i + 0];
		i1 = indices[i + 1];
		i2 = indices[i + 2];

		if ( i0 >= surf->numPoints || i1 >= surf->numPoints || i2 >= surf->numPoints )
			continue;

		float* v0 = surf->points[i0];
		float* v1 = surf->points[i1];
		float* v2 = surf->points[i2];

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

static void collect_one_light_poly_entire_texture(  world_t &worldData, srfSurfaceFace_t *surf, shader_t *shader, int model_idx, 
													const vec3_t light_color, float emissive_factor, int light_style,
													int* num_lights, int* allocated_lights, light_poly_t** lights )
{
	int *indices = (int *)((byte *)surf + surf->ofsIndices);
	float *v0, *v1, *v2;
	int i, i0, i1, i2;
	vec3_t e0, e1, normal;

	for ( i = 0; i < surf->numIndices; i += 3 )
	{
		i0 = indices[i + 0];
		i1 = indices[i + 1];
		i2 = indices[i + 2];

		if ( i0 >= surf->numPoints || i1 >= surf->numPoints || i2 >= surf->numPoints )
			continue;

		v0 = surf->points[i0];
		v1 = surf->points[i1];
		v2 = surf->points[i2];

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

		light.material = (void *)vk_rtx_get_material( shader->index );
		light.style = light_style;
		light.type = LIGHT_POLYGON;
		light.emissive_factor = emissive_factor;

		if ( !get_triangle_off_center( light.positions, light.off_center, NULL ) )
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

static void collect_one_light_poly(
	world_t &worldData,
	srfSurfaceFace_t *surf,
	shader_t *shader,
	int model_idx,
	const vec2_t min_light_texcoord,
	const vec2_t max_light_texcoord,
	const vec3_t light_color,
	float emissive_factor,
	int light_style,
	int* num_lights,
	int* allocated_lights,
	light_poly_t** lights
) {
	float *v0, *v1, *v2;
	int i, j, k, i0, i1, i2, axis;
	int tile_u, tile_v, min_tile_u, max_tile_u, min_tile_v, max_tile_v;
	vec2_t uv0, uv1, uv2, tri_min_uv, tri_max_uv;
	vec2_t min_tc, max_tc;
	vec3_t e0, e1, normal, weights;
	vec3_t tri_positions[3], instance_positions[MAX_POLY_VERTS];
	rt_poly_t tri_uv_poly, clipper, clipped;
	
	int *indices = (int *)((byte *)surf + surf->ofsIndices);

	for ( i = 0; i < surf->numIndices; i += 3 ) 
	{
		i0 = indices[i + 0];
		i1 = indices[i + 1];
		i2 = indices[i + 2];

		if ( i0 >= surf->numPoints || i1 >= surf->numPoints || i2 >= surf->numPoints )
			continue;

		v0 = surf->points[i0];
		v1 = surf->points[i1];
		v2 = surf->points[i2];

		VectorSet2( uv0, v0[6], v0[7] );
		VectorSet2( uv1, v1[6], v1[7] );
		VectorSet2( uv2, v2[6], v2[7] );

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
					light->material = (void *)vk_rtx_get_material( shader->index );
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

					get_triangle_off_center( light->positions, light->off_center, NULL );
	
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
collect_frames_emissive_info( shader_t *shader, bool* entire_texture_emissive, vec2_t min_light_texcoord, vec2_t max_light_texcoord, vec3_t light_color )
{
	*entire_texture_emissive = false;
	min_light_texcoord[0] = min_light_texcoord[1] = 1.0f;
	max_light_texcoord[0] = max_light_texcoord[1] = 0.0f;

	bool any_emissive_valid = false;

	uint32_t emissive_image_index = vk_rtx_find_emissive_texture( shader );

	if ( !emissive_image_index )
		return false;

	image_t *image = tr.images[emissive_image_index];

	if ( !image )
		return false;

	if(!any_emissive_valid)
	{
		// emissive light color of first frame
		memcpy(light_color, image->light_color, sizeof(vec3_t));
		//VectorSet( light_color, 255.0, 255.0, 255.0 );
	}
	any_emissive_valid = true;

	*entire_texture_emissive |= image->entire_texture_emissive;
	min_light_texcoord[0] = MIN(min_light_texcoord[0], image->min_light_texcoord[0]);
	min_light_texcoord[1] = MIN(min_light_texcoord[1], image->min_light_texcoord[1]);
	max_light_texcoord[0] = MAX(max_light_texcoord[0], image->max_light_texcoord[0]);
	max_light_texcoord[1] = MAX(max_light_texcoord[1], image->max_light_texcoord[1]);

	return any_emissive_valid;
}

static void collect_light_polys( world_t &worldData, int model_idx, int* num_lights, int* allocated_lights, light_poly_t** lights )
{
	srfSurfaceFace_t *surf;
	msurface_t *sf;
	uint32_t i;

	for ( i = 0, sf = &worldData.surfaces[0]; i < worldData.numsurfaces; i++, sf++ ) 
	{
		surf = (srfSurfaceFace_t *)sf->data;
		if ( surf->surfaceType != SF_FACE )
			continue;

		bool entire_texture_emissive;
		vec2_t min_light_texcoord;
		vec2_t max_light_texcoord;
		vec3_t light_color;

		if ( !collect_frames_emissive_info( sf->shader, &entire_texture_emissive, min_light_texcoord, max_light_texcoord, light_color ) )
		{
			// This algorithm relies on information from the emissive texture,
			// specifically the extents of the emissive pixels in that texture.
			// Ignore surfaces that don't have an emissive texture attached.
			continue;
		}

		float emissive_factor = 4.0f;
		int light_style = 0;

		if ( entire_texture_emissive )
		{
			collect_one_light_poly_entire_texture(  worldData, surf, sf->shader, model_idx, light_color, emissive_factor, light_style,
													num_lights, allocated_lights, lights );
			continue;
		}

		vec4_t plane;
		if ( !get_surf_plane_equation( surf, plane ) )
		{
			// It's possible that some polygons in the game are degenerate, ignore these.
			continue;
		}

		collect_one_light_poly( worldData, surf, sf->shader, model_idx,
							   min_light_texcoord, max_light_texcoord,
							   light_color, emissive_factor, light_style,
							   num_lights, allocated_lights, lights );
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
	int *cluster_lights = (int *)Z_Malloc( MAX_LIGHTS_PER_CLUSTER * vk.numClusters * sizeof(int), TAG_GENERAL, qfalse );
	int *cluster_light_counts = (int *)Z_Malloc( vk.numClusters * sizeof(int), TAG_GENERAL, qtrue );

	// Construct an array of visible lights for each cluster.
	// The array is in `cluster_lights`, with MAX_LIGHTS_PER_CLUSTER stride.

	for ( int nlight = 0; nlight < worldData.num_light_polys; nlight++ )
	{
		light_poly_t *light = worldData.light_polys + nlight;

		if ( light->cluster < 0 )
			continue;

		const byte *pvs = (const byte*)BSP_GetPvs( &worldData, light->cluster);

		FOREACH_BIT_BEGIN( pvs, vk.clusterBytes, other_cluster )
			aabb_t *cluster_aabb = vk.cluster_aabbs + other_cluster;
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
	for (int cluster = 0; cluster < vk.numClusters; cluster++)
	{
		worldData.num_cluster_lights += cluster_light_counts[cluster];
	}

	worldData.cluster_lights = (int*)Z_Malloc(worldData.num_cluster_lights * sizeof(int), TAG_GENERAL, qtrue );
	worldData.cluster_light_offsets = (int*)Z_Malloc((vk.numClusters + 1) * sizeof(int), TAG_GENERAL, qtrue );

	Com_Printf( S_COLOR_MAGENTA "\n\n\nTotal interactions: %d,\n\n", worldData.num_cluster_lights );

	// Compact the previously constructed array into worldData.cluster_lights
	int list_offset = 0;
	for ( int cluster = 0; cluster < vk.numClusters; cluster++ )
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
	worldData.cluster_light_offsets[vk.numClusters] = list_offset;

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
	qboolean is_sky = (shader->isSky || (shader->surfaceFlags & SURF_SKY) ) ? qtrue : qfalse;

	if ( !as_dynamic && !as_dynamic_data && !is_sky )
		return 1;

	return 0;
}

static int filter_dynamic_geometry(shader_t *shader)
{
    qboolean is_geom_dynamic = RB_IsDynamicGeometry(shader);
    qboolean is_mat_dynamic = RB_IsDynamicMaterial(shader);
    qboolean is_sky = (shader->isSky || (shader->surfaceFlags & SURF_SKY)) ? qtrue : qfalse;

    if (is_geom_dynamic && !is_mat_dynamic)
        return 1;

    return 0;
}

static int filter_dynamic_material( shader_t *shader )
{
    qboolean is_geom_dynamic = RB_IsDynamicGeometry(shader);
    qboolean is_mat_dynamic = RB_IsDynamicMaterial(shader);
    qboolean is_sky = (shader->isSky || (shader->surfaceFlags & SURF_SKY)) ? qtrue : qfalse;

    if (!is_geom_dynamic && is_mat_dynamic)
        return 1;

    return 0;
}

static int filter_sky( shader_t *shader )
{
	qboolean is_sky = (shader->isSky || (shader->surfaceFlags & SURF_SKY) ) ? qtrue : qfalse;

	if ( !is_sky )
		return 0;

	return 1;
}

static int skip_invalid_surface( shader_t *shader )
{
	if (shader->isSky || (shader->surfaceFlags & SURF_SKY) )
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
		for ( uint32_t i = 0; i < 2; i++ ) 
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

		geom->host.idx_count += tess.numIndexes;
		geom->host.xyz_count += tess.numVertexes;
		geom->host.cluster_count += num_clusters;
		geom->host.surf_count++;
	}
}

static void vk_rtx_estimate_geometry( world_t &worldData, mnode_t *node, vk_geometry_data_t *geom, int (*filter)(shader_t*) )
{ 
	// should already be done in vk_rtx_reset_world_geometry() with memset
	geom->host.idx_count = 0;
	geom->host.xyz_count = 0;
	geom->host.cluster_count = 0;
	geom->host.xyz_offset = 0;
	geom->host.xyz_offset = 0;
	geom->host.cluster_offset = 0;

	vk_rtx_estimate_geometry_recursive( worldData, node, geom, filter );

	geom->host.idx		= (uint32_t*)calloc( geom->host.idx_count,  sizeof(uint32_t) );
	geom->host.xyz		= (VertexBuffer*)calloc( geom->host.xyz_count, sizeof(VertexBuffer) );
	geom->host.cluster	= (uint32_t*)calloc( MAX( 1, (geom->host.idx_count / 3)),  sizeof(uint32_t) );

	if ( geom->dynamic_flags )
	{
		geom->host.dynamic_surfs = (vk_geometry_dynamic_surf_t*)calloc( geom->host.surf_count, sizeof(vk_geometry_dynamic_surf_t) );
	}
}

static void vk_rtx_collect_surfaces( vk_geometry_data_t *geom, int type, world_t &worldData, mnode_t *node, 
	int (*filter)(shader_t*), int (*filter_visibiliy)(shader_t*) )
{
	uint32_t	i, j;
	msurface_t	*surf, **mark;

	if ( node->contents == -1 ) 
	{
		for ( uint32_t i = 0; i < 2; i++ ) 
		{
			vk_rtx_collect_surfaces( geom, type, worldData, node->children[i], filter, filter_visibiliy );
		}
	}

	vk_geometry_data_accel_t *accel = &geom->accel[type];

	mark = node->firstmarksurface;

	for ( j = 0; j < node->nummarksurfaces; j++ ) 
	{
		surf = mark[j];
		surf->notBrush = qtrue;

		shader_t* shader = tr.shaders[surf->shader->index];

		if ( surf->added || surf->skip )	
			continue;

		if ( !filter_visibiliy( shader ) )
			continue;

		if ( !filter( shader ) )
			continue;

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

		// add surface to host buffer
		const uint32_t num_clusters = (tess.numIndexes / 3);

		assert( geom->host.xyz_offset + tess.numVertexes <= geom->host.xyz_count );
		assert( geom->host.idx_offset + tess.numIndexes <= geom->host.idx_count );
		assert( geom->host.cluster_offset + num_clusters <= geom->host.cluster_count );
		
		vk_rtx_bind_indicies( geom->host.idx + geom->host.idx_offset, accel->xyz_count );
		vk_rtx_bind_vertices( geom->host.xyz + geom->host.xyz_offset, node->cluster );
		vk_rtx_bind_cluster( geom->host.cluster + geom->host.cluster_offset, num_clusters, node->cluster );

		//RB_UploadCluster( &geom->cluster, geom->cluster_offset, node->cluster );
		geom->host.cluster_offset += num_clusters;
		geom->host.idx_offset += tess.numIndexes;
		geom->host.xyz_offset += tess.numVertexes;

		accel->idx_count += tess.numIndexes;
		accel->xyz_count += tess.numVertexes;
		accel->cluster_count += num_clusters;

		tess.numVertexes = tess.numIndexes = 0;

		surf->added = qtrue;
	}	
}

void vk_rtx_update_dynamic_geometry( VkCommandBuffer cmd_buf, vk_geometry_data_t *geom ) 
{
	uint32_t i, type;

	if ( !geom->dynamic_flags )
		return;

	const int frame_idx = vk.frame_counter & 1;

	uint32_t idx_count = 0;
	uint32_t xyz_count = 0;
	uint32_t cluster_count = 0;

	// reset
	geom->host.idx_offset = 0;
	geom->host.xyz_offset = 0;
	geom->host.cluster_offset = 0;

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

			tess.numVertexes = tess.numIndexes = 0;
		}

		idx_count += accel->idx_count;
		xyz_count += accel->xyz_count;
		cluster_count += accel->cluster_count;

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
}

static void vk_rtx_reset_world_geometry( vk_geometry_data_t *geom )
{
	uint32_t i, j;

	// host
	if ( geom->host.idx )
		free( geom->host.idx );

	if ( geom->host.xyz )
		free( geom->host.xyz );

	if ( geom->host.dynamic_surfs )
		free( geom->host.dynamic_surfs );

	if ( geom->host.dynamic_surfs )
		free( geom->host.cluster );

	Com_Memset( &geom->host, 0, sizeof(vk_geometry_host_t) );
	
	// blas
	for ( i = 0; i < BLAS_TYPE_COUNT; i++ )
	{
		vk_rtx_buffer_destroy( &geom->idx[i] );
		vk_rtx_buffer_destroy( &geom->xyz[i] );
		vk_rtx_buffer_destroy( &geom->cluster[i] );

		for ( j = 0; j < vk.swapchain_image_count; j++ ) 
		{

			vk_rtx_destroy_blas( &geom->accel[i].blas[j] );
		}

		Com_Memset( &geom->accel[i], 0, sizeof(vk_geometry_data_accel_t) );
	}
}

void vk_rtx_reset_world_geometries( world_t &worldData ) 
{
	uint32_t i, j;

	vkgeometry_t *geometry = &worldData.geometry;

	vk_rtx_reset_world_geometry( &geometry->sky_static );
	vk_rtx_reset_world_geometry( &geometry->world_static );
	vk_rtx_reset_world_geometry( &geometry->world_dynamic_material );
	vk_rtx_reset_world_geometry( &geometry->world_dynamic_geometry );
}

void vk_rtx_init_geometry( vk_geometry_data_t *geom, uint32_t blas_type_flags, uint32_t dynamic_flags, qboolean fast_build, qboolean allow_update, qboolean instanced )
{
	geom->blas_type_flags	= blas_type_flags;
	geom->dynamic_flags		= dynamic_flags;
	geom->fast_build		= fast_build;
	geom->allow_update		= allow_update;
	geom->instanced			= instanced;
}

static void vk_rtx_set_geomertry_accel_offsets( vk_geometry_data_t *geom, int type )
{
	vk_geometry_data_accel_t *accel = &geom->accel[type];

	accel->idx_offset		= geom->host.idx_offset;
	accel->xyz_offset		= geom->host.xyz_offset;
	accel->surf_offset		= geom->host.surf_offset;
	accel->cluster_offset	= geom->host.cluster_offset;
}

//#define AS_BUFFER_FLAGS (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)

#define AS_VERTEX_BUFFER_FLAGS ( \
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | \
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | \
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | \
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR )

#define AS_INDEX_BUFFER_FLAGS ( \
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | \
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | \
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | \
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR )

static void vk_rtx_build_geometry_buffer( vk_geometry_data_t *geom ) 
{
	uint32_t i, type;
	uint32_t idx_count = 0;
	uint32_t xyz_count = 0;
	uint32_t cluster_count = 0;
	const uint32_t offset = 0 ;

	for ( type = 0; type < BLAS_TYPE_COUNT; ++type ) 
	{
		idx_count		+= geom->accel[type].idx_count;
		xyz_count		+= geom->accel[type].xyz_count;
		cluster_count	+= geom->accel[type].cluster_count;
	}

	// dynamic geometries overallocation
	uint32_t alloc_idx_count		= idx_count;
	uint32_t alloc_xyz_count		= xyz_count;
	uint32_t alloc_cluster_count	= cluster_count;
	if ( geom->dynamic_flags & BLAS_DYNAMIC_FLAG_ALL )
	{
		alloc_idx_count *= 2;
		alloc_xyz_count *= 2;
		alloc_cluster_count *= 2;
	}

	uint32_t num_instances = geom->dynamic_flags ? NUM_COMMAND_BUFFERS : 1;

	for ( i = 0; i < num_instances; ++i )
	{
		VK_CreateAttributeBuffer( &geom->idx[i], MAX( 1, alloc_idx_count ) * sizeof(uint32_t), (VkBufferUsageFlagBits) AS_INDEX_BUFFER_FLAGS );
		VK_CreateAttributeBuffer( &geom->xyz[i], MAX( 1, alloc_xyz_count ) * sizeof(VertexBuffer), (VkBufferUsageFlagBits)AS_VERTEX_BUFFER_FLAGS );
		VK_CreateAttributeBuffer( &geom->cluster[i], MAX( 1, alloc_cluster_count ) * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT );

		if ( idx_count <= 0 || xyz_count <= 0 )
			continue;

		vk_rtx_upload_buffer_data_offset( &geom->idx[i], 0, idx_count * sizeof(uint32_t), (const byte*)geom->host.idx + offset * sizeof(uint32_t) );
		vk_rtx_upload_buffer_data_offset( &geom->xyz[i], 0, xyz_count * sizeof(VertexBuffer), (const byte*)geom->host.xyz + offset * sizeof(VertexBuffer) );
		vk_rtx_upload_buffer_data_offset( &geom->cluster[i], 0, cluster_count * sizeof(uint32_t), (const byte*)geom->host.cluster + offset * sizeof(uint32_t) );
	}
}

void vk_rtx_debug_geom( vk_geometry_data_t *geom )
{
	uint32_t type;

	for (type = 0; type < BLAS_TYPE_COUNT; ++type)
	{
		if ( !(geom->blas_type_flags & (1 << type) ) )
			continue;

		vk_geometry_data_accel_t *accel = &geom->accel[type];

		Com_Printf("type:%d: idx_cnt: %d xyz_cnt: %d, cluster_cnt: %d, idx_offset: %d, xyz_offset: %d, cluster_offset: %d\n", 
			type,
			accel->idx_count,
			accel->xyz_count,
			accel->cluster_count,
			accel->idx_offset,
			accel->xyz_offset,
			accel->cluster_offset
		);
	}
}

static void vk_rtx_inject_light_poly_debug( vk_geometry_data_t *geom, world_t& worldData, float offset )
{
	tess.shader			= tr.whiteShader;
	tess.shader->sort	= 10;

	tess.numVertexes = tess.numIndexes = 0;
	for ( int nlight = 0; nlight < worldData.num_light_polys; nlight++ )
	{
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

		color4ub_t color = { 255, 255, 255, 255 };
		RB_AddTriangle( a, b, c, color );
	}

	vk_geometry_data_accel_t *accel = &geom->accel[BLAS_TYPE_OPAQUE];
	const uint32_t num_clusters = (tess.numIndexes / 3);

	assert( geom->host.xyz_offset + tess.numVertexes <= geom->host.xyz_count );
	assert( geom->host.idx_offset + tess.numIndexes <= geom->host.idx_count );
	assert( geom->host.cluster_offset + num_clusters <= geom->host.cluster_count );

	vk_rtx_bind_indicies( geom->host.idx + geom->host.idx_offset, accel->xyz_count );
	vk_rtx_bind_vertices( geom->host.xyz + geom->host.xyz_offset, 0 );
	vk_rtx_bind_cluster( geom->host.cluster + geom->host.cluster_offset, num_clusters, 0 );

	//RB_UploadCluster( &geom->cluster, geom->cluster_offset, 0 );
	geom->host.cluster_offset += num_clusters;
	geom->host.idx_offset += tess.numIndexes;
	geom->host.xyz_offset += tess.numVertexes;

	accel->idx_count += tess.numIndexes;
	accel->xyz_count += tess.numVertexes;
	accel->xyz_count += tess.numVertexes;
	accel->cluster_count += num_clusters;
	
	tess.numVertexes = tess.numIndexes = 0;
}

void R_PreparePT( world_t &worldData ) 
{
	if ( !vk.rtxActive )
		return;

	uint32_t i;

	build_pvs2( &worldData );

	vk.numFixedCluster	= worldData.numClusters;
	vk.numClusters		= worldData.numClusters;
	vk.numMaxClusters	= worldData.numClusters * 3;
	vk.clusterBytes		= worldData.clusterBytes;
	
	vk.vis				= (const byte*)calloc(vk.numMaxClusters, sizeof(byte) * worldData.clusterBytes);
	memcpy((void*)vk.vis, worldData.vis, worldData.numClusters * sizeof(byte) * worldData.clusterBytes);
	
	vk.cluster_aabbs		= (aabb_t*)calloc(worldData.numClusters, sizeof(aabb_t));
	for ( i = 0; i < worldData.numClusters; i++ ) 
	{
		VectorSet(vk.cluster_aabbs[i].mins, FLT_MAX, FLT_MAX, FLT_MAX );
		VectorSet(vk.cluster_aabbs[i].maxs, -FLT_MAX, -FLT_MAX, -FLT_MAX );
	}

	vk_compute_cluster_aabbs( worldData.nodes, worldData.numClusters );

	// polygonal lights
	worldData.num_light_polys = 0;
	worldData.allocated_light_polys = 0;
	worldData.light_polys = NULL;

	collect_light_polys( worldData, -1, &worldData.num_light_polys, &worldData.allocated_light_polys, &worldData.light_polys );

	collect_cluster_lights( worldData );
	vkpt_light_buffers_create( worldData  );

	// reset acceleration structures, redundant
	vk_rtx_reset_world_geometries( worldData );

	vk_geometry_data_t *sky_static				= &worldData.geometry.sky_static;
	vk_geometry_data_t *world_static			= &worldData.geometry.world_static;
	vk_geometry_data_t *world_dynamic_material	= &worldData.geometry.world_dynamic_material;
	vk_geometry_data_t *world_dynamic_geometry	= &worldData.geometry.world_dynamic_geometry;

	// initialize .. 													dynamic_flags					fast_build  allow_update	instanced	
	vk_rtx_init_geometry( sky_static,				BLAS_TYPE_FLAG_OPAQUE,	BLAS_DYNAMIC_FLAG_NONE,		qtrue,		qfalse,			qfalse );
	vk_rtx_init_geometry( world_static,				BLAS_TYPE_FLAG_ALL,		BLAS_DYNAMIC_FLAG_NONE,		qtrue,		qfalse,			qfalse );
	vk_rtx_init_geometry( world_dynamic_material,	BLAS_TYPE_FLAG_ALL,		BLAS_DYNAMIC_FLAG_ALL,		qfalse,		qtrue,			qfalse );
	vk_rtx_init_geometry( world_dynamic_geometry,	BLAS_TYPE_FLAG_ALL,		BLAS_DYNAMIC_FLAG_ALL,		qfalse,		qtrue,			qfalse );

	// esitmate size of world geometries
	vk_rtx_estimate_geometry( worldData, worldData.nodes, sky_static,				filter_sky );
	vk_rtx_estimate_geometry( worldData, worldData.nodes, world_static,				filter_static );
	vk_rtx_estimate_geometry( worldData, worldData.nodes, world_dynamic_material,	filter_dynamic_material );
	vk_rtx_estimate_geometry( worldData, worldData.nodes, world_dynamic_geometry,	filter_dynamic_geometry );


	// sky
	vk_rtx_collect_surfaces( sky_static, BLAS_TYPE_OPAQUE, worldData, worldData.nodes, filter_sky, filter_opaque );
	vk_rtx_build_geometry_buffer( sky_static );

	// opaque
	// add debug vertices and indicies count
	{
		//world_static->host.idx_count += worldData.num_light_polys * 3;
		//world_static->host.xyz_count += worldData.num_light_polys * 3;
	}

	vk_rtx_collect_surfaces( world_static, BLAS_TYPE_OPAQUE, worldData, worldData.nodes, filter_static, filter_opaque );
	{
		//vk_rtx_inject_light_poly_debug( world_static, worldData, 3.0f );
	}
	vk_rtx_set_geomertry_accel_offsets( world_static, BLAS_TYPE_TRANSPARENT );
	vk_rtx_collect_surfaces( world_static, BLAS_TYPE_TRANSPARENT, worldData, worldData.nodes, filter_static, filter_transparent );
	vk_rtx_build_geometry_buffer( world_static );

	// dynamic  material
	vk_rtx_collect_surfaces( world_dynamic_material, BLAS_TYPE_OPAQUE, worldData, worldData.nodes, filter_dynamic_material, filter_opaque );
	vk_rtx_set_geomertry_accel_offsets( world_dynamic_material, BLAS_TYPE_TRANSPARENT );
	vk_rtx_collect_surfaces( world_dynamic_material, BLAS_TYPE_TRANSPARENT, worldData, worldData.nodes, filter_dynamic_material, filter_transparent );
	vk_rtx_build_geometry_buffer( world_dynamic_material );

	// dynamic geometry
	vk_rtx_collect_surfaces( world_dynamic_geometry, BLAS_TYPE_OPAQUE, worldData, worldData.nodes, filter_dynamic_geometry, filter_opaque );
	vk_rtx_set_geomertry_accel_offsets( world_dynamic_geometry, BLAS_TYPE_TRANSPARENT );
	vk_rtx_collect_surfaces( world_dynamic_geometry, BLAS_TYPE_TRANSPARENT, worldData, worldData.nodes, filter_dynamic_geometry, filter_transparent );
	vk_rtx_build_geometry_buffer( world_dynamic_geometry );

	//vk_rtx_debug_geom( world_dynamic_geometry );

	{	
		accel_build_batch_t batch;
		Com_Memset( &batch, 0, sizeof(accel_build_batch_t) );

		VkCommandBuffer cmd_buf = vkpt_begin_command_buffer(&vk.cmd_buffers_graphics);

		vk_rtx_create_blas_bsp( &batch, sky_static				);
		vk_rtx_create_blas_bsp( &batch, world_static			);
		vk_rtx_create_blas_bsp( &batch, world_dynamic_material	);
		vk_rtx_create_blas_bsp( &batch, world_dynamic_geometry	);

		qvkCmdBuildAccelerationStructuresKHR( cmd_buf, batch.numBuilds, batch.buildInfos, batch.rangeInfoPtrs );
		MEM_BARRIER_BUILD_ACCEL(cmd_buf); /* probably not needed here but doesn't matter */

		assert( vk.buf_light_stats[0].buffer != NULL );

		for ( i = 0; i < NUM_LIGHT_STATS_BUFFERS; i++ )
			qvkCmdFillBuffer( cmd_buf, vk.buf_light_stats[i].buffer, 0, vk.buf_light_stats[i].size, 0 );
	
		vkpt_submit_command_buffer(cmd_buf, vk.queue_graphics, (1 << vk.device_count) - 1, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL);
	}

	vk_rtx_reset_envmap();
	vk_rtx_prepare_envmap( worldData );

	qboolean cmInit = qfalse;

	for ( i = 0; i < worldData.numsurfaces; i++ ) { 
		shader_t *shader = tr.shaders[ worldData.surfaces[i].shader->index ];

		tess.shader = shader;

		if (shader->isSky && !cmInit) {	
			cmInit = qtrue;
			continue;
		}

		if (tess.shader->stages[0] == NULL) 
			continue;

		// add brush models
#if 0
		if (worldData.surfaces[i].blas == NULL && !worldData.surfaces[i].notBrush && !worldData.surfaces[i].added && !worldData.surfaces[i].skip) {
			vk.scratch_buf_ptr = 0;
			tess.numVertexes = 0;
			tess.numIndexes = 0;
			float originalTime = backEnd.refdef.floatTime;
			RB_BeginSurface(worldData.surfaces[i].shader, 0, 0);
			backEnd.refdef.floatTime = originalTime;
			tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

			// create bas
			tess.allowVBO = qfalse;
			rb_surfaceTable[*((surfaceType_t*)worldData.surfaces[i].data)]((surfaceType_t*)worldData.surfaces[i].data);
			vk_rtx_create_entity_blas( NULL, &worldData.surfaces[i].blas, qfalse );
			worldData.surfaces[i].blas->isWorldSurface = qtrue;
			//worldData.surfaces[i].blas->data.isBrushModel = qtrue;
			worldData.surfaces[i].added = qtrue;

			int clu[3] = { -1, -1, -1 };
			vec4_t pos = { 0,0,0,0 };
			for ( j = 0; j < tess.numVertexes; j++) {
				VectorAdd(pos, tess.xyz[j], pos);

				worldData.surfaces[i].blas->c = R_FindClusterForPos3( worldData, tess.xyz[j] );
				if (worldData.surfaces[i].blas->c != -1) 
					break;
			}

			RB_UploadCluster( worldData, 
				&worldData.geometry.cluster_entity_static, worldData.surfaces[i].blas->data.offsetIDX, 
				vk_get_surface_cluster( worldData, worldData.surfaces[i].data) );

			backEnd.refdef.floatTime = originalTime;
			tess.numVertexes = 0;
			tess.numIndexes = 0;
			vk.scratch_buf_ptr = 0;
		}
#endif
	}

	vk.scratch_buf_ptr = 0;

	if (!cmInit) {
		byte black[4] = { 0,0,0,0 };

		vk_rtx_create_cubemap( &vk.img_envmap, 1, 1,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1 );

		for ( int skyIndex = 0; skyIndex < 5; skyIndex++ )
			vk_rtx_upload_image_data(&vk.img_envmap, 1, 1, black, 4, 0, skyIndex);

		vk_rtx_set_envmap_descriptor_binding();
	}

	vk.scratch_buf_ptr = 0;

	vkpt_physical_sky_initialize();

	vk_create_primary_rays_pipelines( worldData );
}
