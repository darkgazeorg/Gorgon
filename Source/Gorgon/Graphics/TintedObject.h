#pragma once

#include "../Graphics.h"
#include "EmptyImage.h"
#include "Animations.h"
#include "TextureAnimation.h"

namespace Gorgon :: Graphics {
    
    /// For ease of use in resource system
    class ITintedObjectProvider : public RectangularAnimationProvider {
    public:
        virtual RectangularAnimation &CreateBase() const = 0;
        
        virtual RGBAf GetColor() const = 0;
        
        virtual void SetColor(const RGBAf &value) = 0;
        
        virtual ITintedObjectProvider &MoveOutProvider() override = 0;
    };
    
	template<class A_>
    class basic_TintedObjectProvider;
    
	template<class A_>
    class basic_TintedObject : public virtual RectangularAnimation {
    public:
		basic_TintedObject(const basic_TintedObjectProvider<A_> &parent, bool create = true);
        
		basic_TintedObject(const basic_TintedObjectProvider<A_> &parent, Gorgon::Animation::ControllerBase &timer);
        
        /// Creates a scalable object from two animations, these animations should not have controllers attached to them.
        /// Any attached controllers will be replaced or removed. A non-const version cannot work as this object needs
        /// to control the controllers of these animations.
		basic_TintedObject(RectangularAnimation &base, const RGBAf &color, bool create = true) : 
            Gorgon::Animation::Base(create), base(base), color(color) 
        {
            if(this->HasController()) {
                base.SetController(this->GetColor());
            }
            else {
                base.RemoveController();
            }
        }
        
        /// Creates a scalable object from two animations, these animations should not have controllers attached to them
        /// Any attached controllers will be replaced or removed. A non-const version cannot work as this object needs
        /// to control the controllers of these animations.
		basic_TintedObject(RectangularAnimation &base, const RGBAf &color, Gorgon::Animation::ControllerBase &timer) : 
            Gorgon::Animation::Base(timer), base(base), color(color) 
        {
            if(this->HasController()) {
                base.SetController(this->GetColor());
            }
            else {
                base.RemoveController();
            }
        }
        
        ~basic_TintedObject() {
            base.DeleteAnimation();
        }
        
        /// Changes the size controller used in this scalable object
        void SetColor(RGBAf value) {
            color = value;
        }
        
        /// Returns the size controller used in this scalable object
        RGBAf GetColor() const {
            return color;
        }
        
		virtual bool Progress(unsigned &) override {
			return true; //individual parts will work automatically
		}
        		
		int GetDuration() const override {
            return base.GetDuration();
        }
        
    protected:
		virtual void drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const override;

		virtual void drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const override;

		virtual Geometry::Size calculatesize(const Geometry::Size &area) const override {
            return base.CalculateSize(area);
		}

		virtual Geometry::Size calculatesize(const SizeController &controller, const Geometry::Size &s) const override {
            return base.CalculateSize(controller, s);
		}        
		
		using Drawable::draw;

		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
						  const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
						  const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const override;


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const override;

                          
        

		virtual Geometry::Size getsize() const override;

	private:
		RectangularAnimation &base;
        RGBAf color;
    };

    /**
     * This object creates a scalable object from a graphic object. A scalable object is controlled by a size controller
     * which may dictate the image to be drawn at original size, stretched or tiled. It also allows finer control for tiling.
     * Size control is only active if the image is drawn using DrawIn without specifying a size controller. If a size controller
     * is specified, it will be used instead.
     * @see RGBAf
     */
	template<class A_>
    class basic_TintedObjectProvider : public ITintedObjectProvider {
    public:
		using AnimationType = typename A_::AnimationType;

		/// Empty constructor
		basic_TintedObjectProvider() = default;

		/// Filling constructor
		explicit basic_TintedObjectProvider(A_ &base, const RGBAf &color = 1.f) :
            base(&base), color(color) 
        { }

		explicit basic_TintedObjectProvider(A_ &&base, const RGBAf &color = 1.f) :
            base(new A_(std::move(base))), color(color), own(true)
        { }
		
