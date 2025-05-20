// Defines the entity object and ents. namespace

#include "game/g_local.h"
#include "qcommon/q_shared.h"

#include "glua.h"
#include "../game/jkg_utilityfunc.h"

#define ConvertVec(a, b) (b[0] = a->x, b[1] = a->y, b[2] = a->z)

void JKG_CBB_SetBB(gentity_t *ent, vec3_t mins, vec3_t maxs);
void JKG_CBB_RemoveBB(gentity_t *ent);

//int EntDataRef = -1;

gentity_t *GLua_CheckEntity(lua_State *L, int idx) {
	GLua_Data_Entity_t *data;
	gentity_t *ent;
	if (!ValidateObject(L, idx, GO_ENTITY)) {
		luaL_typerror(L, idx, "Entity");
	}
	data = (GLua_Data_Entity_t *)lua_touserdata(L, idx);
	// We can safely assume entNum is in valid range
	// As pushing entities validates this
	ent = &g_entities[data->entNum];
	if (!ent->inuse && !ent->LuaUsable)
		return NULL;
	if (ent->IDCode != data->IDCode)
		return NULL;
	return ent;
}

int GLua_IsEntity(lua_State *L, int idx) {
	if (!ValidateObject(L, idx, GO_ENTITY)) {
		return 0;
	}
	return 1;
}

static int GLua_Entity_GetTable(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;

	ent->UsesELS = 1;
	GLua_Push_GetEntDataTableFunc(L);
	GLua_PushEntity(L,ent);
	lua_call(L,1,1);
	return 1;

	/*
	lua_rawgeti(L, LUA_REGISTRYINDEX, EntDataRef);
	if (lua_isnil(L,-1)) {
		return 1;
	}
	lua_pushfstring(L,"ID%i",ent->IDCode);
	lua_gettable(L,-2);
	if (!lua_istable(L,-1)) {
		lua_pop(L,1);
		lua_pushfstring(L,"ID%i",ent->IDCode); // Create a new table
		lua_newtable(L);
		lua_settable(L,-3);
		lua_pushfstring(L,"ID%i",ent->IDCode); // Get it back
		lua_gettable(L,-2);
	}
	return 1;
	*/
}

static int GLua_Entity_Index(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	const char *key = lua_tostring(L,2);
	// Continue even if the entity is invalid, we'll check that later

	if (key[0] == '_' && key[1] == '_') {
		if (ent) {
			goto skipmeta;
		} else {
			return 0;
		}
	}

	lua_getmetatable(L,1);
	lua_getfield(L,-1,key);
	if (!lua_isnoneornil(L,-1)) {
		return 1;
	}
	// Its not a method, access the table and check for this entry
	// That is.. if the player is valid :P
	if (!ent) return 1; // If we get here, there's a nil on top of the stack, so ya :P

	// Check properties
	lua_getfield(L, -2, "__propget");
	if (!lua_isnoneornil(L,-1)) {
		// Check properties
		lua_getfield(L,-1,key);
		if (lua_isnoneornil(L,-1)) {
			// Not defined
			lua_pop(L,2);
		} else if (!lua_isfunction(L,-1)) {
			// Write-only property
			lua_Debug ar;
			memset(&ar, 0, sizeof(lua_Debug));
			lua_getstack(L, 1, &ar);
			lua_getinfo(L, "Sl", &ar);
			Com_Printf("Warning: %s:%i: Attempted to get write-only property '%s' in %s", ar.short_src, ar.currentline, key, lua_tostring(L,1));
			return 0;
		} else {
			lua_pushvalue(L,1);	// Push object
			//lua_pushvalue(L,2);	// Push key
			lua_call(L, 1, 1);
			return 1;
		}
	} else {
		lua_pop(L,1);
	}
	lua_pop(L,2);

skipmeta:

	ent->UsesELS = 1;
	GLua_Push_GetEntDataFunc(L);
	GLua_PushEntity(L,ent);
	lua_pushvalue(L,2);
	lua_call(L,2,1);
	return 1;
}

static int GLua_Entity_NewIndex(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	const char *key = lua_tostring(L,2);
	if (!ent) return 0;

	lua_getmetatable(L,1);

	// Check properties
	lua_getfield(L, -1, "__propset");
	if (!lua_isnoneornil(L,-1)) {
		// Check properties
		lua_getfield(L,-1,key);
		if (lua_isnoneornil(L,-1)) {
			// Not defined
			lua_pop(L,2);
		} else if (!lua_isfunction(L,-1)) {
			// Read-only property
			lua_Debug ar;
			memset(&ar, 0, sizeof(lua_Debug));
			lua_getstack(L, 1, &ar);
			lua_getinfo(L, "Sl", &ar);
			Com_Printf("Warning: %s:%i: Attempted to set read-only property '%s' in %s", ar.short_src, ar.currentline, key, lua_tostring(L,1));
			return 0;
		} else {
			lua_pushvalue(L,1);	// Push object
			//lua_pushvalue(L,2);	// Push key
			lua_pushvalue(L,3);	// Push value
			lua_call(L, 2, 1);
			return 1;
		}
	} else {
		lua_pop(L,1);
	}

	lua_pop(L,1);

	ent->UsesELS = 1;
	GLua_Push_SetEntDataFunc(L);
	GLua_PushEntity(L,ent);
	lua_pushvalue(L,2);
	lua_pushvalue(L,3);
	lua_call(L,3,0);
	return 0;
}

static int GLua_Entity_GetIndex(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;

	lua_pushnumber(L, ent->s.number);
	return 1;
}

static int GLua_Entity_ToString(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) {
		lua_pushstring(L,"[ NULL Entity ]");
	} else {
		lua_pushstring(L,va("Entity [%i: %s]", ent->s.number, ent->classname));
	}
	return 1;
}

static int GLua_Entity_Equal(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	gentity_t *ent2 = GLua_CheckEntity(L, 2);
	if (ent == ent2) { // Since this would be a pointer to the same ent, we can actually do this :P
		lua_pushboolean(L,1);
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}


static int GLua_Entity_IsValid(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) {
		lua_pushboolean(L,0);
	} else {
		lua_pushboolean(L,1);
	}
	return 1;
}

static int GLua_Entity_GetDistance(lua_State* L)
{
	gentity_t* ent = GLua_CheckEntity(L, 1);
	gentity_t* ent2 = GLua_CheckEntity(L, 2);
	if (!ent || !ent2) return 0;

	//we're the same entity, distance from ourself is 0
	if (ent == ent2)
	{
		lua_pushnumber(L, 0.0f);
		return 1;
	}

	vec3_t orig, orig2;

	VectorCopy(ent->r.currentOrigin, orig);
	VectorCopy(ent2->r.currentOrigin, orig2);


	lua_pushnumber(L, Distance(orig, orig2));
	return 1;
}

static int GLua_Entity_GetPos(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	vec3_t orig;
	if (!ent) return 0;
	
	VectorCopy(ent->r.currentOrigin, orig);
	GLua_PushVector(L, orig[0], orig[1], orig[2]);
	return 1;
}

