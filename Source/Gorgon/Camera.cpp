#include"Camera.h"
namespace Gorgon{

    Geometry::Pointf Camera::GetScreenToWorld(const Geometry::Pointf &p){
        auto p1=p-offset;
        p1=p1/zoom;
        p1=Gorgon::Geometry::Pointf(  
            p1.X*cosf(-rotation*Gorgon::PI/180.0f)-p1.Y*sinf(-rotation*Gorgon::PI/180.0f),
            p1.X*sinf(-rotation*Gorgon::PI/180.0f)+p1.Y*cosf(-rotation*Gorgon::PI/180.0f)
        );
        p1=p1+target;
        return p1;
    }
    Geometry::Pointf Camera::GetWorldToScreen(const Geometry::Pointf &p){
        auto p1=p-target;
        p1=p1*zoom;
        p1=Gorgon::Geometry::Pointf(  
            p1.X*cosf(rotation*Gorgon::PI/180.0f)-p1.Y*sinf(rotation*Gorgon::PI/180.0f),
            p1.X*sinf(rotation*Gorgon::PI/180.0f)+p1.Y*cosf(rotation*Gorgon::PI/180.0f)
        );
        p1=p1+offset;
        return p1;
    }
    
    
    Gorgon::Geometry::Transform3D Camera::GetMatrix() {
        Gorgon::Geometry::Transform3D transform;
        //move point to offset
        transform.Translate({offset.X,offset.Y,0.0f});
        
        //scale 
        transform.Scale(zoom,zoom);
        
        //rotate around origin
        transform.Rotate({0.0f,0.0f,1.0f},rotation*Gorgon::PI/180.0f);
        
        //move target to origin
        transform.Translate({-target.X,-target.Y,0.0f});
        
        // Transform is multiplied right to left 
        // if you look bottom to up; 
        //  we re moving our vertices to origin
        //  rotating the vertices around the origin
        //  scaling the vertices around the origin
        //  moving out vertices to offset point
        
        // so we are just looking our world point (target) from screen point(offset)  
        return transform;
    }

    void Camera::SetRotation( float angles ){rotation=angles;}
    float Camera::GetRotation() const {return rotation;} 
    void Camera::SetZoomRate( float rate ){zoom=rate;}
    float Camera::GetZoomRate() const {return zoom;} 
    void Camera::SetOffset(const Gorgon::Geometry::Pointf& offset){this->offset=offset;}
    Gorgon::Geometry::Pointf Camera::GetOffset() const {return offset;}
    void Camera::SetTarget(const Gorgon::Geometry::Pointf& target){this->target=target;}
    Gorgon::Geometry::Pointf Camera::GetTarget() const {return target;}

};