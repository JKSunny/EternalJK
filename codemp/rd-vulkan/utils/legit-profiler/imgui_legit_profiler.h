#pragma once

/*
Legit profiler: ImGui helper class for simple profiler histogram<br />
[Repository](https://github.com/Raikiri/LegitProfiler) - 
[License](https://github.com/Raikiri/LegitProfiler/blob/master/LICENSE.txt)

This is a simple component that renders profiler data on your ImGui interface.

ProfilerGraph class is responsible for rendering one profiling histogram with legend 
(either CPU or GPU) and ProfilersWindow is a simple window with 2 graphs designed to 
render CPU and GPU data respectively.

Licensing You're free to use any parts of this code in your private as well as commercial projects as long you leave an attribution somewhere. MIT license, basically. 
I'd also appreciate if you let me know that you found it useful.
*/

#include "imgui.h"
#include <string>
#include <array>
#include <map>
#include <vector>
#include <algorithm>
#include <chrono>

namespace ImGuiUtils
{
    class ProfilerGraph
    {
    public:
        int frameWidth;
        int frameSpacing;
        float maxFrameTime = 1.0f / 50.0f;

        ProfilerGraph( size_t framesCount )
        {
            frames.resize( framesCount );

            for ( auto &frame : frames )
                frame.tasks.reserve( 100 );

            frameWidth = 3;
            frameSpacing = 1;
        }

        void LoadFrameData( const profilerTask_s *tasks, size_t count )
        {
            auto &currFrame = frames[currFrameIndex];
            currFrame.tasks.resize( 0 );
            for ( size_t taskIndex = 0; taskIndex < count; taskIndex++ )
            {
                if ( taskIndex == 0 )
                    currFrame.tasks.push_back( tasks[taskIndex] );
                else {
                    if ( tasks[taskIndex - 1].color != tasks[taskIndex].color || tasks[taskIndex - 1].name != tasks[taskIndex].name )
                        currFrame.tasks.push_back( tasks[taskIndex] );
                    else
                        currFrame.tasks.back().endTime = tasks[taskIndex].endTime;
                }
            }

            currFrame.taskStatsIndex.resize( currFrame.tasks.size() );

            for ( size_t taskIndex = 0; taskIndex < currFrame.tasks.size(); taskIndex++ )
            {
                auto &task = currFrame.tasks[taskIndex];
                auto it = taskNameToStatsIndex.find( task.name );

                if ( it == taskNameToStatsIndex.end() )
                {
                    taskNameToStatsIndex[task.name] = taskStats.size();
                    TaskStats taskStat;
                    taskStat.maxTime = -1.0;
                    taskStats.push_back( taskStat );
                }

                currFrame.taskStatsIndex[taskIndex] = taskNameToStatsIndex[task.name];
            }

            currFrameIndex = ( currFrameIndex + 1 ) % frames.size();

            RebuildTaskStats(currFrameIndex, 300/*frames.size()*/);
        }

        void RenderTimings( int graphWidth, int legendWidth, int height, int frameIndexOffset )
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            vec2_t widgetPos = { ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y};

            vec2_t graphSize = { (float)graphWidth, (float)height };
            vec2_t graphSizeLegend = { (float)legendWidth, (float)height };
            vec2_t graphPosLegend = { (float)graphWidth, 0.0f };

            VectorAdd2( widgetPos, graphPosLegend, graphPosLegend );
            RenderGraph( drawList, widgetPos, graphSize, frameIndexOffset );
            RenderLegend( drawList, graphPosLegend, graphSizeLegend, frameIndexOffset );
            ImGui::Dummy( ImVec2( float( graphWidth + legendWidth ), float(height) ) );
        }

    private:
        void RebuildTaskStats( size_t endFrame, size_t framesCount )
        {
            for ( auto &taskStat : taskStats )
            {
                taskStat.maxTime = -1.0f;
                taskStat.priorityOrder = size_t(-1);
                taskStat.onScreenIndex = size_t(-1);
            }

            for ( size_t frameNumber = 0; frameNumber < framesCount; frameNumber++ )
            {
                size_t frameIndex = ( endFrame - 1 - frameNumber + frames.size() ) % frames.size();
                auto &frame = frames[frameIndex];

                for ( size_t taskIndex = 0; taskIndex < frame.tasks.size(); taskIndex++ )
                {
                    auto &task = frame.tasks[taskIndex];
                    auto &stats = taskStats[frame.taskStatsIndex[taskIndex]];
                    stats.maxTime = std::max( stats.maxTime, task.endTime - task.startTime );
                }
            }

            std::vector<size_t> statPriorities;
            statPriorities.resize( taskStats.size() );

            for( size_t statIndex = 0; statIndex < taskStats.size(); statIndex++ )
                statPriorities[statIndex] = statIndex;

            std::sort( statPriorities.begin(), statPriorities.end(), [this](size_t left, size_t right ) {
                return taskStats[left].maxTime > taskStats[right].maxTime; 
            });
            
            for ( size_t statNumber = 0; statNumber < taskStats.size(); statNumber++ )
            {
                size_t statIndex = statPriorities[statNumber];
                taskStats[statIndex].priorityOrder = statNumber;
            }
        }

