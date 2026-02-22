#pragma once

#include "../Graphics.h"
#include "EmptyImage.h"
#include "Animations.h"
#include "TextureAnimation.h"

namespace Gorgon :: Graphics {
    
    /// For ease of use in resource system
    class IScalableObjectProvider : public RectangularAnimationProvider {
    public:
        virtual RectangularAnimation &CreateBase() const = 0;
        
        virtual SizeController GetController() const = 0;
        
        virtual void SetController(const SizeController &value) = 0;
        
        virtual IScalableObjectProvider &MoveOutProvider() override = 0;
    };
    
	template<class A_>
    class basic_ScalableObjectProvider;
    
	template<class A_>
    class basic_ScalableObject : public virtual RectangularAnimation {
    public:
		basic_ScalableObject(const basic_ScalableObjectProvider<A_> &parent, bool create = true);
        
		basic_ScalableObject(const basic_ScalableObjectProvider<A_> &parent, Gorgon::Animation::ControllerBase &timer);
        
        /// Creates a scalable object from two animations, these animations should not have controllers attached to them.
        /// Any attached controllers will be replaced or removed. A non-const version cannot work as this object needs
        /// to control the controllers of these animations.
		basic_ScalableObject(RectangularAnimation &base, const SizeController &ctrl, bool create = true) : 
            Gorgon::Animation::Base(create), base(base), ctrl(ctrl) 
        {
            if(this->HasController()) {
                base.SetController(this->GetController());
            }
            else {
                base.RemoveController();
            }
        }
        
        /// Creates a scalable object from two animations, these animations should not have controllers attached to them
        /// Any attached controllers will be replaced or removed. A non-const version cannot work as this object needs
        /// to control the controllers of these animations.
		basic_ScalableObject(RectangularAnimation &base, const SizeController &ctrl, Gorgon::Animation::ControllerBase &timer) : 
            Gorgon::Animation::Base(timer), base(base), ctrl(ctrl) 
        {
            if(this->HasController()) {
                base.SetController(this->GetController());
            }
            else {
                base.RemoveController();
            }
        }
        
        ~basic_ScalableObject() {
            base.DeleteAnimation();
        }
        
        /// Changes the size controller used in this scalable object
        void SetSizeController(SizeController value) {
            ctrl = value;
        }
        
        /// Returns the size controller used in this scalable object
        SizeController GetSizeController() const {
            return ctrl;
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

		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
						  const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
						  const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const override;


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const override;

                          
        

		virtual Geometry::Size getsize() const override;

	private:
		RectangularAnimation &base;
        SizeController ctrl;
    };

    /**
     * This object creates a scalable object from a graphic object. A scalable object is controlled by a size controller
     * which may dictate the image to be drawn at original size, stretched or tiled. It also allows finer control for tiling.
     * Size control is only active if the image is drawn using DrawIn without specifying a size controller. If a size controller
     * is specified, it will be used instead.
     * @see SizeController
     */
	template<class A_>
    class basic_ScalableObjectProvider : public IScalableObjectProvider {
    public:
		using AnimationType = typename A_::AnimationType;

		/// Empty constructor
		basic_ScalableObjectProvider() = default;

		/// Filling constructor
		explicit basic_ScalableObjectProvider(A_ &base, const SizeController &controller = Tiling::Both) :
            base(&base), ctrl(controller) 
        { }

		explicit basic_ScalableObjectProvider(A_ &&base, const SizeController &controller = Tiling::Both) :
            base(new A_(std::move(base))), ctrl(controller), own(true)
        { }
		
		basic_ScalableObjectProvider(basic_ScalableObjectProvider &&other) :
            base(other.base), ctrl(other.ctrl) 
        { 
            other.base = nullptr;
        }

		~basic_ScalableObjectProvider() {
			if(own) {
				delete this->base;
			}
		}
 
        //types are derived not to type the same code for every class
		virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new typename std::remove_reference<decltype(*this)>::type(std::move(*this));
            
            return *ret;
        }
       
        basic_ScalableObject<A_> &CreateAnimation(bool create = true) const override {
            return *new basic_ScalableObject<A_>(*this, create);
        }
        
		basic_ScalableObject<A_> &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
            return *new basic_ScalableObject<A_>(*this, timer);
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

        /// Returns the size controller
        SizeController GetController() const override {
            return ctrl;
        }

		/// Sets the controller
		void SetController(const SizeController &value) override {
			ctrl = value;
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
		SizeController ctrl = {Tiling::Both};

		bool own = false;
    };

	template<class A_>
    basic_ScalableObject<A_>::basic_ScalableObject(const basic_ScalableObjectProvider<A_> &parent, bool create) :
        Gorgon::Animation::Base(create),
        base(parent.CreateBase()), ctrl(parent.GetController())
    {
        if(this->HasController()) {
            base.SetController(this->GetController());
        }
    }
        
	template<class A_>
	basic_ScalableObject<A_>::basic_ScalableObject(const basic_ScalableObjectProvider<A_> &parent, Gorgon::Animation::ControllerBase &timer) :
        Gorgon::Animation::Base(timer), 
        base(parent.CreateBase()), ctrl(parent.GetController())
    {
        if(this->HasController()) {
            base.SetController(this->GetController());
        }
    }
        
	template<class A_>
	void basic_ScalableObject<A_>::drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const {
        base.DrawIn(target, ctrl, r, color);
    }

	template<class A_>
	void basic_ScalableObject<A_>::drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const {
		base.DrawIn(target, controller, r, color);
	}

        
        
	template<class A_>
	void basic_ScalableObject<A_>::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
							const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
							const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
							const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const 
	{
		base.Draw(target, p1, p2, p3, p4, tex1, tex2, tex3, tex4, color);
	}


	template<class A_>
	void basic_ScalableObject<A_>::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
                                        const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const 
    {
		base.Draw(target, p1, p2, p3, p4, color);
	}

	template<class A_>
	Geometry::Size basic_ScalableObject<A_>::getsize() const {
		return base.GetSize();
	}

	using ScalableObject = basic_ScalableObject<RectangularAnimationProvider>;
	using ScalableObjectProvider = basic_ScalableObjectProvider<RectangularAnimationProvider>;

	using ScalableBitmapAnimation = basic_ScalableObject<BitmapAnimationProvider>;
	using ScalableBitmapAnimationProvider = basic_ScalableObjectProvider<BitmapAnimationProvider>;

	using ScalableBitmap = basic_ScalableObject<Bitmap>;
	using ScalableBitmapProvider = basic_ScalableObjectProvider<Bitmap>;

}
