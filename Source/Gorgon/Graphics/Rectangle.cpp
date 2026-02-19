#include "Rectangle.h"

namespace Gorgon :: Graphics {

	Rectangle::Rectangle(const IRectangleProvider &prov, bool create /*= true*/) : Gorgon::Animation::Base(create), prov(prov),
		tl(prov.CreateTL()),
		tm(prov.CreateTM()),
		tr(prov.CreateTR()),
		ml(prov.CreateML()),
		mm(prov.CreateMM()),
		mr(prov.CreateMR()),
		bl(prov.CreateBL()),
		bm(prov.CreateBM()),
		br(prov.CreateBR())
    {
		if(HasController()) {
			tl.SetController(GetController());
			tm.SetController(GetController());
			tr.SetController(GetController());
			ml.SetController(GetController());
			mm.SetController(GetController());
			mr.SetController(GetController());
			bl.SetController(GetController());
			bm.SetController(GetController());
			br.SetController(GetController());
		}
	}

	Rectangle::Rectangle(const IRectangleProvider &prov, Gorgon::Animation::ControllerBase &timer) : Gorgon::Animation::Base(timer), prov(prov),
		tl(prov.CreateTL()),
		tm(prov.CreateTM()),
		tr(prov.CreateTR()),
		ml(prov.CreateML()),
		mm(prov.CreateMM()),
		mr(prov.CreateMR()),
		bl(prov.CreateBL()),
		bm(prov.CreateBM()),
		br(prov.CreateBR())
    {
		if(HasController()) {
			tl.SetController(GetController());
			tm.SetController(GetController());
			tr.SetController(GetController());
			ml.SetController(GetController());
			mm.SetController(GetController());
			mr.SetController(GetController());
			bl.SetController(GetController());
			bm.SetController(GetController());
			br.SetController(GetController());
		}
	}

	Geometry::Size Rectangle::getsize() const {
		int maxt = std::max(std::max(tl.GetHeight(), tm.GetHeight()), tr.GetHeight());
		int maxb = std::max(std::max(bl.GetHeight(), bm.GetHeight()), br.GetHeight());

		int maxl = std::max(std::max(tl.GetWidth(), ml.GetWidth()), bl.GetWidth());
		int maxr = std::max(std::max(tr.GetWidth(), mr.GetWidth()), br.GetWidth());

		return {maxl+maxr+mm.GetWidth(), maxt+maxb+mm.GetHeight()};
	}

	void Rectangle::drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const {
        int maxt = std::max(std::max(tl.GetHeight(), tm.GetHeight()), tr.GetHeight());
        int maxb = std::max(std::max(bl.GetHeight(), bm.GetHeight()), br.GetHeight());
        
        int maxl = std::max(std::max(tl.GetWidth(), ml.GetWidth()), bl.GetWidth());
        int maxr = std::max(std::max(tr.GetWidth(), mr.GetWidth()), br.GetWidth());
        
        if(r.Height < maxt+maxb) {
            //partial drawing...
        }
        
        
        tl.Draw(target, r.X + maxl - tl.GetWidth(), r.Y + maxt - tl.GetHeight(), color);
        tm.DrawIn(target, prov.GetSideTiling() ? Tiling::Horizontal : Tiling::None, 
                  Geometry::Rectanglef(r.X + maxl, r.Y + maxt - tm.GetHeight(), r.Width-maxl-maxr, (Float)tm.GetHeight()),
                  color);
        tr.Draw(target, r.Right() - maxr, r.Y + maxt - tr.GetHeight(), color);

        ml.DrawIn(target, prov.GetSideTiling() ? Tiling::Vertical : Tiling::None, 
                  Geometry::Rectanglef(r.X + maxl - ml.GetWidth(), r.Y + maxt, (Float)ml.GetWidth(), r.Height-maxt-maxb),
                  color);        
        mm.DrawIn(target, prov.GetCenterTiling() ? Tiling::Both : Tiling::None, 
                  Geometry::Rectanglef(r.X + maxl, r.Y + maxt, r.Width-maxl-maxr, r.Height-maxt-maxb),
                  color);
        mr.DrawIn(target, prov.GetSideTiling() ? Tiling::Vertical : Tiling::None, 
                  Geometry::Rectanglef(r.Right() - maxr, r.Y + maxt, (Float)mr.GetWidth(), r.Height-maxt-maxb),
                  color);
        
        
        bl.Draw(target, r.X + maxl - bl.GetWidth(), r.Bottom() - maxb, color);
        bm.DrawIn(target, prov.GetSideTiling() ? Tiling::Horizontal : Tiling::None, 
                  Geometry::Rectanglef(r.X + maxl, r.Bottom() - maxb, r.Width-maxl-maxr, (Float)bm.GetHeight()),
                  color);
        br.Draw(target, r.Right() - maxr, r.Bottom() - maxb, color);
	}

