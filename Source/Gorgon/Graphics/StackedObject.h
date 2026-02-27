#pragma once

#include "EmptyImage.h"
#include "Animations.h"
#include "TextureAnimation.h"

namespace Gorgon :: Graphics {
    
    /// For ease of use in resource system
    class IStackedObjectProvider : public RectangularAnimationProvider {
    public:
        virtual RectangularAnimation &CreateTop() const = 0;
        virtual RectangularAnimation &CreateBottom() const = 0;
        
        virtual Geometry::Point GetOffset() const = 0;
        virtual void SetOffset(const Geometry::Point&) = 0;
        
        virtual IStackedObjectProvider &MoveOutProvider() override = 0;
    };
    
	template<class A_>
    class basic_StackedObjectProvider;
    
	template<class A_>
    class basic_StackedObject : public virtual RectangularAnimation {
    public:
		basic_StackedObject(const basic_StackedObjectProvider<A_> &parent, bool create = true);
        
		basic_StackedObject(const basic_StackedObjectProvider<A_> &parent, Gorgon::Animation::ControllerBase &timer);
        
        /// Creates a stacked object from two animations, these animations should not have controllers attached to them.
        /// Any attached controllers will be replaced or removed. A non-const version cannot work as this object needs
        /// to control the controllers of these animations.
		basic_StackedObject(RectangularAnimation &top, RectangularAnimation &bottom, const Geometry::Point &offset, bool create = true) : 
            Gorgon::Animation::Base(create), top(top), offset(offset), bottom(bottom) 
        {
            if(this->HasController()) {
                top.SetController(this->GetController());
                bottom.SetController(this->GetController());
            }
            else {
                top.RemoveController();
                bottom.RemoveController();
            }
        }
        
        /// Creates a bottomed object from two animations, these animations should not have controllers attached to them
        /// Any attached controllers will be replaced or removed. A non-const version cannot work as this object needs
        /// to control the controllers of these animations.
		basic_StackedObject(RectangularAnimation &top, RectangularAnimation &bottom, const Geometry::Point &offset, Gorgon::Animation::ControllerBase &timer) : 
            Gorgon::Animation::Base(timer), top(top), offset(offset), bottom(bottom)
        {
            if(this->HasController()) {
                top.SetController(this->GetController());
                bottom.SetController(this->GetController());
            }
            else {
                top.RemoveController();
                bottom.RemoveController();
            }
        }
        
        ~basic_StackedObject() {
            top.DeleteAnimation();
            bottom.DeleteAnimation();
        }

		virtual bool Progress(unsigned &) override {
			return true; //individual parts will work automatically
		}
        		
		int GetDuration() const override {
            auto dur = top.GetDuration();
            if(dur > 0)
                return dur;
            
            
            return bottom.GetDuration();
        }
        
    protected:
		virtual void drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const override;

		virtual void drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const override;

		virtual Geometry::Size calculatesize(const Geometry::Size &area) const override {
            return Union(top.CalculateSize(area), bottom.CalculateSize(area));
		}

		virtual Geometry::Size calculatesize(const SizeController &controller, const Geometry::Size &s) const override {
            return Union(top.CalculateSize(controller, s), bottom.CalculateSize(controller, s));
		}        

		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
						  const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
						  const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const override;


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const override;


		virtual Geometry::Size getsize() const override;

	private:
		RectangularAnimation &top, &bottom;
        Geometry::Point offset;
    };

    /**
     * This object creates an object that has two subobjects drawn on top of each other.
     * Top object can have an offset.
     */
	template<class A_>
    class basic_StackedObjectProvider : public IStackedObjectProvider {
    public:
		using AnimationType = typename A_::AnimationType;

		/// Empty constructor
		basic_StackedObjectProvider() = default;

		/// Filling constructor
		basic_StackedObjectProvider(A_ &bottom, A_ &top, const Geometry::Point &offset ={0,0}) :
			top(&top), bottom(&bottom), offset(offset) {}

		/// Filling constructor, nullptr is allowed but not recommended
		basic_StackedObjectProvider(A_ *bottom, A_ *top, const Geometry::Point &offset ={0,0}) :
			top(top), bottom(bottom), offset(offset) {}

		/// Filling constructor
		basic_StackedObjectProvider(const AssumeOwnershipTag &, A_ &bottom, A_ &top, const Geometry::Point &offset ={0,0}) :
			top(&top), bottom(&bottom), offset(offset) { own = true; }

		/// Filling constructor, nullptr is allowed but not recommended
		basic_StackedObjectProvider(const AssumeOwnershipTag &, A_ *bottom, A_ *top, const Geometry::Point &offset ={0,0}) :
			top(top), bottom(bottom), offset(offset) { own = true; }

		basic_StackedObjectProvider(A_ &&bottom, A_ &&top, const Geometry::Point &offset = {0,0}) :
            top(new A_(std::move(top))), bottom(new A_(std::move(bottom))), offset(offset), own(true)
        { }
		
		basic_StackedObjectProvider(basic_StackedObjectProvider &&other) :
            top(other.top), bottom(other.bottom), offset(other.offset)
        { 
            other.top = nullptr;
            other.bottom = nullptr;
            other.offset = {0, 0};
			other.own = false;
		}

		~basic_StackedObjectProvider() {
			if(own) {
				delete this->top;
				delete this->bottom;
			}
		}

		basic_StackedObjectProvider &operator =(basic_StackedObjectProvider &&other) {
			if(own) {
				delete this->top;
				delete this->bottom;
			}

			top = other.top;
			bottom = other.bottom;
			offset = other.offset;
			own = other.own;

			other.top = nullptr;
			other.bottom = nullptr;
			other.offset ={0, 0};
			other.own = false;

			return *this;
		}


