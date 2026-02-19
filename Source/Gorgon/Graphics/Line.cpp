#include "Line.h"

namespace Gorgon :: Graphics {
	
	Line::Line(const ILineProvider &prov, bool create /*= true*/) : prov(prov), Gorgon::Animation::Base(create),
		start(prov.CreateStart()),
		middle(prov.CreateMiddle()),
		end(prov.CreateEnd()) 
	{
		if(HasController()) {
			start.SetController(GetController());
			middle.SetController(GetController());
			end.SetController(GetController());
		}
	}

	Line::Line(const ILineProvider &prov, Gorgon::Animation::ControllerBase &timer) : prov(prov), Gorgon::Animation::Base(timer),
		start(prov.CreateStart()),
		middle(prov.CreateMiddle()),
		end(prov.CreateEnd()) 
	{
		if(HasController()) {
			start.SetController(GetController());
			middle.SetController(GetController());
			end.SetController(GetController());
		}
	}

	void Line::drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const {
		if(prov.GetOrientation() == Orientation::Horizontal) {
			start.Draw(target, r.TopLeft(), color);
			middle.DrawIn(target, prov.GetTiling() ? Tiling::Horizontal : Tiling::None, 
                          Geometry::Rectanglef(r.X + start.GetWidth(), r.Y, r.Width-start.GetWidth()-end.GetWidth(), (Float)middle.GetHeight()), color);
			end.Draw(target, r.Right()-end.GetWidth(), r.Y, color);
		}
		else {
			start.Draw(target, r.TopLeft(), color);
			middle.DrawIn(target, prov.GetTiling() ? Tiling::Vertical : Tiling::None, Geometry::Rectanglef(r.X, r.Y + start.GetHeight(), (Float)middle.GetWidth(), r.Height-start.GetHeight()-end.GetHeight()), color);
			end.Draw(target, r.X, r.Bottom()-end.GetHeight(), color);
		}
	}

	void Line::drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const {
		SizeController c = controller;
		if(prov.GetOrientation() == Orientation::Horizontal) {
			if(!prov.GetTiling())
				c.Horizontal =  c.Stretch;

			float w = r.Width - start.GetWidth() - end.GetWidth();
			start.DrawIn(target, c, r.X, r.Y, (Float)start.GetWidth(), r.Height);
			middle.DrawIn(target, c, r.X + start.GetWidth(), r.Y, w, r.Height);
			end.DrawIn(target, c, r.Right() - end.GetWidth(), r.Y, (Float)end.GetWidth(), r.Height);
		}
		else {
			if(!prov.GetTiling())
				c.Vertical =  c.Stretch;

			float h = r.Height - start.GetHeight() - end.GetHeight();
			start.DrawIn(target, c, r.X, r.Y, r.Height, (Float)start.GetHeight());
			middle.DrawIn(target, c, r.X, r.Y + start.GetHeight(), r.Width, h);
			end.DrawIn(target, c, r.X, r.Bottom() - end.GetHeight(), r.Width, (Float)end.GetHeight());
		}
	}

	void Line::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
					const Geometry::Pointf &p3, const Geometry::Pointf &p4, 
					const Geometry::Pointf &, const Geometry::Pointf &,
					const Geometry::Pointf &, const Geometry::Pointf &, RGBAf color) const 
	{
		draw(target, p1, p2, p3, p4, color);
	}

	void Line::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, 
					const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const 
	{
		Utils::NotImplemented();
	}

	void Line::draw(TextureTarget &target, const Geometry::Pointf &p, RGBAf color) const {
		drawin(target, {p, Geometry::Sizef(getsize())}, color);
	}

}
