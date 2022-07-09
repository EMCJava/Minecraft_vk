//
// Created by samsa on 7/6/2022.
//

#ifndef MINECRAFT_VK_UTILITY_IMGUIADDONS_CURVEEDITOR_HPP
#define MINECRAFT_VK_UTILITY_IMGUIADDONS_CURVEEDITOR_HPP

#include <Include/imgui_include.hpp>
#include <vector>

namespace ImGuiAddons
{

class CurveEditor
{
    using Point = ImPlotPoint;

    std::vector<Point> m_Points;
    int                draggingBeginIndex = -1, draggingCurrentIndex { };

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
};

}   // namespace ImGuiAddons

#endif   // MINECRAFT_VK_UTILITY_IMGUIADDONS_CURVEEDITOR_HPP