        //types are derived not to type the same code for every class
		virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new typename std::remove_reference<decltype(*this)>::type(std::move(*this));
            
            return *ret;
        }
       
        basic_StackedObject<A_> &CreateAnimation(bool create = true) const override {
            return *new basic_StackedObject<A_>(*this, create);
        }
        
		basic_StackedObject<A_> &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
            return *new basic_StackedObject<A_>(*this, timer);
        }
        
        /// Creates a top animation without controller.
		RectangularAnimation &CreateTop() const override {
            if(top) {
                return top->CreateAnimation(false);
            }
            else {
                return EmptyImage::Instance();
            }
        }
        
        /// Creates a bottom animation without controller.
		RectangularAnimation &CreateBottom() const override {
            if(bottom) {
                return bottom->CreateAnimation(false);
            }
            else {
                return EmptyImage::Instance();
            }
        }

		/// Returns the top component. Could return nullptr
		A_ *GetTop() const {
			return top;
		}

		/// Returns the bottom component. Could return nullptr
		A_ *GetBottom() const {
			return bottom;
		}

		/// Sets the top provider, ownership semantics will not be changed
		void SetTop(A_ *value) {
			if(own)
				delete top;

			top = value;
		}

		/// Sets the bottom provider, ownership semantics will not be changed
		void SetBottom(A_ *value) {
			if(own)
				delete bottom;

			bottom = value;
		}
		
		/// Returns the offset of the top image
		Geometry::Point GetOffset() const override {
            return offset;
        }
        
        ///Sets the offset of the top image
        void SetOffset(const Geometry::Point &value) override {
            offset = value;
        }

        /// Sets the providers in this object
        void SetProviders(A_ &top, A_ &bottom) {
			if(own) {
				delete this->top;
				delete this->bottom;
			}
            this->top = &top;
            this->bottom = &bottom;
        }

		/// Assumes the ownership of the providers
		void OwnProviders() {
			own = true;
		}
		
		/// Prepares the providers. Provider type should support this operation, otherwise
		/// this function will cause a compile time error.
		void Prepare() {
            if(top) top->Prepare();
            if(bottom) bottom->Prepare();
        }

		Geometry::Size GetSize() const override {
			return top ? top->GetSize() : Geometry::Size{0, 0};
		}

    private:
		A_ *top = nullptr;
		A_ *bottom = nullptr;
        Geometry::Point offset = {0,0};

		bool own = false;
    };

	template<class A_>
    basic_StackedObject<A_>::basic_StackedObject(const basic_StackedObjectProvider<A_> &parent, bool create) :
        Gorgon::Animation::Base(create), 
        top(parent.CreateTop()), bottom(parent.CreateBottom()), 
        offset(parent.GetOffset())
    {
        if(this->HasController()) {
            top.SetController(this->GetController());
            bottom.SetController(this->GetController());
        }
    }
        
	template<class A_>
	basic_StackedObject<A_>::basic_StackedObject(const basic_StackedObjectProvider<A_> &parent, Gorgon::Animation::ControllerBase &timer) :
        Gorgon::Animation::Base(timer),
        top(parent.CreateTop()), bottom(parent.CreateBottom()), 
        offset(parent.GetOffset())
    {
        if(this->HasController()) {
            top.SetController(this->GetController());
            bottom.SetController(this->GetController());
        }
    }
        
	template<class A_>
	void basic_StackedObject<A_>::drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const {
        bottom.DrawIn(target, r, color);
        top.DrawIn(target, r+offset, color);
    }

	template<class A_>
	void basic_StackedObject<A_>::drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const {
		bottom.DrawIn(target, controller, r, color);
		top.DrawIn(target, controller, r+offset, color);
	}

        
        
	template<class A_>
	void basic_StackedObject<A_>::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
							const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
							const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
							const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const 
	{
		auto f = Geometry::Sizef((float)bottom.GetSize().Width/top.GetSize().Width, (float)bottom.GetSize().Height/top.GetSize().Height);
		auto bp2 = p1 + (p2-p1) * f;
		auto bp3 = p1 + (p3-p1) * f;
		auto bp4 = p1 + (p4-p1) * f;

		bottom.Draw(target, p1, bp2, bp3, bp4, tex1, tex2, tex3, tex4, color);
		top.Draw(target, p1+offset, p2+offset, p3+offset, p4+offset, tex1, tex2, tex3, tex4, color);
	}


	template<class A_>
	void basic_StackedObject<A_>::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
                                       const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const 
    {
		auto f = Geometry::Sizef((float)bottom.GetSize().Width/top.GetSize().Width, (float)bottom.GetSize().Height/top.GetSize().Height);
		auto bp2 = p1 + (p2-p1) * f;
		auto bp3 = p1 + (p3-p1) * f;
		auto bp4 = p1 + (p4-p1) * f;
		bottom.Draw(target, p1, bp2, bp3, bp4, color);
		top.Draw(target, p1+offset, p2+offset, p3+offset, p4+offset, color);
	}


	template<class A_>
	Geometry::Size basic_StackedObject<A_>::getsize() const {
		return top.GetSize();
	}

	using StackedObject = basic_StackedObject<RectangularAnimationProvider>;
	using StackedObjectProvider = basic_StackedObjectProvider<RectangularAnimationProvider>;

	using StackedBitmapAnimation = basic_StackedObject<BitmapAnimationProvider>;
	using StackedBitmapAnimationProvider = basic_StackedObjectProvider<BitmapAnimationProvider>;

	using StackedBitmap = basic_StackedObject<Bitmap>;
	using StackedBitmapProvider = basic_StackedObjectProvider<Bitmap>;

}
