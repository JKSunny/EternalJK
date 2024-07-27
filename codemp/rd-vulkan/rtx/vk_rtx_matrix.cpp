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

void mult_matrix_matrix( float *p, const float *a, const float *b )
{
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			p[i * 4 + j] =
				a[0 * 4 + j] * b[i * 4 + 0] +
				a[1 * 4 + j] * b[i * 4 + 1] +
				a[2 * 4 + j] * b[i * 4 + 2] +
				a[3 * 4 + j] * b[i * 4 + 3];
		}
	}

}

void create_entity_matrix( float matrix[16], trRefEntity_t *e, qboolean enable_left_hand )
{
	orientationr_t ori;

	R_RotateForEntity( e, &backEnd.viewParms, &ori );
	Matrix16Copy( ori.modelMatrix, matrix );
}

void create_projection_matrix( float matrix[16], float znear, float zfar, float fov_x, float fov_y )
{
	float xmin, xmax, ymin, ymax;
	float width, height, depth;

	ymax = znear * tan(fov_y * M_PI / 360.0);
	ymin = -ymax;

	xmax = znear * tan(fov_x * M_PI / 360.0);
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;
	depth = zfar - znear;

	matrix[0] = 2 * znear / width;
	matrix[4] = 0;
	matrix[8] = (xmax + xmin) / width;
	matrix[12] = 0;

	matrix[1] = 0;
	matrix[5] = -2 * znear / height;
	matrix[9] = (ymax + ymin) / height;
	matrix[13] = 0;

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = (zfar + znear) / depth;
	matrix[14] = 2 * zfar * znear / depth;

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 1;
	matrix[15] = 0;
}

void create_orthographic_matrix( float matrix[16], float xmin, float xmax,
								 float ymin, float ymax, float znear, float zfar )
{
	float width, height, depth;

	width = xmax - xmin;
	height = ymax - ymin;
	depth = zfar - znear;

	matrix[0] = 2 / width;
	matrix[4] = 0;
	matrix[8] = 0;
	matrix[12] = -(xmax + xmin) / width;

	matrix[1] = 0;
	matrix[5] = 2 / height;
	matrix[9] = 0;
	matrix[13] = -(ymax + ymin) / height;

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = 1 / depth;
	matrix[14] = -znear / depth;

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
}

void create_view_matrix( float matrix[16], trRefdef_t *fd )
{
	matrix[0]  = -fd->viewaxis[1][0];
	matrix[4]  = -fd->viewaxis[1][1];
	matrix[8]  = -fd->viewaxis[1][2];
	matrix[12] = DotProduct(fd->viewaxis[1], fd->vieworg);

	matrix[1]  = fd->viewaxis[2][0];
	matrix[5]  = fd->viewaxis[2][1];
	matrix[9]  = fd->viewaxis[2][2];
	matrix[13] = -DotProduct(fd->viewaxis[2], fd->vieworg);

	matrix[2]  = fd->viewaxis[0][0];
	matrix[6]  = fd->viewaxis[0][1];
	matrix[10] = fd->viewaxis[0][2];
	matrix[14] = -DotProduct(fd->viewaxis[0], fd->vieworg);

	matrix[3]  = 0;
	matrix[7]  = 0;
	matrix[11] = 0;
	matrix[15] = 1;
}