static int GLua_Entity_SetPos(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	GLuaVec_t * orig = GLua_CheckVector(L, 2);
	int ignoreclient = lua_toboolean(L,3);
	vec3_t neworigin;
	int dolink = 0;
	if (!ent) return 0;
	memcpy(&neworigin, orig, 12);
	
	if (ent->r.linked) {
		dolink = 1;	// So we dont link ents that shouldn't be linked (ie target_xx ents)
	}
	if (dolink) trap->UnlinkEntity ((sharedEntity_t *)ent);

	if(ent->client && !ignoreclient)
	{
		VectorCopy(neworigin, ent->client->ps.origin);
		VectorCopy(neworigin, ent->r.currentOrigin);
		ent->client->ps.origin[2] += 1;

		VectorClear (ent->client->ps.velocity);
		ent->client->ps.pm_time = 160;		// hold time
		ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		
		ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
	}
	else
	{
		G_SetOrigin( ent, neworigin );
	}

	if (dolink) trap->LinkEntity( (sharedEntity_t *)ent );

	return 0;
}

static int GLua_Entity_GetAngles(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	vec3_t orig;
	if (!ent) return 0;
	
	VectorCopy(ent->r.currentAngles, orig);
	GLua_PushVector(L, orig[0], orig[1], orig[2]);
	return 1;
}

static int GLua_Entity_SetAngles(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	GLuaVec_t * angs = GLua_CheckVector(L, 2);
	vec3_t newangs;
	int dolink = 0;
	if (!ent) return 0;
	memcpy(&newangs, angs, 12);

	if (ent->r.linked) {
		dolink = 1;	// So we dont link ents that shouldn't be linked (ie target_xx ents)
	}

	if (ent->client)
	{
		SetClientViewAngle( ent, newangs );
	}
	else
	{
		G_SetAngles(ent, newangs);
	}
	if (dolink) trap->LinkEntity( (sharedEntity_t *)ent );

	return 0;
}

static int GLua_Entity_GetTarget(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	
	lua_pushstring(L,ent->target);
	return 1;
}

static int GLua_Entity_GetTargetName(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	
	lua_pushstring(L,ent->targetname);
	return 1;
}

static int GLua_Entity_SetTarget(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	ent->target = G_NewString( luaL_checkstring(L,2) );
	return 0;
}

static int GLua_Entity_SetTargetName(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	ent->targetname = G_NewString( luaL_checkstring(L,2) );
	return 0;
}

static int GLua_Entity_Use(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	gentity_t *other, *activator;
	if (lua_isnoneornil(L,2)) {
		other = 0;
	} else {
		other = GLua_CheckEntity(L, 2);
	}
	if (lua_isnoneornil(L,3)) {
		activator = 0;
	} else {
		activator = GLua_CheckEntity(L, 3);
	}
	if (!ent) return 0;

	if (ent->use) {
		ent->use(ent, other, activator);
	}
	return 0;
}

static int GLua_Entity_Free(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->client && !ent->NPC) { // NPC's have a client field too, but we CAN free those
		luaL_error(L, "Attempted to free a client entity");
		return 0;
	}
	G_FreeEntity(ent);
	return 0;
}

static int GLua_Entity_GetClassName(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	lua_pushstring(L,ent->classname);
	return 1;
}

static int GLua_Entity_IsPlayer(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->client && ent->s.eType == ET_PLAYER) {
		lua_pushboolean(L,1);
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}

