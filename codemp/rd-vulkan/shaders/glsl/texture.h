#if defined(PER_PIXEL_LIGHTING) && !defined(USE_FAST_LIGHT)
	layout (constant_id = 9) const int normal_texture_set = 0;
	layout (constant_id = 10) const int physical_texture_set = 0;
	layout (constant_id = 11) const int env_texture_set = 0;
#endif

#ifdef VK_BINDLESS
	// bindless texture indexes
	#define VK_BINDLESS_BINDING_TEXTURES	0
	#define VK_BINDLESS_BINDING_CUBEMAPS	1
	#define VK_BINDLESS_BINDING_BRDFLUT		2

	// Textures
	layout(set = VK_DESC_TEXTURES_BINDLESS, binding = VK_BINDLESS_BINDING_TEXTURES ) uniform sampler2D texure_array[];
	layout(set = VK_DESC_TEXTURES_BINDLESS, binding = VK_BINDLESS_BINDING_CUBEMAPS ) uniform samplerCube cubemap_array[];
	layout(set = VK_DESC_TEXTURES_BINDLESS, binding = VK_BINDLESS_BINDING_BRDFLUT ) uniform sampler2D brdflut_texture;

	vec4 global_texture( in uint idx, in vec2 tex_coord )
	{
		return texture(texure_array[nonuniformEXT(global_data.texture_idx[idx])], tex_coord);
	}

	ivec2 global_texture_size( in uint idx, in int lod )
	{
		return textureSize(texure_array[nonuniformEXT(global_data.texture_idx[idx])], lod);
	}

	// texture idx offsets
	#define texture0						0
	#define texture1						1
	#define texture2						2
	#define fog_texture						3
	#define normal_texture					4
	#define physical_texture				5
	#define env_texture						cubemap_array[nonuniformEXT(global_data.texture_idx[6])]
#else
	layout(set = 1, binding = 0) uniform sampler2D texture0;

	#ifdef USE_TX1
		layout(set = 2, binding = 0) uniform sampler2D texture1;
	#endif

	#ifdef USE_TX2
		layout(set = 3, binding = 0) uniform sampler2D texture2;
	#endif

	#ifdef USE_FOG
		layout(set = 4, binding = 0) uniform sampler2D fog_texture;
	#endif

	#if defined(PER_PIXEL_LIGHTING) && !defined(USE_FAST_LIGHT)
		layout (set = 5, binding = 0) uniform sampler2D brdflut_texture;
		layout (set = 6, binding = 0) uniform sampler2D normal_texture;
		layout (set = 7, binding = 0) uniform sampler2D physical_texture;
		layout (set = 8, binding = 0) uniform samplerCube env_texture;
		//layout(set = 10, binding = 0) uniform samplerCube irradiance_texture;
	#endif

	#define global_texture texture
	#define global_texture_size textureSize
#endif

float CorrectAlpha(float threshold, float alpha, vec2 tc)
{
	ivec2 ts = global_texture_size(texture0, 0);
	float dx = max(abs(dFdx(tc.x * float(ts.x))), 0.001);
	float dy = max(abs(dFdy(tc.y * float(ts.y))), 0.001);
	float dxy = max(dx, dy); // apply the smallest boost
	float scale = max(1.0 / dxy, 1.0);
	float ac = threshold + (alpha - threshold) * scale;
	return ac;
}

