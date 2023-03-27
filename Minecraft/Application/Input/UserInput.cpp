//
// Created by Lo Yu Sum on 2/7/2023.
//

#include <Include/GraphicAPI.hpp>

#include "UserInput.hpp"

UserInput::UserInput( GLFWwindow* window )
    : m_EventWindow( window )
{
}

void
UserInput::Update( )
{
    if ( !m_EventWindow ) return;

    UpdateMouse( );
}

void
UserInput::UpdateMouse( )
{
    m_PrimaryKey.isPressed = false;
    int mouseLeftState     = glfwGetMouseButton( m_EventWindow, GLFW_MOUSE_BUTTON_LEFT );
    if ( mouseLeftState == GLFW_PRESS && !m_PrimaryKey.isDown )
        m_PrimaryKey.isPressed = true;
    m_PrimaryKey.isDown = mouseLeftState;

    m_SecondaryKey.isPressed = false;
    int mouseRightState    = glfwGetMouseButton( m_EventWindow, GLFW_MOUSE_BUTTON_RIGHT );
    if ( mouseRightState == GLFW_PRESS && !m_SecondaryKey.isDown )
        m_SecondaryKey.isPressed = true;
    m_SecondaryKey.isDown = mouseRightState;
}
