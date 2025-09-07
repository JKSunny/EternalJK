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

#define FNV_OFFSET_BASIS_32     2166136261u
#define FNV_PRIME_32            16777619u
#define MAX_GROUPS              1024
#define HASH_BUCKETS            128

typedef struct GroupEntry {
    Vk_IN_Group_Def     def;
    uint32_t            group_id;
    uint32_t            hash;
    struct GroupEntry   *next;
} GroupEntry;

static GroupEntry       group_entries[MAX_GROUPS];
static GroupEntry       *group_buckets[HASH_BUCKETS];
static uint32_t         group_count = 0;

static Vk_IN_Group_Def  last_def;
static uint32_t         last_group_id = UINT32_MAX;
static qboolean         has_last_def = qfalse;

void reset_in_group_count( void ) 
{
    uint32_t i;

    for ( i = 0; i < HASH_BUCKETS; i++ )
        group_buckets[i] = NULL;

    group_count = 0;
    has_last_def = qfalse; 
}

uint32_t pack_group_vbo_ibo( uint32_t vbo_idx, uint32_t ibo_idx ) 
{
    return (vbo_idx << 12) | (ibo_idx & 0xFFF);
}

void set_group_vbo_ibo( uint32_t packed, VBO_t **vbo, IBO_t **ibo ) 
{
    *vbo = tr.vbos[packed >> 12];
    *ibo = tr.ibos[packed & 0xFFF];
}

uint32_t pack_stage_bits( uint32_t group_bits, int stage ) 
{
    group_bits &= ~(0x7 << 18);           // clear bits 18–20
    group_bits |= ((stage & 0x7) << 18);  // set stage bits
    return group_bits;
}

uint32_t pack_vk_in_group_flags( surfaceType_t surfType, int forceRGBGen, 
    int fogIndex, Vk_Depth_Range depthRange, bool forceEntAlpha, bool alphaDepth, 
    bool distortion )
{
    uint32_t packed = 0;
    packed |= (surfType         & 0xF)      << 0;   // 4 bits - 0-3
    packed |= (forceRGBGen      & 0x1F)     << 4;   // 5 bits - 4-8
    packed |= (fogIndex         & 0x7F)     << 9;   // 7 bits - 9-15
    packed |= (depthRange       & 0x3)      << 16;  // 2 bits - 16-17
    packed |= (forceEntAlpha    ? 1 : 0)    << 21;  // 1 bit: 21
    packed |= (alphaDepth       ? 1 : 0)    << 22;  // 1 bit: 22
    packed |= (distortion       ? 1 : 0)    << 23;  // 1 bit: 23
    return packed;
}

void unpack_vk_in_group_flags( uint32_t packed, surfaceType_t* surfType, int* forceRGBGen, 
    int* fogIndex, Vk_Depth_Range* depthRange, int* stage, bool* forceEntAlpha, bool* alphaDepth,
    bool *distortion )
{
    if (surfType)       *surfType    = ( surfaceType_t)( (packed >> 0)  & 0xF );
    if (forceRGBGen)    *forceRGBGen = ( colorGen_t)(    (packed >> 4)  & 0x1F );
    if (fogIndex)       *fogIndex    = ( packed >> 9)  & 0x7F;
    if (depthRange)     *depthRange  = ( Vk_Depth_Range)( (packed >> 16) & 0x3 );
    if (stage)          *stage       = ( packed >> 18) & 0x7;
    if (forceEntAlpha)  *forceEntAlpha = ((packed >> 21) & 0x1) != 0;
    if (alphaDepth)     *alphaDepth    = ((packed >> 22) & 0x1) != 0;
    if (distortion)     *distortion    = ((packed >> 23) & 0x1) != 0;
}

surfaceType_t unpack_surfType_from_group_bits( uint32_t group_bits ) 
{
    return (surfaceType_t)(group_bits & 0xF);  // bits 3 - 0-3
}

Vk_Depth_Range unpack_depthRange_from_group_bits( uint32_t group_bits ) 
{
    return (Vk_Depth_Range)( (group_bits >> 16) & 0x3 );
}

uint32_t hash_ptr(const void* ptr) {
    uintptr_t val = (uintptr_t)ptr;
    return (uint32_t)((val >> 4) ^ (val >> 9) ^ (val));
}

uint32_t hash_vk_in_group_def(const Vk_IN_Group_Def* def) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    hash ^= def->group_bits;        hash *= FNV_PRIME_32;
    hash ^= def->vbo_ibo_idx;       hash *= FNV_PRIME_32;
    hash ^= hash_ptr(def->shader);  hash *= FNV_PRIME_32;
    return hash;
}

bool equals_vk_in_group_def(const Vk_IN_Group_Def* a, const Vk_IN_Group_Def* b) {
    return a->group_bits    == b->group_bits    &&
           a->vbo_ibo_idx   == b->vbo_ibo_idx   &&
           a->shader        == b->shader;
}

static uint32_t vk_alloc_indirect_group( const Vk_IN_Group_Def *def ) {
    Vk_IN_Group *group;

    group = &tr.indirect_groups[tr.indirect_groups_count];
    group->def = *def;
	
    if (group->indirect == NULL) {
        group->max_indirect = 5;    // initail size
        group->indirect = (Vk_IN_Group_Indirect *)calloc( group->max_indirect, sizeof(Vk_IN_Group_Indirect) );
    }

    if ( !group->indirect )
        ri.Error(ERR_FATAL, "Failed to alloc initial indirect group array");

    group->num_indirect = 0;

    return tr.indirect_groups_count++;
}

