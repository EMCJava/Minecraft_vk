#include <Include/GlobalConfig.hpp>
#include <Minecraft/Application/MainApplication.hpp>
#include <Utility/Math/Math.hpp>

#include <iostream>

int
main( )
{
    bool result = GlobalConfig::LoadFromFile( "config.ini" );
    (void) result;
    assert( result );

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