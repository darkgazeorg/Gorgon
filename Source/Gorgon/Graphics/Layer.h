#pragma once

#include "../Layer.h"
#include "TextureTargets.h"
#include "../Geometry/Point3D.h"
#include "../Geometry/Transform3D.h"
#include "../Geometry/Rectangle.h"
#include "../GL/Simple.h"
#include "../GL/FrameBuffer.h"

namespace Gorgon :: Graphics {

    /// @cond INTERNAL
    namespace internal {

        /// This class represents a drawable surface.
        class Surface {
        public:
            Surface(const Surface &) = delete;

            Surface(Surface &&s) {
                vertices = s.vertices;
                texture  = s.texture;
                source   = s.source;
                color    = s.color;
                drawmode = s.drawmode;

                s.texture = nullptr;
                s.source  = nullptr;
            }

            Surface &operator =(const Surface &) = delete;

            Surface &operator =(Surface &&s) {
                vertices = s.vertices;
                texture  = s.texture;
                source   = s.source;
                color    = s.color;
                drawmode = s.drawmode;

                s.texture = nullptr;
                s.source  = nullptr;

                return *this;
            }

            /// Sets the source to the given source. This variant uses texture coordinates given by the source.
            Surface(const TextureSource &source, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
                    const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color, TextureTarget::DrawMode drawmode) : 
                source(&source), color(color), drawmode(drawmode) {

                vertices[0]  = p1;
                vertices[1]  = p2;
                vertices[2]  = p3;
                vertices[3]  = p4;
            }

            /// Uses fill shader to fill an area with solid color.
            Surface(const Geometry::Pointf &p1, const Geometry::Pointf &p2,
                    const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
                    RGBAf color, TextureTarget::DrawMode drawmode) : color(color), drawmode(drawmode) {
                vertices[0]  = p1;
                vertices[1]  = p2;
                vertices[2]  = p3;
                vertices[3]  = p4;
            }

            /// Sets the source to the given source. This variant uses supplied texture coordinates.
            Surface(const TextureSource &source, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
                const Geometry::Pointf &p3, const Geometry::Pointf &p4,
                const Geometry::Pointf &t1, const Geometry::Pointf &t2,
                const Geometry::Pointf &t3, const Geometry::Pointf &t4, 
                RGBAf color, TextureTarget::DrawMode drawmode) : source(&source), color(color), drawmode(drawmode) {

                texture = new Geometry::Pointf[4];

                texture [0]  = t1;
                texture [1]  = t2;
                texture [2]  = t3;
                texture [3]  = t4;
                
                vertices[0]  = p1;
                vertices[1]  = p2;
                vertices[2]  = p3;
                vertices[3]  = p4;
            }

            /// If the source is set.
            bool IsSet() {
                return source != nullptr;
            }

            /// Returns the GL texture to be drawn
            GL::Texture TextureID() const {
                return source->GetID();
            }

            /// Returns the transformed vertex coordinates
            GL::QuadVertices GetVertices(const Geometry::Transform3D &transform) {
                return transform*vertices;
            }

            /// Returns the texture coordinates of this surface
            GL::QuadTextureCoords GetTextureCoords() {
                if(texture)
                    return {texture[0], texture[1], texture[2], texture[3]};
                else {
                    auto texture=source->GetCoordinates();
                    return {texture[0], texture[1], texture[2], texture[3]};
                }
            }

            bool IsPartial() {
                return source->IsPartial();
            }
            
            /// Returns the color mode of the texture
            ColorMode GetMode() const {
                return source->GetMode();
            }
            
            /// Returns the tint color for this surface
            RGBAf GetColor() const {
                return color;
            }

            TextureTarget::DrawMode GetDrawMode() const {
                return drawmode;
            }

            void SetDrawMode(TextureTarget::DrawMode value) {
                drawmode = value;
            }

            ~Surface() {
                if(texture) delete[] texture;
                texture      = nullptr;
            }

        private:

            GL::QuadVertices vertices;
            Geometry::Pointf *texture = nullptr;

            const TextureSource *source = nullptr;
            RGBAf color;
            
            TextureTarget::DrawMode drawmode = TextureTarget::Normal;
        };
    }
    /// @endcond

    /**
    * This layer allows drawing texture images on. Any graphics that are drawn over a layer stays there unless
    * Clear function is called. This allows static layers to be drawn only once. Layers stack, and child layers
    * are drawn on top of parent. Clearing parent layer will not clear child layers.
    * 
    * It is possible to draw surfaces without a texture to this layer. 
    * Graphics clipping is optional and disabled by default. This means any texture falling outside the layer will
    * still be visible. Use EnableClipping to change this behavior. Additionally, this layer allows tinting the 
    * textures given to it. Use SetTintColor to change the tinting color. Tint color is multiplicatively effective
    * to the underlying layers. This means if you set tint color of a layer, you don't need to set it separately for
    * its children. It is better to use Draw functions of the images or animations instead of Draw functions of the 
    * layer. 
    *
    * #### Drawing modes
    * Currently Normal, and FrameBuffer modes are supported.
    */
    class Layer : public Gorgon::Layer, public Graphics::TextureTarget {
        struct Operation {
            enum {
                NewMask
            } type;