static int GLua_Entity_ToPlayer(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->client && ent->s.eType == ET_PLAYER) {
		GLua_PushPlayer(L, ent->s.number);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static int GLua_Entity_IsNPC(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->NPC && ent->s.eType == ET_NPC) {
		lua_pushboolean(L,1);
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}

static int GLua_Entity_ToNPC(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->NPC  && ent->s.eType == ET_NPC) {
		GLua_PushNPC(L, ent);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static int GLua_Entity_SetActive(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int active = lua_toboolean(L,2);
	if (!ent) return 0;
	if (active) {
		ent->flags |= FL_INACTIVE;
	} else {
		ent->flags &= ~FL_INACTIVE;
	}
	return 0;
}

static int GLua_Entity_IsActive(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->flags & FL_INACTIVE) {
		lua_pushboolean(L,1);
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}

static int GLua_Entity_HasSpawnVars(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (g_spawnvars[ent->s.number].count != 0) {
		lua_pushboolean(L,1);
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}

static int GLua_Entity_GetSpawnVar(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	lua_pushstring(L, JKG_Pairs_GetKey(&g_spawnvars[ent->s.number], luaL_checkstring(L,2)));
	return 1;
}

static int GLua_Entity_GetSpawnVars(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	KeyPairSet_t *spv;
	unsigned int i;
	if (!ent) return 0;
	spv = &g_spawnvars[ent->s.number];
	lua_newtable(L);
	for(i=0; i < spv->count; i++) {
		lua_pushstring(L, spv->pairs[i].key);
		lua_pushstring(L, spv->pairs[i].value);
		lua_settable(L,-3);
	}
	return 1;
}

static int GLua_Entity_LinkEntity(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->client || ent->NPC) return 0;
	trap->LinkEntity((sharedEntity_t *)ent);
	return 0;
}

static int GLua_Entity_UnlinkEntity(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->client || ent->NPC) return 0;
	trap->UnlinkEntity((sharedEntity_t *)ent);
	return 0;
}

static int GLua_Entity_IsLinked(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->r.linked) {
		lua_pushboolean(L,1);
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}

static int GLua_Entity_SetPlayerUsable(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int usable = lua_toboolean(L,2);
	if (!ent) return 0;
	if (ent->client || ent->NPC) return 0;
	if (usable) {
		ent->r.svFlags |= SVF_PLAYER_USABLE;
	} else {
		ent->r.svFlags &= ~SVF_PLAYER_USABLE;
	}
	return 0;
}

static int GLua_Entity_IsPlayerUsable(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	if (ent->r.svFlags & SVF_PLAYER_USABLE) {
		lua_pushboolean(L,1);
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}

static int GLua_Entity_SetBoundingBox(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	GLuaVec_t *lmins = GLua_CheckVector(L,2);
	GLuaVec_t *lmaxs = GLua_CheckVector(L,3);
	int dolink;
	vec3_t mins;
	vec3_t maxs;
	if (!ent) return 0;
	if (ent->client || ent->NPC) return 0;
	dolink = ent->r.linked;
	if (dolink)
		trap->UnlinkEntity((sharedEntity_t *)ent);
	//memcpy(&mins, lmins, 12);
	//memcpy(&maxs, lmaxs, 12);
	ConvertVec(lmins, mins);
	ConvertVec(lmaxs, maxs);
	JKG_CBB_SetBB(ent, mins, maxs); 
	if (dolink)
		trap->LinkEntity((sharedEntity_t *)ent);
	return 0;
}

static int GLua_Entity_GetBoundingBox(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	GLua_PushVector(L,ent->r.mins[0],ent->r.mins[1],ent->r.mins[2]);
	GLua_PushVector(L,ent->r.maxs[0],ent->r.maxs[1],ent->r.maxs[2]);
	return 2;
}

void MatchTeam( gentity_t *teamLeader, int moverState, int time );
static void GLua_moverCallback( gentity_t *ent )
{
	// play sound
	ent->s.loopSound = 0;//stop looping sound
	ent->s.loopIsSoundset = qfalse;
	G_PlayDoorSound( ent, BMS_END );//play end sound

	if ( ent->moverState == MOVER_1TO2 ) 
	{//reached open
		// reached pos2
		MatchTeam( ent, MOVER_POS2, level.time );
		//SetMoverState( ent, MOVER_POS2, level.time );
	} 
	else if ( ent->moverState == MOVER_2TO1 ) 
	{//reached closed
		MatchTeam( ent, MOVER_POS1, level.time );
		//SetMoverState( ent, MOVER_POS1, level.time );
	}
	ent->blocked = 0;
}

// move(dest, time, linear)
static int GLua_Entity_Move(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	GLuaVec_t *lvec = GLua_CheckVector(L,2);
	int moverState;
	vec3_t vec, delta;
	int time = lua_tointeger(L,3);
	int type = luaL_optinteger(L,4, TR_LINEAR_STOP);

	if (!ent) return 0;
	if (ent->client || ent->NPC) return 0;
	ConvertVec(lvec, vec);
	if (time < 0) {
		// Treat as delta, infinite movement
		ent->s.pos.trType = TR_LINEAR;
		ent->s.pos.trTime = level.time;
		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
		VectorCopy(vec, ent->s.pos.trDelta);
		ent->s.pos.trDuration = 0;
	} else if (time == 0) {
		// Treat as a SetPos and make it stationary
		VectorCopy( vec, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		ent->s.pos.trTime = 0;
		ent->s.pos.trDuration = 0;
		VectorClear( ent->s.pos.trDelta );
		VectorCopy( vec, ent->r.currentOrigin );
	} else {
		// Alright we got targetted movement
		ent->s.pos.trType = (trType_t)type;
		ent->s.pos.trTime = level.time;
		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
		VectorSubtract(vec, ent->r.currentOrigin, delta);
		VectorScale(delta, 1/((float)time/1000), delta);
		VectorCopy(delta, ent->s.pos.trDelta);
		ent->s.pos.trDuration = time;
		if (ent->s.eType == ET_MOVER) {
			moverState = ent->moverState;
			// Movers need special setup
			if ( moverState == MOVER_POS1 || moverState == MOVER_2TO1 )
			{
				VectorCopy( ent->r.currentOrigin, ent->pos1 );
				VectorCopy( vec, ent->pos2 );

				moverState = MOVER_1TO2;
			}
			else
			{
				VectorCopy( ent->r.currentOrigin, ent->pos2 );
				VectorCopy( vec, ent->pos1 );

				moverState = MOVER_2TO1;
			}
			ent->teamMoveType = (trType_t)type;
			MatchTeam( ent, moverState, level.time );
			ent->reached = GLua_moverCallback;
			ent->blocked = 0;

			G_PlayDoorLoopSound( ent );
			G_PlayDoorSound( ent, BMS_START );	//??
		}
	}
	return 0;
}

// movesine(displacement, cycle time)
static int GLua_Entity_MoveSine(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	GLuaVec_t *lvec = GLua_CheckVector(L,2);
	vec3_t vec;
	int time = lua_tointeger(L,3);
	if (!ent) return 0;
	if (ent->client || ent->NPC) return 0;
	ConvertVec(lvec, vec);

	ent->s.pos.trType = TR_SINE;
	ent->s.pos.trTime = level.time;
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	VectorCopy(vec, ent->s.pos.trDelta);
	ent->s.pos.trDuration = time;
	return 0;
}

// rotate(dest, time, linear)
static int GLua_Entity_Rotate(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	GLuaVec_t *lvec = GLua_CheckVector(L,2);
	vec3_t vec, delta;
	int time = lua_tointeger(L,3);
	int type = luaL_optinteger(L,4,TR_LINEAR_STOP);
	if (!ent) return 0;
	if (ent->client || ent->NPC) return 0;
	ConvertVec(lvec, vec);
	if (time < 0) {
		// Treat as delta, infinite movement
		ent->s.apos.trType = TR_LINEAR;
		ent->s.apos.trTime = level.time;
		VectorCopy(ent->r.currentAngles, ent->s.apos.trBase);
		VectorCopy(vec, ent->s.apos.trDelta);
		ent->s.apos.trDuration = 0;
	} else if (time == 0) {
		// Treat as a SetPos and make it stationary
		VectorCopy( vec, ent->s.apos.trBase );
		ent->s.apos.trType = TR_STATIONARY;
		ent->s.apos.trTime = 0;
		ent->s.apos.trDuration = 0;
		VectorClear( ent->s.apos.trDelta );
		VectorCopy( vec, ent->r.currentAngles );
	} else {
		// Alright we got targetted movement
		ent->s.apos.trType = (trType_t)type;
		ent->s.apos.trTime = level.time;

		// Make sure our previous changes to rotations will stack with this one, as well.
		VectorMA( ent->s.apos.trBase, ent->s.apos.trDuration, ent->s.apos.trDelta, ent->s.apos.trBase );
		VectorSubtract(vec, ent->r.currentAngles, delta);
		VectorAdd(ent->s.apos.trBase, delta, ent->s.apos.trBase);
		VectorScale(delta, 1/((float)time/1000), delta);
		VectorCopy(delta, ent->s.apos.trDelta);
		ent->s.apos.trDuration = time;
	}
	return 0;
}

// rotatesine(displacement, cycle time)
static int GLua_Entity_RotateSine(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	GLuaVec_t *lvec = GLua_CheckVector(L,2);
	vec3_t vec;
	int time = lua_tointeger(L,3);
	if (!ent) return 0;
	if (ent->client || ent->NPC) return 0;
	ConvertVec(lvec, vec);

	ent->s.apos.trType = TR_SINE;
	ent->s.apos.trTime = level.time;
	VectorCopy(ent->r.currentAngles, ent->s.apos.trBase);
	VectorCopy(vec, ent->s.apos.trDelta);
	ent->s.apos.trDuration = time;
	return 0;
}


static int GLua_Entity_GetModel(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	char buff[1024] = {0};
	if (!ent) return 0;
	// For safety's sake we'll actually be checkin the ent type
	if (ent->client || ent->NPC) return 0;
	switch(ent->s.eType) {
		case ET_GENERAL:
			trap->GetConfigstring(CS_MODELS + ent->s.modelindex, buff, 1024);
			lua_pushstring(L, buff);
			break;
		case ET_MOVER:
		case ET_PUSH_TRIGGER:
		case ET_TELEPORT_TRIGGER:
			if (ent->s.solid == SOLID_BMODEL) {
				// We got a bmodel here
				lua_pushstring(L, va("*%i" , ent->s.modelindex));
			} else {
				trap->GetConfigstring(CS_MODELS + ent->s.modelindex, buff, 1024);
				lua_pushstring(L, buff);
			}
			break;
		default:
			lua_pushnil(L);
			break;
	}
	return 1;
}

static int GLua_Entity_SetModel(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	const char *model = luaL_checkstring(L,2);
	if (!ent) return 0;
	// For safety's sake we'll actually be checkin the ent type
	if (ent->client || ent->NPC) return 0;
	switch(ent->s.eType) {
		case ET_GENERAL:
			ent->s.modelindex = G_ModelIndex(model);
			break;
		case ET_MOVER:
		case ET_PUSH_TRIGGER:
		case ET_TELEPORT_TRIGGER:
			trap->UnlinkEntity((sharedEntity_t *)ent);
			if (model[0] == '*') {
				trap->SetBrushModel((sharedEntity_t *)ent, model);
				trap->LinkEntity((sharedEntity_t *)ent); // Link to apply the new bmodel
			} else {
				if (ent->s.solid == SOLID_BMODEL) {
					ent->s.solid = 0;
				}
				ent->s.modelindex = G_ModelIndex(model);
				trap->LinkEntity((sharedEntity_t *)ent);
			}
			break;
		default:
			lua_pushnil(L);
			break;
	}
	return 0;
}

static int GLua_Entity_SetNextThink(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int nextthink = lua_tointeger(L,2);
	if (!ent) return 0;
	// Don't do this on clients
	if (ent->client) return 0;
	if (nextthink < 0) {
		ent->nextthink = 0;
	} else {
		ent->nextthink = level.time + nextthink;
	}
	return 0;
}

static int GLua_Entity_AutoBox(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	char modelbuff[1024] = {0};
	vec3_t mins, maxs;
	int dolink;
	if (!ent) return 0;
	// Don't do this on clients/NPC's
	if (ent->client || ent->NPC) return 0;
	switch(ent->s.eType) {
		case ET_GENERAL:
			trap->GetConfigstring(CS_MODELS + ent->s.modelindex, modelbuff, 1024);
			break;
		case ET_MOVER:
		case ET_PUSH_TRIGGER:
		case ET_TELEPORT_TRIGGER:
			if (ent->s.solid == SOLID_BMODEL) {
				// We got a bmodel here
				// Don't autobox it
				return 0;
			} else {
				trap->GetConfigstring(CS_MODELS + ent->s.modelindex, modelbuff, 1024);
			}
			break;
		default:
			break;
	}
	if (!modelbuff[0]) { 
		return 0;
	}
	JKG_GetAutoBoxForModel(modelbuff, ent->s.angles, ent->s.iModelScale == 0 ? 1 : ((float)ent->s.iModelScale / 100.0f), mins, maxs);
	SnapVector(mins);
	SnapVector(maxs);
	dolink = ent->r.linked;
	if (dolink)
		trap->UnlinkEntity((sharedEntity_t *)ent);
	JKG_CBB_SetBB(ent, mins, maxs); 
	if (dolink)
		trap->LinkEntity((sharedEntity_t *)ent);
	return 0;
}

static int GLua_Entity_SetContents(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int contents = lua_tointeger(L,2);
	if (!ent) return 0;
	// Don't do this on clients
	if (ent->client) return 0;
	ent->r.contents = contents;
	return 0;
}

static int GLua_Entity_GetContents(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	// Don't do this on clients
	if (ent->client) return 0;
	lua_pushnumber(L, ent->r.contents);
	return 1;
}

static int GLua_Entity_SetClipmask(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int clipmask = lua_tointeger(L,2);
	if (!ent) return 0;
	// Don't do this on clients
	if (ent->client) return 0;
	ent->clipmask = clipmask;
	return 0;
}

static int GLua_Entity_GetClipmask(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	// Don't do this on clients
	if (ent->client) return 0;
	lua_pushnumber(L, ent->clipmask);
	return 1;
}

static int GLua_Entity_SetTakeDamage(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int takedamage = lua_toboolean(L,2);
	if (!ent) return 0;
	ent->takedamage = takedamage;
	return 0;
}

static int GLua_Entity_CanTakeDamage(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	lua_pushboolean(L, ent->takedamage);
	return 1;
}

static int GLua_Entity_SetHealth(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int health = lua_tointeger(L,2);
	if (!ent) return 0;
	ent->health = health;
	return 0;

}

static int GLua_Entity_GetHealth(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	lua_pushinteger(L, ent->health);
	return 1;
}

void G_EntitySound( gentity_t *ent, int channel, int soundIndex );
void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

static int GLua_Entity_PlaySound(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	G_SoundOnEnt(ent, (soundChannel_t)luaL_checkint(L,2), luaL_checkstring(L,3));
	return 0;
}

static int GLua_Entity_PlaySoundIdx(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	G_EntitySound(ent, luaL_checkint(L,2), luaL_checkint(L,3));
	return 0;
}

static int GLua_Entity_EnableShaderAnim(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int enable = lua_toboolean(L,2);
	if (!ent) return 0;
	if (enable) {
		ent->s.eFlags |= EF_SHADER_ANIM;
	} else {
		ent->s.eFlags &= ~EF_SHADER_ANIM;
	}
	return 0;
}

static int GLua_Entity_SetShaderFrame(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int frame = lua_tointeger(L,2);
	if (!ent) return 0;
	ent->s.frame = frame;
	return 0;
}

static int GLua_Entity_SetLoopSound(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	const char *sound = NULL;
	sfxHandle_t index;

	if (!ent) return 0;
	if (lua_isnoneornil(L,2)) {
		ent->s.loopSound = 0;
		ent->s.loopIsSoundset = qfalse;
		return 0;
	}

	sound = luaL_checkstring(L,2);
	if (!sound[0]) {
		return 0;
	}

	index = G_SoundIndex( sound );

	if (index)
	{
		ent->s.loopSound = index;
		ent->s.loopIsSoundset = qfalse;
	}
	else
	{
		//G_DebugPrint( WL_WARNING, "Q3_SetLoopSound: can't find sound file: '%s'\n", name );
	}
	return 0;
}

void UnLockDoors(gentity_t *const ent);
void LockDoors(gentity_t *const ent);

static int GLua_Entity_SetDoorLocked(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int locked = lua_toboolean(L,2);
	if (!ent) return 0;
	if (locked) {
		LockDoors(ent);
	} else {
		UnLockDoors(ent);
	}
	return 0;
}


static int GLua_Entity_SetWait(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int wait = lua_tointeger(L,2);
	if (!ent) return 0;
	ent->wait = wait;
	return 0;
}

static int GLua_Entity_GetWait(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	lua_pushinteger(L, ent->wait);
	return 1;
}

static int GLua_Entity_SetDelay(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int delay = lua_tointeger(L,2);
	if (!ent) return 0;
	ent->delay = delay;
	return 0;
}

static int GLua_Entity_GetDelay(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	lua_pushinteger(L, ent->delay);
	return 1;
}

static int GLua_Entity_SetRandom(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	int random = lua_tointeger(L,2);
	if (!ent) return 0;
	ent->random = random;
	return 0;
}

static int GLua_Entity_GetRandom(lua_State *L) {
	gentity_t *ent = GLua_CheckEntity(L, 1);
	if (!ent) return 0;
	lua_pushinteger(L, ent->random);
	return 1;
}

/**************************************************
* ents_m
*
* Methods of the entity class
**************************************************/

static const struct luaL_reg ents_m [] = {
	/* Metamethods */
	{"__index", GLua_Entity_Index},
	{"__newindex", GLua_Entity_NewIndex},
	{"__tostring", GLua_Entity_ToString},
	{"__eq", GLua_Entity_Equal},
	/* Normal Methods */
	{"GetTable", GLua_Entity_GetTable},
	{"GetIndex", GLua_Entity_GetIndex},
	{"IsValid", GLua_Entity_IsValid},
	{"SetPos", GLua_Entity_SetPos},
	{"GetPos", GLua_Entity_GetPos},
	{"SetOrigin", GLua_Entity_SetPos},
	{"GetOrigin", GLua_Entity_GetPos},
	{"GetDistance", GLua_Entity_GetDistance},
	{"SetAngles", GLua_Entity_SetAngles},
	{"GetAngles", GLua_Entity_GetAngles},
	{"GetTarget", GLua_Entity_GetTarget},
	{"GetTargetName", GLua_Entity_GetTargetName},
	{"SetTarget", GLua_Entity_SetTarget},
	{"SetTargetName", GLua_Entity_SetTargetName},
	{"Use", GLua_Entity_Use},
	{"Free", GLua_Entity_Free},
	{"GetClassName", GLua_Entity_GetClassName},
	{"IsPlayer", GLua_Entity_IsPlayer},
	{"ToPlayer", GLua_Entity_ToPlayer},
	{"IsNPC", GLua_Entity_IsNPC},
	{"ToNPC", GLua_Entity_ToNPC},
	{"SetActive", GLua_Entity_SetActive},
	{"IsActive", GLua_Entity_IsActive},
	{"HasSpawnVars", GLua_Entity_HasSpawnVars},
	{"GetSpawnVar", GLua_Entity_GetSpawnVar},
	{"GetSpawnVars", GLua_Entity_GetSpawnVars},
	{"LinkEntity", GLua_Entity_LinkEntity},
	{"UnlinkEntity", GLua_Entity_UnlinkEntity},
	{"IsLinked", GLua_Entity_IsLinked},
	{"SetPlayerUsable", GLua_Entity_SetPlayerUsable},
	{"IsPlayerUsable", GLua_Entity_IsPlayerUsable},
	{"SetBoundingBox", GLua_Entity_SetBoundingBox},
	{"GetBoundingBox", GLua_Entity_GetBoundingBox},
	{"AutoBox", GLua_Entity_AutoBox},
	{"Move", GLua_Entity_Move},
	{"MoveSine", GLua_Entity_MoveSine},
	{"Rotate", GLua_Entity_Rotate},
	{"RotateSine", GLua_Entity_RotateSine},
	{"GetModel", GLua_Entity_GetModel},
	{"SetModel", GLua_Entity_SetModel},
	{"SetNextThink", GLua_Entity_SetNextThink},
	{"SetContents", GLua_Entity_SetContents},
	{"GetContents", GLua_Entity_GetContents},
	{"SetClipmask", GLua_Entity_SetClipmask},
	{"GetClipmask", GLua_Entity_GetClipmask},
	{"SetTakeDamage", GLua_Entity_SetTakeDamage}, 
	{"CanTakeDamage", GLua_Entity_CanTakeDamage}, 
	{"SetHealth", GLua_Entity_SetHealth},
	{"GetHealth", GLua_Entity_GetHealth},
	{"PlaySound" , GLua_Entity_PlaySound},
	{"PlaySoundIdx", GLua_Entity_PlaySoundIdx},
	{"EnableShaderAnim", GLua_Entity_EnableShaderAnim},
	{"SetShaderFrame", GLua_Entity_SetShaderFrame},
	{"SetDoorLocked", GLua_Entity_SetDoorLocked},
	{"SetLoopSound", GLua_Entity_SetLoopSound},
	{NULL, NULL},
};

/**************************************************
* ents_p
*
* Properties of the entity class
**************************************************/

static const struct GLua_Prop ents_p [] = {
	{"Index",		GLua_Entity_GetIndex,		NULL},
	{"Model",		GLua_Entity_GetModel,		GLua_Entity_SetModel},
	{"Origin",		GLua_Entity_GetPos,			GLua_Entity_SetPos},
	{"Pos",			GLua_Entity_GetPos,			GLua_Entity_SetPos},
	{"Angles",		GLua_Entity_GetAngles,		GLua_Entity_SetAngles},
	{"Target",		GLua_Entity_GetTarget,		GLua_Entity_SetTarget},
	{"TargetName",	GLua_Entity_GetTargetName,	GLua_Entity_SetTargetName},
	{"IsPlayer",	GLua_Entity_IsPlayer,		NULL},	
	{"IsNPC",		GLua_Entity_IsNPC,			NULL},
	{"Active",		GLua_Entity_IsActive,		GLua_Entity_SetActive},
	{"PlayerUsable",GLua_Entity_IsPlayerUsable, GLua_Entity_SetPlayerUsable},
	{"BoundingBox", GLua_Entity_GetBoundingBox, GLua_Entity_SetBoundingBox},
	{"Contents",	GLua_Entity_GetContents,	GLua_Entity_SetContents},
	{"Clipmask",	GLua_Entity_GetClipmask,	GLua_Entity_SetClipmask},
	{"TakeDamage",	GLua_Entity_CanTakeDamage,	GLua_Entity_SetTakeDamage},
	{"Health",		GLua_Entity_GetHealth,		GLua_Entity_SetHealth},
	{"Wait",		GLua_Entity_GetWait,		GLua_Entity_SetWait},
	{"Delay",		GLua_Entity_GetDelay,		GLua_Entity_SetDelay},
	{"Random",		GLua_Entity_GetRandom,		GLua_Entity_SetRandom},
	{NULL,			NULL,						NULL},
};

GLua_Data_EntityFactory_t *GLua_CheckEntityFactory(lua_State *L, int idx) {
	GLua_Data_EntityFactory_t *data;
	if (!ValidateObject(L, idx, GO_ENTITYFACTORY)) {
		luaL_typerror(L, idx, "EntityFactory");
	}
	data = (GLua_Data_EntityFactory_t *)lua_touserdata(L,1);
	return data;
}


static int GLua_EntityFactory_Index(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	const char *key = lua_tostring(L,2);
	GLUA_UNUSED(entfact);

	lua_getmetatable(L,1);
	lua_getfield(L,-1,key);
	return 1;
}

static int GLua_EntityFactory_NewIndex(lua_State *L) {
	return 0;
}

static int GLua_EntityFactory_GarbageCollect(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	JKG_Pairs_Clear(&entfact->keys);
	return 0;
}

static int GLua_EntityFactory_ToString(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	if (JKG_Pairs_FindKey(&entfact->keys,"classname") != -1) {
		lua_pushfstring(L,"Entity Factory [%s]", JKG_Pairs_GetKey(&entfact->keys,"classname"));
	} else {
		lua_pushstring(L,"Entity Factory");
	}
	return 1;
}

static int GLua_EntityFactory_SetSpawnVar(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	const char *spawnvar_key = luaL_checkstring(L,2);
	const char *spawnvar_value = luaL_checkstring(L,3);
	JKG_Pairs_SetKey(&entfact->keys, spawnvar_key, spawnvar_value);
	return 0;
}

static int GLua_EntityFactory_GetSpawnVar(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	const char *spawnvar_key = luaL_checkstring(L,2);
	lua_pushstring(L, JKG_Pairs_GetKey(&entfact->keys, spawnvar_key));
	return 1;
}

static int GLua_EntityFactory_GetSpawnVars(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	unsigned int i;
	lua_newtable(L);
	for (i=0; i<entfact->keys.count; i++) {
		lua_pushstring(L, entfact->keys.pairs[i].key);
		lua_pushstring(L, entfact->keys.pairs[i].value);
		lua_settable(L,-3);
	}
	return 1;
}

static int GLua_EntityFactory_RemoveSpawnVar(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	const char *key = luaL_checkstring(L,2);
	int index = JKG_Pairs_FindKey(&entfact->keys, key);
	if (index != -1) {
		JKG_Pairs_Remove(&entfact->keys, index);
	}
	return 0;
}

static int GLua_EntityFactory_ClearSpawnVars(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	int keepclass = lua_toboolean(L,2);
	const char *classname = NULL;
	if (keepclass && JKG_Pairs_FindKey(&entfact->keys, "classname") != -1) {
		classname = G_NewString( JKG_Pairs_GetKey( &entfact->keys, "classname" ) );
	}
	JKG_Pairs_Clear(&entfact->keys);
	if (classname) {
		JKG_Pairs_Add(&entfact->keys, "classname", classname);
	}
	return 0;
}

static int GLua_EntityFactory_GetClassname(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	lua_pushstring(L, JKG_Pairs_GetKey(&entfact->keys, "classname"));
	return 1;
}

static int GLua_EntityFactory_SetClassname(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	const char *classname = luaL_checkstring(L,2);
	JKG_Pairs_SetKey(&entfact->keys, "classname",classname);
	return 0;
}

static int GLua_EntityFactory_GetVarCount(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	lua_pushinteger(L, entfact->keys.count);
	return 1;
}

void G_SpawnEntity(gentity_t **outent);
char *G_AddSpawnVarToken( const char *string );

static int GLua_EntityFactory_Create(lua_State *L) {
	GLua_Data_EntityFactory_t *entfact = GLua_CheckEntityFactory(L, 1);
	gentity_t *ent = NULL;
	unsigned int i;
	qboolean oldspawning = level.spawning;
	if (level.spawning && level.numSpawnVars != 0) {
		// Game is currently busy spawning an ent
		// Usually means we tried to create an ent inside a custom ent's spawn function
		trap->Print("WARNING: Entity Factory tried to spawn an entity while the spawner was in use\n");
		GLua_PushEntity(L, NULL);
		return 1;
	}

	if (!JKG_Pairs_GetKey(&entfact->keys, "classname")) {
		GLua_PushEntity(L, NULL);
		return 1; // Dont even bother spawning an ent without classname
	}
	// Spawn it
	level.spawning = qtrue;
	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;
	for (i=0; i<entfact->keys.count; i++) {
		if (i>63) break; // No more than 64 keys
		level.spawnVars[i][0] = G_AddSpawnVarToken(entfact->keys.pairs[i].key);
		level.spawnVars[i][1] = G_AddSpawnVarToken(entfact->keys.pairs[i].value);
		level.numSpawnVars++;
	}

	G_SpawnEntity(&ent);
	GLua_PushEntity(L, ent);
	level.spawning = oldspawning;
	return 1;
}
static const struct luaL_reg entfact_m [] = {
	{"__index", GLua_EntityFactory_Index},
	{"__newindex", GLua_EntityFactory_NewIndex},
	{"__tostring", GLua_EntityFactory_ToString},
	{"__gc", GLua_EntityFactory_GarbageCollect},
	{"SetSpawnVar", GLua_EntityFactory_SetSpawnVar},
	{"GetSpawnVar", GLua_EntityFactory_GetSpawnVar},
	{"GetSpawnVars", GLua_EntityFactory_GetSpawnVars},
	{"RemoveSpawnVar", GLua_EntityFactory_RemoveSpawnVar},
	{"ClearSpawnVars", GLua_EntityFactory_ClearSpawnVars},
	{"SetClassname", GLua_EntityFactory_SetClassname},
	{"GetClassname", GLua_EntityFactory_GetClassname},
	{"GetVarCount", GLua_EntityFactory_GetVarCount},
	{"Create", GLua_EntityFactory_Create},
	{NULL, NULL},
};

void GLua_PushEntity(lua_State *L, gentity_t *ent) {
	GLua_Data_Entity_t *data;
	int entNum;
	if (!ent) { // Push a NULL entity
		data = (GLua_Data_Entity_t *)lua_newuserdata(L, sizeof(GLua_Data_Entity_t));
		data->entNum = 0;
		data->IDCode = g_entities[0].IDCode - 1; // Invalid on purpose
		luaL_getmetatable(L,"Entity");
		lua_setmetatable(L,-2);
		return;
	}
	entNum = ent->s.number;

	if (g_entities[entNum].inuse || g_entities[entNum].LuaUsable) {
		// Ent is valid
		data = (GLua_Data_Entity_t *)lua_newuserdata(L, sizeof(GLua_Data_Entity_t));
		data->entNum = entNum;
		data->IDCode = ent->IDCode;
		luaL_getmetatable(L,"Entity");
		lua_setmetatable(L,-2);
	} else {
		// Invalid entity, push null entity
		data = (GLua_Data_Entity_t *)lua_newuserdata(L, sizeof(GLua_Data_Entity_t));
		data->entNum = entNum;
		data->IDCode = ent->IDCode - 1; // Invalid on purpose
		luaL_getmetatable(L,"Entity");
		lua_setmetatable(L,-2);
	}
	return;
}

static int GLua_Entities_GetByIndex(lua_State *L) {
	int num = luaL_checkint(L,1);
	if (lua_gettop(L) < 1) {
		return 0;
	} else {
		if (num < 0 || num >= MAX_ENTITIESTOTAL) {
			GLua_PushEntity(L, NULL); 
		} else {
			GLua_PushEntity(L, &g_entities[num]); 
		}
		return 1;
	}
}

static int GLua_Entities_GetAll(lua_State *L) {
	int idx;
	int i;
	lua_newtable(L);
	for (i=0, idx=1; i < level.num_entities; i++) {
		if (g_entities[i].inuse) {
			GLua_PushEntity(L, &g_entities[i]);
			lua_rawseti(L,-2,idx++);
		}
	}
	for (i=0; i < level.num_logicalents; i++) {
		if (g_logicalents[i].inuse) {
			GLua_PushEntity(L, &g_logicalents[i]);
			lua_rawseti(L,-2,idx++);
		}
	}
	return 1;
}

static int GLua_Entities_GetByClass(lua_State *L) {
	int idx;
	const char *classname = luaL_checkstring(L,1);
	gentity_t *ent = NULL;
	lua_newtable(L);
	idx = 1;
	while ((ent = G_Find(ent, FOFS(classname), classname)) != NULL) {
		GLua_PushEntity(L, ent); 
		lua_rawseti(L,-2,idx++);
	}
	return 1;
}

static int GLua_Entities_GetByName(lua_State *L) {
	int idx;
	const char *targname = luaL_checkstring(L,1);
	gentity_t *ent = NULL;
	lua_newtable(L);
	idx = 1;
	while ((ent = G_Find(ent, FOFS(targetname), targname)) != NULL) {
		GLua_PushEntity(L, ent); 
		lua_rawseti(L,-2,idx++);
	}
	return 1;
}

static int GLua_Entities_FindInBox(lua_State *L) {
	int idx;
	int i;
	GLuaVec_t *mins = GLua_CheckVector(L,1);
	GLuaVec_t *maxs = GLua_CheckVector(L,2);
	vec3_t mn, mx;
	int ents[MAX_GENTITIES];
	int hits;
	lua_newtable(L);
	idx = 1;
	ConvertVec(mins, mn);
	ConvertVec(maxs, mx);

	hits = trap->EntitiesInBox(mn, mx, ents, MAX_GENTITIES);
	for (i=0; i<hits; i++) {
		GLua_PushEntity(L, &g_entities[ents[i]]);
		lua_rawseti(L,-2,idx++);
	}
	return 1;
}

static int GLua_Entities_CreateEntityFactory(lua_State *L) {
	const char *classname = NULL;
	GLua_Data_EntityFactory_t *data;
	if (!lua_isnoneornil(L,1)) {
		classname = luaL_checkstring(L,1);
		
	}
	data = (GLua_Data_EntityFactory_t *)lua_newuserdata(L, sizeof(GLua_Data_EntityFactory_t));
	data->keys.count = 0;
	data->keys.pairs = NULL;

	if (classname) {
		JKG_Pairs_Add(&data->keys, "classname", classname);
	}
	luaL_getmetatable(L,"EntityFactory");
	lua_setmetatable(L,-2);
	return 1;
}

static int GLua_Entities_EntCount(lua_State *L) {
	int i;
	int count;
	for (i=0, count = 0; i<level.num_entities; i++) {
		if (g_entities[i].inuse) count++;
	}
	lua_pushinteger(L,count);
	return 1;
}

static int GLua_Entities_EntCountAllocated(lua_State *L) {
	lua_pushinteger(L,level.num_entities);
	return 1;
}

static int GLua_Entities_LogicalEntCount(lua_State *L) {
	int i;
	int count;
	for (i=0, count = 0; i<level.num_logicalents; i++) {
		if (g_logicalents[i].inuse) count++;
	}
	lua_pushinteger(L,count);
	return 1;
}

static int GLua_Entities_LogicalEntCountAllocated(lua_State *L) {
	lua_pushinteger(L,level.num_logicalents);
	return 1;
}

static int GLua_Entities_UseTarget(lua_State *L) {
	gentity_t *ent = NULL;
	gentity_t *activator = NULL;
	gentity_t *t;
	const char *string = luaL_checkstring(L,1);
	if (!lua_isnoneornil(L,2)) ent = GLua_CheckEntity(L, 2);
	if (!lua_isnoneornil(L,3)) activator = GLua_CheckEntity(L, 3);
	
	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), string)) != NULL ) {
		GlobalUse(t, ent, activator);
	}
	return 0;
}

