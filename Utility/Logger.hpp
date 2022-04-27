//
// Created by loys on 16/1/2022.
//

#ifndef MINECRAFT_VK_UTILITY_LOGGER_HPP
#define MINECRAFT_VK_UTILITY_LOGGER_HPP

#include <iostream>
#include <sstream>
#include <string>

class Logger
{

private:
    static constexpr auto RED         = "\033[38;5;9m";
    static constexpr auto RESET_COLOR = "\033[38;5;15m";

public:
    enum class Color {
        wReset,
        eRed,
        eColorSize
    };

    Logger( )
    {
        std::ios_base::sync_with_stdio( false );
        std::cin.tie( nullptr );
    }

    Logger( Logger const& )            = delete;
    Logger& operator=( Logger const& ) = delete;
    Logger( Logger&& )                 = delete;
    Logger& operator=( Logger&& )      = delete;

    inline static Logger&
    getInstance( )
    {
        static Logger instance;
        return instance;
    }

    static inline const char*
    prefix( const Color& color )
    {
        switch ( color )
        {
        case Color::wReset:
            return RESET_COLOR;
        case Color::eRed:
            return RED;
        default:
            break;
        }

        return RESET_COLOR;
    }

    template <typename... Tys>
    void
    LogLine( Tys&&... elms )
    {
        Log( std::forward<Tys>( elms )..., '\n' );
    }

    template <typename Ty, typename... Tys>
    void
    Log( Ty&& elm, Tys&&... elms )
    {
        if constexpr ( std::is_same<typename std::decay<Ty>::type, Color>::value )
        {
            LogWithColor( elm, elms... );
        } else
        {
            std::stringstream output;
            output << std::forward<Ty>( elm );
            ( ( output << ' ' << std::forward<Tys>( elms ) ), ... ) << std::flush;

            Log( output.str( ) );
        }
    }

    template <typename ColorTy, typename... Tys>
    void
    LogWithColor( ColorTy&& color, Tys&&... elms )
    {
        Log( prefix( color ) );
        Log( std::forward<Tys>( elms )... );
        Log( prefix( Color::wReset ) );
    }

    template <typename Ty>
    inline void
    Log( Ty&& elm )
    {
        std::cout << std::forward<Ty>( elm ) << std::flush;
    }

    template <typename Ty>
    inline Logger&
    operator<<( const Ty& elm )
    {
        Log( elm );
        return *this;
    }
};

#endif   // MINECRAFT_VK_UTILITY_LOGGER_HPP