uint32_t vk_find_indirect_group_ext(const Vk_IN_Group_Def* def)
{
    uint32_t hash = hash_vk_in_group_def(def);
    uint32_t bucket = hash % HASH_BUCKETS;

    GroupEntry* entry = group_buckets[bucket];
    while ( entry ) {

        if ( entry->hash == hash && equals_vk_in_group_def( &entry->def, def ) ) 
        {
            last_def = *def;
            last_group_id = entry->group_id;
            has_last_def = qtrue;
            return last_group_id;
        }

        entry = entry->next;
    }

    if ( group_count >= MAX_GROUPS ) {
        ri.Error(ERR_DROP, "Maximum indirect groups reached");
        return ~0U;
    }

    last_group_id = vk_alloc_indirect_group( def );

    GroupEntry *new_entry = &group_entries[group_count++];
    new_entry->def      = *def;
    new_entry->group_id = last_group_id;
    new_entry->hash     = hash;
    new_entry->next     = group_buckets[bucket];
    group_buckets[bucket] = new_entry;

    last_def = *def;
    has_last_def = qtrue;

    return last_group_id;
}

static inline Vk_Model_Instance *vk_get_indirect_instance( uint32_t group_id, int numIndexes, int indexOffset )
{
    Vk_IN_Group *group = &tr.indirect_groups[group_id];

    // find subgrup
    for (uint32_t i = 0; i < group->num_indirect; ++i) {
        Vk_IN_Group_Indirect *ind = &group->indirect[i];

        if (ind->indexCount == (uint32_t)numIndexes &&
            ind->firstIndex == (uint32_t)indexOffset) 
        {
            if ( ind->num_instances == ind->max_instances ) 
            {
                size_t old_size = ind->max_instances;
	            ind->max_instances *= 2;
	            ind->instances = (Vk_Model_Instance*)realloc(ind->instances, ind->max_instances * sizeof(Vk_Model_Instance));
                

                if ( !ind->instances )
		            ri.Error(ERR_FATAL, "Failed to realloc instances");
                else
                    memset(&ind->instances[old_size], 0, (ind->max_instances - old_size) * sizeof(Vk_Model_Instance));
            }

            return &ind->instances[ind->num_instances++];
        }
    }

    // init a new subgroup ( grow if needed )
    if ( group->num_indirect == group->max_indirect ) 
    {
        size_t old_size = group->max_indirect;
	    group->max_indirect = MAX(1, group->max_indirect * 2);  // double the size
	    group->indirect = (Vk_IN_Group_Indirect*)realloc( group->indirect, group->max_indirect * sizeof(Vk_IN_Group_Indirect) );
	    

        if ( !group->indirect )
		    ri.Error(ERR_FATAL, "Failed to realloc instance indirect");
        else
            memset(&group->indirect[old_size], 0, (group->max_indirect - old_size) * sizeof(Vk_IN_Group_Indirect));
    }

    Vk_IN_Group_Indirect *new_ind = &group->indirect[group->num_indirect++];
    new_ind->indexCount    = (uint32_t)numIndexes;
    new_ind->firstIndex    = (uint32_t)indexOffset;

    new_ind->num_instances = 0;

    if ( new_ind->instances == NULL ) 
    {
        new_ind->max_instances = 128;
        new_ind->instances = (Vk_Model_Instance *)calloc( new_ind->max_instances, sizeof(Vk_Model_Instance) );
    }
    new_ind->firstInstance = 0; // set later during draw command generation

    return &new_ind->instances[new_ind->num_instances++];
}

void vk_build_indirect_instance( uint32_t entityNum, Vk_IN_Group_Def *group, 
	    const float *mvp, const maliasmesh_t *mesh, const shader_t *shader, const uint32_t bone_index )
{
	if ( mesh->indexOffset < 0 )
		return;

	uint32_t stage;
    shaderStage_t *pStage;

	group->shader		= (shader_t*)shader;
    group->vbo_ibo_idx  = pack_group_vbo_ibo( mesh->vbo->index, mesh->ibo->index );

    trRefEntity_t *ent = &backEnd.refdef.entities[entityNum];

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		pStage = shader->stages[stage];

		if (!pStage || !pStage->active)
			break;

        // pack stage index
        group->group_bits = pack_stage_bits( group->group_bits, stage );

		uint32_t group_id = vk_find_indirect_group_ext( group );

		if ( group_id == ~0U )
			break;

		Vk_Model_Instance *instance = vk_get_indirect_instance( group_id, mesh->numIndexes, mesh->indexOffset );
		Com_Memcpy( instance->mvp, mvp, sizeof(mat4_t) );


        if ( ent == &backEnd.entity2D || ent == &tr.worldEntity )
        {
		    instance->entity_index = vk.cmd->entity_ssbo_index[REFENTITYNUM_WORLD];
        }
        else 
        {
            instance->entity_index = vk.cmd->entity_ssbo_index[entityNum];
		    instance->bone_index = bone_index;
        }
	}
}

void free_indirect_instances( void )
{
    // free groups and sub-groups..
}