		basic_TintedObjectProvider(basic_TintedObjectProvider &&other) :
            base(other.base), color(other.color) 
        { 
            other.base = nullptr;
        }

		~basic_TintedObjectProvider() {
			if(own) {
				delete this->base;
			}
		}
 
        //types are derived not to type the same code for every class
		virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new typename std::remove_reference<decltype(*this)>::type(std::move(*this));
            
            return *ret;
        }
       
        basic_TintedObject<A_> &CreateAnimation(bool create = true) const override {
            return *new basic_TintedObject<A_>(*this, create);
        }
        
		basic_TintedObject<A_> &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
            return *new basic_TintedObject<A_>(*this, timer);
        }
        
        /// Creates a base animation without controller.
		RectangularAnimation &CreateBase() const override {
            if(base) {
                return base->CreateAnimation(false);
            }
            else {
                return EmptyImage::Instance();
            }
        }
        
		/// Returns the base component. Could return nullptr
		A_ *GetBase() const {
			return base;
		}

        /// Returns the tint color
        RGBAf GetColor() const override {
            return color;
        }

		/// Sets the tint color of the object
		void SetColor(const RGBAf &value) override {
			color = value;
		}

		/// Sets the base provider, ownership semantics will not be changed
		void SetBase(A_ *value) {
			if(own)
				delete base;

			base = value;
		}

		/// Assumes the ownership of the providers
		void OwnProvider() {
			own = true;
		}
		
		/// Prepares the providers. Provider type should support this operation, otherwise
		/// this function will cause a compile time error.
		void Prepare() {
            if(base) base->Prepare();
        }

		Geometry::Size GetSize() const override {
			return base ? base->GetSize() : Geometry::Size{0, 0};
		}
        
    private:
		A_ *base = nullptr;
		RGBAf color = 1.0f;

		bool own = false;
    };

	template<class A_>
    basic_TintedObject<A_>::basic_TintedObject(const basic_TintedObjectProvider<A_> &parent, bool create) :
        Gorgon::Animation::Base(create),
        base(parent.CreateBase()), color(parent.GetColor())
    {
        if(this->HasController()) {
            base.SetController(this->GetController());
        }
    }
        
	template<class A_>
	basic_TintedObject<A_>::basic_TintedObject(const basic_TintedObjectProvider<A_> &parent, Gorgon::Animation::ControllerBase &timer) :
        Gorgon::Animation::Base(timer), 
        base(parent.CreateBase()), color(parent.GetColor())
    {
        if(this->HasController()) {
            base.SetController(this->GetController());
        }
    }
        
	template<class A_>
	void basic_TintedObject<A_>::drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const {
        base.DrawIn(target, r, this->color*color);
    }

	template<class A_>
	void basic_TintedObject<A_>::drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const {
		base.DrawIn(target, controller, r, this->color*color);
	}

        
        
	template<class A_>
	void basic_TintedObject<A_>::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
							const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
							const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
							const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const 
	{
		base.Draw(target, p1, p2, p3, p4, tex1, tex2, tex3, tex4, this->color*color);
	}


	template<class A_>
	void basic_TintedObject<A_>::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
                                        const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const 
    {
		base.Draw(target, p1, p2, p3, p4, this->color*color);
	}

	template<class A_>
	Geometry::Size basic_TintedObject<A_>::getsize() const {
		return base.GetSize();
	}

	using TintedObject = basic_TintedObject<RectangularAnimationProvider>;
	using TintedObjectProvider = basic_TintedObjectProvider<RectangularAnimationProvider>;

	using TintedBitmapAnimation = basic_TintedObject<BitmapAnimationProvider>;
	using TintedBitmapAnimationProvider = basic_TintedObjectProvider<BitmapAnimationProvider>;

	using TintedBitmap = basic_TintedObject<Bitmap>;
	using TintedBitmapProvider = basic_TintedObjectProvider<Bitmap>;

}
