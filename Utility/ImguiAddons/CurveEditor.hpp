//
// Created by samsa on 7/6/2022.
//

#ifndef MINECRAFT_VK_UTILITY_IMGUIADDONS_CURVEEDITOR_HPP
#define MINECRAFT_VK_UTILITY_IMGUIADDONS_CURVEEDITOR_HPP

#include <Include/imgui_include.hpp>

#include <iostream>
#include <vector>

namespace ImGuiAddons
{

class CurveEditor
{
public:
    using Point = ImPlotPoint;

private:
    std::vector<Point> m_Points;
    int draggingBeginIndex = -1, draggingCurrentIndex { };

public:
    CurveEditor( )
        : m_Points( 0 ) { };

    explicit CurveEditor( const std::vector<Point>& points )
        : m_Points( points ) { };

    explicit CurveEditor( std::vector<Point>&& lpoints )
        : m_Points( std::forward<std::vector<Point>&&>( lpoints ) ) { };

    /*
     *
     * Return true if data changed
     *
     * */
    bool Render( );

    double Sample( float X );

    void SetCurve( std::vector<Point>&& points )
    {
        m_Points = std::move( points );
    }

    friend inline std::ostream& operator<<( std::ostream& os, const CurveEditor& ce );
};

inline std::ostream&
operator<<( std::ostream& os, const CurveEditor& ce )
{

    if ( ce.m_Points.empty( ) ) return os;

    std::cout << "{(" << ce.m_Points[ 0 ].x << " " << ce.m_Points[ 0 ].y << ")";
    for ( int i = 1; i < ce.m_Points.size( ); i++ )
    {
        std::cout << ", (" << ce.m_Points[ i ].x << " " << ce.m_Points[ i ].y << ")";
    }

    return std::cout << '}' << std::flush;
}

}   // namespace ImGuiAddons

#endif   // MINECRAFT_VK_UTILITY_IMGUIADDONS_CURVEEDITOR_HPP
