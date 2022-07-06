//
// Created by samsa on 7/6/2022.
//

#include "CurveEditor.hpp"

#include <Utility/Logger.hpp>

#include <algorithm>

namespace ImGuiAddons
{

bool
CurveEditor::Render( )
{
    static const ImPlotDragToolFlags flags    = ImPlotDragToolFlags_Delayed;
    static const ImPlotAxisFlags     ax_flags = ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks;

    bool changed = false;
    if ( ImPlot::BeginPlot( "##Bezier", ImVec2( -1, 0 ), ImPlotFlags_NoTitle | ImPlotFlags_Crosshairs ) )
    {
        ImPlot::SetupAxes( nullptr, nullptr, ax_flags, ax_flags );
        ImPlot::SetupAxesLimits( 0, 1, 0, 1, ImPlotCond_Always );

        if ( ImPlot::IsPlotHovered( ) && ImGui::IsMouseDoubleClicked( 0 ) )
        {
            const auto mousePos    = ImPlot::GetPlotMousePos( );
            auto       insertPoint = std::find_if( m_Points.begin( ), m_Points.end( ), [ &mousePos ]( const Point& p ) { return p.x >= mousePos.x; } );
            m_Points.insert( insertPoint, mousePos );
        }

        bool dragging = false;
        for ( int i = 0; i < m_Points.size( ); ++i )
        {
            auto id = i;
            if ( draggingBeginIndex >= 0 )
            {
                if ( draggingCurrentIndex == i )
                {
                    id = draggingBeginIndex;
                } else
                {
                    if ( draggingCurrentIndex > draggingBeginIndex && i >= draggingBeginIndex && i < draggingCurrentIndex )
                        ++id;
                    else if ( draggingCurrentIndex < draggingBeginIndex && i > draggingCurrentIndex && i <= draggingBeginIndex )
                        --id;
                }
            }

            const auto previousData = m_Points[ i ];
            if ( ImPlot::DragPoint( id, &m_Points[ i ].x, &m_Points[ i ].y, ImVec4( 0, 0.9f, 1, 1 ), 4, flags ) )
            {
                if ( previousData.x != m_Points[ i ].x || previousData.y != m_Points[ i ].y )
                {
                    changed = true;
                }

                ImPlot::Annotation( m_Points[ i ].x, m_Points[ i ].y, ImVec4( 0, 0, 0, 0 ), ImVec2( 0, -5 ), true, "%.3f, %.3f", m_Points[ i ].x, m_Points[ i ].y );

                dragging = true;
                if ( draggingBeginIndex < 0 )
                {
                    draggingCurrentIndex = draggingBeginIndex = i;
                }

                m_Points[ i ].x = std::clamp( m_Points[ i ].x, 0., 1. );
                m_Points[ i ].y = std::clamp( m_Points[ i ].y, 0., 1. );
            }

            if ( i > 0 && m_Points[ i ].x < m_Points[ i - 1 ].x )
            {
                std::swap( m_Points[ i ], m_Points[ i - 1 ] );
                draggingCurrentIndex += draggingCurrentIndex == i ? -1 : 1;
            }
        }

        if ( !dragging && draggingBeginIndex >= 0 )
        {
            draggingBeginIndex = -1;
        }

        ImPlot::SetNextLineStyle( ImVec4( 0, 0.9f, 0, 1 ), 2 );
        ImPlot::PlotLine( "##bez", &m_Points[ 0 ].x, &m_Points[ 0 ].y, m_Points.size( ), 0, 0, sizeof( Point ) );

        ImPlot::EndPlot( );
    }

    return changed;
}

}   // namespace ImGuiAddons