//
// Created by Lo Yu Sum on 5/14/2023.
//

#ifndef MINECRAFT_VK_UTILITY_ANIMATION_ANIMATIONLINEAR_HPP
#define MINECRAFT_VK_UTILITY_ANIMATION_ANIMATIONLINEAR_HPP

#include "Animation.hpp"

class AnimationLinear : public Animation
{
protected:
    float m_Progress;
    bool  m_FowardDirection { };

public:
    AnimationLinear( float prograss = 0.f )
        : m_Progress( prograss )
    { }

    inline void SetDirection( bool forward )
    {
        m_FowardDirection = forward;
    }

    [[nodiscard]] inline float Lerp( float min, float max ) const
    {
        return std::lerp( min, max, m_Progress );
    }

    inline bool Advance( float deltaPrograss )
    {
        if ( m_FowardDirection && deltaPrograss < 1 )
        {
            m_Progress += deltaPrograss;
            m_Progress = m_Progress > 1 ? 1 : m_Progress;

            return true;
        } else if ( deltaPrograss > 0 )
        {
            m_Progress -= deltaPrograss;
            m_Progress = m_Progress < 0 ? 0 : m_Progress;

            return true;
        }

        return false;
    }
};


#endif   // MINECRAFT_VK_UTILITY_ANIMATION_ANIMATIONLINEAR_HPP
