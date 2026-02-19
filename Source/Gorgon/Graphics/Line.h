#pragma once

#include "Animations.h"
#include "TextureAnimation.h"
#include "EmptyImage.h"
#include "Bitmap.h"

namespace Gorgon :: Graphics {
	class Line;

	/// Interface for LineProviders
	class ILineProvider : public RectangularAnimationProvider {
	public:
		ILineProvider(Orientation orientation) : orientation(orientation) 
		{ }

		virtual ILineProvider &MoveOutProvider() override = 0;

		/// Creates a start animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateStart() const = 0;

		/// Creates a start animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateMiddle() const = 0;

		/// Creates a start animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateEnd() const = 0;

		/// Changes the orientation of the provider. Instances will require redrawing before this change is
		/// reflected.
		virtual void SetOrientation(Orientation value) {
			orientation = value;
		}

		/// Returns the orientation of the line provider
		virtual Orientation GetOrientation() const {
			return orientation;
		}

		/// Sets whether the middle part would be tiled. If set to false it will be stretched to fit the
		/// given area. Instances will require redrawing before this change is reflected. Tiling is 
		/// recommended for all applications.
		virtual void SetTiling(bool value) {
			tiling = value;
		}

		/// Returns if the middle part will be tiled.
		virtual bool GetTiling() const {
			return tiling;
		}

	private:
		Orientation orientation;
		bool tiling = true;
	};

	/**
	 * This class allows drawing a line like image that is made out of three parts. Lines can be scaled
	 * along its orientation.
	 * See basic_LineProvider for details.
	 */
	class Line : public RectangularAnimation {
	public:
		Line(const ILineProvider &prov, Gorgon::Animation::ControllerBase &timer);

		Line(const ILineProvider &prov, bool create = true);
        
        virtual ~Line() {
            start.DeleteAnimation();
            middle.DeleteAnimation();
            end.DeleteAnimation();
        }

		virtual bool Progress(unsigned &) override {
			return true; //individual parts will work automatically
		}
		
		int GetDuration() const override {
            auto dur = start.GetDuration();
            if(dur > 0)
                return dur;
            
            dur = middle.GetDuration();
            if(dur > 0)
                return dur;
            
            dur = end.GetDuration();
            
            return dur;
        }

	protected:
		virtual void drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const override;

		virtual void drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const override;

		virtual Geometry::Size calculatesize(const Geometry::Size &area) const override {
			if(prov.GetOrientation() == Orientation::Horizontal) {
				return{area.Width, getfixedsize()};
			}
			else {
				return{getfixedsize(), area.Height};
			}
		}

		virtual Geometry::Size getsize() const override {
			if(prov.GetOrientation() == Orientation::Horizontal) {
				return{start.GetWidth()+middle.GetWidth()+end.GetWidth(), getfixedsize()};
			}
			else {
				return{getfixedsize(), start.GetHeight()+middle.GetHeight()+end.GetHeight()};
			}
		}

		virtual Geometry::Size calculatesize(const SizeController &controller, const Geometry::Size &s) const override {
			if(prov.GetOrientation() == Orientation::Horizontal) {
				return controller.CalculateSize({middle.GetWidth(), getfixedsize()}, {start.GetWidth()+end.GetWidth(), 0}, s);
			}
			else {
				return controller.CalculateSize({getfixedsize(), middle.GetHeight()}, {0, start.GetHeight()+end.GetHeight()}, s);
			}
		}

		/// Returns the largest size in the non-scalable direction
		int getfixedsize() const {
			if(prov.GetOrientation() == Orientation::Horizontal) {
				return std::max(std::max(start.GetHeight(), middle.GetHeight()), end.GetHeight());
			}
			else {
				return std::max(std::max(start.GetWidth(), middle.GetWidth()), end.GetWidth());
			}
		}

		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
						  const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, 
						  const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const override;


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const override;


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p, RGBAf color) const override;

	private:
		RectangularAnimation &start;
		RectangularAnimation &middle;
		RectangularAnimation &end;

