#pragma once

#include "EmptyImage.h"
#include "Animations.h"
#include "TextureAnimation.h"

namespace Gorgon :: Graphics {
    
    /// For ease of use in resource system
    class IMaskedObjectProvider : public RectangularAnimationProvider {
    public:
        virtual RectangularAnimation &CreateBase() const = 0;
        virtual RectangularAnimation &CreateMask() const = 0;
        
        virtual IMaskedObjectProvider &MoveOutProvider() override = 0;
    };
    
	template<class A_>
    class basic_MaskedObjectProvider;
    
	template<class A_>
    class basic_MaskedObject : public virtual RectangularAnimation {
    public:
		basic_MaskedObject(const basic_MaskedObjectProvider<A_> &parent, bool create = true);
        
		basic_MaskedObject(const basic_MaskedObjectProvider<A_> &parent, Gorgon::Animation::ControllerBase &timer);
        
        /// Creates a masked object from two animations, these animations should not have controllers attached to them.
        /// Any attached controllers will be replaced or removed. A non-const version cannot work as this object needs
        /// to control the controllers of these animations.
		basic_MaskedObject(RectangularAnimation &base, RectangularAnimation &mask, bool create = true) : 
            Gorgon::Animation::Base(create), base(base), mask(mask) 
        {
            if(this->HasController()) {
                base.SetController(this->GetController());
                mask.SetController(this->GetController());
            }
            else {
                base.RemoveController();
                mask.RemoveController();
            }
        }
        
        /// Creates a masked object from two animations, these animations should not have controllers attached to them
        /// Any attached controllers will be replaced or removed. A non-const version cannot work as this object needs
        /// to control the controllers of these animations.
		basic_MaskedObject(RectangularAnimation &base, RectangularAnimation &mask, Gorgon::Animation::ControllerBase &timer) : 
            Gorgon::Animation::Base(timer), base(base), mask(mask)
        {
            if(this->HasController()) {
                base.SetController(this->GetController());
                mask.SetController(this->GetController());
            }
            else {
                base.RemoveController();
                mask.RemoveController();
            }
        }
        
        ~basic_MaskedObject() {
            base.DeleteAnimation();
            mask.DeleteAnimation();
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
		
		using RectangularAnimation::draw;

		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
						  const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
						  const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const override;


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const override;


		virtual Geometry::Size getsize() const override;

	private:
        const basic_MaskedObjectProvider<A_> &parent;
		RectangularAnimation &base, &mask;
    };

    /**
     * This object creates a masked object from two graphics object. 
     */
	template<class A_>
    class basic_MaskedObjectProvider : public IMaskedObjectProvider {
    public:
		using AnimationType = typename A_::AnimationType;

		/// Empty constructor
		basic_MaskedObjectProvider() = default;

		/// Filling constructor
		basic_MaskedObjectProvider(A_ &base, A_ &mask) :
            base(&base), mask(&mask) 
        { }

		/// Filling constructor, nullptr is allowed but not recommended
		basic_MaskedObjectProvider(A_ *base, A_ *mask) :
            base(base), mask(mask) 
        { }

		basic_MaskedObjectProvider(A_ &&base, A_ &&mask) :
            base(new A_(std::move(base))), mask(new A_(std::move(mask))), own(true)
        { }
		
		basic_MaskedObjectProvider(basic_MaskedObjectProvider &&other) :
            base(other.base), mask(other.mask) 
        { 
            other.base = nullptr;
            other.mask = nullptr;
        }

		~basic_MaskedObjectProvider() {
			if(own) {
				delete this->base;
				delete this->mask;
			}
		}
 
        //types are derived not to type the same code for every class
		virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new typename std::remove_reference<decltype(*this)>::type(std::move(*this));
            
            return *ret;
        }
       
        basic_MaskedObject<A_> &CreateAnimation(bool create = true) const override {
            return *new basic_MaskedObject<A_>(*this, create);
        }
        
		basic_MaskedObject<A_> &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
            return *new basic_MaskedObject<A_>(*this, timer);
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
        
        /// Creates a mask animation without controller.
		RectangularAnimation &CreateMask() const override {
            if(mask) {
                return mask->CreateAnimation(false);
            }
            else {
                return EmptyImage::Instance();
            }
        }

		/// Returns the base component. Could return nullptr
		A_ *GetBase() const {
			return base;
		}

		/// Returns the mask component. Could return nullptr
		A_ *GetMask() const {
			return mask;
		}

