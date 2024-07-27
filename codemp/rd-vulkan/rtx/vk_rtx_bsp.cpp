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
qboolean RB_ClusterVisIdent(byte* aVis, byte* bVis) {
	if (!memcmp(aVis, bVis, vk.clusterBytes * sizeof(byte))) return qtrue;
	return qfalse;
}
int RB_CheckClusterExist(byte* cVis) {
	for (int i = vk.numFixedCluster; i < vk.numClusters; i++) {
		byte* allVis = (byte*)(vk.vis + i * vk.clusterBytes); // cast hmm
		if (RB_ClusterVisIdent(allVis, cVis)) return i;
	}
	return -1;
}

int RB_TryMergeCluster(int cluster[3], int defaultC) {
	if ((cluster[0] == -1 && cluster[1] == -1) || (cluster[1] == -1 && cluster[2] == -1) || (cluster[2] == -1 && cluster[0] == -1)) return -1;
	//if ((cluster[0] == -1 && cluster[1] == -1) && cluster[2] == defaultC) return -1;
	//if ((cluster[1] == -1 && cluster[2] == -1) && cluster[0] == defaultC) return -1;
	//if ((cluster[2] == -1 && cluster[0] == -1) && cluster[1] == defaultC) return -1;
	//if ((cluster[0] == -1 && cluster[1] == -1) || (cluster[1] == -1 && cluster[2] == -1) || (cluster[2] == -1 && cluster[0] == -1)) return -1;

	if (cluster[0] != cluster[1] || cluster[1] != cluster[2] || cluster[0] != cluster[2]) {
		vec3_t mins = { 99999, 99999, 99999 };
		vec3_t maxs = { -99999, -99999, -99999 };
		if (cluster[0] != -1) {
			mins[0] = mins[0] < vk.clusterList[cluster[0]].mins[0] ? mins[0] : vk.clusterList[cluster[0]].mins[0];
			mins[1] = mins[1] < vk.clusterList[cluster[0]].mins[1] ? mins[1] : vk.clusterList[cluster[0]].mins[1];
			mins[2] = mins[2] < vk.clusterList[cluster[0]].mins[2] ? mins[2] : vk.clusterList[cluster[0]].mins[2];
			maxs[0] = maxs[0] > vk.clusterList[cluster[0]].maxs[0] ? maxs[0] : vk.clusterList[cluster[0]].maxs[0];
			maxs[1] = maxs[1] > vk.clusterList[cluster[0]].maxs[1] ? maxs[1] : vk.clusterList[cluster[0]].maxs[1];
			maxs[2] = maxs[2] > vk.clusterList[cluster[0]].maxs[2] ? maxs[2] : vk.clusterList[cluster[0]].maxs[2];
		}
		if (cluster[1] != -1) {
			mins[0] = mins[0] < vk.clusterList[cluster[1]].mins[0] ? mins[0] : vk.clusterList[cluster[1]].mins[0];
			mins[1] = mins[1] < vk.clusterList[cluster[1]].mins[1] ? mins[1] : vk.clusterList[cluster[1]].mins[1];
			mins[2] = mins[2] < vk.clusterList[cluster[1]].mins[2] ? mins[2] : vk.clusterList[cluster[1]].mins[2];
			maxs[0] = maxs[0] > vk.clusterList[cluster[1]].maxs[0] ? maxs[0] : vk.clusterList[cluster[1]].maxs[0];
			maxs[1] = maxs[1] > vk.clusterList[cluster[1]].maxs[1] ? maxs[1] : vk.clusterList[cluster[1]].maxs[1];
			maxs[2] = maxs[2] > vk.clusterList[cluster[1]].maxs[2] ? maxs[2] : vk.clusterList[cluster[1]].maxs[2];
		}
		if (cluster[2] != -1) {
			mins[0] = mins[0] < vk.clusterList[cluster[2]].mins[0] ? mins[0] : vk.clusterList[cluster[2]].mins[0];
			mins[1] = mins[1] < vk.clusterList[cluster[2]].mins[1] ? mins[1] : vk.clusterList[cluster[2]].mins[1];
			mins[2] = mins[2] < vk.clusterList[cluster[2]].mins[2] ? mins[2] : vk.clusterList[cluster[2]].mins[2];
			maxs[0] = maxs[0] > vk.clusterList[cluster[2]].maxs[0] ? maxs[0] : vk.clusterList[cluster[2]].maxs[0];
			maxs[1] = maxs[1] > vk.clusterList[cluster[2]].maxs[1] ? maxs[1] : vk.clusterList[cluster[2]].maxs[1];
			maxs[2] = maxs[2] > vk.clusterList[cluster[2]].maxs[2] ? maxs[2] : vk.clusterList[cluster[2]].maxs[2];
		}
		if (defaultC != -1) {
			mins[0] = mins[0] < vk.clusterList[defaultC].mins[0] ? mins[0] : vk.clusterList[defaultC].mins[0];
			mins[1] = mins[1] < vk.clusterList[defaultC].mins[1] ? mins[1] : vk.clusterList[defaultC].mins[1];
			mins[2] = mins[2] < vk.clusterList[defaultC].mins[2] ? mins[2] : vk.clusterList[defaultC].mins[2];
			maxs[0] = maxs[0] > vk.clusterList[defaultC].maxs[0] ? maxs[0] : vk.clusterList[defaultC].maxs[0];
			maxs[1] = maxs[1] > vk.clusterList[defaultC].maxs[1] ? maxs[1] : vk.clusterList[defaultC].maxs[1];
			maxs[2] = maxs[2] > vk.clusterList[defaultC].maxs[2] ? maxs[2] : vk.clusterList[defaultC].maxs[2];
		}


		byte* vis = (byte*)calloc(1, sizeof(byte) * vk.clusterBytes); // cast hmm
		for (int i = 0; i < vk.numFixedCluster; i++) {
			if ((vk.clusterList[i].mins[0] >= mins[0] && vk.clusterList[i].mins[1] >= mins[1] && vk.clusterList[i].mins[2] >= mins[2]) &&
				(vk.clusterList[i].maxs[0] <= maxs[0] && vk.clusterList[i].maxs[1] <= maxs[1] && vk.clusterList[i].maxs[2] <= maxs[2])) {

				byte* allVis = (byte*)(vk.vis + i * vk.clusterBytes);
				for (int b = 0; b < vk.clusterBytes; b++) {

					vis[b] = vis[b] | allVis[b];
				}
				//const byte* clusterVis = vk.vis + cluster * worldData.clusterBytes;
				int x = 2;
			}
		}

		int c = RB_CheckClusterExist(vis);
		if (c == -1) {
			byte* allVis = (byte*)(vk.vis + vk.numClusters * vk.clusterBytes);
			for (int b = 0; b < vk.clusterBytes; b++) {
				allVis[b] = vis[b];
			}
			c = vk.numClusters;
			vk.numClusters++;
		}
		free(vis);

		return c;

		//else c = 0;
	}
	return -1;
}

