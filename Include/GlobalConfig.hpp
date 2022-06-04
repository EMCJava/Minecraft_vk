//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_INCLUDE_GLOBALCONFIG_HPP
#define MINECRAFT_VK_INCLUDE_GLOBALCONFIG_HPP

#include <Include/nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>

struct GlobalConfig {

private:
    static void
    ReplaceConfig( nlohmann::json& source, nlohmann::json& target, std::vector<std::string> path )
    {

        if ( source.is_null( ) )
            return;

        if ( source.is_object( ) )
        {
            for ( auto& [ key, val ] : source.items( ) )
            {
                path.push_back( key );
                ReplaceConfig( source[ key ], target[ key ], path );
                path.pop_back( );
            }
        } else
        {

            if ( !target.is_null( ) )
            {
                std::cout << "Replacing config ";
                for ( const auto& step : path )
                {
                    std::cout << step << "->";
                }
                std::cout << source << " with " << source << std::endl;
            }

            target = source;
        }
    }

public:
    static inline nlohmann::json&
    getConfigData( )
    {
        static GlobalConfig gc;
        (void) gc;

        static nlohmann::json j;
        return j;
    }

    static inline nlohmann::json&
    getMinecraftConfigData( )
    {
        return getConfigData( )[ "minecraft" ];
    }

    static bool
    LoadFromFile( const std::string& path )
    {
        // initialize config instance
        auto& existing_config = getConfigData( );

        std::ifstream  file( path );
        nlohmann::json j;

        if ( file )
            j = nlohmann::json::parse( file,
                                       /* callback */ nullptr,
                                       /* allow exceptions */ true,
                                       /* ignore_comments */ true );
        else
            return false;

        bool hasData = !j.is_null( );
        if ( hasData )
        {
            std::cout << "GlobalConfig:" << std::endl;
            std::cout << j.dump( 4 ) << std::endl;
        }

        ReplaceConfig( j, getConfigData( ), { } );

        file.close( );
        return true;
    }
};

#endif   // MINECRAFT_VK_INCLUDE_GLOBALCONFIG_HPP
