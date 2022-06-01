#include <Include/GlobalConfig.hpp>
#include <Minecraft/Application/MainApplication.hpp>
#include <Minecraft/World/Chunk/ChunkPool.hpp>
#include <Minecraft/World/MinecraftWorld.hpp>

#include <iostream>

int
main( )
{
    if ( !GlobalConfig::LoadFromFile( "../config.ini" ) )
    {
        bool result = GlobalConfig::LoadFromFile( "../../config.ini" );
        assert( result );
    }

/*    ChunkPool pool( 16 );
    pool.SetCentre( { 0, 10, 10 } );

    for ( int i = 0; i < 1; ++i )
        for ( int j = 0; j < 20; ++j )
            for ( int k = 0; k < 20; ++k )
                pool.AddCoordinate( { i, j, k } );

    pool.StartThread( );
    while ( pool.PoolSize( ) > 300 )
    {
        for ( int i = 0; i < 1; ++i )
        {
            for ( int j = 0; j < 20; ++j )
            {
                for ( int k = 0; k < 20; ++k )
                {
                    std::cout << (pool.IsChunkLoaded( { i, j, k } ) ? "@ " : pool.IsChunkLoading( { i, j, k } ) ? "* " : "- ");
                }
                std::cout << std::endl;
            }

            std::cout << std::endl;
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    }

    return 0;*/

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