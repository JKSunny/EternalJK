/*
Copyright (C) 2018 Christoph Schied
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define VISBUF_INSTANCE_ID_MASK     0x000003FF
#define VISBUF_INSTANCE_PRIM_MASK   0x3FFFFC00
#define VISBUF_INSTANCE_PRIM_SHIFT  10
#define VISBUF_STATIC_INSTANCE_MASK     0x3FFFFFFF
#define VISBUF_WORLD_INSTANCE_FLAG  0x40000000
#define VISBUF_STATIC_INSTANCE_FLAG 	0x80000000

uint visbuf_pack_instance(uint instance_id, uint primitive_id, bool is_world_instance)
{
	return (instance_id & VISBUF_INSTANCE_ID_MASK) 
		| ((primitive_id << VISBUF_INSTANCE_PRIM_SHIFT) & VISBUF_INSTANCE_PRIM_MASK) 
		| (is_world_instance ? VISBUF_WORLD_INSTANCE_FLAG : 0);
}

uint visbuf_pack_static_instance(uint instance_id)
{
	return (instance_id & VISBUF_STATIC_INSTANCE_MASK)
		| VISBUF_STATIC_INSTANCE_FLAG;
}

uint visbuf_get_instance_id(uint u)
{
	return u & VISBUF_INSTANCE_ID_MASK;
}

uint visbuf_get_instance_prim(uint u)
{
	return (u & VISBUF_INSTANCE_PRIM_MASK) >> VISBUF_INSTANCE_PRIM_SHIFT;
}

uint visbuf_get_static_instance_id(uint u)
{
	return u & VISBUF_STATIC_INSTANCE_MASK;
}

bool visbuf_is_world_instance(uint u)
{
	return (u & VISBUF_WORLD_INSTANCE_FLAG) != 0; 
}

bool visbuf_is_static_instance(uint u)
{
	return (u & VISBUF_STATIC_INSTANCE_FLAG) != 0;
}