	void Rectangle::drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const {
		SizeController c = controller;

		if(!prov.GetCenterTiling()) {
			c.Horizontal =  c.Stretch;
			c.Vertical =  c.Stretch;
		}

		float maxt = (float)std::max(std::max(tl.GetHeight(), tm.GetHeight()), tr.GetHeight());
		float maxb = (float)std::max(std::max(bl.GetHeight(), bm.GetHeight()), br.GetHeight());

		float maxl = (float)std::max(std::max(tl.GetWidth(), ml.GetWidth()), bl.GetWidth());
		float maxr = (float)std::max(std::max(tr.GetWidth(), mr.GetWidth()), br.GetWidth());

		auto newr = c.CalculateArea(Geometry::Sizef(mm.GetSize()), {maxl+maxr, maxt+maxb}, r.GetSize());
		newr = newr + r.TopLeft();

		drawin(target, newr, color);
	}

	void Rectangle::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
					const Geometry::Pointf &p3, const Geometry::Pointf &p4,
					const Geometry::Pointf &, const Geometry::Pointf &,
					const Geometry::Pointf &, const Geometry::Pointf &, RGBAf color) const {
		draw(target, p1, p2, p3, p4, color);
	}

	void Rectangle::draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
					const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const {
		Utils::NotImplemented();
	}

	void Rectangle::draw(TextureTarget &target, const Geometry::Pointf &p, RGBAf color) const {
		drawin(target, {p, Geometry::Sizef(getsize())}, color);
	}
	
    BitmapRectangleProvider SliceHorizontal(const Bitmap &source, int t, int b, int tl, int tr, int l, int r, int bl, int br) {
        return BitmapRectangleProvider(
            source.Slice({0, 0, tl, t}),
            source.Slice({tl, 0, tr, t}),
            source.Slice({tr, 0, source.GetWidth(), t}),
            source.Slice({0, t, l, b}),
            source.Slice({l, t, r, b}),
            source.Slice({r, t, source.GetWidth(), b}),
            source.Slice({0, b, bl, source.GetHeight()}),
            source.Slice({bl, b, br, source.GetHeight()}),
            source.Slice({br, b, source.GetWidth(), source.GetHeight()})
        );
    }

    BitmapRectangleProvider SliceVertical(const Bitmap &source, int l, int r, int tl, int bl, int t, int b, int tr, int br) {
        return BitmapRectangleProvider(
            source.Slice({0, 0, l, tl}),
            source.Slice({l, 0, r, t}),
            source.Slice({r, 0, source.GetWidth(), tr}),
            source.Slice({0, tl, l, bl}),
            source.Slice({l, t, r, b}),
            source.Slice({r, tr, source.GetWidth(), br}),
            source.Slice({0, bl, l, source.GetHeight()}),
            source.Slice({l, b, r, source.GetHeight()}),
            source.Slice({r, br, source.GetWidth(), source.GetHeight()})
        );
    }


}
