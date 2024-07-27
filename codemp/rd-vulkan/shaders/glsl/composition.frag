#version 450

layout(set = 0, binding = 0) uniform sampler2D in_gbuffer;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 out_color;

	#define RGB9E5_EXPONENT_BITS          5
	#define RGB9E5_MANTISSA_BITS          9
	#define RGB9E5_EXP_BIAS               15
	#define RGB9E5_MAX_VALID_BIASED_EXP   31

	#define MAX_RGB9E5_EXP               ( RGB9E5_MAX_VALID_BIASED_EXP - RGB9E5_EXP_BIAS )
	#define RGB9E5_MANTISSA_VALUES       ( 1<<RGB9E5_MANTISSA_BITS )
	#define MAX_RGB9E5_MANTISSA          ( RGB9E5_MANTISSA_VALUES-1 )
	#define MAX_RGB9E5                   ( (float(MAX_RGB9E5_MANTISSA))/RGB9E5_MANTISSA_VALUES * (1<<MAX_RGB9E5_EXP) )
	#define EPSILON_RGB9E5               ( (1.0/RGB9E5_MANTISSA_VALUES) / (1<<RGB9E5_EXP_BIAS) )

	float clamp_range_for_rgb9e5( float x ) 
	{
		return clamp( x, 0.0, MAX_RGB9E5 );
	}

	int floor_log2( float x ) 
	{
		uint f = floatBitsToUint( x );
		uint biasedexponent = ( f & 0x7F800000u ) >> 23;
		return int( biasedexponent ) - 127;
	}


	// https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_shared_exponent.txt
	uint float3_to_rgb9e5( vec3 rgb ) 
	{
		float rc = clamp_range_for_rgb9e5( rgb.x );
		float gc = clamp_range_for_rgb9e5( rgb.y );
		float bc = clamp_range_for_rgb9e5( rgb.z );

		float maxrgb = max( rc, max( gc, bc ) );
		int exp_shared = max( -RGB9E5_EXP_BIAS-1, floor_log2( maxrgb ) ) + 1 + RGB9E5_EXP_BIAS;
		float denom = exp2( float( exp_shared - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS ) );

		int maxm = int( floor( maxrgb / denom + 0.5 ) );
		if ( maxm == MAX_RGB9E5_MANTISSA + 1 ) {
			denom *= 2;
			exp_shared += 1;
		}

		int rm = int( floor( rc / denom + 0.5 ) );
		int gm = int( floor( gc / denom + 0.5 ) );
		int bm = int( floor( bc / denom + 0.5 ) );

		return ( uint( rm ) << ( 32 - 9 ) )
			|  ( uint( gm ) << ( 32 - 9 * 2 ) )
			|  ( uint( bm ) << ( 32 - 9 * 3 ) )
			| uint( exp_shared );
	}

	uint bitfield_extract( uint value, uint offset, uint bits ) 
	{
		uint mask = ( 1u << bits ) - 1u;
		return ( value >> offset ) & mask;
	}

	vec3 rgb9e5_to_float3( uint v ) 
	{
		int exponent =
			int( bitfield_extract( v, 0, RGB9E5_EXPONENT_BITS ) ) - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS;
		float scale = exp2( float( exponent ) );

		return vec3(
			float( bitfield_extract( v, 32 - RGB9E5_MANTISSA_BITS, RGB9E5_MANTISSA_BITS ) ) * scale,
			float( bitfield_extract( v, 32 - RGB9E5_MANTISSA_BITS * 2, RGB9E5_MANTISSA_BITS ) ) * scale,
			float( bitfield_extract( v, 32 - RGB9E5_MANTISSA_BITS * 3, RGB9E5_MANTISSA_BITS ) ) * scale
		);
	}

	uint pack_unorm( float val, uint bitCount ) 
	{
		uint maxVal = ( 1u << bitCount ) - 1;
		return uint( clamp( val, 0.0, 1.0 ) * maxVal + 0.5 );
	}

	float unpack_unorm( uint pckd, uint bitCount ) 
	{
		uint maxVal = ( 1u << bitCount ) - 1;
		return float( pckd & maxVal ) / maxVal;
	}

	uint pack_color_888( vec3 color ) 
	{
		color = sqrt( color );
		uint pckd = 0;
		pckd += pack_unorm( color.x, 8 );
		pckd += pack_unorm( color.y, 8 ) << 8;
		pckd += pack_unorm( color.z, 8 ) << 16;
		return pckd;
	}

	vec3 unpack_color_888( uint p ) 
	{
		vec3 color = vec3(
			unpack_unorm( p, 8),
			unpack_unorm( p >> 8, 8 ),
			unpack_unorm( p >> 16, 8 )
		);
		return color * color;
	}

	float pack_normal_11_10_11( vec3 n ) 
	{
		uint pckd = 0;
		pckd += pack_unorm( n.x * 0.5 + 0.5, 11 );
		pckd += pack_unorm( n.y * 0.5 + 0.5, 10 ) << 11;
		pckd += pack_unorm( n.z * 0.5 + 0.5, 11 ) << 21;
		return uintBitsToFloat( pckd );
	}

	vec3 unpack_normal_11_10_11( float pckd ) 
	{
		uint p = floatBitsToUint( pckd );
		return normalize( vec3(
			unpack_unorm( p, 11 ),
			unpack_unorm( p >> 11, 10 ),
			unpack_unorm( p >> 21, 11 )
		) * 2.0 - 1.0 );
	}

	float roughness_to_perceptual_roughness( float r ) 
	{
		return sqrt( r );
	}

	float perceptual_roughness_to_roughness( float r ) 
	{
		return r * r;
	}

	uvec4 pack_gbuffer(	in vec3 diffuse,
						in vec3 specular,
						in vec3 normal,
						in float roughness,
						in float ao )
	{
		vec4 res = 0.0.xxxx;

		res.x = uintBitsToFloat( pack_color_888( diffuse ) );
		res.y = pack_normal_11_10_11( normal );

		vec2 roughness_ao = vec2( roughness_to_perceptual_roughness (roughness ), ao );

		res.z = uintBitsToFloat( packUnorm2x16( roughness_ao ) );
		res.w = uintBitsToFloat( float3_to_rgb9e5( specular ) );

		return floatBitsToUint( res );
	}

	void unpack_gbuffer( in uvec4 gbuffer,
						 out vec3 diffuse,
						 out vec3 specular,
						 out vec3 normal, 
						 out float roughness,
						 out float ao ) 
	{
		vec4 res = vec4( 0 );

		diffuse = unpack_color_888( gbuffer.x );
		normal = unpack_normal_11_10_11( uintBitsToFloat( gbuffer.y ) );

		vec2 roughness_ao = unpackUnorm2x16( gbuffer.z );
		roughness = perceptual_roughness_to_roughness( roughness_ao.x );
		ao = roughness_ao.y;

		specular = rgb9e5_to_float3( gbuffer.w );
	}



void main()
{
    uvec4 gbuffer = floatBitsToUint(texture(in_gbuffer, inUV));
	
	vec3 diffuse, specular, normal;
	float roughness, AO;

	unpack_gbuffer( 
			gbuffer, 
			diffuse.rgb,
			specular.rgb,
			normal.rgb,
			roughness,
			AO );



    out_color.rgb = ( normal * 0.5 + 0.5 );
    //out_color.rgb = diffuse.rgb;

    out_color.w = 1.0;
}