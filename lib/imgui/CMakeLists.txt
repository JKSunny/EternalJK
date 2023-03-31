#============================================================================
# Copyright (C) 2013 - 2018, OpenJK contributors
#
# This file is part of the OpenJK source code.
#
# OpenJK is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#============================================================================

# We only execute this script if the user wants to use our bundled libimgui.
#
# It is built as a static relocatable library, and linked into any target that
# requires it.

# This only has effect in this directory (lib/imgui).
include_directories(include/)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

add_library(bundled_libimgui STATIC	include/imconfig.h
									include/cimgui.cpp
									include/cimgui.h
									include/imgui.cpp
									include/imgui.h
									include/imgui_demo.cpp
									include/imgui_draw.cpp
									include/imgui_internal.h
									include/imgui_tables.cpp
									include/imgui_widgets.cpp
									include/imstb_rectpack.h
									include/imstb_textedit.h
									include/imstb_truetype.h
                                  )

# Let consumers get at our bundled library in the standard CMake way.  These
# variables are not set in the cache, but instead shadow the variables in the
# cache.  (These are the same variables exported by the standard CMake FindPNG
# module).
set(IMGUI_LIBRARIES bundled_libimgui PARENT_SCOPE)
set(IMGUI_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/imgui/include/ PARENT_SCOPE)

mark_as_advanced(IMGUI_LIBRARIES IMGUI_INCLUDE_DIRS)

# Build as position-independent code, since it will probably be linked into a
# dynamic library.
set_property(TARGET bundled_libimgui PROPERTY POSITION_INDEPENDENT_CODE True)
