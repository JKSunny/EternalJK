#version 450
#extension GL_GOOGLE_include_directive : require

#include "global.h"

layout(set = 0, binding = 0) uniform sampler2D depth_texture;

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 0) out vec4 out_color;

layout(push_constant) uniform SSAOParams {
	vkExtractSSAO_t push;
};

#include "common/ssao.glsl"

float applyAOResponse(float visibility) 
{
	float ao = clamp(visibility, 0.0, 1.0);
	ao = pow(ao, push.power);
	ao = clamp(1.0 - (1.0 - ao) * push.intensity, 0.0, 1.0);
	return ao;
}

bool depthFalloffReject( in float depth )
{
	float depth_falloff = push.depth_falloff;

	if (depth_falloff <= 0.0)
		return false;

	vec2 invSize		= vec2(push.inv_width, push.inv_height);
	float viewCenter = max(depth, 1e-3);
	float dx			= sampleDepth(frag_tex_coord + vec2(invSize.x, 0.0));
	float dy			= sampleDepth(frag_tex_coord + vec2(0.0, invSize.y));
	float dxm			= sampleDepth(frag_tex_coord - vec2(invSize.x, 0.0));
	float dym			= sampleDepth(frag_tex_coord - vec2(0.0, invSize.y));
	float vx  = max(dx,  1e-3);
	float vy  = max(dy,  1e-3);
	float vxm = max(dxm, 1e-3);
	float vym = max(dym, 1e-3);

	float maxRel = max(
		max(abs(vx - viewCenter), abs(vxm - viewCenter)),
		max(abs(vy - viewCenter), abs(vym - viewCenter))
	) / viewCenter;

	float gradTol = depth_falloff;
	if (depth_falloff < 0.5)
		gradTol = max(depth_falloff * 2.5, 0.02);

	return maxRel > gradTol;
}

float computeSSAO( vec3 P, vec3 normal )
{
	vec3 randVec = normalize( vec3(
		rand01( frag_tex_coord * 13.1 ) * 2.0 - 1.0,
		rand01( frag_tex_coord * 31.7 ) * 2.0 - 1.0,
		rand01( frag_tex_coord * 57.3 ) * 2.0 - 1.0
	) );

	vec3 tangent = normalize( randVec - normal * dot( randVec, normal ) );
	vec3 bitangent = cross( normal, tangent );
	mat3 tbn = mat3( tangent, bitangent, normal );

	float radius = push.radius;
	float bias = push.bias;
	int samples = clamp( int( push.samples + 0.5 ), 1, 32 );

	float occlusion = 0.0;

	float proj00 = 1.0 / max( push.projection.x, 1e-6 );
	float proj11 = 1.0 / max( push.projection.y, 1e-6 );

	float screenRadius = getScreenRadius( radius, P.z );

	for ( int i = 0; i < 32; ++i )
	{
		if ( i >= samples )
			break;

		float u1 = halton2( i + 1 );
		float u2 = halton3( i + 1 );

		float phi = u1 * 6.28318530718;

		// Cosine-weighted hemisphere.
		float cosTheta = sqrt( 1.0 - u2 );
		float sinTheta = sqrt( u2 );

		vec3 sampleDir = vec3(
			cos( phi ) * sinTheta,
			sin( phi ) * sinTheta,
			cosTheta
		);

		sampleDir = tbn * sampleDir;

		float t = ( float( i ) + 0.5 ) / float( samples );
		float sampleRadius = radius * sqrt( t );

		vec3 samplePos = P + sampleDir * sampleRadius;

		if ( samplePos.z >= -0.001 )
			continue;

		vec2 sampleNdc = vec2(
			samplePos.x * proj00 / -samplePos.z,
			samplePos.y * proj11 / -samplePos.z
		);

		vec2 sampleUV = sampleNdc * 0.5 + 0.5;

		if ( sampleUV.x <= 0.0 || sampleUV.x >= 1.0 ||
			 sampleUV.y <= 0.0 || sampleUV.y >= 1.0 )
			continue;

		float sampleDepth = sampleDepth(sampleUV);

		if ( depthIsEmpty( sampleDepth ) )
			continue;

		float sampleViewZ = -sampleDepth;
		float surfaceDelta;
#ifdef USE_REAL_NORMAL
		vec3 sampleViewPos = reconstructViewPosition( sampleUV, sampleDepth );
		vec3 sampleDelta = sampleViewPos - P;

		// use 1.0 once real normals are used, currently missing interpolation with reconstructed normals
		float normalWeight = 0.5; 

		float normalSurfaceDelta = length( sampleDelta );
		float oldSurfaceDelta = abs( sampleViewZ - P.z );
		surfaceDelta = mix( oldSurfaceDelta, normalSurfaceDelta, normalWeight );

		float normalDepthDelta = dot( sampleDelta, normal );
		float oldDepthDelta = sampleViewZ - samplePos.z;
		float depthDelta = mix( oldDepthDelta, normalDepthDelta, normalWeight );

		if ( depthDelta <= bias )
			continue;
#else
		float deltaZ = sampleViewZ - samplePos.z;
		if ( deltaZ <= bias ) // Only surfaces in front of the sample point can occlude it.
			continue;

		// Reject completely unrelated geometry.
		surfaceDelta = abs( sampleViewZ - P.z );
#endif

		if ( surfaceDelta > radius )
			continue;

		float rangeWeight = 1.0 -
			clamp( surfaceDelta / radius, 0.0, 1.0 );

		occlusion += rangeWeight;
	}

	// normalize by the requested number of samples.
	occlusion /= float( samples );
	occlusion = clamp( occlusion, 0.0, 1.0 );

    float ao = applyAOResponse( 1.0 - occlusion);

	// fade AO when the world-space radius projects to too few pixels.
	return mix( 1.0, ao, smoothstep( 0.0, 8.0, screenRadius ) );
}