        void RenderGraph( ImDrawList *drawList, vec2_t graphPos, vec2_t graphSize, size_t frameIndexOffset )
        {
            vec2_t maxPoint;
            VectorAdd2( graphPos, graphSize, maxPoint );
            Rect( drawList, graphPos, maxPoint, 0xffffffff, false );
            float maxFrameTime = 1.0f / 50.0f;
            float heightThreshold = 1.0f;

            for ( size_t frameNumber = 0; frameNumber < frames.size(); frameNumber++ )
            {
                size_t frameIndex = ( currFrameIndex - frameIndexOffset - 1 - frameNumber + 2 * frames.size() ) % frames.size();
                vec2_t framePos =  { graphSize[0] - 1 - frameWidth - (frameWidth + frameSpacing) * frameNumber, graphSize[1] - 1 };
                VectorAdd2( graphPos, framePos, framePos );
          
                if ( framePos[0] < graphPos[0] + 1 )
                    break;

                vec2_t taskPos;
                Com_Memcpy( taskPos, framePos, sizeof(vec2_t) );
                auto &frame = frames[frameIndex];

                for ( const auto& task : frame.tasks )
                {
                    float taskStartHeight = ( float(task.startTime) / maxFrameTime ) * graphSize[1];
                    float taskEndHeight = ( float(task.endTime) / maxFrameTime ) * graphSize[1];
                
                    if ( abs( taskEndHeight - taskStartHeight ) > heightThreshold ) {
             
                        vec2_t minPoint = { 0.0f, -taskStartHeight };
                        vec2_t maxPoint = { (float)frameWidth, -taskEndHeight };
                        VectorAdd2( taskPos, minPoint, minPoint );
                        VectorAdd2( taskPos, maxPoint, maxPoint );

                        Rect( drawList, minPoint, maxPoint, task.color, true );
                    }
                }
            }
        }

        void RenderLegend( ImDrawList *drawList, vec2_t legendPos, vec2_t legendSize, size_t frameIndexOffset )
        {
            float markerLeftRectMargin = 3.0f;
            float markerLeftRectWidth = 5.0f;
            float maxFrameTime = 1.0f / 50.0f;
            float markerMidWidth = 30.0f;
            float markerRightRectWidth = 10.0f;
            float markerRigthRectMargin = 3.0f;
            float markerRightRectHeight = 10.0f;
            float markerRightRectSpacing = 4.0f;
            float nameOffset = 30.0f;
            vec2_t textMargin = { 5.0f, -3.0f };

            auto &currFrame = frames[(currFrameIndex - frameIndexOffset - 1 + 2 * frames.size()) % frames.size()];
            size_t maxTasksCount = size_t( legendSize[1] / ( markerRightRectHeight + markerRightRectSpacing ) );

            for ( auto &taskStat : taskStats )
                taskStat.onScreenIndex = size_t( -1 );

            size_t tasksToShow = std::min<size_t>( taskStats.size(), maxTasksCount );
            size_t tasksShownCount = 0;

            for ( size_t taskIndex = 0; taskIndex < currFrame.tasks.size(); taskIndex++ )
            {
                auto &task = currFrame.tasks[taskIndex];
                auto &stat = taskStats[currFrame.taskStatsIndex[taskIndex]];

                if ( stat.priorityOrder >= tasksToShow )
                    continue;

                if ( stat.onScreenIndex == size_t( -1 ) )
                    stat.onScreenIndex = tasksShownCount++;
                else
                    continue;

                float taskStartHeight = ( float(task.startTime) / maxFrameTime ) * legendSize[1];
                float taskEndHeight = ( float(task.endTime) / maxFrameTime ) * legendSize[1];

                vec2_t markerLeftRectMin = { markerLeftRectMargin, legendSize[1] };
                VectorAdd2( legendPos, markerLeftRectMin, markerLeftRectMin );

                vec2_t markerLeftRectMax = { markerLeftRectWidth, 0.0f };
                VectorAdd2( markerLeftRectMin, markerLeftRectMax, markerLeftRectMax );

                markerLeftRectMin[1] -= taskStartHeight;
                markerLeftRectMax[1] -= taskEndHeight;

                vec2_t markerRightRectMin = {
                    markerLeftRectMargin + markerLeftRectWidth + markerMidWidth, 
                    legendSize[1] - markerRigthRectMargin - ( markerRightRectHeight + markerRightRectSpacing ) * stat.onScreenIndex
                };
                VectorAdd2( legendPos, markerRightRectMin, markerRightRectMin );
        
                vec2_t markerRightRectMax = { markerRightRectWidth, -markerRightRectHeight };
                VectorAdd2( markerRightRectMin, markerRightRectMax, markerRightRectMax );       
            
                RenderTaskMarker( drawList, markerLeftRectMin, markerLeftRectMax, markerRightRectMin, markerRightRectMax, task.color );

                float taskTimeMs = float( task.endTime - task.startTime );
                /*std::ostringstream timeText;
                timeText.precision(2);
                timeText << std::fixed << std::string("[") << (taskTimeMs * 1000.0f);
                */

                vec2_t m1, m2;
                vec2_t _nameOffset = { nameOffset, 0.0f };
                VectorAdd2( markerRightRectMax, textMargin, m1 );
                VectorAdd2( m1, _nameOffset, m2 );

                Text( drawList, m1, task.color, va( "%.2f", taskTimeMs * 1000.0f ) );
                Text( drawList, m2, task.color, ( std::string( "ms] " ) + task.name ).c_str() );
            }
        }