            std::size_t index;
        };
        friend void Initialize();
    public:
        
        /// Initializing constructor
        Layer(const Geometry::Bounds &bounds) : Gorgon::Layer(bounds)
        { }

        /// Constructor that sets the layer to cover entire parent, no matter how big it is. The
        /// location of the layer is set to be the origin
        Layer() : Gorgon::Layer() { }

        /// Constructor that places the layer to the given location
        Layer(const Geometry::Point &location) :
        Gorgon::Layer(location)
        { }
        
        /// Copy constructor is disabled.
        Layer(const Layer&) = delete;
        
        /// Move constructor
        Layer(Layer &&other) {
            Swap(other);
        }
        
        void Swap(Layer &other) {
            using std::swap;
            
            swap(bounds, other.bounds);
            swap(isvisible, other.isvisible);
            swap(children, other.children);
            swap(color, other.color);
            swap(tint, other.tint);
            swap(mode, other.mode);
            swap(surfaces, other.surfaces);
            
            if(parent==other.parent) return;
            
            auto myparent = parent;
            auto otherparent = other.parent;
            
            int myind = -1;
            int otherind = -1;
            
            if(myparent) {
                myind = myparent->Children.FindLocation(this);
            }
            
            if(otherparent) {
                otherind = otherparent->Children.FindLocation(other);
            }
            
            if(myparent) {
                myparent->Remove(this);
                myparent->Insert(other, myind);
            }
            
            if(otherparent) {
                otherparent->Remove(other);
                otherparent->Insert(this, otherind);
            }
        }

        using TextureTarget::Draw;

        /// Prefer using Draw functions of image or animations
        virtual void Draw(const TextureSource &image, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
                        const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color = RGBAf(1.f)) override {

            surfaces.emplace_back(image, p1, p2, p3, p4, color*this->color, mode);
        }

        /// Prefer using Draw functions of image or animations
        virtual void Draw(const Geometry::Pointf &p1, const Geometry::Pointf &p2,
                        const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color = RGBAf(1.f)) override {

            surfaces.emplace_back(p1, p2, p3, p4, color*this->color, mode);
        }

        /// Prefer using Draw functions of image or animations
        virtual void Draw(
            const TextureSource &image, 
            const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
            const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
            const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
            const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color = RGBAf(1.f)) override {

            surfaces.emplace_back(image, p1, p2, p3, p4, tex1, tex2, tex3, tex4, color*this->color, mode);
        }

        /// Prefer using Draw functions of image or animations
        virtual void Draw(const TextureSource &image, Tiling tiling, const Geometry::Rectanglef &location, RGBAf color = RGBAf(1.f)) override;

        virtual void Clear() override {
            surfaces.clear();
        }

        /// Render this layer to the GL. This function is used internally and not necessary to be called
        virtual void Render() override;

        ///Get current drawing mode. See Layer page to see available drawing modes
        virtual DrawMode GetDrawMode() const override { return mode; }

        /// Change current drawing mode. See Layer page to see available drawing modes
        virtual void SetDrawMode(DrawMode mode) override { this->mode=mode; }

        /// Queues the start of a new mask. Only one mask buffer exists and it will be cleared and reused.
        virtual void NewMask() override {
            Operation op = {Operation::NewMask, surfaces.size()+operations.size()};
            operations.push_back(op);
        }

        /// Changes the tint color of the layer, every image pixel will be multiplied by this color
        virtual void SetTintColor(RGBAf value) { tint = value; }

        /// Returns the tint color of the layer, every image pixel will be multiplied by this color
        virtual RGBAf GetTintColor() const { return tint; }

        /// Changes the tint color of the layer, every image pixel will be multiplied by this color.
        /// This value effects only the images drawn after it is set.
        virtual void SetColor(RGBAf value) { color = value; }

        /// Changes the tint color of the layer, every image pixel will be multiplied by this color
        virtual RGBAf GetColor() const { return color; }

        virtual Geometry::Size GetTargetSize() const override {
            if(bounds.Width() != 0 && bounds.Height() != 0)
                return bounds.GetSize();
            else
                return Gorgon::Layer::GetEffectiveBounds().GetSize(); 
        }

        /// Enables graphics clipping from the visible borders of the layer
        void EnableClipping() {
            clippingenabled = true;
        }

        /// Disables graphics clipping
        void DisableClipping() {
            clippingenabled = false;
        }

        /// Returns if the clipping is enabled
        bool IsClippingEnabled() const {
            return clippingenabled;
        }

        using Gorgon::Layer::GetSize;

    private:
        std::vector<internal::Surface> surfaces;
        std::vector<Operation> operations;

        bool clippingenabled = false;
        static Geometry::Rectangle cliprectangle;

        DrawMode mode = Graphics::TextureTarget::Normal;
        RGBAf tint = RGBAf(1.f);
        RGBAf color = RGBAf(1.f);

        static GL::FrameBuffer mask;
    };
    

}
