/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "tr_local.h"

/*
  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]
*/

typedef struct {
	int		i2;
	int		facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	32

static	edgeDef_t	edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static	int			numEdgeDefs[SHADER_MAX_VERTEXES];
static	int			facing[SHADER_MAX_INDEXES / 3];

static void R_AddEdgeDef(int i1, int i2, int facing) {
	int		c = numEdgeDefs[i1];
	if (c == MAX_EDGE_DEFS) {
		return;		// overflow
	}
	edgeDefs[i1][c].i2 = i2;
	edgeDefs[i1][c].facing = facing;

	numEdgeDefs[i1]++;
}
// One increment pass then one decrement pass over whatever is currently in
// tess.indexes. Emission is chunked because a capped, all-edges volume needs 24
// indices per facing triangle and overruns SHADER_MAX_INDEXES on any real
// surface; vanilla is in immediate mode and has no such limit. Chunking is safe
// only because the stencil ops wrap - modular arithmetic does not care how the
// draws are split.
static void R_FlushShadowVolume( const uint32_t *pipeline )
{
	if ( !tess.numIndexes )
		return;

	vk_bind_pipeline( pipeline[0] );
	vk_bind_index();
	vk_bind_geometry( TESS_XYZ | TESS_RGBA0 );
	vk_draw_geometry( DEPTH_RANGE_NORMAL, qtrue );
	vk_bind_pipeline( pipeline[1] );
	vk_draw_geometry( DEPTH_RANGE_NORMAL, qtrue );

	tess.numIndexes = 0;
}

/*
=================
R_BuildShadowVolume

Builds and draws the shadow volume the way rd-vanilla's R_RenderShadowEdges
does, which is NOT the Quake 3 silhouette algorithm this file used to carry.

Two deliberate differences, both from rww's JKA version:

Every edge of every light-facing triangle is emitted, not just silhouette
edges. On a clean mesh the interior edges produce pairs of opposite winding
that cancel, so the result is identical - but it is immune to edges shared by
more than two triangles, which the silhouette test mishandles and which real
.glm assets have. rww's own comment: "we are going to render all edges even
though it is a tiny bit slower".

The volume is capped, near and far, because Carmack's Reverse needs it.
=================
*/
static void R_BuildShadowVolume( const uint32_t *pipeline )
{
	const int numTris = tess.numIndexes / 3;
	const int numVerts = tess.numVertexes;
	int i, j;

	// facing[] and edgeDefs[] were filled against the original index list; the
	// volume is emitted into the same array, so read the triangle list out first.
	static uint32_t sourceIndexes[SHADER_MAX_INDEXES];
	Com_Memcpy( sourceIndexes, tess.indexes, tess.numIndexes * sizeof( tess.indexes[0] ) );

	tess.numIndexes = 0;
	tess.numVertexes = numVerts * 2;

	color4ub_t *colors = &tess.svars.colors[0][0]; // needs 2x SHADER_MAX_VERTEXES
	for ( i = 0; i < tess.numVertexes; i++ )
		Vector4Set( colors[i], 50, 50, 50, 255 );

	// sides: one extruded quad per edge of every facing triangle
	for ( i = 0; i < numVerts; i++ )
	{
		const int c = numEdgeDefs[i];

		for ( j = 0; j < c; j++ )
		{
			if ( !edgeDefs[i][j].facing )
				continue;

			if ( tess.numIndexes > (int)ARRAY_LEN( tess.indexes ) - 6 )
				R_FlushShadowVolume( pipeline );

			const int i2 = edgeDefs[i][j].i2;

			tess.indexes[tess.numIndexes + 0] = i;
			tess.indexes[tess.numIndexes + 1] = i + numVerts;
			tess.indexes[tess.numIndexes + 2] = i2;
			tess.indexes[tess.numIndexes + 3] = i2;
			tess.indexes[tess.numIndexes + 4] = i + numVerts;
			tess.indexes[tess.numIndexes + 5] = i2 + numVerts;
			tess.numIndexes += 6;
		}
	}

	// caps: the facing triangle itself, and its extruded copy wound backwards
	for ( i = 0; i < numTris; i++ )
	{
		if ( !facing[i] )
			continue;

		if ( tess.numIndexes > (int)ARRAY_LEN( tess.indexes ) - 6 )
			R_FlushShadowVolume( pipeline );

		const int o1 = sourceIndexes[i * 3 + 0];
		const int o2 = sourceIndexes[i * 3 + 1];
		const int o3 = sourceIndexes[i * 3 + 2];

		tess.indexes[tess.numIndexes + 0] = o1;
		tess.indexes[tess.numIndexes + 1] = o2;
		tess.indexes[tess.numIndexes + 2] = o3;
		tess.indexes[tess.numIndexes + 3] = o3 + numVerts;
		tess.indexes[tess.numIndexes + 4] = o2 + numVerts;
		tess.indexes[tess.numIndexes + 5] = o1 + numVerts;
		tess.numIndexes += 6;
	}

	R_FlushShadowVolume( pipeline );
	tess.numVertexes = numVerts;
}

/*
=================
RB_ShadowTessEnd

triangleFromEdge[ v1 ][ v2 ]


  set triangle from edge( v1, v2, tri )
  if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
  }
=================
*/
void RB_ShadowTessEnd(void) {
	int			i, numTris;
	vec3_t		lightDir;
	uint32_t	pipeline[2];

	if (glConfig.stencilBits < 4)
		return;

#if 1
	vec3_t	entLight;
	vec3_t	worldxyz;
	float	groundDist;

#ifdef USE_PMLIGHT
	if (r_dlightMode->integer == 2 && R_STENCIL_SHADOWS())
		VectorCopy(backEnd.currentEntity->shadowLightDir, entLight);
	else
#endif
		VectorCopy(backEnd.currentEntity->modelLightDir, entLight);

	entLight[2] = 0.0f;
	VectorNormalize(entLight);

	//Oh well, just cast them straight down no matter what onto the ground plane.
	//This presets no chance of screwups and still looks better than a stupid
	//shader blob.
	VectorSet(lightDir, entLight[0] * 0.3f, entLight[1] * 0.3f, 1.0f);
	
	// project vertexes away from light direction
	for (i = 0; i < tess.numVertexes; i++) {
		//add or.origin to vert xyz to end up with world oriented coord, then figure
		//out the ground pos for the vert to project the shadow volume to
		VectorAdd(tess.xyz[i], backEnd.ori.origin, worldxyz);
		groundDist = worldxyz[2] - backEnd.currentEntity->e.shadowPlane;
		groundDist += 16.0f; //fudge factor
		VectorMA(tess.xyz[i], -groundDist, lightDir, tess.xyz[i + tess.numVertexes]);
	}
#else
#ifdef USE_PMLIGHT
	if (r_dlightMode->integer == 2 && R_STENCIL_SHADOWS())
		VectorCopy(backEnd.currentEntity->shadowLightDir, lightDir);
	else
#endif
		VectorCopy(backEnd.currentEntity->modelLightDir, lightDir);

	// clamp projection by height
	if (lightDir[2] > 0.1) {
		float s = 0.1 / lightDir[2];
		VectorScale(lightDir, s, lightDir);
	}

	// project vertexes away from light direction
	for (i = 0; i < tess.numVertexes; i++) {
		VectorMA(tess.xyz[i], -512, lightDir, tess.xyz[i + tess.numVertexes]);
	}
#endif 

	// decide which triangles face the light
	Com_Memset(numEdgeDefs, 0, tess.numVertexes * sizeof(numEdgeDefs[0]));

	numTris = tess.numIndexes / 3;
	for (i = 0; i < numTris; i++)
	{
		int		i1, i2, i3;
		vec3_t	d1, d2, normal;
		float	*v1, *v2, *v3;
		float	d;

		i1 = tess.indexes[ i*3 + 0 ];
		i2 = tess.indexes[ i*3 + 1];
		i3 = tess.indexes[ i*3 + 2];

		v1 = tess.xyz[ i1 ];
		v2 = tess.xyz[ i2 ];
		v3 = tess.xyz[ i3 ];

		VectorSubtract( v2, v1, d1 );
		VectorSubtract( v3, v1, d2 );
		CrossProduct( d1, d2, normal );

		d = DotProduct( normal, lightDir );
		if ( d > 0 ) {
			facing[i] = 1;
		}
		else {
			facing[i] = 0;
		}

		// create the edges
		R_AddEdgeDef( i1, i2, facing[i] );
		R_AddEdgeDef( i2, i3, facing[i] );
		R_AddEdgeDef( i3, i1, facing[i] );
	}

	vk_bind( tr.whiteImage );

	// mirrors have the culling order reversed
	if ( backEnd.viewParms.portalView == PV_MIRROR ) {
		pipeline[0] = vk.std_pipeline.shadow_volume_pipelines[0][1];
		pipeline[1] = vk.std_pipeline.shadow_volume_pipelines[1][1];
	}
	else {
		pipeline[0] = vk.std_pipeline.shadow_volume_pipelines[0][0];
		pipeline[1] = vk.std_pipeline.shadow_volume_pipelines[1][0];
	}

	R_BuildShadowVolume( pipeline );

	if ( R_SHADOW_DEBUG )
		ri.Printf( PRINT_ALL, "G2SHADOWDEBUG: CPU shadow volume: %d triangles in\n", numTris );

	backEnd.doneShadows = qtrue;
	tess.numIndexes = 0;
}


/*
=================
RB_ShadowFinish

Darken everything that is is a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish(void)
{
	float tmp[16];
	int i;

	if ( R_SHADOW_DEBUG )
		ri.Printf( PRINT_ALL, "G2SHADOWDEBUG: RB_ShadowFinish called, doneShadows=%d r_shadows=%d stencilBits=%d\n",
			backEnd.doneShadows, r_shadows->integer, glConfig.stencilBits );

	if (!backEnd.doneShadows)
		return;

	backEnd.doneShadows = qfalse;

	if (!R_STENCIL_SHADOWS())
		return;

	if (glConfig.stencilBits < 4)
		return;

	if ( R_SHADOW_DEBUG )
		ri.Printf( PRINT_ALL, "G2SHADOWDEBUG: RB_ShadowFinish drawing darkening quad\n" );

	static const vec3_t verts[4] = {
		{ -100, 100, -10 },
		{  100, 100, -10 },
		{ -100,-100, -10 },
		{  100,-100, -10 }
	};

	vk_bind(tr.whiteImage);

	for (i = 0; i < 4; i++)
	{
		VectorCopy(verts[i], tess.xyz[i]);
		// black at 50% alpha, as rd-vanilla does
		Vector4Set(tess.svars.colors[0][i], 0, 0, 0, 128);
	}
	tess.numVertexes = 4;

	Com_Memcpy(tmp, vk_world.modelview_transform, 64);
	Com_Memset(vk_world.modelview_transform, 0, 64);

	vk_world.modelview_transform[0] = 1.0f;
	vk_world.modelview_transform[5] = 1.0f;
	vk_world.modelview_transform[10] = 1.0f;
	vk_world.modelview_transform[15] = 1.0f;

	vk_bind_pipeline(vk.std_pipeline.shadow_finish_pipeline);
	vk_update_mvp(NULL);
	vk_bind_geometry(TESS_XYZ | TESS_RGBA0);
	vk_draw_geometry(DEPTH_RANGE_NORMAL, qfalse);

	Com_Memcpy(vk_world.modelview_transform, tmp, 64);

	tess.numIndexes = 0;
	tess.numVertexes = 0;
}


/*
=================
RB_ProjectionShadowDeform

=================
*/
void RB_ProjectionShadowDeform(void)
{
	int		i;
	float	*xyz;
	vec3_t	ground;
	float	groundDist, d, h;
	vec3_t	light;
	vec3_t	lightDir;

	xyz = (float*)tess.xyz;

	ground[0] = backEnd.ori.axis[0][2];
	ground[1] = backEnd.ori.axis[1][2];
	ground[2] = backEnd.ori.axis[2][2];

	groundDist = backEnd.ori.origin[2] - backEnd.currentEntity->e.shadowPlane;

#ifdef USE_PMLIGHT
	if (r_dlightMode->integer == 2 && R_STENCIL_SHADOWS())
		VectorCopy(backEnd.currentEntity->shadowLightDir, lightDir);
	else
#endif
		VectorCopy(backEnd.currentEntity->modelLightDir, lightDir);

	d = DotProduct(lightDir, ground);
	// don't let the shadows get too long or go negative
	if (d < 0.5)
	{
		VectorMA(lightDir, (0.5 - d), ground, lightDir);
		d = DotProduct(lightDir, ground);
	}
	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;


	for (i = 0; i < tess.numVertexes; i++, xyz += 4)
	{
		h = DotProduct(xyz, ground) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}
