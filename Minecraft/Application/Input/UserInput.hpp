//
// Created by Lo Yu Sum on 2/7/2023.
//

#ifndef MINECRAFT_VK_MINECRAFT_APPLICATION_INPUT_USERINPUT_HPP
#define MINECRAFT_VK_MINECRAFT_APPLICATION_INPUT_USERINPUT_HPP

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

    void UpdateMouse( );

public:
    explicit UserInput( GLFWwindow* window = nullptr );

    void SetEventWindow( GLFWwindow* window ) { m_EventWindow = window; }

    void Update( );

    [[nodiscard]] KeyState GetPrimaryKey( ) const { return m_PrimaryKey; }
    [[nodiscard]] KeyState GetSecondaryKey( ) const { return m_SecondaryKey; }
};


#endif   // MINECRAFT_VK_MINECRAFT_APPLICATION_INPUT_USERINPUT_HPP
