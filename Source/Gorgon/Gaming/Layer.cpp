#include"Layer.h"
#include "../Graphics.h"
#include "../GL.h"
#include "Gorgon/Utils/Assert.h"
#include "../Graphics/Shaders.h"
namespace Gorgon{
    extern Graphics::RGBAf LayerColor;
    extern Geometry::Size ScreenSize;
    namespace Gaming{

        void Layer::Render(){
            using namespace Gorgon::Graphics::internal;
            using namespace Gorgon::Graphics;
            
            if(!isvisible) return;
            
            auto prev_col = LayerColor;
            LayerColor *= tint;
            
            dotransformandclip();
            
            Gorgon::Geometry::Transform3D layerTransform=Transform;
            Transform*=camera.GetMatrix();
            
            bool isclipset = false;
            
            Geometry::Rectangle prevclip;
            
            if(clippingenabled) {
                isclipset = GL::IsClipEnabled();
                GL::SetClipEnabled(true);
                
                prevclip = cliprectangle;
                
                cliprectangle = {Clip.Left, (Gorgon::ScreenSize.Height-Clip.Top)-Clip.Height(), Clip.Width(), Clip.Height()};
                GL::SetClipRectangle(cliprectangle);
            }
            
            ActivateQuadVertices();
            
            int ind = 0;
            auto nextop = operations.begin();
            int nextopind = -1;
            if(nextop != operations.end())
            nextopind = (int)nextop->index;
            
            DrawMode current = Normal;
            
            for(auto &surface : surfaces) {            
                while(ind == nextopind) {
                    switch(nextop->type) {
                        case Operation::NewMask:
                        mask.Use();
                        GL::SetClearColor(1.0f, 0, 0, 0);
                        GL::Clear();
                        GL::SetDefaultClear();
                        mask.RenderToScreen();
                        break;
                    }
                    
                    ++nextop;
                    if(nextop != operations.end())
                    nextopind = (int)nextop->index;
                    else
                    nextopind = -1;
                    
                    ind++;
                }
                
                if(surface.GetDrawMode() == ToMask && current != ToMask) {
                    GL::SetMaskBlending();
                    mask.Use();
                }
                
                if(surface.GetDrawMode() != ToMask && current == ToMask) {
                    GL::SetDefaultBlending();
                    mask.RenderToScreen();
                }
                
                current = surface.GetDrawMode();
                
                if(!surface.IsSet()) {
                    if(surface.GetDrawMode() == UseMask) {
                        MaskedFillShader::Use()
                        .SetTint(surface.GetColor()*LayerColor)
                        .SetVertexCoords(surface.GetVertices(Transform))
                        .SetMask(mask.GetTexture())
                        ;
                    }
                    else {
                        FillShader::Use(surface.GetDrawMode() == ToMask ? ShaderMode::ToMask : ShaderMode::Normal)
                        .SetTint(surface.GetColor()*LayerColor)
                        .SetVertexCoords(surface.GetVertices(Transform))
                        ;
                    }
                }
                else {
                    auto tex=surface.GetTextureCoords();
                    
                    if(surface.GetDrawMode()==FrameBuffer) {
                        tex.FlipY();
                    }
                    
                    if(surface.GetMode() == ColorMode::Alpha) {
                        if(surface.GetDrawMode() == UseMask) {
                            MaskedAlphaShader::Use()
                            .SetTint(surface.GetColor()*LayerColor)
                            .SetAlpha(surface.TextureID())
                            .SetVertexCoords(surface.GetVertices(Transform))
                            .SetTextureCoords(tex)
                            .SetMask(mask.GetTexture())
                            ;
                        }
                        else {
                            AlphaShader::Use(surface.GetDrawMode() == ToMask ? ShaderMode::ToMask : ShaderMode::Normal)
                            .SetTint(surface.GetColor()*LayerColor)
                            .SetAlpha(surface.TextureID())
                            .SetVertexCoords(surface.GetVertices(Transform))
                            .SetTextureCoords(tex)
                            ;
                        }
                    }
                    else {
                        if(surface.GetDrawMode() == UseMask) {
                            MaskedShader::Use()
                            .SetTint(surface.GetColor()*LayerColor)
                            .SetDiffuse(surface.TextureID())
                            .SetVertexCoords(surface.GetVertices(Transform))
                            .SetTextureCoords(tex)
                            .SetMask(mask.GetTexture())
                            ;
                        }
                        else {
                            SimpleShader::Use(surface.GetDrawMode() == ToMask ? ShaderMode::ToMask : ShaderMode::Normal)
                            .SetTint(surface.GetColor()*LayerColor)
                            .SetDiffuse(surface.TextureID())
                            .SetVertexCoords(surface.GetVertices(Transform))
                            .SetTextureCoords(tex)
                            ;
                        }
                    }
                }
                
                DrawQuadVertices();
                ind++;
            }
            
            if(current == ToMask) {
                GL::SetDefaultBlending();
                mask.RenderToScreen();
            }
            
            Transform=layerTransform;

            
            for(auto &l : children) {
                l.Render();
            }
            
            if(clippingenabled) {
                if(!isclipset)
                GL::SetClipEnabled(false);
                else {
                    cliprectangle = prevclip;
                    GL::SetClipRectangle(cliprectangle);
                }
            }
            
            reverttransformandclip();
            LayerColor = prev_col;
        }
        
        
    };
};