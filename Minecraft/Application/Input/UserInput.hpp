//
// Created by Lo Yu Sum on 2/7/2023.
//

#ifndef MINECRAFT_VK_MINECRAFT_APPLICATION_INPUT_USERINPUT_HPP
#define MINECRAFT_VK_MINECRAFT_APPLICATION_INPUT_USERINPUT_HPP

#include <Minecraft/util/MinecraftType.h>

struct KeyState {
    bool isDown    = false;
    bool isPressed = false;
};

class UserInput
{
private:
    GLFWwindow* m_EventWindow { };

    KeyState m_PrimaryKey;
    KeyState m_SecondaryKey;

    FloatTy m_MouseSensitivity { 0.5f };

    void UpdateMouse( );

public:
    explicit UserInput( GLFWwindow* window = nullptr );

    inline void SetEventWindow( GLFWwindow* window ) { m_EventWindow = window; }
    inline void SetMouseSensitivity( float mouseSensitivity ) { m_MouseSensitivity = mouseSensitivity; }

    void Update( );

    [[nodiscard]] inline KeyState GetPrimaryKey( ) const { return m_PrimaryKey; }
    [[nodiscard]] inline KeyState GetSecondaryKey( ) const { return m_SecondaryKey; }

    [[nodiscard]] inline FloatTy GetMouseSensitivity( ) const { return m_MouseSensitivity; }
};


#endif   // MINECRAFT_VK_MINECRAFT_APPLICATION_INPUT_USERINPUT_HPP
