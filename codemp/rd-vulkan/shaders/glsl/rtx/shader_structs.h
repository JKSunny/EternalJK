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

#ifndef SHADER_STRUCTS_H

// shared structures between GLSL and C
#ifdef GLSL
    #define STRUCT(content, name) struct name { content };
    #define BOOL(n)		bool n;
    #define INT(n)		int n;
    #define UINT(n)		uint n;
    #define FLOAT(n)	float n;
    #define VEC2(n)		vec2 n;
    #define VEC3(n)		vec3 n;
    #define VEC4(n)		vec4 n;
    #define MAT4(n)		mat4 n;
    #define MAT4X3(n)	mat4x3 n;
    #define MAT3X4(n)	mat3x4 n;
    #define UVEC2(n)	uvec2 n;
    #define UVEC3(n)	uvec3 n;
	#define IVEC3(n)	ivec3 n;
	#define IVEC4(n)	ivec4 n;
	#define uint32_t	uint
	#define MAX_LIGHT_STYLES		64
	#define BONESREF(n)	mat3x4 n[72];	
#else
    #define STRUCT(content, name) typedef struct { content } name;
    #define BOOL(n)		unsigned int n;
    #define INT(n)		int n;
    #define UINT(n)		unsigned int n;
    #define FLOAT(n)	float n;
    #define VEC2(n)		float n[2];
    #define VEC3(n)		float n[3];
    #define VEC4(n)		float n[4];
    #define MAT4(n)		float n[16];
    #define MAT4X3(n)	float n[12];
    #define MAT3X4(n)	float n[12];
    #define UVEC2(n)	unsigned int n[2];
    #define UVEC3(n)	unsigned int n[3];
	#define IVEC3(n)	int n[3];
	#define IVEC4(n)	int n[4];
	#define BONESREF(n)	mat3x4_t n[72];	
#endif

#endif // SHADER_STRUCTS_H