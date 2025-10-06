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

static rtx_material_t rtx_materials[MAX_SHADERS];


void vk_rtx_clear_material_list( void ) 
{
	Com_Memset( &rtx_materials, 0, sizeof(rtx_materials) );
}

void vk_rtx_clear_material( uint32_t index ) 
{
	Com_Memset( rtx_materials + index, 0, sizeof(rtx_material_t) );
}

 rtx_material_t *vk_rtx_get_material( uint32_t index ) 
{
	if ( index >= MAX_SHADERS )
		return NULL;

	if ( !rtx_materials[index].active )
		vk_rtx_clear_material( index );

	return &rtx_materials[index];
}

qboolean RB_IsTransparent( shader_t *shader ) 
{
	// skip certain objects that are transparent but should be handled like opaque objects
	if ( strstr(shader->name, "glass" ) )
		return qfalse;

	if ( ( shader->contentFlags & CONTENTS_TRANSLUCENT ) == CONTENTS_TRANSLUCENT && shader->sort == SS_OPAQUE ) 
		return qfalse;

	if ( ( shader->contentFlags & CONTENTS_TRANSLUCENT ) == CONTENTS_TRANSLUCENT || shader->sort > SS_OPAQUE ) 
		return qtrue;

	return qfalse;
}

qboolean RB_IsSky(shader_t* shader)
{
	return (qboolean)(shader->isSky || shader->sun || (shader->surfaceFlags & SURF_SKY));
}

qboolean RB_IsDynamicGeometry( shader_t *shader ) 
{
	return (qboolean)((shader->numDeforms > 0) || (backEnd.currentEntity->e.frame > 0 || backEnd.currentEntity->e.oldframe > 0));
}

qboolean RB_IsDynamicMaterial( shader_t *shader ) {
	uint32_t i, j;
	qboolean changes = qfalse;

	for ( i = 0; i < MAX_SHADER_STAGES; i++ ) 
	{
		if ( shader->stages[i] != NULL && shader->stages[i]->active ) 
		{
			for ( j = 0; j < shader->stages[i]->numTexBundles; j++ ) 
			{

				if ( shader->stages[i]->bundle[j].numImageAnimations > 0 ) 
					return qtrue;

				if ( (shader->stages[i]->bundle[j].tcGen != TCGEN_BAD) && (shader->stages[i]->bundle[j].numTexMods > 0 ) ) 
					return qtrue;

				if ( shader->stages[i]->bundle[0].rgbGen == CGEN_WAVEFORM )
					return qtrue;
			}
		}
	}
	return changes;
}

