#pragma once
#include"Gorgon/Geometry.h"
#include"Gorgon/Geometry/Transform3D.h"
namespace Gorgon{


    class Camera{
        public:
        /// The Point in the world space where your camera focus
        /// Example: You can give the player's position
        Gorgon::Geometry::Pointf Target={0.0f,0.0f};
        
        /// The Point in the screen space where your camera focus to target
        /// Example: You can give middle of the screen { w/2 , h/2}
        Gorgon::Geometry::Pointf Offset={0.0f,0.0f};
        
        /// Zoom rate to target position 
        float Zoom=1.0f;
        /// rotation around target position (angles)
        float Rotation=0.0f;
        
        Geometry::Pointf GetScreenToWorld(const Geometry::Pointf &p);
        
        Geometry::Pointf GetWorldToScreen(const Geometry::Pointf &p);

        Gorgon::Geometry::Transform3D GetMatrix();

        void ResetCamera(){
            *this=Camera();
        }

    };



};