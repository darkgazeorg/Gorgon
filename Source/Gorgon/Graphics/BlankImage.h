#pragma once

#include "Animations.h"

namespace Gorgon :: Graphics {
  
	/**
	 * Pure color blank image, default size is 0x0, but can be drawn
	 * with any size. Instance function could be used for 0x0 instance
	 * or a new blank image can be constructed for any specific size.
	 * The color can be specified or left white.
	 */
	class BlankImage : public RectangularAnimationProvider, public RectangularAnimation {
	public:
		explicit BlankImage(Geometry::Size size, RGBAf color = 1.f) : size(size), color(color) {}

		BlankImage(int w, int h, RGBAf color = 1.f) : BlankImage({w, h}, color) {}

		explicit BlankImage(RGBAf color = 1.f) : color(color) {}
		
		static BlankImage &Get() {
            static BlankImage b;
            
            return b;
        }
		
		virtual BlankImage &MoveOutProvider() override {
            return *new BlankImage(size, color);
        }

		virtual BlankImage &CreateAnimation(Gorgon::Animation::ControllerBase &) const override {
			return const_cast<BlankImage&>(*this);
		}

		virtual BlankImage &CreateAnimation(bool =true) const override {
			return const_cast<BlankImage&>(*this);
		}

		virtual bool Progress(unsigned &) override {
			return true;
		}
		
		int GetDuration() const override {
            return 0;
        }

		virtual void DeleteAnimation() const override {
		}

		virtual void SetController(Gorgon::Animation::ControllerBase &) override { }
		
		/// Returns the color of this blank image
		RGBAf GetColor() const {
			return color;
		}

		/// Sets the color of this blank image
		void SetColor(RGBAf value) {
			color = value;
		}

		/// Sets the size of this blank image
		void SetSize(Geometry::Size value) {
			size = value;
		}

		Geometry::Size GetSize() const override {
			return size;
		}

	protected:
		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, const Geometry::Pointf &, const Geometry::Pointf &, const Geometry::Pointf &, const Geometry::Pointf &, RGBAf c) const override {
			target.Draw(p1, p2, p3, p4, color*c);
		}

		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf c) const override {
			target.Draw(p1, p2, p3, p4, color*c);
		}

		virtual void drawstretched(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf c) const override {
			target.Draw(r, color*c);
		}

		virtual void drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf c) const override {
			target.Draw(r, color*c);
		}

		virtual void drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf c) const override {
			target.Draw(r, color*c);
		}

		virtual Geometry::Size getsize() const override {
			return size;
		}

		virtual Geometry::Size calculatesize(const Geometry::Size &area) const override {
			return area;
		}

		virtual Geometry::Size calculatesize(const SizeController &controller, const Geometry::Size &s) const override {
			return s;
		}

	private:
		Geometry::Size size = {0, 0};
		RGBAf color = {1.0f};
	};

}
