#pragma once


#include "../Geometry/Point.h"
#include "../Geometry/Size.h"
#include "../Geometry/Rectangle.h"

#include "../Graphics.h"
#include "TextureTargets.h"


namespace Gorgon :: Graphics {

	/// Represents a drawable object, that can be drawn to the given point. Size of the object
	/// is assumed to be fixed. Drawing a single drawable can cause many texture to be added
	/// to the given texture target
	class Drawable {
	public:
        virtual ~Drawable() { }

		/// Draw to the given coordinates
		void Draw(TextureTarget &target, int x, int y, RGBAf color = RGBAf(1.f)) const {
			draw(target, {(float)x, (float)y}, color);
		}

		/// Draw to the given coordinates
		void Draw(TextureTarget &target, const Geometry::Point &p, RGBAf color = RGBAf(1.f)) const {
			draw(target, {(float)p.X, (float)p.Y}, color);
		}

		/// Draw to the given coordinates
		void Draw(TextureTarget &target, float x, float y, RGBAf color = RGBAf(1.f)) const {
			draw(target, {x, y}, color);
		}

		/// Draw to the given coordinates
		void Draw(TextureTarget &target, const Geometry::Pointf &p, RGBAf color = RGBAf(1.f)) const {
			draw(target, p, color);
		}

	protected:
		/// This is the only function that needs to implemented for a drawable
		virtual void draw(TextureTarget &target, const Geometry::Pointf &p, RGBAf color) const = 0;
	};


	/// A drawable object that does not have a size and requires a region to draw.
	class SizelessDrawable {
	public:
       virtual ~SizelessDrawable() { }

	protected:
		/// This function should draw this drawable inside the given rectangle
		virtual void drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const = 0;

		/// This function should draw this drawable inside the given rectangle according to the given controller.
		/// If this object already have a size controller, this given controller should be given priority.
		virtual void drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const = 0;

		/// This function should return the size of the object when it is requested to be drawn in the given area. If size contains
		/// is a negative value, this function should try to return native size of the object. If no such size exists, a logical size
		/// should be returned.
		virtual Geometry::Size calculatesize(const Geometry::Size &area) const = 0;

		/// This function should return the size of the object when it is requested to be drawn in the given area. This variant should
		/// use the given size controller. If the object already has a controller, given controller should be given priority. If h parameter
		/// is a negative value, this function should try to return native size of the object. If no such size exists, a logical size
		/// should be returned.
		virtual Geometry::Size calculatesize(const SizeController &controller, const Geometry::Size &s) const = 0;
	};


	////This is a basic drawing object
	class RectangularDrawable : public virtual Drawable {
	public:

		using Drawable::Draw;

		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, int x, int y, int w, int h, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, {(float)x, (float)y, (float)w, (float)h}, color);
		}

		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, const Geometry::Point &p, int w, int h, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, {(Geometry::Pointf)p, (float)w, (float)h}, color);
		}

		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, int x, int y, const Geometry::Size &size, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, {(float)x, (float)y, (Geometry::Sizef)size}, color);
		}

		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, const Geometry::Point &p, const Geometry::Size &size, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, {(Geometry::Pointf)p, (Geometry::Sizef)size}, color);
		}

		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, const Geometry::Rectangle &r, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, (Geometry::Rectanglef)r, color);
		}


		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, float x, float y, float w, float h, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, {x, y, w, h}, color);
		}

		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, const Geometry::Pointf &p, float w, float h, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, {p, w, h}, color);
		}

		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, float x, float y, const Geometry::Sizef &size, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, {x, y, size}, color);
		}

		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, const Geometry::Pointf &p, const Geometry::Sizef &size, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, {p, size}, color);
		}

		/// Draw to the given area by stretching object to fit
		void DrawStretched(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color = RGBAf(1.f)) const {
			drawstretched(target, r, color);
		}

		/// Draw to the given area by fitting the bitmap down to the size of the area
		void DrawShrinked(TextureTarget &target, Placement align = Placement::MiddleCenter) {
			DrawShrinked(target, {0, 0}, target.GetTargetSize(), align);
		}

		/// Draw to the given area by fitting the bitmap down to the size of the area
		void DrawShrinked(TextureTarget &target, float x, float y, const Geometry::Sizef &size, Placement align = Placement::MiddleCenter) {
			DrawShrinked(target, {x, y}, size, align);
		}

		/// Draw to the given area by fitting the bitmap down to the size of the area
		void DrawShrinked(TextureTarget &target, const Geometry::Point &p, float w, float h, Placement align = Placement::MiddleCenter) {
			DrawShrinked(target, p, {w, h}, align);
		}

		/// Draw to the given area by fitting the bitmap down to the size of the area
		void DrawShrinked(TextureTarget &target, float x, float y, float w, float h, Placement align = Placement::MiddleCenter) {
			DrawShrinked(target, {x, y}, {w, h}, align);
		}

		/// Draw to the given area by fitting the bitmap down to the size of the area
		void DrawShrinked(TextureTarget &target, const Geometry::Pointf &p, const Geometry::Sizef &size, Placement align = Placement::MiddleCenter) {
			DrawStretched(target, ShrinkFit(align, GetSize(), size) + p);
		}


		/// Draw the object to the target by specifying coordinates for four corners
		void Draw(TextureTarget &target, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, RGBAf color = RGBAf(1.f)) const {
			draw(target, {x1, y1}, {x2, y2}, {x3, y3}, {x4, y4}, color);
		}

		/// Draw the object to the target by specifying coordinates for four corners
		void Draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color = RGBAf(1.f)) const {
			draw(target, p1, p2, p3, p4, color);
		}


		/// Draw the object rotated to the given angle in radians, full C++11 support will enable the use of 90deg like qualifiers
		void DrawRotated(TextureTarget &target, const Geometry::Point &p, float angle, const Geometry::Pointf &origin=Geometry::Point(0, 0), RGBAf color = RGBAf(1.f)) const;


		/// Draw the object rotated to the given angle in radians, full C++11 support will enable the use of 90deg like qualifiers
		void DrawRotated(TextureTarget &target, const Geometry::Point &p, float angle, RGBAf color) const {
            DrawRotated(target, p, angle, Geometry::Point(0, 0), color);
        }
        
		/// Draw the object rotated to the given angle in radians, full C++11 support will enable the use of 90deg like qualifiers
		void DrawRotated(TextureTarget &target, int x, int y, float angle, float oX, float oY, RGBAf color = RGBAf(1.f)) const {
			DrawRotated(target, {x, y}, angle, {oX, oY}, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, int x, int y, int w, int h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, {(float)x, (float)y, (float)w, (float)h}, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, const Geometry::Point &p, int w, int h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, {(Geometry::Pointf)p, (float)w, (float)h}, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, int x, int y, const Geometry::Size &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, {(float)x, (float)y, (Geometry::Sizef)size}, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, const Geometry::Point &p, const Geometry::Size &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, {(Geometry::Pointf)p, (Geometry::Sizef)size}, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, const Geometry::Rectangle &r, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, (Geometry::Rectanglef)r, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, float x, float y, float w, float h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, {x, y, w, h}, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, const Geometry::Pointf &p, float w, float h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, {p, w, h}, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, float x, float y, const Geometry::Sizef &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, {x, y, size}, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, const Geometry::Pointf &p, const Geometry::Sizef &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, {p, size}, color);
		}

		/// Draws the object to the target using the given tiling information
		void DrawIn(TextureTarget &target, Tiling tiling, const Geometry::Rectanglef &r, RGBAf color = RGBAf(1.f)) const {
			drawin(target, tiling, r, color);
		}

		/// Draw to the given area
		void DrawIn(TextureTarget &target, int x, int y, int w, int h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, {(float)x, (float)y, (float)w, (float)h}, color);
		}

		/// Draw to the given area
		void DrawIn(TextureTarget &target, const Geometry::Point &p, int w, int h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, {(Geometry::Pointf)p, (float)w, (float)h}, color);
		}

		/// Draw to the given area
		void DrawIn(TextureTarget &target, int x, int y, const Geometry::Size &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, {(float)x, (float)y, (Geometry::Sizef)size}, color);
		}

		/// Draw to the given area
		void DrawIn(TextureTarget &target, const Geometry::Point &p, const Geometry::Size &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, {(Geometry::Pointf)p, (Geometry::Sizef)size}, color);
		}

		/// Draw in the given area
		void DrawIn(TextureTarget &target, const Geometry::Rectangle &r, RGBAf color = RGBAf(1.f)) const {
			drawin(target, Geometry::Rectanglef(r), color);
		}

		/// Draw to fill the given target
		void DrawIn(TextureTarget &target, RGBAf color = RGBAf(1.f)) const {
			drawin(target, {0, 0, target.GetTargetSize()}, color);
		}

		/// Draw to the given coordinates with the given size according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, int x, int y, int w, int h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, {(float)x, (float)y, (float)w, (float)h}, color);
		}

		/// Draw to the given area according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, const Geometry::Point &p, int w, int h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, {p, (float)w, (float)h}, color);
		}

		/// Draw to the given area the given size according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, int x, int y, const Geometry::Size &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, {(float)x, (float)y, size}, color);
		}

		/// Draw to the given area with the given size according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, const Geometry::Point &p, const Geometry::Size &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, {(Geometry::Pointf)p, (Geometry::Sizef)size}, color);
		}


		/// Draw in the given area according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, const Geometry::Rectangle &r, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, Geometry::Rectanglef(r), color);
		}

		/// Draw to fill the given target according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, {0, 0, target.GetTargetSize()}, color);
		}

		/// Draw to the given area
		void DrawIn(TextureTarget &target, float x, float y, float w, float h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, {x, y, w, h}, color);
		}

		/// Draw to the given area
		void DrawIn(TextureTarget &target, const Geometry::Pointf &p, float w, float h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, {p, w, h}, color);
		}

		/// Draw to the given area
		void DrawIn(TextureTarget &target, float x, float y, const Geometry::Sizef &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, {x, y, size}, color);
		}

		/// Draw to the given area
		void DrawIn(TextureTarget &target, const Geometry::Pointf &p, const Geometry::Sizef &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, {p, size}, color);
		}

		/// Draw in the given area
		void DrawIn(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color = RGBAf(1.f)) const {
			drawin(target, r, color);
		}

		/// Draw to the given coordinates with the given size according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, float x, float y, float w, float h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, {x, y, w, h}, color);
		}

		/// Draw to the given area according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, const Geometry::Pointf &p, float w, float h, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, {p, w, h}, color);
		}

		/// Draw to the given area the given size according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, float x, float y, const Geometry::Sizef &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, {x, y, size}, color);
		}

		/// Draw to the given area with the given size according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, const Geometry::Pointf &p, const Geometry::Sizef &size, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, {p, size}, color);
		}


		/// Draw in the given area according to the given controller
		void DrawIn(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color = RGBAf(1.f)) const {
			drawin(target, controller, r, color);
		}

		/// Calculates the adjusted size of this drawable depending on the given area. This function calculates
		/// actual the size of the drawable when it is drawn in an area that has the given size. When a size parameter
		/// is set to -1, the object returns its native size. Native size might not be available for some objects
		/// however, they will try to return a logical value.
		const Geometry::Size CalculateSize(const Geometry::Size &area) const {
			return calculatesize(area);
		}

		/// Calculates the adjusted size of this drawable depending on the given area. This function calculates
		/// actual the size of the drawable when it is drawn in an area that has the given size. When a size parameter
		/// is set to -1, the object returns its native size. Native size might not be available for some objects
		/// however, they will try to return a logical value.
		const Geometry::Size CalculateSize(int w=-1, int h=-1) const {
			return calculatesize({w, h});
		}

		/// Calculates the adjusted size of this drawable depending on the given area and controller. This function calculates
		/// actual the size of the drawable when it is drawn in an area that has the given size. When a size parameter
		/// is set to -1, the object returns its native size. Native size might not be available for some objects
		/// however, they will try to return a logical value.
		const Geometry::Size CalculateSize(const SizeController &controller, const Geometry::Size &area) const {
			return calculatesize(controller, area);
		}

		/// Calculates the adjusted size of this drawable depending on the given area and controller. This function calculates
		/// actual the size of the drawable when it is drawn in an area that has the given size. When a size parameter
		/// is set to -1, the object returns its native size. Native size might not be available for some objects
		/// however, they will try to return a logical value.
		const Geometry::Size CalculateSize(const SizeController &controller, int w=-1, int h=-1) const {
			return calculatesize(controller, {w, h});
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, 
                  float x1, float y1, 
                  float x2, float y2, 
                  float x3, float y3, 
                  float x4, float y4, 
                  float u1, float v1, 
                  float u2, float v2, 
                  float u3, float v3, 
                  float u4, float v4, 
                  RGBAf color = RGBAf(1.f)) const {
			draw(target, {x1, y1}, {x2, y2}, {x3, y3}, {x4, y4}, {u1, v1}, {u2, v2}, {u3, v3}, {u4, v4}, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4, RGBAf color = RGBAf(1.f)) const {
			draw(target, p1, p2, p3, p4, {u1, v1}, {u2, v2}, {u3, v3}, {u4, v4}, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, const Geometry::Pointf &t1, const Geometry::Pointf &t2, const Geometry::Pointf &t3, const Geometry::Pointf &t4, RGBAf color = RGBAf(1.f)) const {
			draw(target, {x1, y1}, {x2, y2}, {x3, y3}, {x4, y4}, t1, t2, t3, t4, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, const Geometry::Pointf &t1, const Geometry::Pointf &t2, const Geometry::Pointf &t3, const Geometry::Pointf &t4, RGBAf color = RGBAf(1.f)) const {
			draw(target, p1, p2, p3, p4, t1, t2, t3, t4, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, float x, float y, float w, float h, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4, RGBAf color = RGBAf(1.f)) const {
			draw(target, {x, y}, {x+w, y}, {x+w, y+h}, {x, y+h}, {u1, v1}, {u2, v2}, {u3, v3}, {u4, v4}, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, float x, float y, float w, float h, const Geometry::Pointf &t1, const Geometry::Pointf &t2, const Geometry::Pointf &t3, const Geometry::Pointf &t4, RGBAf color = RGBAf(1.f)) const {
			draw(target, {x, y}, {x+w, y}, {x+w, y+h}, {x, y+h}, t1, t2, t3, t4, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, const Geometry::Pointf &p, float w, float h, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4, RGBAf color = RGBAf(1.f)) const {
			Draw(target, p.X, p.Y, w, h, {u1, v1}, {u2, v2}, {u3, v3}, {u4, v4}, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, const Geometry::Pointf &p, float w, float h, const Geometry::Pointf &t1, const Geometry::Pointf &t2, const Geometry::Pointf &t3, const Geometry::Pointf &t4, RGBAf color = RGBAf(1.f)) const {
			Draw(target, p.X, p.Y, w, h, t1, t2, t3, t4, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, float x, float y, const Geometry::Sizef &size, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4, RGBAf color = RGBAf(1.f)) const {
			Draw(target, x, y, size.Width, size.Height, {u1, v1}, {u2, v2}, {u3, v3}, {u4, v4}, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated
		void Draw(TextureTarget &target, float x, float y, const Geometry::Sizef &size, const Geometry::Pointf &t1, const Geometry::Pointf &t2, const Geometry::Pointf &t3, const Geometry::Pointf &t4, RGBAf color = RGBAf(1.f)) const {
			Draw(target, x, y, size.Width, size.Height, t1, t2, t3, t4, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, const Geometry::Pointf &p, const Geometry::Sizef &size, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4, RGBAf color = RGBAf(1.f)) const {
			Draw(target, p.X, p.Y, size.Width, size.Height, {u1, v1}, {u2, v2}, {u3, v3}, {u4, v4}, color);
		}

		/// Draws the object with the given screen and texture coordinates. Texture coordinates are given between 0 and 1. If the 
		/// coordinates are out of bounds the texture is repeated. This function is not guaranteed to be implemented and can be
		/// directed the draw call where texture coordinates are not specified. This function can be removed in the future.
		void Draw(TextureTarget &target, const Geometry::Pointf &p, const Geometry::Sizef &size, const Geometry::Pointf &t1, const Geometry::Pointf &t2, const Geometry::Pointf &t3, const Geometry::Pointf &t4, RGBAf color = RGBAf(1.f)) const {
			Draw(target, p.X, p.Y, size.Width, size.Height, t1, t2, t3, t4, color);
		}

		/// Returns the size of this object
		const Geometry::Size GetSize() const {
			return getsize();
		}

		/// Returns the width of the drawable
		int GetWidth() const { return getsize().Width; }

		/// Returns the height of the drawable
		int GetHeight() const { return getsize().Height; }

	protected:

		/// This function should draw the object to the given point.
		virtual void draw(TextureTarget &target, const Geometry::Pointf &p, RGBAf color) const override {
			auto size=getsize();
			draw(target, p, {float(p.X+size.Width), float(p.Y)}, {p.X+size.Width, p.Y+size.Height}, {p.X, p.Y+size.Height}, color);
		}

		/// This method should draw to object inside the given quad with the given texture coordinates.
		/// The texture should be considered repeating and any values outside 0-1 range should be treated
		/// as such
		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
			const Geometry::Pointf &p3, const Geometry::Pointf &p4,
			const Geometry::Pointf &tex1, const Geometry::Pointf &tex2,
			const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const = 0;

		/// This function should draw the object inside the given quad. The object should be stretched as
		/// necessary
		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
			const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const = 0;

		/// This function should draw the object to the target area. The object should be stretched along
		/// both dimensions to fit into the given area.
		/// It might be logical to override this as it is possible to avoid additional function calls and
		/// if statements
		virtual void drawstretched(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const {
			drawin(target, Tiling::None, r, color);
		}

		/// This function should draw the object to the target area. The object should be tiled or cut
		/// to fit the given area
		/// It might be logical to override this as it is possible to avoid additional function calls and
		/// if statements
		virtual void drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const { 
			drawin(target, Tiling::Both, r, color); 
		}

		/// This function should draw this drawable inside the given rectangle according to the given controller.
		/// If this object already have a size controller, this given controller should be given priority.
		virtual void drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const = 0;

		/// This function should return the size of the object when it is requested to be drawn in the given area. If size contains
		/// is a negative value, this function should try to return native size of the object. If no such size exists, a logical size
		/// should be returned.
		virtual Geometry::Size calculatesize(const Geometry::Size &area) const = 0;

		/// This function should return the size of the object when it is requested to be drawn in the given area. This variant should
		/// use the given size controller. If the object already has a controller, given controller should be given priority. If h parameter
		/// is a negative value, this function should try to return native size of the object. If no such size exists, a logical size
		/// should be returned.
		virtual Geometry::Size calculatesize(const SizeController &controller, const Geometry::Size &s) const = 0;

		/// Should return the exact size of this object
		virtual Geometry::Size getsize() const = 0;
	};

	/// This is an interface for solid texture based image. It handles the drawing automatically. Does not supply implementation
	/// for Texture.
	class Image : public virtual RectangularDrawable, public virtual TextureSource {
	public:

	protected:
		virtual Geometry::Size getsize() const override {
			return GetImageSize();
		}

		virtual Geometry::Size calculatesize(const Geometry::Size &s) const override {
			return GetImageSize();
		}

		virtual Geometry::Size calculatesize(const SizeController &controller, const Geometry::Size &s) const override {
			return controller.CalculateSize(getsize(), s);
		}

		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
			const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const override {
			target.Draw(*this, p1, p2, p3, p4, color);
		}

		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
			const Geometry::Pointf &p3, const Geometry::Pointf &p4,
			const Geometry::Pointf &tex1, const Geometry::Pointf &tex2,
			const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const override {
			target.Draw(*this, p1, p2, p3, p4, tex1, tex2, tex3, tex4, color);
		}

		virtual void drawstretched(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const override {
			target.Draw(*this, Tiling::None, r, color);
		}

		virtual void drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const override {
			target.Draw(*this, Tiling::Both, r, color);
		}

		virtual void drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const override {
			target.Draw(*this, controller.GetTiling(), controller.CalculateArea((Geometry::Sizef)getsize(), r.GetSize())+r.TopLeft(), color);
		}
	};
	

}
