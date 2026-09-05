#version 450

// Shadow volume extrusion, geometry stage. Mirrors rd-vanilla's
// R_RenderShadowEdges (tr_shadows.cpp) exactly: every edge of every light-facing
// triangle gets an extruded quad, and the volume is capped near and far because
// Carmack's Reverse needs it. There is no silhouette test - interior edges emit
// pairs of opposite winding that cancel in the stencil, which costs a little
// fill and buys immunity to edges shared by more than two triangles.
//
// That also means no adjacency: this consumes the model's ordinary index buffer.

layout(triangles) in;
layout(triangle_strip, max_vertices = 24) out;

// Must match shadow_volume.vert's stripped-down gl_PerVertex member for member;
// without this, gl_in[] assumes the full default block and fails SPIR-V
// interface validation against the vertex stage.
in gl_PerVertex {
	vec4 gl_Position;
} gl_in[];

layout(push_constant) uniform Transform {
	mat4 mvp;
	vec3 lightDir;
	// entity.origin.z - shadowPlane + fudge in one constant, so extrusion needs
	// a single add (matches RB_ShadowTessEnd's groundDist).
	float groundOffset;
	// r_g2_shadowedges (Debug builds): bit 0 emits the sides, bit 1 the caps.
	int edgeMask;
};

out gl_PerVertex {
	vec4 gl_Position;
};

vec3 extrude( vec3 pos )
{
	float groundDist = pos.z + groundOffset;
	return pos - groundDist * lightDir;
}

void vertex( vec3 p )
{
	gl_Position = mvp * vec4( p, 1.0 );
	EmitVertex();
}

// Same two triangles, in the same order, that R_BuildShadowVolume emits for one
// edge: (a, aExt, b) then (b, aExt, bExt).
void emitSide( vec3 a, vec3 b )
{
	if ( ( edgeMask & 1 ) == 0 )
		return;

	vertex( a );
	vertex( extrude( a ) );
	vertex( b );
	EndPrimitive();

	vertex( b );
	vertex( extrude( a ) );
	vertex( extrude( b ) );
	EndPrimitive();
}

void main() {
	vec3 p0 = gl_in[0].gl_Position.xyz;
	vec3 p1 = gl_in[1].gl_Position.xyz;
	vec3 p2 = gl_in[2].gl_Position.xyz;

	if ( dot( cross( p1 - p0, p2 - p0 ), lightDir ) <= 0.0 )
		return; // this triangle does not face the light

	emitSide( p0, p1 );
	emitSide( p1, p2 );
	emitSide( p2, p0 );

	if ( ( edgeMask & 2 ) == 0 )
		return;

	vertex( p0 );
	vertex( p1 );
	vertex( p2 );
	EndPrimitive();

	vertex( extrude( p2 ) );
	vertex( extrude( p1 ) );
	vertex( extrude( p0 ) );
	EndPrimitive();
}