/* Iterator! */
static int GLua_Entities_GetByNameIterator(lua_State *L) {
	gentity_t *start = NULL;
	gentity_t *next = NULL;
	const char *targname = luaL_checkstring(L, 1);
	if (!lua_isnoneornil(L,2)) start = GLua_CheckEntity(L, 2);

	next = G_Find(start, FOFS(targetname), targname);
	if (next) {
		GLua_PushEntity(L, next);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static int GLua_Entities_GetByNameIterate(lua_State *L) {
	lua_pushcclosure(L, GLua_Entities_GetByNameIterator, 0);
	lua_pushstring(L, luaL_checkstring(L,1));
	lua_pushnil(L);
	return 3;
}

/* Iterator! */
static int GLua_Entities_GetByClassIterator(lua_State *L) {
	gentity_t *start = NULL;
	gentity_t *next = NULL;
	const char *classname = luaL_checkstring(L, 1);
	if (!lua_isnoneornil(L,2)) start = GLua_CheckEntity(L, 2);

	next = G_Find(start, FOFS(classname), classname);
	if (next) {
		GLua_PushEntity(L, next);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

static int GLua_Entities_GetByClassIterate(lua_State *L) {
	lua_pushcclosure(L, GLua_Entities_GetByClassIterator, 0);
	lua_pushstring(L, luaL_checkstring(L,1));
	lua_pushnil(L);
	return 3;
}

static const struct luaL_reg ents_f [] = {
	{"GetByIndex", GLua_Entities_GetByIndex},
	{"GetAll", GLua_Entities_GetAll},
	{"GetByClass", GLua_Entities_GetByClass},
	{"GetByClassI", GLua_Entities_GetByClassIterate},
	{"GetByName", GLua_Entities_GetByName},
	{"GetByNameI", GLua_Entities_GetByNameIterate},
	{"FindInBox", GLua_Entities_FindInBox},
	{"UseTarget", GLua_Entities_UseTarget},
	{"CreateEntityFactory", GLua_Entities_CreateEntityFactory},
	{"EntCount", GLua_Entities_EntCount},
	{"EntCountAllocated", GLua_Entities_EntCountAllocated},
	{"LogicalEntCount", GLua_Entities_LogicalEntCount},
	{"LogicalEntCountAllocated", GLua_Entities_LogicalEntCountAllocated},
	{NULL, NULL},
};

///////////////////////////
// Entity spawning
///////////////////////////

/// Define entity events
static void GLua_EntEV_OnThink(gentity_t *self) {
	lua_State *L = GLua_GetState();
	if (!L) return;
	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, self);
	lua_pushstring(L, "OnThink");
	if (lua_pcall(L,2,0,0)) {
		trap->Print("GLua: Failed to call ent.OnThink: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

static void GLua_EntEV_OnUse(gentity_t *self, gentity_t *other, gentity_t *activator) {
	lua_State *L = GLua_GetState();
	if (!L) return;
	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, self);
	lua_pushstring(L, "OnUse");
	GLua_PushEntity(L, other);
	GLua_PushEntity(L, activator);
	if (lua_pcall(L,4,0,0)) {
		trap->Print("GLua: Failed to call ent.OnUse: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

static void GLua_EntEV_OnRemove(gentity_t *self) {
	lua_State *L = GLua_GetState();
	if (!L) return;
	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, self);
	lua_pushstring(L, "OnRemove");
	if (lua_pcall(L,2,0,0)) {
		trap->Print("GLua: Failed to call ent.OnRemove: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

static void GLua_EntEV_OnTakeDamage(gentity_t *self, gentity_t *attacker, int damage) {
	lua_State *L = GLua_GetState();
	if (!L) return;
	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, self);
	lua_pushstring(L, "OnTakeDamage");
	GLua_PushEntity(L, attacker);
	lua_pushnumber(L, damage);
	if (lua_pcall(L,4,0,0)) {
		trap->Print("GLua: Failed to call ent.OnTakeDamage: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

static void GLua_EntEV_OnDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) {
	lua_State *L = GLua_GetState();
	if (!L) return;
	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, self);
	lua_pushstring(L, "OnDie");
	GLua_PushEntity(L, inflictor);
	GLua_PushEntity(L, attacker);
	lua_pushnumber(L, damage);
	lua_pushnumber(L, mod);
	if (lua_pcall(L,6,0,0)) {
		trap->Print("GLua: Failed to call ent.OnDie: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

static void GLua_EntEV_OnTouch(gentity_t *self, gentity_t *other, trace_t* tr) {
	lua_State *L = GLua_GetState();
	if (!L) return;
	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, self);
	lua_pushstring(L, "OnTouch");
	GLua_PushEntity(L, other);
	if (lua_pcall(L,3,0,0)) {
		trap->Print("GLua: Failed to call ent.OnTouch: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

static void GLua_EntEV_OnReached(gentity_t *self) {
	lua_State *L = GLua_GetState();
	if (!L) return;
	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, self);
	lua_pushstring(L, "OnReached");
	if (lua_pcall(L,2,0,0)) {
		trap->Print("GLua: Failed to call ent.OnReached: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

static void GLua_EntEV_OnBlocked(gentity_t *self, gentity_t *other) {
	lua_State *L = GLua_GetState();
	if (!L) return;
	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, self);
	lua_pushstring(L, "OnBlocked");
	GLua_PushEntity(L, other);
	if (lua_pcall(L,2,0,0)) {
		trap->Print("GLua: Failed to call ent.OnBlocked: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

/// Define spawners
static void GLua_SP_Generic(gentity_t *ent) {
	lua_State *L = GLua_GetState();
	ent->think = GLua_EntEV_OnThink;
	ent->use = GLua_EntEV_OnUse;
	ent->pain = GLua_EntEV_OnTakeDamage;
	ent->die = GLua_EntEV_OnDie;
	ent->remove = GLua_EntEV_OnRemove;

	ent->s.eType = ET_GENERAL;
	ent->r.contents = 0;

	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, ent);
	lua_pushstring(L, "OnSpawn");
	if (lua_pcall(L,2,0,0)) {
		trap->Print("GLua: Failed to call ent.OnSpawn: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

static void GLua_SP_Logical(gentity_t *ent) {
	lua_State *L = GLua_GetState();
	ent->think = GLua_EntEV_OnThink;
	ent->use = GLua_EntEV_OnUse;
	ent->remove = GLua_EntEV_OnRemove;

	ent->s.eType = ET_GENERAL;
	ent->r.contents = 0;
	ent->r.svFlags = SVF_NOCLIENT;

	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, ent);
	lua_pushstring(L, "OnSpawn");
	if (lua_pcall(L,2,0,0)) {
		trap->Print("GLua: Failed to call ent.OnSpawn: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

static void GLua_SP_Mover(gentity_t *ent) {
	lua_State *L = GLua_GetState();
	ent->think = GLua_EntEV_OnThink;
	ent->use = GLua_EntEV_OnUse;
	ent->pain = GLua_EntEV_OnTakeDamage;
	ent->die = GLua_EntEV_OnDie;
	ent->reached = GLua_EntEV_OnReached;
	ent->blocked = GLua_EntEV_OnBlocked;
	ent->remove = GLua_EntEV_OnRemove;

	ent->s.eType = ET_MOVER;
	ent->r.contents = CONTENTS_SOLID;

	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, ent);
	lua_pushstring(L, "OnSpawn");
	if (lua_pcall(L,2,0,0)) {
		trap->Print("GLua: Failed to call ent.OnSpawn: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;

}

static void GLua_SP_Trigger(gentity_t *ent) {
	lua_State *L = GLua_GetState();
	ent->think = GLua_EntEV_OnThink;
	ent->use = GLua_EntEV_OnUse;
	ent->remove = GLua_EntEV_OnRemove;
	ent->touch = GLua_EntEV_OnTouch;

	ent->s.eType = ET_MOVER;
	ent->r.contents = CONTENTS_TRIGGER;
	ent->r.svFlags = SVF_NOCLIENT;

	GLua_Push_CallEntityFunc();
	GLua_PushEntity(L, ent);
	lua_pushstring(L, "OnSpawn");
	if (lua_pcall(L,2,0,0)) {
		trap->Print("GLua: Failed to call ent.OnSpawn: %s\n", lua_tostring(L,-1));
		lua_pop(L,1);
	}
	return;
}

/// Main spawner
int GLua_Spawn_Entity(gentity_t* ent) {
	const char *classname = ent->classname;
	int enttype = GLua_GetEntityTypeID(classname);
	if (!enttype) {
		// Not valid, so the classname is not defined in lua
		return 0;
	}
	// Valid entity ^^
	// Run the spawner, which will place all entity info inside its table
	// After that we just pass it to the proper GLua_SP_* function
	GLua_SpawnEntity(ent, classname);
	switch (enttype) {
		case 1:
			GLua_SP_Generic(ent);
			break;
		case 2:
			GLua_SP_Logical(ent);
			break;
		case 3:
			GLua_SP_Mover(ent);
			break;
		case 4:
			GLua_SP_Trigger(ent);
			break;
		default:
			trap->Print("WARNING: Internal error in GLua_Spawn_Entity: Invalid ent type!\n");
	}
	return 1;
}


void GLua_Define_Entity(lua_State *L) {

	STACKGUARD_INIT(L)

	luaL_register(L, "ents", ents_f);
	lua_pop(L,1);

	luaL_newmetatable(L,"Entity");
	luaL_register(L, NULL, ents_m);
	GLua_RegisterProperties(L, ents_p, 0);
	
	lua_pushstring(L,"ObjID");
	lua_pushinteger(L, GO_ENTITY);
	lua_settable(L,-3);

	lua_pushstring(L,"ObjName");
	lua_pushstring(L,"Entity");
	lua_settable(L,-3);

	lua_pop(L,1);

	luaL_newmetatable(L,"EntityFactory");
	luaL_register(L, NULL, entfact_m);
	
	lua_pushstring(L,"ObjID");
	lua_pushinteger(L, GO_ENTITYFACTORY);
	lua_settable(L,-3);

	lua_pushstring(L,"ObjName");
	lua_pushstring(L,"EntityFactory");
	lua_settable(L,-3);

	lua_pop(L,1);

	STACKGUARD_CHECK(L)
}