static qboolean RB_NeedsColor() {

	for (int i = 0; i < MAX_SHADER_STAGES; i++) {
		if (tess.shader->stages[i] != NULL && tess.shader->stages[i]->active) {
			if (tess.shader->stages[i]->bundle[0].rgbGen == CGEN_WAVEFORM) {
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean RB_StageNeedsColor( shaderStage_t *stage ) 
{
	//if ( strstr( tess.shader->name, "fog" ) )
		//return qtrue;

	if ( stage == NULL || !stage->active ) 
		return qfalse;

	if ( stage->bundle[0].rgbGen == CGEN_WAVEFORM || stage->bundle[0].rgbGen == CGEN_CONST ) {
		return qtrue;
	}

	return qfalse;
}

qboolean RB_SkipObject(shader_t* shader) {
	
	if ( strstr( shader->name, "glass" ) )
		return qfalse;

	if ( RB_IsSky(shader) )
		return qfalse;

	if ( strstr( shader->name, "Shadow" )
		|| shader->surfaceFlags == SURF_NODRAW /*|| shader->surfaceFlags == SURF_NONE*///SURF_SKIP
		|| shader->stages[0] == NULL 
		|| !shader->stages[0]->active )
		return qtrue;

	return qfalse;
}

// refactor me
uint32_t vk_rtx_find_emissive_texture( const shader_t *shader, rtx_material_t *material )
{
	uint32_t i, j;

	uint32_t lastValidStage = 0;
	uint32_t numStages = 0;
	shaderStage_t *pStage;

	for ( i = 0; i < MAX_RTX_STAGES; i++ ) 
	{
		pStage = shader->stages[i];

		if ( !pStage || !pStage->active )
			continue;

		lastValidStage = i;
		numStages++;

		if ( !pStage->glow )
			continue;

		for ( j = 0; j < pStage->numTexBundles; ++j ) 
		{
			if ( pStage->bundle[j].glow && pStage->bundle[j].image[0] ) 
			{
				//Com_Printf("found glow texture: %d = %s", pStage->bundle[j].image[0]->index, pStage->bundle[j].image[0]->imgName );
				
				return pStage->bundle[j].image[0]->index;
			}
		}

	}

	// no glow texture found, try surfacelight fallback type
	if ( !shader->surfacelight )
		return 0;

	// masked light texture is usualy in the last stage. 
	uint32_t stage = (numStages == 0) ? 0 : lastValidStage;
	pStage = shader->stages[stage];

	if ( !pStage ) // check if stage 0 exists
		return 0;

	const int bundle = (pStage->numTexBundles == 0) ? 0 : pStage->numTexBundles-1;
	image_t *fallback = pStage->bundle[bundle].image[0];

	if ( fallback ) 
	{
		if ( material != NULL )
			material->surface_light = shader->surfacelight;

		return fallback->index;
	}
	return 0;
}

uint32_t RB_GetMaterial( shader_t *shader ) 
{
	uint32_t material = 0;
	material = MATERIAL_KIND_REGULAR;

	if ( vk_rtx_find_emissive_texture( shader, NULL ) )
		material |= MATERIAL_FLAG_LIGHT;
	
	if ( strstr( shader->name, "glass" ) )
		material = MATERIAL_KIND_GLASS;

	if ( shader->sort == SS_PORTAL && strstr( shader->name, "mirror" ) != NULL ) 
		material |= MATERIAL_FLAG_MIRROR;

	if ( ( backEnd.currentEntity && backEnd.currentEntity->e.renderfx & RF_FIRST_PERSON ) )
		material |= MATERIAL_FLAG_WEAPON;

	if ( RB_IsSky(shader) )
		material |= MATERIAL_KIND_SKY;

	// sunny
	if ( strstr( shader->name, "botton") || strstr( shader->name, "door02") ) 
		material = MATERIAL_KIND_GLASS;

	//material |= MATERIAL_FLAG_CORRECT_ALBEDO;

	return material;
}

uint32_t RB_GetNextTex( shader_t *shader, int stage ) 
{
	int indexAnim = 0;
	if (shader->stages[stage]->bundle[0].numImageAnimations > 1) 
	{
		// tess.shadertime is wrong if shader != tess.shader
		// sunny
		indexAnim = (int)(tess.shaderTime * shader->stages[stage]->bundle[0].imageAnimationSpeed * FUNCTABLE_SIZE);
		indexAnim >>= FUNCTABLE_SIZE2;
		if (indexAnim < 0) {
			indexAnim = 0;	// may happen with shader time offsets
		}
		indexAnim %= shader->stages[stage]->bundle[0].numImageAnimations;
	}
	return indexAnim;
}

uint32_t RB_GetNextTexEncoded( shader_t *shader, int stage ) 
{
	if (shader->stages[stage] != NULL && shader->stages[stage]->active) 
	{
		int indexAnim = RB_GetNextTex( shader, stage );

		uint32_t blend = 0;
		uint32_t stateBits = shader->stages[stage]->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
		
		if ((stateBits & GLS_SRCBLEND_BITS) > GLS_SRCBLEND_ONE && (stateBits & GLS_DSTBLEND_BITS) > GLS_DSTBLEND_ONE) 
			blend = TEX0_NORMAL_BLEND_MASK;

		if (stateBits == 19) 
			blend = TEX0_MUL_BLEND_MASK;

		if (stateBits == 34 || stateBits == 1073742080) 
			blend = TEX0_ADD_BLEND_MASK;

		if (stateBits == 101) 
			blend = TEX0_NORMAL_BLEND_MASK;

		qboolean color = RB_StageNeedsColor( shader->stages[stage] );

		uint32_t nextidx = (uint32_t)indexAnim;
		uint32_t idx = shader->stages[stage]->bundle[0].image[nextidx]->index;

		shader->stages[stage]->bundle[0].image[nextidx]->frameUsed = tr.frameCount;
		return (idx) ;//| (blend) | (color ? TEX0_COLOR_MASK : 0);
	}
	return TEX0_IDX_MASK;
}

void vk_rtx_update_shader_material( shader_t *shader, shader_t *updatedShader )
{
	uint32_t index, flags;

	if ( !vk.rtxActive )
		return;

	// clear material first so mat->uploaded[idx] is reset reupload in the next frame in
	// vk_rtx_upload_materials() 

	if ( updatedShader )
	{
		vk_rtx_clear_material( (uint32_t)updatedShader->index );
		vk_rtx_shader_to_material( updatedShader, index, flags );
	}

	if ( shader )
	{
		vk_rtx_clear_material( (uint32_t)shader->index );
		vk_rtx_shader_to_material( shader, index, flags );
	}
}

VkResult vk_rtx_shader_to_material( shader_t *shader, uint32_t &index, uint32_t &flags )
{
	shader_t			*state;
	const shaderStage_t *pStage;
	rtx_material_t		*mat;
	uint32_t			i;

	state = (shader->remappedShader) ? shader->remappedShader : NULL;

	if ( shader->updatedShader )
		state = shader->updatedShader;

	index = 0U;
	flags = 0U;

	mat = vk_rtx_get_material( (uint32_t)shader->index );

	if ( !mat )
		return VK_SUCCESS;

	index	= (uint32_t)shader->index;
	flags	= RB_GetMaterial( shader );	// ~sunny, this should be shader or remapped no?

	mat->index			= index;
	mat->remappedIndex	= (state) ? (uint32_t)state->index : 0U;
	mat->active			= qtrue;
	mat->albedo			= RB_GetNextTexEncoded( shader, 0 );
	mat->emissive		= vk_rtx_find_emissive_texture( shader, mat );

	if ( mat->emissive )
		mat->emissive_factor = 1.0f;

	if ( mat->index >= (int)MATERIAL_INDEX_MASK || mat->remappedIndex >= (int)MATERIAL_INDEX_MASK  )
		ri.Error( ERR_DROP, "%s() - MATERIAL_INDEX_MASK(4095) hit. Need to finaly seperate material_index from material_flags", __func__ );

	for ( i = 0; i < MAX_RTX_STAGES; i++ ) 
	{
		pStage = shader->stages[i];

		if ( !pStage || !pStage->active )
			break;

		if ( pStage->vk_pbr_flags ) 
		{
			if ( pStage->vk_pbr_flags & PBR_HAS_NORMALMAP )
				mat->normals = pStage->normalMap->index;

			if ( pStage->vk_pbr_flags & PBR_HAS_PHYSICALMAP || pStage->vk_pbr_flags & PBR_HAS_SPECULARMAP )
				mat->phyiscal = pStage->physicalMap->index;

			Com_Memcpy( mat->specular_scale, pStage->specularScale, sizeof(vec4_t));
		}
	}

	return VK_SUCCESS;
}

VkResult vk_rtx_upload_materials( LightBuffer *lbo )
{
	uint32_t i;
	rtx_material_t *mat;

	for ( i = 0; i < MAX_SHADERS; i++ ) 
	{
		mat = vk_rtx_get_material( i );

		if ( !mat || !mat->active || mat->uploaded[vk.current_frame_index] ) 
			continue;

		uint32_t *data = lbo->material_table + i * MATERIAL_UINTS;
		memset(data, 0, sizeof(uint32_t) * MATERIAL_UINTS);
		if ( mat->albedo )		data[0] |= mat->albedo;
		if ( mat->emissive )	data[0] |= mat->emissive << 16;
		if ( mat->normals )		data[1] |= mat->normals;
		if ( mat->phyiscal )	data[1] |= mat->phyiscal << 16;

		data[2] =	floatToHalf( mat->specular_scale[0] );
		data[2] |=	floatToHalf( mat->specular_scale[1] ) << 16;
		data[3] =	floatToHalf( mat->specular_scale[2] );
		data[3] |=	floatToHalf( mat->specular_scale[3] ) << 16;

		data[4] =	mat->remappedIndex;

		mat->uploaded[vk.current_frame_index] = qtrue;
	}

	return VK_SUCCESS;
}