#pragma once

#include "Animations.h"

namespace Gorgon :: Graphics {
    
	/**
	 * This class is an empty image that will not draw anything if drawn on a layer.
	 * Its size is always 0x0 and it satisfies the requirements for animation. Only
	 * one EmptyImage is enough but this is not enforced. This class is separated into
	 * two due to a bug in Visual Studio
	 */
	class EmptyImage : 
		public virtual RectangularAnimation, public virtual RectangularAnimationProvider
	{
	public:

		virtual ~EmptyImage() { }
		
		virtual EmptyImage &MoveOutProvider() override {
            return *new EmptyImage;
        }

		void DeleteAnimation() const override { }
		
		virtual void SetController(Gorgon::Animation::ControllerBase &) override { }

		virtual bool Progress(unsigned &leftover) override {
			return true;
		}
		
		int GetDuration() const override {
            return 0;
        }

		virtual EmptyImage &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
			return Instance();
		}


		virtual EmptyImage &CreateAnimation(bool create=true) const override {
			return Instance();
		}

		Geometry::Size GetSize() const override {
			return {0, 0};
		}

		/// Returns the instance for empty image. Only one instance is enough.
		static EmptyImage &Instance() { static EmptyImage me; return me; }

	protected:
		virtual void drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const override final {
		}


		virtual Geometry::Size calculatesize(const Geometry::Size &area) const override final {
			return{0,0};
		}


		virtual Geometry::Size calculatesize(const SizeController &controller, const Geometry::Size &s) const override final {
			return {0,0};
		}


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, const Geometry::Pointf &tex1, const Geometry::Pointf &tex2, const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const override final {
		}


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const override final {
		}


		virtual Geometry::Size getsize() const override final {
			return {0, 0};
		}
	};

}