		const ILineProvider &prov;
	};

	/**
	 * This class allows instancing of a line like image that is made out of three 
	 * parts. The first part is the start of the line, the second part is the middle 
	 * and the third part is the end of the line. Middle part can be repeated or
	 * stretched. A line provider can have empty animations. Provider will use 
	 * EmptyImage for missing parts. A_ must derive from RectangularAnimationProvider.
	 * For best results, try to keep height of parts same for horizontal, widths same
	 * for vertical lines.
	 */
	template<class A_>
	class basic_LineProvider : public ILineProvider {
	public:
		using AnimationType = Line;

		/// Empty constructor, line can be instanced even if it is completely empty
		explicit basic_LineProvider(Orientation orientation = Orientation::Horizontal) : ILineProvider(orientation)
		{ }

		/// Filling constructor
		basic_LineProvider(Orientation orientation, A_  &start, A_ &middle, A_ &end) : ILineProvider(orientation),
			start(&start), middle(&middle), end(&end) {}

		/// Filling constructor. This variant will move in the animations, freeing them with this item.
		basic_LineProvider(Orientation orientation, A_  &&start, A_ &&middle, A_ &&end) : ILineProvider(orientation),
			start(new A_(std::move(start))), middle(new A_(std::move(middle))), end(new A_(std::move(end))),
			own(true)
        { }

		/// Filling constructor, nullptr is acceptable, however, it is not advised to use only one side, that is
		/// a waste of resources, a regular image can also be tiled or stretched to fit to an area. 
		basic_LineProvider(Orientation orientation, A_ *start, A_ *middle, A_ *end) : ILineProvider(orientation),
			start(start), middle(middle), end(end) {}

		/// Move constructor
		basic_LineProvider(basic_LineProvider &&other) : ILineProvider(other.GetOrientation()),
			start(other.start), middle(other.middle), end(other.end), own(other.own)
		{
			other.own = false;
			other.start = nullptr;
			other.middle = nullptr;
			other.end = nullptr;
            SetTiling(other.GetTiling());
		}

		basic_LineProvider(const basic_LineProvider &) = delete;

        //types are derived not to type the same code for every class
		virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new typename std::remove_reference<decltype(*this)>::type(std::move(*this));
            
            return *ret;
        }
        
		~basic_LineProvider() {
			if(own) {
				delete start;
				delete middle;
				delete end;
			}
		}

		Line &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
			return *new Line(*this, timer);
		}

		Line &CreateAnimation(bool create = true) const override {
			return *new Line(*this, create);
		}

		virtual RectangularAnimation &CreateStart() const override {
			if(start)
				return start->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}

		virtual RectangularAnimation &CreateMiddle() const override {
			if(middle)
				return middle->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}

		virtual RectangularAnimation &CreateEnd() const override {
			if(end)
				return end->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}

		/// Returns the start animation, might return nullptr
		A_ *GetStart() const {
			return start;
		}

		/// Returns the middle animation, might return nullptr
		A_ *GetMiddle() const {
			return middle;
		}

		/// Returns the end animation, might return nullptr
		A_ *GetEnd() const {
			return end;
		}
		
		/// Changes the start animation, ownership semantics will not change
		void SetStart(A_ *value) {
            if(own)
                delete start;
            
            start = value;
        }
		
		/// Changes the middle animation, ownership semantics will not change
		void SetMiddle(A_ *value) {
            if(own)
                delete middle;
            
            middle = value;
        }
		
		/// Changes the end animation, ownership semantics will not change
		void SetEnd(A_ *value) {
            if(own)
                delete end;
            
            end = value;
        }

		/// Prepares all animation providers if the they support Prepare function.
		void Prepare() {
			if(start)
				start->Prepare();
			if(middle)
				middle->Prepare();
			if(end)
				end->Prepare();
		}

		/// Issuing this function will make this line to own its providers
		/// destroying them along with itself
		void OwnProviders() {
			own = true;
		}

		/// Issuing this function will make this line to disown its providers
		/// and set them to nullptr
		void ReleaseProviders() {
			own = false;
            start = nullptr;
            middle = nullptr;
            end = nullptr;
		}

		Geometry::Size GetSize() const override {
			int w = 0;
			int h = 0;

			if(GetOrientation() == Orientation::Horizontal) {
				if(start) {
					auto sz = start->GetSize();
					w += sz.Width;
					if(h < sz.Height)
						h = sz.Height;
				}
				if(middle) {
					auto sz = middle->GetSize();
					w += sz.Width;
					if(h < sz.Height)
						h = sz.Height;
				}
				if(end) {
					auto sz = end->GetSize();
					w += sz.Width;
					if(h < sz.Height)
						h = sz.Height;
				}
			}
			else {
				if(start) {
					auto sz = start->GetSize();
					h += sz.Height;
					if(w < sz.Width)
						w = sz.Width;
				}
				if(middle) {
					auto sz = middle->GetSize();
					h += sz.Height;
					if(w < sz.Width)
						w = sz.Width;
				}
				if(end) {
					auto sz = end->GetSize();
					h += sz.Height;
					if(w < sz.Width)
						w = sz.Width;
				}
			}

			return {w, h};
		}

	private:
		A_ *start = nullptr;
		A_ *middle = nullptr;
		A_ *end = nullptr;

		bool own = false;
	};

	using LineProvider = basic_LineProvider<RectangularAnimationProvider>;
	using BitmapLineProvider = basic_LineProvider<Bitmap>;
	using AnimatedBitmapLineProvider = basic_LineProvider<BitmapAnimationProvider>;
}
