#include"Camera.h"
namespace Gorgon::Gaming{

    Geometry::Pointf Camera::GetScreenToWorld(const Geometry::Pointf &p){
        auto p1=p-Offset;
        p1=p1/Zoom;
        p1=Gorgon::Geometry::Pointf(  
            p1.X*cosf(-Rotation*Gorgon::PI/180.0f)-p1.Y*sinf(-Rotation*Gorgon::PI/180.0f),
            p1.X*sinf(-Rotation*Gorgon::PI/180.0f)+p1.Y*cosf(-Rotation*Gorgon::PI/180.0f)
        );
        p1=p1+Target;
        return p1;
    }
    Geometry::Pointf Camera::GetWorldToScreen(const Geometry::Pointf &p){
        auto p1=p-Target;
        p1=p1*Zoom;
        p1=Gorgon::Geometry::Pointf(  
            p1.X*cosf(Rotation*Gorgon::PI/180.0f)-p1.Y*sinf(Rotation*Gorgon::PI/180.0f),
            p1.X*sinf(Rotation*Gorgon::PI/180.0f)+p1.Y*cosf(Rotation*Gorgon::PI/180.0f)
        );
        p1=p1+Offset;
        return p1;
    }
    
    
    Gorgon::Geometry::Transform3D Camera::GetMatrix() {
        Gorgon::Geometry::Transform3D transform;
        //move point to offset
        transform.Translate({Offset.X,Offset.Y,0.0f});
        
        //scale 
        transform.Scale(Zoom,Zoom);
        
        //rotate around origin
        transform.Rotate({0.0f,0.0f,1.0f},Rotation*Gorgon::PI/180.0f);
        
        //move target to origin
        transform.Translate({-Target.X,-Target.Y,0.0f});
        
        // Transform is multiplied right to left 
        // if you look bottom to up; 
        //  we re moving our vertices to origin
        //  rotating the vertices around the origin
        //  scaling the vertices around the origin
        //  moving out vertices to offset point
        
        // so we are just looking our world point (target) from screen point(offset)  
        return transform;
    }
};