#if 0
int RB_GetCluster() {
	vec3_t mins = { 99999, 99999, 99999 };
	vec3_t maxs = { -99999, -99999, -99999 };

	for (int i = 0; i < (tess.numVertexes); i++) {
		int cluster = R_FindClusterForPos3(tess.xyz[i]);
		if (cluster == -1) cluster = R_FindClusterForPos(tess.xyz[i]);
		if (cluster == -1) cluster = R_FindClusterForPos2(tess.xyz[i]);

		if (cluster != -1) {
			mins[0] = mins[0] < vk.clusterList[cluster].mins[0] ? mins[0] : vk.clusterList[cluster].mins[0];
			mins[1] = mins[1] < vk.clusterList[cluster].mins[1] ? mins[1] : vk.clusterList[cluster].mins[1];
			mins[2] = mins[2] < vk.clusterList[cluster].mins[2] ? mins[2] : vk.clusterList[cluster].mins[2];
			maxs[0] = maxs[0] > vk.clusterList[cluster].maxs[0] ? maxs[0] : vk.clusterList[cluster].maxs[0];
			maxs[1] = maxs[1] > vk.clusterList[cluster].maxs[1] ? maxs[1] : vk.clusterList[cluster].maxs[1];
			maxs[2] = maxs[2] > vk.clusterList[cluster].maxs[2] ? maxs[2] : vk.clusterList[cluster].maxs[2];
		}
	}

	byte* vis = (byte*)calloc(1, sizeof(byte) * vk.clusterBytes);
	for (int i = 0; i < vk.numFixedCluster; i++) {
		if ((vk.clusterList[i].mins[0] >= mins[0] && vk.clusterList[i].mins[1] >= mins[1] && vk.clusterList[i].mins[2] >= mins[2]) &&
			(vk.clusterList[i].maxs[0] <= maxs[0] && vk.clusterList[i].maxs[1] <= maxs[1] && vk.clusterList[i].maxs[2] <= maxs[2])) {

			byte* allVis = (byte*)(vk.vis + i * vk.clusterBytes);
			for (int b = 0; b < vk.clusterBytes; b++) {

				vis[b] = vis[b] | allVis[b];
			}
			//const byte* clusterVis = vk.vis + cluster * worldData.clusterBytes;
			int x = 2;
		}

	}

	byte* allVis = (byte*)(vk.vis + vk.numClusters * vk.clusterBytes);
	for (int b = 0; b < vk.clusterBytes; b++) {
		allVis[b] = vis[b];
	}

	free(vis);
	int c = vk.numClusters;
	vk.numClusters++;
	return c;
}
#endif

// another try for the pvs
//int BSP_PointLeaf(vec3_t p)
//{
//	mnode_t* node;
//	float		d;
//	cplane_t* plane;
//
//	node = worldData.nodes;
//
//	while (node->plane) {
//		plane = node->plane;
//		d = DotProduct(p, plane->normal) - plane->dist;
//		if (d < 0)
//			node = node->children[1];
//		else
//			node = node->children[0];
//	}
//
//	return node->cluster;
//}

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

void build_pvs2( world_t *world )
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
// Computes a point at a small distance above the center of the triangle.
// Returns qfalse if the triangle is degenerate, qtrue otherwise.
qboolean get_triangle_off_center(const float* positions, float* center, float* anti_center)
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

static void RB_UploadCluster( world_t &worldData, vkbuffer_t *buffer, uint32_t offsetIDX, int defaultC ) 
{
	uint32_t* clusterData = (uint32_t*)calloc(tess.numIndexes/3, sizeof(uint32_t));

	for (int i = 0; i < (tess.numIndexes / 3); i++) {
		int c = -1;
#if 0
		vec4_t pos = { 0,0,0,0 };
		for (int j = 0; j < 3; j++) {
			VectorAdd(pos, tess.xyz[tess.indexes[(i * 3) + j]], pos);
			VectorAdd(pos, tess.normal[tess.indexes[(i * 3) + j]], pos);
		}
		VectorScale(pos, 1.0f / 3.0f, pos);
		
		// the cluster calculation in Quake III is unreliable, therefore we need multiple fallbacks
		int cluster[3];
		cluster[0] = R_FindClusterForPos3( worldData, tess.xyz[tess.indexes[(i * 3) + 0]] );
		cluster[1] = R_FindClusterForPos3( worldData, tess.xyz[tess.indexes[(i * 3) + 1]] );
		cluster[2] = R_FindClusterForPos3( worldData, tess.xyz[tess.indexes[(i * 3) + 2]] );

		if(cluster[0] == -1) cluster[0] = R_FindClusterForPos( worldData, tess.xyz[tess.indexes[(i * 3) + 0]] );
		if (cluster[1] == -1) cluster[1] = R_FindClusterForPos( worldData, tess.xyz[tess.indexes[(i * 3) + 1]] );
		if (cluster[2] == -1) cluster[2] = R_FindClusterForPos( worldData, tess.xyz[tess.indexes[(i * 3) + 2]] );

		if (cluster[0] == -1) cluster[0] = R_FindClusterForPos2( worldData, tess.xyz[tess.indexes[(i * 3) + 0]] );
		if (cluster[1] == -1) cluster[1] = R_FindClusterForPos2( worldData, tess.xyz[tess.indexes[(i * 3) + 1]] );
		if (cluster[2] == -1) cluster[2] = R_FindClusterForPos2( worldData, tess.xyz[tess.indexes[(i * 3) + 2]] );

		// if each vertex is in a different cluster merge them to one where the whole triangle is inside
		c = RB_TryMergeCluster(cluster, defaultC);
		
		// if we still got no cluster try the center
		if (c == -1) c = R_FindClusterForPos2( worldData, pos );
		if (c == -1) c = R_FindClusterForPos( worldData, pos );
		if (c == -1) c = R_FindClusterForPos3( worldData, pos );
		if (c == -1) {
			// use default cluster as last resort
			c = defaultC;
		}
#else
		c = defaultC;
#endif
		clusterData[i] = c;
	}

	vk_rtx_upload_buffer_data_offset( buffer, offsetIDX * sizeof(uint32_t), (tess.numIndexes/3) * sizeof(uint32_t), (const byte*)clusterData );
	free(clusterData);
}