        static void Rect( ImDrawList *drawList, vec2_t minPoint, vec2_t maxPoint, uint32_t col, bool filled = true )
        {
            if( filled )
                drawList->AddRectFilled( ImVec2( minPoint[0], minPoint[1] ), ImVec2( maxPoint[0], maxPoint[1] ), col );
            else
                drawList->AddRect( ImVec2( minPoint[0], minPoint[1] ), ImVec2( maxPoint[0], maxPoint[1] ), col );
        }

        static void Text( ImDrawList *drawList, vec2_t point, uint32_t col, const char *text )
        {
            drawList->AddText( ImVec2( point[0], point[1] ), col, text );
        }

        static void Triangle( ImDrawList *drawList, std::array<vec2_t, 3> points, uint32_t col, bool filled = true )
        {
            if ( filled )
                drawList->AddTriangleFilled( ImVec2( points[0][0], points[0][1] ), ImVec2( points[1][0], points[1][1] ), ImVec2( points[2][0], points[2][1] ), col );
            else
                drawList->AddTriangle( ImVec2( points[0][0], points[0][1] ), ImVec2( points[1][0], points[1][1] ), ImVec2( points[2][0], points[2][1] ), col );
        }

        static void RenderTaskMarker( ImDrawList *drawList, vec2_t leftMinPoint, vec2_t leftMaxPoint, vec2_t rightMinPoint, vec2_t rightMaxPoint, uint32_t col )
        {
            Rect( drawList, leftMinPoint, leftMaxPoint, col, true );
            Rect( drawList, rightMinPoint, rightMaxPoint, col, true );

            std::array<ImVec2, 4> points = {
                ImVec2( leftMaxPoint[0], leftMinPoint[1] ),
                ImVec2( leftMaxPoint[0], leftMaxPoint[1] ),
                ImVec2( rightMinPoint[0], rightMaxPoint[1] ),
                ImVec2( rightMinPoint[0], rightMinPoint[1] )
            };

            drawList->AddConvexPolyFilled( points.data(), int( points.size() ), col );
        }

        struct FrameData
        {
            std::vector<profilerTask_s> tasks;
            std::vector<size_t>         taskStatsIndex;
        };

        struct TaskStats
        {
            double maxTime;
            size_t priorityOrder;
            size_t onScreenIndex;
        };

