#include <Include/GLM.hpp>
#include <Include/GlobalConfig.hpp>
#include <Minecraft/Application/MainApplication.hpp>

#include <iostream>
#include <cassert>

int
main( )
{
    if ( !GlobalConfig::LoadFromFile( "../config.ini" ) )
    {
        bool result = GlobalConfig::LoadFromFile( "../../config.ini" );
        assert( result );
    }

    bool success = true;

    try
    {
        MainApplication app;
        app.run( );
    }
    catch ( const std::exception& e )
    {
        std::cerr << e.what( ) << std::endl;
        success = false;
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}