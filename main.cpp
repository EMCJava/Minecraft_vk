#include <Minecraft/MainApplication.hpp>
#include <Include/GLM.hpp>
#include <Include/GlobalConfig.hpp>
#include <iostream>

int main( )
{
    GlobalConfig::LoadFromFile( "../config.ini" );

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