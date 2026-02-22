#include "Layer.h"
#include "../Graphics.h"
#include "../GL.h"
#include "Gorgon/Utils/Assert.h"
#include "Shaders.h"

namespace Gorgon { 

    extern Graphics::RGBAf LayerColor;
    extern Geometry::Size ScreenSize;

namespace Graphics {

    void Layer::Draw(const TextureSource &image, Tiling tiling, const Geometry::Rectanglef &location, RGBAf color) {
        if(tiling==Tiling::None) {
            Draw(image, location.TopLeft(), location.TopRight(), location.BottomRight(), location.BottomLeft(), color);
        }
        else if(!image.IsPartial()) {			
            if(tiling == Tiling::Both) {
                auto texc = image.GetCoordinates();
                auto sz   = image.GetImageSize();

                Geometry::Rectanglef r ={texc[0], texc[2]};
                auto wr = location.Width / sz.Width;
                auto hr = location.Height / sz.Height;

                Draw(image,
                    location.TopLeft(), location.TopRight(), location.BottomRight(), location.BottomLeft(),
                    r.TopLeft(), {r.X + r.Width*wr, r.Y}, {r.X + r.Width*wr, r.Y + r.Height*hr}, {r.X, r.Y + r.Height*hr},
                    color);
            }
            else if(tiling == Tiling::Horizontal) {
                auto texc = image.GetCoordinates();
                auto sz   = image.GetImageSize();

                Geometry::Rectanglef r ={texc[0], texc[2]};
                auto wr = location.Width / sz.Width;

                Draw(image,
                    location.TopLeft(), location.TopRight(), location.BottomRight(), location.BottomLeft(),
                    r.TopLeft(), {r.X + r.Width*wr, r.Y}, {r.X + r.Width*wr, r.Bottom()}, r.BottomLeft(),
                    color);
            }
            else {
                auto texc = image.GetCoordinates();
                auto sz   = image.GetImageSize();

                Geometry::Rectanglef r ={texc[0], texc[2]};
                auto hr = location.Height / sz.Height;

                Draw(image,
                    location.TopLeft(), location.TopRight(), location.BottomRight(), location.BottomLeft(),
                    r.TopLeft(), r.TopRight(), {r.Right(), r.Y + r.Height*hr}, {r.X, r.Y + r.Height*hr},
                    color);
            }
        }
        else {
            Utils::NotImplemented("Tiled atlas rendering");
        }
    }

    void Layer::Render() {
        using namespace internal;

        if(!isvisible) return;

        auto prev_col = LayerColor;
        LayerColor *= tint;

        dotransformandclip();

        bool isclipset = false;
        
        Geometry::Rectangle prevclip;

        if(clippingenabled) {
            isclipset = glIsEnabled(GL_SCISSOR_TEST);
            glEnable(GL_SCISSOR_TEST);
            
            prevclip = cliprectangle;
            
            cliprectangle = {Clip.Left, (ScreenSize.Height-Clip.Top)-Clip.Height(), Clip.Width(), Clip.Height()};
            glScissor(cliprectangle.X, cliprectangle.Y, cliprectangle.Width, cliprectangle.Height);
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
                    glClearColor(1.0f, 0, 0, 0);
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
                glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ONE, GL_ZERO);
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

        for(auto &l : children) {
            l.Render();
        }

        if(clippingenabled) {
            if(!isclipset)
                glDisable(GL_SCISSOR_TEST);
            else {
                cliprectangle = prevclip;
                glScissor(cliprectangle.X, cliprectangle.Y, cliprectangle.Width, cliprectangle.Height);
            }
        }

        reverttransformandclip();
        LayerColor = prev_col;
    }

    GL::FrameBuffer Layer::mask;
    Geometry::Rectangle Layer::cliprectangle;

} }