#if defined(PER_PIXEL_LIGHTING) && !defined(USE_FAST_LIGHT)
	float WrapLambert( in float NL, in float w )
	{
		return clamp((NL + w) / pow(1.0 + w, 2.0), 0.0, 1.0);
	}

	vec3 Diffuse_Lambert(in vec3 DiffuseColor)
	{
		return DiffuseColor * (1.0 / PI);
	}

	vec3 CalcNormal( in vec3 vertexNormal, in vec2 frag_tex_coord )
	{
		if ( normal_texture_set > -1 ) {
			vec3 n = global_texture(normal_texture, frag_tex_coord).agb - vec3(0.5);

			n.xy *= u_NormalScale.xy;
			n.z = sqrt(clamp((0.25 - n.x * n.x) - n.y * n.y, 0.0, 1.0));
			n = n.x * in_ntb.tangent + n.y * in_ntb.binormal + n.z * vertexNormal;

			return normalize(n);
		}
	
		else
			return normalize(vertexNormal);
	}

	vec3 CalcDiffuse( in vec3 diffuse, in float NE, in float NL,
		in float LH, in float roughness )
	{
		return Diffuse_Lambert(diffuse);
	}

	vec3 F_Schlick( in vec3 SpecularColor, in float VH )
	{
		float Fc = pow(1 - VH, 5);
		return clamp(50.0 * SpecularColor.g, 0.0, 1.0) * Fc + (1 - Fc) * SpecularColor; //hacky way to decide if reflectivity is too low (< 2%)
	}

	float D_GGX( in float NH, in float a )
	{
		float a2 = a * a;
		float d = (NH * a2 - NH) * NH + 1;
		return a2 / (PI * d * d);
	}

	// Appoximation of joint Smith term for GGX
	// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
	float V_SmithJointApprox( in float a, in float NV, in float NL )
	{
		float Vis_SmithV = NL * (NV * (1 - a) + a);
		float Vis_SmithL = NV * (NL * (1 - a) + a);
		return 0.5 * (1.0 / (Vis_SmithV + Vis_SmithL));
	}

	vec3 CalcSpecular( in vec3 specular, in float NH, in float NL,
		in float NE, float LH, in float VH, in float roughness )
	{
		vec3  F = F_Schlick(specular, VH);
		float D = D_GGX(NH, roughness);
		float V = V_SmithJointApprox(roughness, NE, NL);

		return D * F * V;
	}

	vec3 CalcIBLContribution( in float roughness, in vec3 N, in vec3 E,
		in float NE, in vec3 specular )
	{
		vec3 R = reflect(-E, N);
		R.y *= -1.0f;
	
		vec3 cubeLightColor = textureLod(env_texture, R, roughness * 6).rgb * 1.0;
		vec2 EnvBRDF = texture(brdflut_texture, vec2(NE, 1.0 - roughness)).rg;

		return cubeLightColor * (specular.rgb * EnvBRDF.x + EnvBRDF.y);
	}

	float CalcLightAttenuation(float normDist)
	{
		// zero light at 1.0, approximating q3 style
		float attenuation = 0.5 * normDist - 0.5;
		return clamp(attenuation, 0.0, 1.0);
	}

#ifdef VK_DLIGHT_GPU
	vec3 CalcDynamicLightContribution(
	in float roughness,
	in vec3 N,
	in vec3 E,
	in vec3 viewOrigin,
	in vec3 viewDir,
	in float NE,
	in vec3 diffuse,
	in vec3 specular,
	in vec3 vertexNormal
	)
	{
		vec3 outColor = vec3(0.0);
		vec3 position = viewOrigin - viewDir;

		for ( int i = 0; i < lights.num_lights; i++ )
		{
			#if 0
				if ( ( u_LightMask & ( 1 << i ) ) == 0 ) {
					continue;
				}
			#endif

			vkUniformLightEntry_t light = lights.light[i];

			vec3  L  = light.origin.xyz - position;
			float sqrLightDist = dot(L, L);

			// sunny, skip if light is out of radius
			float radiusSqr = light.radius * light.radius;
			if (sqrLightDist > radiusSqr) {
				continue;
			}

			float attenuation = CalcLightAttenuation(light.radius * light.radius / sqrLightDist);

			// sunny, skip lights with negligible intensity
			if (attenuation < 0.001) {
				continue;
			}

			L /= sqrt(sqrLightDist);

			vec3  H  = normalize(L + E);
			float NL = clamp(dot(N, L), 0.0, 1.0);
			float LH = clamp(dot(L, H), 0.0, 1.0);
			float NH = clamp(dot(N, H), 0.0, 1.0);
			float VH = clamp(dot(E, H), 0.0, 1.0);

			vec3 reflectance = diffuse + CalcSpecular(specular, NH, NL, NE, LH, VH, roughness);

			outColor += light.color * reflectance * attenuation * NL;
		}

		return outColor;
	}
#endif
#endif