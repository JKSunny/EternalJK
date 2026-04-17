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

#ifndef CHECKERBOARD_GLSL_
#define CHECKERBOARD_GLSL_

// Convert a checkerboarded pixel position (left and right fields) to flat-screen position
ivec2 checker_to_flat(ivec2 pos, int width)
{
    uint half_width = width / 2;
    bool is_even_checkerboard = pos.x < half_width;

    return ivec2(
        is_even_checkerboard
            ? (pos.x * 2) + (pos.y & 1)
            : ((pos.x - half_width) * 2) + ((pos.y & 1) ^ 1),
        pos.y);
}

// Convert a flat-screen (regular) pixel position to checkerboarded (left and right fields)
ivec2 flat_to_checker(ivec2 pos, int width)
{
    uint half_width = width / 2;
    bool is_even_checkerboard = (pos.x & 1) == (pos.y & 1);

    return ivec2(
        (pos.x / 2) + (is_even_checkerboard ? 0 : half_width),
        pos.y);
}

#endif // CHECKERBOARD_GLSL_