        std::vector<TaskStats>          taskStats;
        std::map<std::string, size_t>   taskNameToStatsIndex;
        std::vector<FrameData>          frames;
        size_t                          currFrameIndex = 0;
    };

  class ProfilersWindow
  {
  public:
        ProfilersWindow():
            cpuGraph(300),
            gpuGraph(300)
        {
            pause = false;
            frameOffset = 0;
            frameWidth = 3;
            frameSpacing = 1;
            prevFpsFrameTime = std::chrono::system_clock::now();
            fpsFramesCount = 0;
            avgFrameTime = 1.0f;
        }

        void TextColumn( const char *label, const char *content, float columnWidth ) 
        {
            ImGui::PushID( label );
	        ImGui::Columns(2);
	        ImGui::SetColumnWidth( 0, columnWidth );
	        ImGui::Text( label );
	        ImGui::NextColumn();
            ImGui::Text( content );
	        ImGui::Columns(1);
	        ImGui::PopID();
        }

        void Render( bool *p_open )
        {
            fpsFramesCount++;
            auto currFrameTime = std::chrono::system_clock::now();
            {
                float fpsDeltaTime = std::chrono::duration<float>( currFrameTime - prevFpsFrameTime ).count();
                if ( fpsDeltaTime > 0.5f )
                {
                    this->avgFrameTime = fpsDeltaTime / float(fpsFramesCount);
                    fpsFramesCount = 0;
                    prevFpsFrameTime = currFrameTime;
                }
            }

            ImGui::Begin( "Legit profiler", p_open, ImGuiWindowFlags_NoScrollbar );
            ImVec2 canvasSize = ImGui::GetContentRegionAvail();

            int sizeMargin = int( ImGui::GetStyle().ItemSpacing[1] );
            int maxGraphHeight = 300;
            
            // hide gpu graph for now
            //int availableGraphHeight = ( int( canvasSize[1] ) - sizeMargin ) / 2;
            int availableGraphHeight = ( int( canvasSize[1] ) - sizeMargin );

            int graphHeight = std::min( maxGraphHeight, availableGraphHeight );
            int legendWidth = 200;
            int graphWidth = int( canvasSize[0] ) - legendWidth;

            //gpuGraph.RenderTimings( graphWidth, legendWidth, graphHeight, frameOffset );
            cpuGraph.RenderTimings( graphWidth, legendWidth, graphHeight, frameOffset );
        
            //if ( graphHeight * 2 + sizeMargin + sizeMargin < canvasSize[1] )
            //{
            //    ImGui::Columns( 2 );

                ImGui::Checkbox( "Pause", &pause );

                /*
                ImGui::DragInt( "Frame offset", &frameOffset, 1.0f, 0, 400 );
                ImGui::NextColumn();

                ImGui::SliderInt( "Frame width", &frameWidth, 1, 4 );
                ImGui::SliderInt( "Frame spacing", &frameSpacing, 0, 2 );
                ImGui::SliderFloat( "Transparency", &ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w, 0.0f, 1.0f );
                ImGui::Columns( 1 );
                */
            //}

            #ifdef USE_VK_STATS
                float columnWidth = 200;
                TextColumn( "max_vertex_usage", va( "%iKb", (int)((vk.stats.vertex_buffer_max + 1023) / 1024) ), columnWidth );
                TextColumn( "max_push_size", va( "%ib", vk.stats.push_size_max ), columnWidth);
                TextColumn( "pipeline handles", va( "%i", vk.pipeline_create_count ), columnWidth);
                TextColumn( "pipeline descriptors", va( "%i, base: %i", vk.pipelines_count, vk.pipelines_world_base ), columnWidth);
                TextColumn( "image chunks", va( "%i", vk_world.num_image_chunks, vk.pipelines_world_base ), columnWidth);
                TextColumn( "ID3 shaders", va( "%i", tr.numShaders ), columnWidth);

                #ifdef USE_VBO
                    const int vbo_mode = MIN( r_vbo->integer, 3 );
                    const char *vbo_mode_string[4] = { "off", "world", "world + models", "models" };

                    TextColumn( "VBO mode:", vbo_mode_string[vbo_mode], columnWidth );

                    if ( r_vbo->integer )
                        TextColumn( "VBO buffers", va( "%i, world index: %i", vk.vbo_count, vk.vbo_world_index ) ,columnWidth);
                #endif
            #endif

            if ( !pause )
                frameOffset = 0;

            gpuGraph.frameWidth = frameWidth;
            gpuGraph.frameSpacing = frameSpacing;
            cpuGraph.frameWidth = frameWidth;
            cpuGraph.frameSpacing = frameSpacing;

            ImGui::End();
        }

        using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
        
        bool            pause;
        ProfilerGraph   cpuGraph;
        ProfilerGraph   gpuGraph;
        int             frameOffset;
        int             frameWidth;
        int             frameSpacing;     
        TimePoint       prevFpsFrameTime;
        size_t          fpsFramesCount;
        float           avgFrameTime;
    };
}