#ifdef USE_RTX
// multiple different ways to find a cluster
 int R_FindClusterForPos( world_t &worldData, const vec3_t p) {
	mnode_t* node;
	float		d;
	cplane_t* plane;

	node = tr.world->nodes;
	while (1) {
		if (node->contents != -1) {
			break;
		}
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if (d >= 0) {
			node = node->children[0];
		}
		else {
			node = node->children[1];
		}
	}

	return node->cluster;
}

 int R_FindClusterForPos2( world_t &worldData, const vec3_t p) {
	mnode_t* node;
	float		d;
	cplane_t* plane;

	node = worldData.nodes;
	while (1) {
		if (node->contents != -1) {
			break;
		}
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0) {
			node = node->children[0];
		}
		else {
			node = node->children[1];
		}
	}

	return node->cluster;
 }

int R_FindClusterForPos3( world_t &worldData, const vec3_t p ) 
{
	uint32_t i;

	for ( i = 0; i < worldData.numClusters; i++ ) 
	{
		if ( vk.clusterList[i].mins[0] <= p[0] && p[0] <= vk.clusterList[i].maxs[0] &&
			vk.clusterList[i].mins[1] <= p[1] && p[1] <= vk.clusterList[i].maxs[1] &&
			vk.clusterList[i].mins[2] <= p[2] && p[2] <= vk.clusterList[i].maxs[2] ) {
			return i;
		}
	}
	return -1;
}

int R_GetClusterFromSurface( world_t &worldData, surfaceType_t *surf) 
{
	uint32_t i;

	for ( i = 0; i < worldData.numnodes; i++) 
	{
		mnode_t* node = &worldData.nodes[i];
		if (node->contents == -1) continue;

		msurface_t** mark = node->firstmarksurface;
		int c = node->nummarksurfaces;
		for (int j = 0; j < c; j++) {
			if (mark[j]->data == surf) return node->cluster;
		}
	}
	return -1;
}

void R_RecursiveCreateAS( world_t &worldData, mnode_t* node, 
	uint32_t* countIDXstatic, uint32_t* countXYZstatic,
	uint32_t* countIDXdynamicData, uint32_t* countXYZdynamicData, 
	uint32_t* countIDXdynamicAS, uint32_t* countXYZdynamicAS, qboolean transparent) 
{	
	do 
	{
		if (node->contents != -1)
			break;

		R_RecursiveCreateAS( worldData, node->children[0], countIDXstatic, countXYZstatic, countIDXdynamicData, countXYZdynamicData, countIDXdynamicAS, countXYZdynamicAS, transparent);
		node = node->children[1];
	} while (1);
	{
		// leaf node, so add mark surfaces
		uint32_t	i,j,c;
		msurface_t	*surf, **mark;
		
		mark = node->firstmarksurface;
		c = node->nummarksurfaces;
		for ( j = 0; j < c; j++ ) {
			tess.numVertexes = 0;
			tess.numIndexes = 0;
			surf = mark[j];
			surf->notBrush = qtrue;

			shader_t* shader = tr.shaders[surf->shader->index];
			
			if ( RB_IsTransparent(shader) != transparent ) 
				continue;

			if ( RB_SkipObject(shader)
				|| *surf->data == SF_BAD || *surf->data == SF_SKIP) {
				surf->skip = qtrue;
				continue;
			}

			if (strstr(shader->name, "fog")) {
				continue;
				shader->stages[1]->active = qfalse;
			}
			if (strstr(shader->name, "console/sphere2")) {
				memcpy(shader->stages[0], shader->stages[1], sizeof(shaderStage_t));
				memcpy(shader->stages[1], shader->stages[2], sizeof(shaderStage_t));
				//shader->rtstages[1]->active = qtrue;
				shader->stages[2]->active = qfalse;
				int x = 2;
			}
			
			tess.shader = shader;

			// sunny importatant!
			tess.allowVBO = qfalse;
			rb_surfaceTable[*surf->data](surf->data);
			if (tess.numIndexes == 0) 
				continue;

			if (!surf->added && !surf->skip) {		
				int clusterIDX = node->cluster;

				uint32_t material = 0;
				// different buffer and offsets for static, dynamic data and dynamic as
				uint32_t* countIDX;
				uint32_t* countXYZ;
				vkbuffer_t *idx_buffer;
				vkbuffer_t* xyz_buffer;
				uint32_t* idx_buffer_offset;
				uint32_t* xyz_buffer_offset;

				qboolean dynamic = qfalse;
				if (tess.shader->surfaceFlags == SURF_NODRAW) continue;

				// for dm0 some strange object in the distance
				/*if (Distance(tess.xyz, (vec3_t){ -1830.72034, 3114.09717, 166.582550 }) < 250) {
					continue;
				}*/

				// if as is static we need one buffer
				if (!RB_ASDynamic(tess.shader) && !RB_ASDataDynamic(tess.shader)) {
					countIDX = countIDXstatic;
					countXYZ = countXYZstatic;
					idx_buffer = &vk.geometry.idx_world_static;
					xyz_buffer = &vk.geometry.xyz_world_static;
					idx_buffer_offset = &vk.geometry.idx_world_static_offset;
					xyz_buffer_offset = &vk.geometry.xyz_world_static_offset;

					//if ( RB_IsLight(tess.shader) ) 

					RB_UploadCluster( worldData, &vk.geometry.cluster_world_static, vk.geometry.cluster_world_static_offset, node->cluster);
					vk.geometry.cluster_world_static_offset += (tess.numIndexes/3);
				}
				// if the data of an object changes we need one as buffer but #swapchain object data buffers
				else if (!RB_ASDynamic(tess.shader) && RB_ASDataDynamic(tess.shader)) {
					countIDX = countIDXdynamicData;
					countXYZ = countXYZdynamicData;
					idx_buffer = &vk.geometry.idx_world_dynamic_data[0]; // hmm
					xyz_buffer = &vk.geometry.xyz_world_dynamic_data[0]; // hmm
					idx_buffer_offset = &vk.geometry.idx_world_dynamic_data_offset;
					xyz_buffer_offset = &vk.geometry.xyz_world_dynamic_data_offset;
					dynamic = qtrue;

					// keep track of dynamic data surf
					vk.updateDataOffsetXYZ[vk.updateDataOffsetXYZCount].shader = tess.shader;
					vk.updateDataOffsetXYZ[vk.updateDataOffsetXYZCount].num_vertices = tess.numVertexes;
					vk.updateDataOffsetXYZ[vk.updateDataOffsetXYZCount].surf = surf;
					vk.updateDataOffsetXYZ[vk.updateDataOffsetXYZCount].index_offset = *idx_buffer_offset;
					vk.updateDataOffsetXYZ[vk.updateDataOffsetXYZCount].vertex_offset = *xyz_buffer_offset;
					vk.updateDataOffsetXYZ[vk.updateDataOffsetXYZCount].cluster = clusterIDX;
					vk.updateDataOffsetXYZCount++;

					//if ( RB_IsLight(tess.shader) ) 

					RB_UploadCluster( worldData, &vk.geometry.cluster_world_dynamic_data, vk.geometry.cluster_world_dynamic_data_offset, node->cluster);
					vk.geometry.cluster_world_dynamic_data_offset += (tess.numIndexes / 3);
				}
				// object changes we need #swapchain as buffer
				else if (RB_ASDynamic(tess.shader)) {
					countIDX = countIDXdynamicAS;
					countXYZ = countXYZdynamicAS;
					idx_buffer = &vk.geometry.idx_world_dynamic_as[0]; // hmm
					xyz_buffer = &vk.geometry.xyz_world_dynamic_as[0]; // hmm
					idx_buffer_offset = &vk.geometry.idx_world_dynamic_as_offset[0]; // hmm
					xyz_buffer_offset = &vk.geometry.xyz_world_dynamic_as_offset[0]; // hmm
					dynamic = qtrue;

					// keep track of dynamic as surf
					vk.updateASOffsetXYZ[vk.updateASOffsetXYZCount].shader = tess.shader;
					vk.updateASOffsetXYZ[vk.updateASOffsetXYZCount].num_vertices = tess.numVertexes;
					vk.updateASOffsetXYZ[vk.updateASOffsetXYZCount].surf = surf;
					vk.updateASOffsetXYZ[vk.updateASOffsetXYZCount].index_offset = *idx_buffer_offset;
					vk.updateASOffsetXYZ[vk.updateASOffsetXYZCount].vertex_offset = *xyz_buffer_offset;
					vk.updateASOffsetXYZ[vk.updateASOffsetXYZCount].countXYZ = *countXYZ;
					vk.updateASOffsetXYZ[vk.updateASOffsetXYZCount].cluster = clusterIDX;
					vk.updateASOffsetXYZCount++;

					//if ( RB_IsLight(tess.shader) ) 

					RB_UploadCluster( worldData, &vk.geometry.cluster_world_dynamic_as, vk.geometry.cluster_world_dynamic_as_offset, node->cluster);
					vk.geometry.cluster_world_dynamic_as_offset += (tess.numIndexes / 3);
				}
				else {
					surf->skip = qtrue;
					continue;
				}
				
				// upload indices
				vk_rtx_upload_indices( idx_buffer, (*idx_buffer_offset), (*countXYZ) );
				if ( dynamic ) {
					for ( i = 1; i < vk.swapchain_image_count; i++ )
						vk_rtx_upload_indices( &idx_buffer[i], (*idx_buffer_offset), (*countXYZ) );
				}

				// upload vertices
				vk_rtx_upload_vertices( xyz_buffer, (*xyz_buffer_offset), clusterIDX );
				if ( dynamic ) {
					for ( i = 1; i < vk.swapchain_image_count; i++ ) {
						vk_rtx_upload_vertices( &xyz_buffer[i], (*xyz_buffer_offset), clusterIDX );
					}
				}	
				surf->added = qtrue;
	
				(*idx_buffer_offset) += tess.numIndexes;
				(*xyz_buffer_offset) += tess.numVertexes;
				(*countIDX) += tess.numIndexes;
				(*countXYZ) += tess.numVertexes;
			}

			tess.numVertexes = 0;
			tess.numIndexes = 0;
		}	
	}
}