void inverse( const float *m, float *inv )
{
	inv[0] = m[5]  * m[10] * m[15] -
	         m[5]  * m[11] * m[14] -
	         m[9]  * m[6]  * m[15] +
	         m[9]  * m[7]  * m[14] +
	         m[13] * m[6]  * m[11] -
	         m[13] * m[7]  * m[10];
	
	inv[1] = -m[1]  * m[10] * m[15] +
	          m[1]  * m[11] * m[14] +
	          m[9]  * m[2] * m[15] -
	          m[9]  * m[3] * m[14] -
	          m[13] * m[2] * m[11] +
	          m[13] * m[3] * m[10];
	
	inv[2] = m[1]  * m[6] * m[15] -
	         m[1]  * m[7] * m[14] -
	         m[5]  * m[2] * m[15] +
	         m[5]  * m[3] * m[14] +
	         m[13] * m[2] * m[7] -
	         m[13] * m[3] * m[6];
	
	inv[3] = -m[1] * m[6] * m[11] +
	          m[1] * m[7] * m[10] +
	          m[5] * m[2] * m[11] -
	          m[5] * m[3] * m[10] -
	          m[9] * m[2] * m[7] +
	          m[9] * m[3] * m[6];
	
	inv[4] = -m[4]  * m[10] * m[15] +
	          m[4]  * m[11] * m[14] +
	          m[8]  * m[6]  * m[15] -
	          m[8]  * m[7]  * m[14] -
	          m[12] * m[6]  * m[11] +
	          m[12] * m[7]  * m[10];
	
	inv[5] = m[0]  * m[10] * m[15] -
	         m[0]  * m[11] * m[14] -
	         m[8]  * m[2] * m[15] +
	         m[8]  * m[3] * m[14] +
	         m[12] * m[2] * m[11] -
	         m[12] * m[3] * m[10];
	
	inv[6] = -m[0]  * m[6] * m[15] +
	          m[0]  * m[7] * m[14] +
	          m[4]  * m[2] * m[15] -
	          m[4]  * m[3] * m[14] -
	          m[12] * m[2] * m[7] +
	          m[12] * m[3] * m[6];
	
	inv[7] = m[0] * m[6] * m[11] -
	         m[0] * m[7] * m[10] -
	         m[4] * m[2] * m[11] +
	         m[4] * m[3] * m[10] +
	         m[8] * m[2] * m[7] -
	         m[8] * m[3] * m[6];
	
	inv[8] = m[4]  * m[9] * m[15] -
	         m[4]  * m[11] * m[13] -
	         m[8]  * m[5] * m[15] +
	         m[8]  * m[7] * m[13] +
	         m[12] * m[5] * m[11] -
	         m[12] * m[7] * m[9];
	
	inv[9] = -m[0]  * m[9] * m[15] +
	          m[0]  * m[11] * m[13] +
	          m[8]  * m[1] * m[15] -
	          m[8]  * m[3] * m[13] -
	          m[12] * m[1] * m[11] +
	          m[12] * m[3] * m[9];
	
	inv[10] = m[0]  * m[5] * m[15] -
	          m[0]  * m[7] * m[13] -
	          m[4]  * m[1] * m[15] +
	          m[4]  * m[3] * m[13] +
	          m[12] * m[1] * m[7] -
	          m[12] * m[3] * m[5];
	
	inv[11] = -m[0] * m[5] * m[11] +
	           m[0] * m[7] * m[9] +
	           m[4] * m[1] * m[11] -
	           m[4] * m[3] * m[9] -
	           m[8] * m[1] * m[7] +
	           m[8] * m[3] * m[5];
	
	inv[12] = -m[4]  * m[9] * m[14] +
	           m[4]  * m[10] * m[13] +
	           m[8]  * m[5] * m[14] -
	           m[8]  * m[6] * m[13] -
	           m[12] * m[5] * m[10] +
	           m[12] * m[6] * m[9];
	
	inv[13] = m[0]  * m[9] * m[14] -
	          m[0]  * m[10] * m[13] -
	          m[8]  * m[1] * m[14] +
	          m[8]  * m[2] * m[13] +
	          m[12] * m[1] * m[10] -
	          m[12] * m[2] * m[9];
	
	inv[14] = -m[0]  * m[5] * m[14] +
	           m[0]  * m[6] * m[13] +
	           m[4]  * m[1] * m[14] -
	           m[4]  * m[2] * m[13] -
	           m[12] * m[1] * m[6] +
	           m[12] * m[2] * m[5];
	
	inv[15] = m[0] * m[5] * m[10] -
	          m[0] * m[6] * m[9] -
	          m[4] * m[1] * m[10] +
	          m[4] * m[2] * m[9] +
	          m[8] * m[1] * m[6] -
	          m[8] * m[2] * m[5];

	float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	det = 1.0f / det;

	for(int i = 0; i < 16; i++)
		inv[i] = inv[i] * det;
}

void mult_matrix_vector( float *p, const float *a, const float *b )
{
	int j;
	for (j = 0; j < 4; j++) {
		p[j] =
			a[0 * 4 + j] * b[0] +
			a[1 * 4 + j] * b[1] +
			a[2 * 4 + j] * b[2] +
			a[3 * 4 + j] * b[3];
	}
}