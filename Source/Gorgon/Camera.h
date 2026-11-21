#pragma once
#include"Gorgon/Geometry.h"
#include"Gorgon/Geometry/Transform3D.h"
namespace Gorgon{


    class Camera{
        /// The Point in the world space where your camera focus
        /// Example: You can give the player's position
        Gorgon::Geometry::Pointf target={0.0f,0.0f};
        
        /// The Point in the screen space where your camera focus to target
        /// Example: You can give middle of the screen { w/2 , h/2}
        Gorgon::Geometry::Pointf offset={0.0f,0.0f};
        
        /// Zoom rate to target position 
        float zoom=1.0f;
        /// rotation around target position (angles)
        float rotation=0.0f;
        
        public:
        Geometry::Pointf GetScreenToWorld(const Geometry::Pointf &p);
        
        Geometry::Pointf GetWorldToScreen(const Geometry::Pointf &p);

        Gorgon::Geometry::Transform3D GetMatrix();

        void SetRotation( float angles );
        float GetRotation() const; 

        void SetZoomRate( float rate );
        float GetZoomRate() const; 

        void SetOffset(const Gorgon::Geometry::Pointf& offset);
        Gorgon::Geometry::Pointf GetOffset() const;

        void SetTarget(const Gorgon::Geometry::Pointf& offset);
        Gorgon::Geometry::Pointf GetTarget() const;

        void ResetCamera(){
            *this=Camera();
        }

    };



};