#version 450
#extension GL_GOOGLE_include_directive : require

#include "global.h"

layout(set = 0, binding = 0) uniform sampler2D ssao_texture;
layout(set = 1, binding = 0) uniform sampler2D depth_texture;

layout(location = 0) in vec2 frag_tex_coord;
layout(location = 0) out vec4 out_color;

layout(push_constant) uniform SSAOParams {
	vkBlurSSAO_t push;
};

#define IS_BLUR_PASS
#include "common/ssao.glsl"

float bilateralDepthWeight( float centerDepth, float sampleDepth, float sharpness )
{
	float cd	= max( centerDepth, 1e-3 );
	float delta = abs( sampleDepth - centerDepth );
	float rel	= delta / cd;

	return exp2( -rel * rel * max( sharpness, 0.01 ) );
}

void main()
{
	int radius		= int( push.depth_radius );
	float centerAo	= sampleSSAO( frag_tex_coord );

	if ( radius <= 0 ) {
		out_color = vec4( centerAo, centerAo, centerAo, 1.0 );
		return;
	}

	float centerDepth = sampleDepth( frag_tex_coord );
	if ( centerDepth <= 0.0 ) {
		out_color = vec4( centerAo, centerAo, centerAo, 1.0 );
		return;
	}

	float depthSharp	= max( push.depth_sharpness, 1.0 );
	vec2 texel			= vec2( push.inv_width, push.inv_height );
	float sum			= 0.0;
	float weightSum		= 0.0;

	for ( int y = -4; y <= 4; ++y ) {
		if ( abs( y ) > radius ) {
			continue;
		}
		for ( int x = -4; x <= 4; ++x ) {
			if ( abs( x ) > radius ) {
				continue;
			}
			vec2 suv		= frag_tex_coord + vec2( x, y ) * texel;
			float ao		= sampleSSAO( suv );
			float sd		= sampleDepth( suv );
			float spatial	= 1.0 - ( abs( float( x ) ) + abs( float( y ) ) ) / ( float( radius ) * 2.0 + 1.0 );
			float depthW	= 1.0;

			depthW = bilateralDepthWeight( centerDepth, sd, depthSharp ) * float( sd > 0.0 );

			float w = spatial * depthW;
			sum += ao * w;
			weightSum += w;
		}
	}

	float result = ( weightSum < 1e-5 ) ? centerAo : ( sum / weightSum );
	out_color = vec4( result, result, result, 1.0 );
}