float computeGTAO(vec3 P, vec3 N)
{
	vec3 V = normalize(-P);

	float radius		= push.radius;
	float intensity		= push.intensity;
	float frameNoise	= push.frame_noise;
	int sliceCount		= int(push.slices);
	int stepCount		= int(push.steps);
	vec2 invScreen		= vec2(push.inv_width, push.inv_height);
	float screenRadius	= clamp( getScreenRadius( radius, P.z ), 8.0, 128.0 );

	vec2 pixel = gl_FragCoord.xy;
	float noise = fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))) + frameNoise );

	float visibilitySum = 0.0;

	for (int slice = 0; slice < sliceCount; slice++) {
		float phi = (float(slice) + noise) * M_PI / float(sliceCount);
		vec2 omega = vec2(cos(phi), sin(phi));

		vec3 sliceDir = reconstructViewPosition(frag_tex_coord + omega * invScreen, sampleDepth(frag_tex_coord)) - P;
		float sliceDirLen = length(sliceDir);

		if (sliceDirLen < 1e-6)
			continue;

		sliceDir /= sliceDirLen;

		vec3 orthoDir = sliceDir - dot(sliceDir, V) * V;
		vec3 axis = cross(sliceDir, V);
		vec3 projN = N - axis * dot(N, axis);

		float projNLen = length(projN);

		if (projNLen < 1e-5)
			continue;

		vec3 projNn = projN / projNLen;

		float sgn = sign(dot(orthoDir, projNn));
		float n = sgn * acos(clamp(dot(projNn, V), -1.0, 1.0));

		float h[2];
		h[0] = -M_HALF_PI;
		h[1] =  M_HALF_PI;

		for (int side = 0; side < 2; side++) {
			float dirSign = (side == 0) ? -1.0 : 1.0;
			float cosHorizon = -1.0;

			for (int s = 1; s <= stepCount; s++) {
				float t = (float(s) - 0.5 + noise) / float(stepCount);
				vec2 offset = omega * dirSign * t * screenRadius * invScreen;
				vec2 sampleUV = frag_tex_coord + offset;

				if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
					break;

				float sampleDepth = sampleDepth(sampleUV);
				if (depthIsEmpty(sampleDepth))
					return 1.0;

				vec3 S = reconstructViewPosition(sampleUV, sampleDepth) - P;
				float dist = length(S);
				if (dist < 1e-4)
					continue;

				float falloff = clamp(1.0 - (dist / (radius * 2.0)), 0.0, 1.0);
				float cosH = dot(S / dist, V);
				cosHorizon = max(cosHorizon, mix(-1.0, cosH, falloff));
			}

			h[side] = dirSign * acos(clamp(cosHorizon, -1.0, 1.0));
		}

		h[0] = n + max(h[0] - n, -M_HALF_PI);
		h[1] = n + min(h[1] - n,  M_HALF_PI);

		float sinN = sin(n);
		float visibility =
			0.25 * (-cos(2.0 * h[0] - n) + cos(n) + 2.0 * h[0] * sinN) +
			0.25 * (-cos(2.0 * h[1] - n) + cos(n) + 2.0 * h[1] * sinN);

		visibilitySum += projNLen * visibility;
	}

	float occlusion = clamp(visibilitySum / float(sliceCount), 0.0, 1.0);

	return pow(occlusion, intensity);
}


void main() {
	float depth = sampleDepth(frag_tex_coord);

	if ( depthIsEmpty( depth ) ) {
		out_color = vec4(1.0);
		return;
	}

	vec3 P = reconstructViewPosition( frag_tex_coord, depth );
	vec3 dx = dFdx(P);
	vec3 dy = dFdy(P);
	vec3 N	= normalize(cross(dx, dy));

	// Orient normal toward the camera.
	if ( dot(N, -P) < 0.0 )
		N = -N;

	// GTAO
	if ( push.mode == 2u ) {
		float ao = computeGTAO( P, N );
		out_color = vec4(ao, ao, ao, 1.0);
		return;
	}

	// SSAO
	if ( depthFalloffReject( depth ) ) {
		out_color = vec4(1.0);
		return;
	}

	float ao = computeSSAO( P, N );
	out_color = vec4(ao, ao, ao, 1.0);
}