void R_CalcClusterAABB( mnode_t* node, int numClusters ) 
{
	do {
		if ( node->contents != -1 )
			break;

		R_CalcClusterAABB( node->children[0], numClusters );
		node = node->children[1];
	} while (1);
	{
		if ( node->cluster < 0 || node->cluster > numClusters )
			return;

		vk.clusterList[node->cluster].mins[0] = MIN(vk.clusterList[node->cluster].mins[0], node->mins[0]);
		vk.clusterList[node->cluster].mins[1] = MIN(vk.clusterList[node->cluster].mins[1], node->mins[1]);
		vk.clusterList[node->cluster].mins[2] = MIN(vk.clusterList[node->cluster].mins[2], node->mins[2]);

		vk.clusterList[node->cluster].maxs[0] = MAX(vk.clusterList[node->cluster].maxs[0], node->maxs[0]);
		vk.clusterList[node->cluster].maxs[1] = MAX(vk.clusterList[node->cluster].maxs[1], node->maxs[1]);
		vk.clusterList[node->cluster].maxs[2] = MAX(vk.clusterList[node->cluster].maxs[2], node->maxs[2]);
	}
}

static void vk_bind_storage_buffer( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkBuffer buffer )
{
	const uint32_t count = 1;

	vk_rtx_add_descriptor_buffer( descriptor, count, binding, stage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
	vk_rtx_set_descriptor_update_size( descriptor, binding, stage, count );
	vk_rtx_bind_descriptor_buffer( descriptor, binding, stage, buffer );
}

static void vk_bind_uniform_buffer( vkdescriptor_t *descriptor, uint32_t binding, VkShaderStageFlagBits stage, VkBuffer buffer )
{
	const uint32_t count = 1;

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


static void vk_create_rtx_descriptor( uint32_t index, int prev_index ) 
{
	VkShaderStageFlagBits flags = (VkShaderStageFlagBits)( VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR );
	vkdescriptor_t *descriptor = &vk.rtxDescriptor[index];

	vk_rtx_add_descriptor_as(descriptor, BINDING_OFFSET_AS, flags);
	vk_rtx_bind_descriptor_as(descriptor, BINDING_OFFSET_AS, flags, &vk.tlas[index].accel_khr);

	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_INSTANCE_DATA,					flags, vk.buffer_blas_instance_data[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_STATIC,				flags, vk.geometry.xyz_world_static.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_STATIC,				flags, vk.geometry.idx_world_static.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA,			flags, vk.geometry.xyz_world_dynamic_data[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA,			flags, vk.geometry.idx_world_dynamic_data[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS,			flags, vk.geometry.xyz_world_dynamic_as[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS,			flags, vk.geometry.idx_world_dynamic_as[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_WORLD_STATIC,			flags, vk.geometry.cluster_world_static.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_DATA,		flags, vk.geometry.cluster_world_dynamic_data.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_AS,		flags, vk.geometry.cluster_world_dynamic_as.buffer );

	// miscellaneous
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_READBACK_BUFFER,					VK_SHADER_STAGE_ALL, vk.buffer_readback.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_DYNAMIC_VERTEX,					VK_SHADER_STAGE_ALL, vk.model_instance.buffer_vertex.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_LIGHT_BUFFER,					VK_SHADER_STAGE_ALL, vk.buffer_light.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_TONEMAP_BUFFER,					VK_SHADER_STAGE_ALL, vk.buffer_tonemap.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_SUN_COLOR_BUFFER,				VK_SHADER_STAGE_ALL, vk.buffer_sun_color.buffer );
	
	// light stats
	{
		uint32_t i, light_stats_index;
		light_stats_index = vk_rtx_add_descriptor_buffer( descriptor, NUM_LIGHT_STATS_BUFFERS, BINDING_OFFSET_LIGHT_STATS_BUFFER, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
		vk_rtx_set_descriptor_update_size( descriptor, BINDING_OFFSET_LIGHT_STATS_BUFFER, VK_SHADER_STAGE_ALL, NUM_LIGHT_STATS_BUFFERS );

		for ( i = 0; i < NUM_LIGHT_STATS_BUFFERS; i++ )
			descriptor->data[light_stats_index].buffer[i] = { vk.buffer_light_stats[i].buffer, 0, vk.buffer_light_stats[i].allocSize };
	}

	// light count history
	{
		uint32_t i, light_count_history_index;
		light_count_history_index = vk_rtx_add_descriptor_buffer( descriptor, LIGHT_COUNT_HISTORY, BINDING_LIGHT_COUNTS_HISTORY_BUFFER, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
		vk_rtx_set_descriptor_update_size( descriptor, BINDING_LIGHT_COUNTS_HISTORY_BUFFER, VK_SHADER_STAGE_ALL, LIGHT_COUNT_HISTORY );
		
		for ( i = 0; i < LIGHT_COUNT_HISTORY; i++ )
			descriptor->data[light_count_history_index].buffer[i] = { vk.buffer_light_counts_history[i].buffer, 0, vk.buffer_light_counts_history[i].allocSize };
	}

	// previous
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_INSTANCE_DATA_PREV,				flags, vk.buffer_blas_instance_data[prev_index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA_PREV,		flags, vk.geometry.xyz_world_dynamic_data[prev_index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA_PREV,		flags, vk.geometry.idx_world_dynamic_data[prev_index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS_PREV,		flags, vk.geometry.xyz_world_dynamic_as[prev_index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS_PREV,		flags, vk.geometry.idx_world_dynamic_as[prev_index].buffer );

	// storage images and *optional samplers
	#define IMG_DO( _handle, _binding, ... ) \
		vk_bind_storage_image(	descriptor, BINDING_OFFSET_IMAGES	+ ##_binding,		VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle].view ); \
		/* next is optional for rtx, rtx and compute descriptors differ, try define guard in glsl::constants.h to not bind sampler and B images */ \
		vk_bind_sampler(		descriptor, BINDING_OFFSET_TEXTURES + ##_binding,		VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle].sampler, vk.rtx_images[RTX_IMG_##_handle].view, VK_IMAGE_LAYOUT_GENERAL );
		LIST_IMAGES
	#undef IMG_DO

	const int offset = ( index * RTX_IMG_NUM_A_B );
	const int offset_prev = ( prev_index * RTX_IMG_NUM_A_B );

	// A_B
	#define IMG_DO( _handle, _binding, ... ) \
		vk_bind_storage_image(	descriptor, BINDING_OFFSET_IMAGES	+ RTX_IMG_##_handle##_A, VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle##_A + offset].view );			/* A */ \
		/* next is *optional for rtx, rtx and compute descriptors differ, try define guard in glsl::constants.h to not bind sampler and B images */ \
		vk_bind_storage_image(	descriptor, BINDING_OFFSET_IMAGES	+ RTX_IMG_##_handle##_B, VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle##_A + offset_prev].view );	/* B */ \
		vk_bind_sampler(		descriptor, BINDING_OFFSET_TEXTURES + RTX_IMG_##_handle##_A, VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle##_A + offset].sampler,		vk.rtx_images[RTX_IMG_##_handle##_A + offset].view,			VK_IMAGE_LAYOUT_GENERAL );	/* A */ \
		vk_bind_sampler(		descriptor, BINDING_OFFSET_TEXTURES + RTX_IMG_##_handle##_B, VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle##_A + offset_prev].sampler,	vk.rtx_images[RTX_IMG_##_handle##_A + offset_prev].view,	VK_IMAGE_LAYOUT_GENERAL );	/* B */
		LIST_IMAGES_A_B
	#undef IMG_DO


	vk_bind_sampler( descriptor, BINDING_OFFSET_ENVMAP,					(VkShaderStageFlagBits)(VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR), vk.envmap.sampler, vk.envmap.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vk_bind_sampler( descriptor, BINDING_OFFSET_BLUE_NOISE,				VK_SHADER_STAGE_RAYGEN_BIT_KHR, vk.blue_noise.sampler, vk.blue_noise.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// physical sky begin
	{
		const int binding_offset_env_tex = ( BINDING_OFFSET_PHYSICAL_SKY - ( BINDING_OFFSET_PHYSICAL_SKY ) );
		vk_bind_sampler(		descriptor, BINDING_OFFSET_PHYSICAL_SKY,		VK_SHADER_STAGE_ALL, vk.physicalSkyImages[binding_offset_env_tex].sampler, vk.physicalSkyImages[binding_offset_env_tex].view,  VK_IMAGE_LAYOUT_GENERAL );
		vk_bind_storage_image(	descriptor, BINDING_OFFSET_PHYSICAL_SKY_IMG,	VK_SHADER_STAGE_ALL, vk.physicalSkyImages[binding_offset_env_tex].view );
	}

	#define VK_BIND_PHYSICAL_SKY_IMAGE( binding ) \
		{ \
			int binding_offset = ( binding - ( BINDING_OFFSET_PHYSICAL_SKY ) ); \
			vk_bind_sampler( descriptor, binding, VK_SHADER_STAGE_ALL, vk.physicalSkyImages[binding_offset].sampler, vk.physicalSkyImages[binding_offset].view,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ); \
		}
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_SKY_TRANSMITTANCE )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_SKY_SCATTERING )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_SKY_IRRADIANCE )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_SKY_CLOUDS )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_TERRAIN_ALBEDO )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_TERRAIN_NORMALS )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_TERRAIN_DEPTH )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_TERRAIN_SHADOWMAP )
	#undef VK_BIND_PHYSICAL_SKY_IMAGE
	// physical sky end

	descriptor->lastBindingVariableSizeExt = qtrue;
	vk_rtx_create_descriptor( descriptor );
	vk_rtx_update_descriptor( descriptor );
}

static void vk_create_compute_descriptor( uint32_t index, int prev_index ) 
{
	VkShaderStageFlagBits flags = (VkShaderStageFlagBits)( VK_SHADER_STAGE_COMPUTE_BIT );
	vkdescriptor_t *descriptor = &vk.computeDescriptor[index];

	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_INSTANCE_DATA,					flags, vk.buffer_blas_instance_data[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_INSTANCE_PREV_TO_CURR,			flags, vk.prevToCurrInstanceBuffer[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_STATIC,				flags, vk.geometry.xyz_world_static.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_STATIC,				flags, vk.geometry.idx_world_static.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA,			flags, vk.geometry.xyz_world_dynamic_data[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA,			flags, vk.geometry.idx_world_dynamic_data[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS,			flags, vk.geometry.xyz_world_dynamic_as[index].buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS,			flags, vk.geometry.idx_world_dynamic_as[index].buffer );

	// miscellaneous
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_READBACK_BUFFER,					VK_SHADER_STAGE_COMPUTE_BIT, vk.buffer_readback.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_DYNAMIC_VERTEX,					VK_SHADER_STAGE_COMPUTE_BIT, vk.model_instance.buffer_vertex.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_LIGHT_BUFFER,					VK_SHADER_STAGE_COMPUTE_BIT, vk.buffer_light.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_TONEMAP_BUFFER,					VK_SHADER_STAGE_COMPUTE_BIT, vk.buffer_tonemap.buffer );
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_SUN_COLOR_BUFFER,				VK_SHADER_STAGE_COMPUTE_BIT, vk.buffer_sun_color.buffer );

	// light stats
	{
		uint32_t i, light_stats_index;
		light_stats_index = vk_rtx_add_descriptor_buffer( descriptor, NUM_LIGHT_STATS_BUFFERS, BINDING_OFFSET_LIGHT_STATS_BUFFER, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
		vk_rtx_set_descriptor_update_size( descriptor, BINDING_OFFSET_LIGHT_STATS_BUFFER, VK_SHADER_STAGE_ALL, NUM_LIGHT_STATS_BUFFERS );

		for ( i = 0; i < NUM_LIGHT_STATS_BUFFERS; i++ )
			descriptor->data[light_stats_index].buffer[i] = { vk.buffer_light_stats[i].buffer, 0, vk.buffer_light_stats[i].allocSize };
	}

	// light count history
	{
		uint32_t i, light_count_history_index;
		light_count_history_index = vk_rtx_add_descriptor_buffer( descriptor, LIGHT_COUNT_HISTORY, BINDING_LIGHT_COUNTS_HISTORY_BUFFER, VK_SHADER_STAGE_ALL, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
		vk_rtx_set_descriptor_update_size( descriptor, BINDING_LIGHT_COUNTS_HISTORY_BUFFER, VK_SHADER_STAGE_ALL, LIGHT_COUNT_HISTORY );

		for ( i = 0; i < LIGHT_COUNT_HISTORY; i++ )
			descriptor->data[light_count_history_index].buffer[i] = { vk.buffer_light_counts_history[i].buffer, 0, vk.buffer_light_counts_history[i].allocSize };
	}

	// previous
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_INSTANCE_DATA_PREV,				flags, vk.buffer_blas_instance_data[prev_index].buffer);
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA_PREV,		flags, vk.geometry.xyz_world_dynamic_data[prev_index].buffer);
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA_PREV,		flags, vk.geometry.idx_world_dynamic_data[prev_index].buffer);
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS_PREV,		flags, vk.geometry.xyz_world_dynamic_as[prev_index].buffer);
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS_PREV,		flags, vk.geometry.idx_world_dynamic_as[prev_index].buffer);

	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_WORLD_STATIC,			flags, vk.geometry.cluster_world_static.buffer);
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_DATA,		flags, vk.geometry.cluster_world_dynamic_data.buffer);
	vk_bind_storage_buffer( descriptor, BINDING_OFFSET_CLUSTER_WORLD_DYNAMIC_AS,		flags, vk.geometry.cluster_world_dynamic_as.buffer);

	// storage images and samplers
	#define IMG_DO( _handle, _binding, ... ) \
		vk_bind_storage_image(	descriptor, BINDING_OFFSET_IMAGES	+ ##_binding,		VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle].view ); \
		vk_bind_sampler(		descriptor, BINDING_OFFSET_TEXTURES + ##_binding,		VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle].sampler, vk.rtx_images[RTX_IMG_##_handle].view, VK_IMAGE_LAYOUT_GENERAL );
		LIST_IMAGES
	#undef IMG_DO

	const int offset = ( index * RTX_IMG_NUM_A_B );
	const int offset_prev = ( prev_index * RTX_IMG_NUM_A_B );

	// A_B
	#define IMG_DO( _handle, _binding, ... ) \
		vk_bind_storage_image(	descriptor, BINDING_OFFSET_IMAGES	+ RTX_IMG_##_handle##_A, VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle##_A + offset].view );			/* A */ \
		vk_bind_storage_image(	descriptor, BINDING_OFFSET_IMAGES	+ RTX_IMG_##_handle##_B, VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle##_A + offset_prev].view );	/* B */ \
		vk_bind_sampler(		descriptor, BINDING_OFFSET_TEXTURES + RTX_IMG_##_handle##_A, VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle##_A + offset].sampler,			vk.rtx_images[RTX_IMG_##_handle##_A + offset].view,		VK_IMAGE_LAYOUT_GENERAL );	/* A */ \
		vk_bind_sampler(		descriptor, BINDING_OFFSET_TEXTURES + RTX_IMG_##_handle##_B, VK_SHADER_STAGE_ALL, vk.rtx_images[RTX_IMG_##_handle##_A + offset_prev].sampler,	vk.rtx_images[RTX_IMG_##_handle##_A + offset_prev].view,	VK_IMAGE_LAYOUT_GENERAL );	/* B */
		LIST_IMAGES_A_B
	#undef IMG_DO

	vk_bind_sampler( descriptor, BINDING_OFFSET_BLUE_NOISE,								flags, vk.blue_noise.sampler, vk.blue_noise.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	// physical sky begin
	{
		const int binding_offset_env_tex = ( BINDING_OFFSET_PHYSICAL_SKY - ( BINDING_OFFSET_PHYSICAL_SKY ) );
		vk_bind_sampler(		descriptor, BINDING_OFFSET_PHYSICAL_SKY,		VK_SHADER_STAGE_ALL, vk.physicalSkyImages[binding_offset_env_tex].sampler, vk.physicalSkyImages[binding_offset_env_tex].view,  VK_IMAGE_LAYOUT_GENERAL );
		vk_bind_storage_image(	descriptor, BINDING_OFFSET_PHYSICAL_SKY_IMG,	VK_SHADER_STAGE_ALL, vk.physicalSkyImages[binding_offset_env_tex].view );
	}

	#define VK_BIND_PHYSICAL_SKY_IMAGE( binding ) \
		{ \
			int binding_offset = ( binding - ( BINDING_OFFSET_PHYSICAL_SKY ) ); \
			vk_bind_sampler( descriptor, binding, VK_SHADER_STAGE_ALL, vk.physicalSkyImages[binding_offset].sampler, vk.physicalSkyImages[binding_offset].view,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ); \
		}
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_SKY_TRANSMITTANCE )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_SKY_SCATTERING )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_SKY_IRRADIANCE )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_SKY_CLOUDS )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_TERRAIN_ALBEDO )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_TERRAIN_NORMALS )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_TERRAIN_DEPTH )
		VK_BIND_PHYSICAL_SKY_IMAGE	( BINDING_OFFSET_TERRAIN_SHADOWMAP )
	#undef VK_BIND_PHYSICAL_SKY_IMAGE

	vk_bind_uniform_buffer( descriptor, BINDING_OFFSET_PRECOMPUTED_SKY_UBO,	flags, vk_rtx_get_atmospheric_buffer() ); // bad
	// physical sky end

	vk_rtx_create_descriptor( descriptor );
	vk_rtx_update_descriptor( descriptor );
}

static void vk_create_primary_rays_pipelines() 
{
	uint32_t i, prev_index;

	for ( i = 0; i < vk.swapchain_image_count; i++ ) 
	{
		prev_index = (i + (vk.swapchain_image_count - 1)) % vk.swapchain_image_count;

		vk_create_rtx_descriptor( i, prev_index );
		vk_create_compute_descriptor( i, prev_index );
	}

	vk_rtx_create_raytracing_pipelines();
	vk_rtx_create_compute_pipelines();
}

static uint32_t num_vertices_static;
static uint32_t num_vertices_dynamic_data;
static uint32_t num_vertices_dynamic_as;
static uint32_t num_indices_static;
static uint32_t num_indices_dynamic_data;
static uint32_t num_indices_dynamic_as;

static void vk_clear_as_vertices_count( void ) 
{
	num_vertices_static			= 0;
	num_vertices_dynamic_data	= 0;
	num_vertices_dynamic_as		= 0;

	num_indices_static			= 0; // aint indices just vertices * 3?
	num_indices_dynamic_data		= 0;
	num_indices_dynamic_as		= 0;

	return;
}

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

static void collect_light_polys( world_t &worldData, int model_idx, int* num_lights, int* allocated_lights, light_poly_t** lights )
{
	srfSurfaceFace_t *face;
	msurface_t *sf;
	uint32_t i, j;

	for ( i = 0, sf = &worldData.surfaces[0]; i < worldData.numsurfaces; i++, sf++ ) 
	{
		face = (srfSurfaceFace_t *)sf->data;

		if ( face->surfaceType != SF_FACE )
			continue;

		uint32_t emissive_image_index = vk_rtx_find_emissive_texture( sf->shader );
		
		if ( !emissive_image_index )
			continue;

		image_t *image = tr.images[emissive_image_index];
			
		// This algorithm relies on information from the emissive texture,
		// specifically the extents of the emissive pixels in that texture.
		// Ignore surfaces that don't have an emissive texture attached.
		if ( !image )
			continue;

		Com_Printf(" found emissive/dglow shader: %s \n", sf->shader->name );

		// if (image->entire_texture_emissive)
		{
			float positions[3 * /*max_vertices / face->numPoints*/ 32];

			for ( j = 0; j < face->numPoints; j++ )
			{
				float *p = positions + j * 3;
				VectorCopy( face->points[j], p );
			}

			int num_vertices = face->numPoints;
			remove_collinear_edges( positions, NULL, &num_vertices );
			
			const int num_triangles = face->numPoints - 2; // ?

			// still poisitons left?
			Com_Printf(".");

			for (  j = 0; j < num_triangles; j++ )
			{
				const int e = face->numPoints;

				int i1 = (j + 2) % e;
				int i2 = (j + 1) % e;

				vec3_t light_color = { 255.0, 125.0, 0.0 };

				light_poly_t light;
				VectorCopy( positions,			light.positions + 0 );
				VectorCopy( positions + i1 * 3,	light.positions + 3 );
				VectorCopy( positions + i2 * 3,	light.positions + 6 );
				VectorCopy( light_color,		light.color );

				light.material = (void*)vk_rtx_get_material( sf->shader->index );
				light.style = 0;

				if ( !get_triangle_off_center( light.positions, light.off_center, NULL ) )
					continue;

				light.cluster = BSP_PointLeaf( worldData.nodes, light.off_center )->cluster;

				if ( light.cluster >= 0 )
				{
					light_poly_t *list_light = append_light_poly( &worldData.num_light_polys, &worldData.allocated_light_polys, &worldData.light_polys );
					memcpy( list_light, &light, sizeof(light_poly_t) );
				}
			}

			continue;
		} 
	}
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

		const byte *pvs = worldData.vis2 + light->cluster * vk.clusterBytes;

		FOREACH_BIT_BEGIN( pvs, vk.clusterBytes, other_cluster )
			byte *cluster_aabb = (byte*)worldData.vis2 + other_cluster * vk.clusterBytes;
			//if (light_affects_cluster(light, cluster_aabb))
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
	//memcpy(&vk.vis, worldData.vis, worldData.numClusters * sizeof(byte) * worldData.clusterBytes);
	//const byte* clusterVis = worldData.vis + cluster * worldData.clusterBytes;

	vk.clusterList		= (cluster_t*)calloc(worldData.numClusters, sizeof(cluster_t));
	for ( i = 0; i < worldData.numClusters; i++ ) {
		vk.clusterList[i].idx = i;
		vk.clusterList[i].mins[0] = vk.clusterList[i].mins[1] = vk.clusterList[i].mins[2] = 99999;
		vk.clusterList[i].maxs[0] = vk.clusterList[i].maxs[1] = vk.clusterList[i].maxs[2] = -99999;
	}

	R_CalcClusterAABB( worldData.nodes, worldData.numClusters );

	// polygonal lights
	worldData.num_light_polys = 0;
	worldData.allocated_light_polys = 0;
	worldData.light_polys = NULL;

	collect_light_polys( worldData, -1, &worldData.num_light_polys, &worldData.allocated_light_polys, &worldData.light_polys );

	collect_cluster_lights( worldData );

	vk_clear_as_vertices_count();

	tess.numVertexes = 0;
	tess.numIndexes = 0;
	R_RecursiveCreateAS( worldData, worldData.nodes, &num_indices_static, &num_vertices_static, &num_indices_dynamic_data, &num_vertices_dynamic_data, &num_indices_dynamic_as, &num_vertices_dynamic_as, qfalse);

	VkCommandBuffer cmd_buf = vk_begin_command_buffer();
	
	const qboolean instanced = qfalse;

	// world static
	{	
		vk_rtx_create_blas( cmd_buf, 
			&vk.geometry.xyz_world_static, 0, &vk.geometry.idx_world_static, 0, 
			num_vertices_static, num_indices_static,
			&vk.blas_static.world,
			qfalse, qtrue, qfalse, instanced );
	}

	// world dynamic data
	{
		vk_rtx_create_blas( cmd_buf,  
			&vk.geometry.xyz_world_dynamic_data[0], 0, &vk.geometry.idx_world_dynamic_data[0], 0, 
			num_vertices_dynamic_data, num_indices_dynamic_data,
			&vk.blas_dynamic.data_world,
			qtrue, qtrue, qfalse, instanced );
	}

	// world dynamic as
	{
		for ( i = 0; i < vk.swapchain_image_count; i++) 
		{
			vk_rtx_create_blas( cmd_buf, 
				&vk.geometry.xyz_world_dynamic_as[i], 0, &vk.geometry.idx_world_dynamic_as[i], 0,  
				num_vertices_dynamic_as, num_indices_dynamic_as,
				&vk.blas_dynamic.as_world[i],
				qtrue, qfalse, qtrue, instanced );
		}
	}

	vk_clear_as_vertices_count();

	tess.numVertexes = 0;
	tess.numIndexes = 0;	
	R_RecursiveCreateAS( worldData, worldData.nodes, &num_indices_static, &num_vertices_static, &num_indices_dynamic_data, &num_vertices_dynamic_data, &num_indices_dynamic_as, &num_vertices_dynamic_as, qtrue);
	
	// world static trans
	{
		vk_rtx_create_blas( cmd_buf, 			
			&vk.geometry.xyz_world_static, vk.geometry.xyz_world_static_offset, &vk.geometry.idx_world_static, vk.geometry.idx_world_static_offset,
			num_vertices_static, num_indices_static,
			&vk.blas_static.world_transparent, 
			qfalse, qtrue, qfalse, instanced );
	}

	// world dynamic data trans
	{
		vk_rtx_create_blas( cmd_buf,  
			&vk.geometry.xyz_world_dynamic_data[0], vk.geometry.xyz_world_dynamic_data_offset, &vk.geometry.idx_world_dynamic_data[0], vk.geometry.idx_world_dynamic_data_offset,
			num_vertices_dynamic_data, num_indices_dynamic_data,
			&vk.blas_dynamic.data_world_transparent,
			qtrue, qtrue, qfalse, instanced );
	}

	// world dynamic as trans
	{
		for ( i = 0; i < vk.swapchain_image_count; i++ ) 
		{
			vk_rtx_create_blas( cmd_buf, 
				&vk.geometry.xyz_world_dynamic_as[i], vk.geometry.xyz_world_dynamic_as_offset[0], &vk.geometry.idx_world_dynamic_as[i], vk.geometry.idx_world_dynamic_as_offset[0],
				num_vertices_dynamic_as, num_indices_dynamic_as,
				&vk.blas_dynamic.as_world_transparent[i],
				qtrue, qfalse, qtrue, instanced );
		}
	}

	vk_end_command_buffer( cmd_buf );


	vk_rtx_reset_envmap();
	vk_rtx_prepare_envmap( worldData );

	qboolean cmInit = qfalse;

	for ( i = 0; i < worldData.numsurfaces; i++ ) { 
		shader_t *shader = tr.shaders[ worldData.surfaces[i].shader->index ];

		tess.shader = shader;

		if (shader->isSky && !cmInit) {
#if 0
			int		width, height;
			byte* pic;
			if (shader->sky->outerbox[0] != NULL) {

				width = shader->sky->outerbox[0]->width;
				height = shader->sky->outerbox[0]->height;
				vk_rtx_create_cubemap( &vk.envmap, width, height,
					VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1 );

				R_LoadImage(shader->sky->outerbox[3]->imgName, &pic, &width, &height);
				if (width == 0 || height == 0) goto skyFromStage;
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 0); // back
				ri.Z_Free(pic);

				R_LoadImage(shader->sky->outerbox[1]->imgName, &pic, &width, &height);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 1); // front
				ri.Z_Free(pic);

				R_LoadImage(shader->sky->outerbox[4]->imgName, &pic, &width, &height);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 2); // bottom
				ri.Z_Free(pic);

				R_LoadImage(shader->sky->outerbox[5]->imgName, &pic, &width, &height);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 3); // up
				ri.Z_Free(pic);

				R_LoadImage(shader->sky->outerbox[0]->imgName, &pic, &width, &height);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 4); // right
				ri.Z_Free(pic);

				R_LoadImage(shader->sky->outerbox[2]->imgName, &pic, &width, &height);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 5); // left
				ri.Z_Free(pic);

				//vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 5);
			}
			else if (shader->stages[0] != NULL) {
			skyFromStage:
				width = shader->stages[0]->bundle[0].image[0]->width;
				height = shader->stages[0]->bundle[0].image[0]->height;

				vk_rtx_create_cubemap( &vk.envmap, width, height,
					VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1 );

				R_LoadImage(shader->stages[0]->bundle[0].image[0]->imgName/*"textures/skies/bluedimclouds.tga"*/, &pic, &width, &height);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 0);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 1);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 2);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 3);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 4);
				vk_rtx_upload_image_data(&vk.envmap, width, height, pic, 4, 0, 5);
			}
			vk_rtx_create_sampler(&vk.envmap, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
#endif	
			cmInit = qtrue;
			continue;
		}


		if (tess.shader->stages[0] == NULL) 
			continue;

		// add brush models
