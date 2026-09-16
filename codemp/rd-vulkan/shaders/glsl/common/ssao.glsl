//
// math
//
float halton2( in int i ) 
{
	float r = 0.0;
	float f = 0.5;
	int n	= i;

	while ( n > 0 ) {
		r += f * float( n & 1 );
		n /= 2;
		f *= 0.5;
	}
	return r;
}

float halton3( in int i ) 
{
	float r	= 0.0;
	float f	= 1.0 / 3.0;
	int n	= i;

	while ( n > 0 ) {
		r += f * float( n % 3 );
		n /= 3;
		f *= 1.0 / 3.0;
	}
	return r;
}

float rand01( vec2 co ) 
{
	return fract( sin( dot( co, vec2( 12.9898, 78.233 ) ) ) * 43758.5453 );
}

float getScreenRadius( in float radius, in float viewZ )
{
	return radius / push.projection.x * 0.5 / (-viewZ) / push.inv_width;
}


//
// sampling
//
#ifdef IS_BLUR_PASS
float sampleSSAO( in vec2 uv ) 
{
#ifdef USE_VK_IMGUI
	return textureLod(ssao_texture, uv * push.texture_scale, 0.0).r;
#else
	return textureLod(ssao_texture, uv, 0.0).r;
#endif
}
#endif

float sampleDepth( in vec2 uv ) 
{
#ifdef USE_VK_IMGUI
	return textureLod(depth_texture, uv * push.texture_scale, 0.0).r;
#else
	return textureLod(depth_texture, uv, 0.0).r;
#endif
}

//
// depth
//
bool depthIsEmpty( in float d ) 
{
	return d <= 0.0;
}

vec3 reconstructViewPosition( in vec2 uv, in float linear_depth )
{
    vec2 ndc = uv * 2.0 - 1.0;

    float z = -linear_depth;

    return vec3(
        ndc.x * linear_depth * push.projection.x,
        ndc.y * linear_depth * push.projection.y,
        z
    );
}
