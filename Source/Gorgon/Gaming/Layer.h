#pragma once

#include"../Graphics/Layer.h"
#include"../Camera.h"
#include"../Graphics/Shaders.h"
namespace Gorgon::Gaming{

    class Layer : public Gorgon::Graphics::Layer {
        public:
        /// Initializing constructor
        Layer(const Geometry::Bounds &bounds) : Gorgon::Graphics::Layer(bounds)
        { }

        /// Constructor that sets the layer to cover entire parent, no matter how big it is. The
        /// location of the layer is set to be the origin
        Layer() : Gorgon::Graphics::Layer() { }

        /// Constructor that places the layer to the given location
        Layer(const Geometry::Point &location) :
        Gorgon::Graphics::Layer(location)
        { }
        
        /// Copy constructor is disabled.
        Layer(const Layer&) = delete;
        
        /// Move constructor
        Layer(Layer &&other) {
            Swap(other);
        }
        
        void Swap(Layer &other) {
            Graphics::Layer::Swap(other);
        }
        using TextureTarget::Draw;

        /// Prefer using Draw functions of image or animations
        virtual void Draw(const Graphics::TextureSource &image, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
                        const Geometry::Pointf &p3, const Geometry::Pointf &p4, Graphics::RGBAf color = Graphics::RGBAf(1.f)) override {
            Graphics::Layer::Draw(image, p1, p2, p3, p4,color);
        }

        /// Prefer using Draw functions of image or animations
        virtual void Draw(const Geometry::Pointf &p1, const Geometry::Pointf &p2,
                        const Geometry::Pointf &p3, const Geometry::Pointf &p4, Gorgon::Graphics::RGBAf color = Gorgon::Graphics::RGBAf(1.f)) override {
            Graphics::Layer::Draw(p1, p2, p3, p4,color);
        }

        /// Prefer using Draw functions of image or animations
        virtual void Draw(
            const Gorgon::Graphics::TextureSource &image, 
            const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
            const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
            const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
            const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, Gorgon::Graphics::RGBAf color = Gorgon::Graphics::RGBAf(1.f)) override {

            Gorgon::Graphics::Layer::Draw(image, p1, p2, p3, p4, tex1, tex2, tex3, tex4, color);
        }

        /// Prefer using Draw functions of image or animations
        virtual void Draw(const Gorgon::Graphics::TextureSource &image, Gorgon::Graphics::Tiling tiling, const Geometry::Rectanglef &location, Gorgon::Graphics::RGBAf color = Gorgon::Graphics::RGBAf(1.f)) override{
            Graphics::Layer::Draw(image,tiling,location,color);    
        }

        virtual void Clear() override {
            Graphics::Layer::Clear();
        }

        /// Render this layer to the GL. This function is used internally and not necessary to be called
        virtual void Render() override;


        ///Get current drawing mode. See Layer page to see available drawing modes
        virtual DrawMode GetDrawMode() const override { return Graphics::Layer::GetDrawMode(); }

        /// Change current drawing mode. See Layer page to see available drawing modes
        virtual void SetDrawMode(DrawMode mode) override { Graphics::Layer::SetDrawMode(mode); }

        /// Queues the start of a new mask. Only one mask buffer exists and it will be cleared and reused.
        virtual void NewMask() override {Graphics::Layer::NewMask();}

        /// Changes the tint color of the layer, every image pixel will be multiplied by this color
        virtual void SetTintColor(Graphics::RGBAf value) { this->tint=value; }

        /// Returns the tint color of the layer, every image pixel will be multiplied by this color
        virtual Graphics::RGBAf GetTintColor() const { return tint; }

        /// Changes the tint color of the layer, every image pixel will be multiplied by this color.
        /// This value effects only the images drawn after it is set.
        virtual void SetColor(Graphics::RGBAf value) { Graphics::Layer::SetColor(value); }

        /// Changes the tint color of the layer, every image pixel will be multiplied by this color
        virtual Graphics::RGBAf GetColor() const { return color; }

        virtual Geometry::Size GetTargetSize() const override {
            return Graphics::Layer::GetTargetSize();
        }

        /// Enables graphics clipping from the visible borders of the layer
        void EnableClipping() {
            Graphics::Layer::EnableClipping();
        }

        /// Disables graphics clipping
        void DisableClipping() {
            Graphics::Layer::DisableClipping();
        }

        /// Returns if the clipping is enabled
        bool IsClippingEnabled() const {
            return Graphics::Layer::IsClippingEnabled();
        }
        using Gorgon::Layer::GetSize;

        Camera& GetCamera(){return camera;}
                
        protected:
            Camera camera{};
    };
};