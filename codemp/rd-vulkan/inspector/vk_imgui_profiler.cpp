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

#include <imgui.h>
#include "vk_imgui.h"
#include <icons/FontAwesome5/IconsFontAwesome5.h>
#include "icons/FontAwesome5/fa5solid900.cpp"

#include <utils/legit-profiler/imgui_legit_profiler.h>

ImGuiUtils::ProfilersWindow profilersWindow;

typedef struct profilerFrame_s {
	std::chrono::high_resolution_clock::time_point frameStartTime;
	std::vector<profilerTask_s> cpu_tasks;
	size_t frame_interval_index;
} profilerFrame_t;

typedef struct {
	profilerFrame_t frames[2];

	struct {
		profilerFrame_t *current;
		profilerFrame_t *previous;
	} frame;
} imgui_profiler_t;

imgui_profiler_t		profiler;

double vk_imgui_profiler_get_time_secconds() {
	return double( std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::high_resolution_clock::now() - profiler.frame.current->frameStartTime ).count() ) / 1e6;
}

size_t vk_imgui_profiler_insert_task( const char *name, uint32_t color, double startTime, double endTime ) 
{
	if ( profiler.frame.current == nullptr )
		return -1;

	profilerFrame_t *frame;
	profilerTask_s	task;

	frame = profiler.frame.current;
	Com_Memset( &task, 0, sizeof(task) );

	Q_strncpyz( task.name, name, sizeof(task.name) );

	task.color		= color;
	task.startTime	= startTime;
	task.endTime	= endTime;

	frame->cpu_tasks.push_back( task );
	return frame->cpu_tasks.size();
}

size_t vk_imgui_profiler_start_task( const char *name, uint32_t color ) 
{
	return vk_imgui_profiler_insert_task( name, color, vk_imgui_profiler_get_time_secconds(), -1.0f );
}

void vk_imgui_profiler_end_task( uint32_t task_id ) {
	if ( profiler.frame.current != nullptr )
		profiler.frame.current->cpu_tasks.back().endTime = vk_imgui_profiler_get_time_secconds();
}

void vk_imgui_profiler_begin_frame( void ) 
{
#ifndef PROFILER_PREPEND_WAITMAXFPS
	if ( profiler.frame.current != nullptr )
		vk_imgui_profiler_end_task( profiler.frame.current->frame_interval_index );
#endif

	profiler.frame.current = &profiler.frames[ imguiGlobal.profiler_index++ ];
	imguiGlobal.profiler_index %= 2;

	profiler.frame.current->cpu_tasks.clear();
#ifndef PROFILER_PREPEND_WAITMAXFPS
	profiler.frame.current->frameStartTime = std::chrono::high_resolution_clock::now();
#else
	vk_imgui_profiler_insert_task( "WaitForTargetFPS", RGBA_LE(0x2c3e50ffu), 0.0, vk_imgui_profiler_get_time_secconds() );
#endif

	/*
	// display entire frame time
	ImGuiIO& io = ImGui::GetIO();
	double endTime = (1000.0f / io.Framerate) * 0.001;
	vk_imgui_profiler_insert_task( "Entire frame", RGBA_LE(0x2c3e50ffu), 0.0f, endTime );
	*/
}

void vk_imgui_profiler_end_frame( void ) 
{
#ifndef PROFILER_PREPEND_WAITMAXFPS
	profiler.frame.current->frame_interval_index = vk_imgui_profiler_start_task( "WaitForTargetFPS", RGBA_LE(0x2c3e50ffu) );
#else
	profiler.frames[ imguiGlobal.profiler_index ].frameStartTime = std::chrono::high_resolution_clock::now();
#endif

	profiler.frame.previous = profiler.frame.current;
}

void vk_imgui_draw_profiler( void )
{
	if( !windows.profiler.p_open )
		return;

	if ( profiler.frame.previous == nullptr )
		return;

	if ( !profilersWindow.pause )
		profilersWindow.cpuGraph.LoadFrameData( profiler.frame.previous->cpu_tasks.data(), profiler.frame.previous->cpu_tasks.size() );
	
	profilersWindow.Render( &windows.profiler.p_open );	
}