		/// Sets the base provider, ownership semantics will not be changed
		void SetBase(A_ *value) {
			if(own)
				delete base;

			base = value;
		}

		/// Sets the mask provider, ownership semantics will not be changed
		void SetMask(A_ *value) {
			if(own)
				delete mask;

			mask = value;
		}

        /// Sets the providers in this object
        void SetProviders(A_ &base, A_ &mask) {
			if(own) {
				delete this->base;
				delete this->mask;
			}
            this->base = &base;
            this->mask = &mask;
        }

		/// Assumes the ownership of the providers
		void OwnProviders() {
			own = true;
		}
		
		/// Prepares the providers. Provider type should support this operation, otherwise
		/// this function will cause a compile time error.
		void Prepare() {
            if(base) base->Prepare();
            if(mask) mask->Prepare();
        }

		Geometry::Size GetSize() const override {
			return base ? base->GetSize() : Geometry::Size{0, 0};
		}

    private:
		A_ *base = nullptr;
		A_ *mask = nullptr;

		bool own = false;
    };

	template<class A_>
    basic_MaskedObject<A_>::basic_MaskedObject(const basic_MaskedObjectProvider<A_> &parent, bool create) :
        Gorgon::Animation::Base(create), parent(parent),
        base(parent.CreateBase()), mask(parent.CreateMask())
    {
        if(this->HasController()) {
            base.SetController(this->GetController());
            mask.SetController(this->GetController());
        }
    }
        
	template<class A_>
	basic_MaskedObject<A_>::basic_MaskedObject(const basic_MaskedObjectProvider<A_> &parent, Gorgon::Animation::ControllerBase &timer) :
        Gorgon::Animation::Base(timer), parent(parent),
        base(parent.CreateBase()), mask(parent.CreateMask())
    {
        if(this->HasController()) {
            base.SetController(this->GetController());
            mask.SetController(this->GetController());
        }
    }
        
	template<class A_>
	void basic_MaskedObject<A_>::drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const {
        auto prev = target.GetDrawMode();
			
		if(prev != target.ToMask)
			target.NewMask();

        target.SetDrawMode(target.ToMask);
        mask.DrawIn(target, r);
			
		if(prev != target.ToMask)
			target.SetDrawMode(target.UseMask);

        base.DrawIn(target, r, color);
        target.SetDrawMode(prev);
    }

	template<class A_>
	void basic_MaskedObject<A_>::drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const {
		auto prev = target.GetDrawMode();

		if(prev != target.ToMask)
			target.NewMask();

		target.SetDrawMode(target.ToMask);
		mask.DrawIn(target, controller, r);

		if(prev != target.ToMask)
			target.SetDrawMode(target.UseMask);

		base.DrawIn(target, controller, r, color);
		target.SetDrawMode(prev);
	}

        
        
	template<class A_>
	void basic_MaskedObject<A_>::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
							const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
							const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
							const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const 
	{
		auto prev = target.GetDrawMode();

		if(prev != target.ToMask)
			target.NewMask();

		target.SetDrawMode(target.ToMask);
		mask.Draw(target, p1, p2, p3, p4, tex1, tex2, tex3, tex4);

		if(prev != target.ToMask)
			target.SetDrawMode(target.UseMask);

		base.Draw(target, p1, p2, p3, p4, tex1, tex2, tex3, tex4, color);
		target.SetDrawMode(prev);
	}


	template<class A_>
	void basic_MaskedObject<A_>::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const {
		auto prev = target.GetDrawMode();

		if(prev != target.ToMask)
			target.NewMask();

		target.SetDrawMode(target.ToMask);
		mask.Draw(target, p1, p2, p3, p4);

		if(prev != target.ToMask)
			target.SetDrawMode(target.UseMask);

		base.Draw(target, p1, p2, p3, p4, color);
		target.SetDrawMode(prev);
	}


	template<class A_>
	Geometry::Size basic_MaskedObject<A_>::getsize() const {
		return base.GetSize();
	}

	using MaskedObject = basic_MaskedObject<RectangularAnimationProvider>;
	using MaskedObjectProvider = basic_MaskedObjectProvider<RectangularAnimationProvider>;

	using MaskedBitmapAnimation = basic_MaskedObject<BitmapAnimationProvider>;
	using MaskedBitmapAnimationProvider = basic_MaskedObjectProvider<BitmapAnimationProvider>;

	using MaskedBitmap = basic_MaskedObject<Bitmap>;
	using MaskedBitmapProvider = basic_MaskedObjectProvider<Bitmap>;

}