#if 0
		if (worldData.surfaces[i].blas == NULL && !worldData.surfaces[i].notBrush && !worldData.surfaces[i].added && !worldData.surfaces[i].skip) {
			vk.scratch_buffer_ptr = 0;
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
				&vk.geometry.cluster_entity_static, worldData.surfaces[i].blas->data.offsetIDX, 
				R_GetClusterFromSurface( worldData, worldData.surfaces[i].data) );

			backEnd.refdef.floatTime = originalTime;
			tess.numVertexes = 0;
			tess.numIndexes = 0;
			vk.scratch_buffer_ptr = 0;
		}
#endif
	}

	vk.scratch_buffer_ptr = 0;

	if (!cmInit) {
		byte black[4] = { 0,0,0,0 };

		vk_rtx_create_cubemap( &vk.envmap, 1, 1,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1 );

		for ( int skyIndex = 0; skyIndex < 5; skyIndex++ )
			vk_rtx_upload_image_data(&vk.envmap, 1, 1, black, 4, 0, skyIndex);

		vk_rtx_create_sampler( &vk.envmap, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
	}

	vk.scratch_buffer_ptr = 0;
	

	// the descriptor system is slightly different compared to Q2RTX.
	// valid image and samper handles are required to create the primary rtx and compute pipelines and descriptors.
	vkpt_light_buffers_create( worldData );

	{
		VkCommandBuffer cmd_buf = vk_begin_command_buffer();

		if ( vk.buffer_light_stats[0].buffer )
		{
			for ( i = 0; i < NUM_LIGHT_STATS_BUFFERS; i++ )
				qvkCmdFillBuffer( cmd_buf, vk.buffer_light_stats[i].buffer, 0, vk.buffer_light_stats[i].allocSize, 0 );
		}

		vk_end_command_buffer( cmd_buf );
	}

	vkpt_physical_sky_initialize();

	vk_create_primary_rays_pipelines();

	vk.worldASInit = qtrue;
}
#endif