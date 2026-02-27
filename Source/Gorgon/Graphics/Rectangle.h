#pragma once

#include "Animations.h"
#include "TextureAnimation.h"
#include "EmptyImage.h"
#include "Bitmap.h"

#include <set>

namespace Gorgon :: Graphics {
	/// Interface for RectangleProviders
	class IRectangleProvider : public RectangularAnimationProvider {
	public:
		IRectangleProvider() {}

		virtual IRectangleProvider &MoveOutProvider() override = 0;

		/// Creates an animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateTL() const = 0;

		/// Creates an animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateTM() const = 0;

		/// Creates an animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateTR() const = 0;

		/// Creates an animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateML() const = 0;

		/// Creates an animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateMM() const = 0;

		/// Creates an animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateMR() const = 0;

		/// Creates an animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateBL() const = 0;

		/// Creates an animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateBM() const = 0;

		/// Creates an animation without controller. This function should always return an animation
		virtual RectangularAnimation &CreateBR() const = 0;

		/// Sets whether the middle part would be tiled. If set to false it will be stretched to fit the
		/// given area. Instances will require redrawing before this change is reflected. This effects
		/// both dimensions. Tiling is recommended for all applications.
		virtual void SetCenterTiling(bool value) {
			ctiling = value;
		}

		/// Returns if the middle part will be tiled.
		virtual bool GetCenterTiling() const {
			return ctiling;
		}

		/// Sets whether the side parts (tm, ml, mr, bm) would be tiled. If set to false it will be stretched 
		/// to fit the given area. Instances will require redrawing before this change is reflected. Tiling is 
		/// recommended for all applications.
		virtual void SetSideTiling(bool value) {
			stiling = value;
		}

		/// Returns if the middle part will be tiled.
		virtual bool GetSideTiling() const {
			return stiling;
		}

	private:
		bool ctiling = true;
        bool stiling = true;
	};

	/**
	* This class allows drawing a rectangle like image that is made out of nine parts. Rectangles can be scaled
	* freely without loss of quality.
	* See basic_RectangleProvider for details.
	*/
	class Rectangle : public RectangularAnimation {
	public:
		Rectangle(const IRectangleProvider &prov, Gorgon::Animation::ControllerBase &timer);

		Rectangle(const IRectangleProvider &prov, bool create = true);

		virtual ~Rectangle() {
			tl.DeleteAnimation();
			tm.DeleteAnimation();
			tr.DeleteAnimation();
			ml.DeleteAnimation();
			mm.DeleteAnimation();
			mr.DeleteAnimation();
			bl.DeleteAnimation();
			bm.DeleteAnimation();
			br.DeleteAnimation();
		}

		virtual bool Progress(unsigned &) override {
			return true; //individual parts will work automatically
		}
		
		int GetDuration() const override {
            auto dur = mm.GetDuration();
            if(dur)
                return dur;
            
            dur = tm.GetDuration();
            if(dur)
                return dur;
            
            dur = ml.GetDuration();
            if(dur)
                return dur;
            
            dur = mr.GetDuration();
            if(dur)
                return dur;
            
            dur = bm.GetDuration();
            if(dur)
                return dur;
            
            dur = tl.GetDuration();
            if(dur)
                return dur;
            
            dur = tr.GetDuration();
            if(dur)
                return dur;
            
            dur = bl.GetDuration();
            if(dur)
                return dur;
            
            dur = br.GetDuration();
            
            return dur;
        }

	protected:
		virtual void drawin(TextureTarget &target, const Geometry::Rectanglef &r, RGBAf color) const override;

		virtual void drawin(TextureTarget &target, const SizeController &controller, const Geometry::Rectanglef &r, RGBAf color) const override;

		virtual Geometry::Size calculatesize(const Geometry::Size &area) const override {
			return area;
		}

		virtual Geometry::Size calculatesize(const SizeController &controller, const Geometry::Size &s) const override {
			return controller.CalculateSize({mm.GetSize()}, {ml.GetWidth()+mr.GetWidth(), tm.GetHeight()+bm.GetHeight()}, s);
		}

		virtual Geometry::Size getsize() const override;

		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2,
						  const Geometry::Pointf &p3, const Geometry::Pointf &p4,
						  const Geometry::Pointf &tex1, const Geometry::Pointf &tex2,
						  const Geometry::Pointf &tex3, const Geometry::Pointf &tex4, RGBAf color) const override;


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p1, const Geometry::Pointf &p2, const Geometry::Pointf &p3, const Geometry::Pointf &p4, RGBAf color) const override;


		virtual void draw(TextureTarget &target, const Geometry::Pointf &p, RGBAf color) const override;

	private:
		RectangularAnimation &tl, &tm, &tr;
		RectangularAnimation &ml, &mm, &mr;
		RectangularAnimation &bl, &bm, &br;

		const IRectangleProvider &prov;
	};

	/**
	* This class allows instancing of a rectangle like image that is made out of three
	* parts. The first part is the start of the rectangle, the second part is the middle
	* and the third part is the end of the rectangle. Middle part can be repeated or
	* stretched. A rectangle provider can have empty animations. Provider will use
	* EmptyImage for missing parts. A_ must derive from RectangularAnimationProvider.
	* For best results, try to keep size of parts compatible. If an image is smaller, it
	* will be placed towards the center. Part names follows a logic, first letter is
	* vertical location, second is horizontal. For instance TL is top left, ML is directly
	* the left of the middle section. Any part that has an M in it will be scaled/tiled.
	*/
	template<class A_>
	class basic_RectangleProvider : public IRectangleProvider {
	public:
		using AnimationType = Rectangle;

		/// Empty constructor, rectangle can be instanced even if it is completely empty
		basic_RectangleProvider() : IRectangleProvider() {}
		
		basic_RectangleProvider(const basic_RectangleProvider &) = delete;

		/// Filling constructor
		basic_RectangleProvider(
			A_  &tl, A_ &tm, A_ &tr,
			A_  &ml, A_ &mm, A_ &mr,
			A_  &bl, A_ &bm, A_ &br
		) : 
			tl(&tl), tm(&tm), tr(&tr),
			ml(&ml), mm(&mm), mr(&mr),
			bl(&bl), bm(&bm), br(&br) 
        { }

		/// Filling constructor using move semantics, rectangle will create and own
		/// new objects. The given objects will be moved to these new objects.
		basic_RectangleProvider(
			A_  &&tl, A_ &&tm, A_ &&tr,
			A_  &&ml, A_ &&mm, A_ &&mr,
			A_  &&bl, A_ &&bm, A_ &&br
		) : 
			tl(new A_(std::move(tl))), tm(new A_(std::move(tm))), tr(new A_(std::move(tr))),
			ml(new A_(std::move(ml))), mm(new A_(std::move(mm))), mr(new A_(std::move(mr))),
			bl(new A_(std::move(bl))), bm(new A_(std::move(bm))), br(new A_(std::move(br))),
			own(true)
        { }

		/// Filling constructor. The given object will be used for the borders. Center
		/// will be empty
		explicit basic_RectangleProvider(
			A_  &border
		) : 
			tl(&border), tm(&border), tr(&border),
			ml(&border), mm(nullptr), mr(&border),
			bl(&border), bm(&border), br(&border) 
        { }
        
		/// Filling constructor using move semantics, rectangle will create and own
		/// new object. The given object will be moved to this new object. The given
		/// object will be used for the borders
		explicit basic_RectangleProvider(
			A_  &&border
		) : 
			tl(new A_(std::move(border))), tm(tl), tr(tl),
			ml(tl), mm(nullptr), mr(tl),
			bl(tl), bm(tl), br(tl),
			own(true)
        { }

		/// Filling constructor. The first given object will be used for the borders. The
		/// second will be used for the center
		basic_RectangleProvider(
			A_  &border, A_ &center
		) : 
			tl(&border), tm(&border), tr(&border),
			ml(&border), mm(&center), mr(&border),
			bl(&border), bm(&border), br(&border) 
        { }

		/// Filling constructor using move semantics, rectangle will create and own
		/// new objects. The given objects will be moved to these new objects. The first
		/// object will be used for the borders, the second will be used for center
		explicit basic_RectangleProvider(
			A_  &&border, A_ &&center
		) : 
			tl(new A_(std::move(border))), tm(tl), tr(tl),
			ml(tl), mm(new A_(std::move(center))), mr(tl),
			bl(tl), bm(tl), br(tl),
			own(true)
        { }

		/// Filling constructor, nullptr is acceptable
		basic_RectangleProvider(
			A_  *tl, A_ *tm, A_ *tr,
			A_  *ml, A_ *mm, A_ *mr,
			A_  *bl, A_ *bm, A_ *br
		) :
			tl(tl), tm(tm), tr(tr),
			ml(ml), mm(mm), mr(mr),
			bl(bl), bm(bm), br(br) 
        { }

		/// Move constructor
		basic_RectangleProvider(basic_RectangleProvider &&other) :
			tl(other.tl), tm(other.tm), tr(other.tr), 
			ml(other.ml), mm(other.mm), mr(other.mr), 
			bl(other.bl), bm(other.bm), br(other.br), 
			own(other.own)
		{
			other.own = false;
            
			other.tl = nullptr;
			other.tm = nullptr;
			other.tr = nullptr;
            
			other.ml = nullptr;
			other.mm = nullptr;
			other.mr = nullptr;
            
			other.bl = nullptr;
			other.bm = nullptr;
			other.br = nullptr;
            
            SetCenterTiling(other.GetCenterTiling());
            SetSideTiling(other.GetSideTiling());
		}

		~basic_RectangleProvider() {
			if(own) {
                //delete unique ones
                std::set<A_*> ptrs = {tl, tm, tr, ml, mm, mr, bl, bm, br};
                
                for(auto ptr : ptrs) {
                    delete ptr;
                }
            }
		}
		
		
		
		basic_RectangleProvider &operator =(const basic_RectangleProvider &) = delete;

        //types are derived not to type the same code for every class
		virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new typename std::remove_reference<decltype(*this)>::type(std::move(*this));
            
            return *ret;
        }

		Rectangle &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
			return *new Rectangle(*this, timer);
		}

		Rectangle &CreateAnimation(bool create = true) const override {
			return *new Rectangle(*this, create);
		}

		virtual RectangularAnimation &CreateTL() const override {
			if(tl)
				return tl->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}


		virtual RectangularAnimation &CreateTM() const override {
			if(tm)
				return tm->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}


		virtual RectangularAnimation &CreateTR() const override {
			if(tr)
				return tr->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}


		virtual RectangularAnimation &CreateML() const override {
			if(ml)
				return ml->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}


		virtual RectangularAnimation &CreateMM() const override {
			if(mm)
				return mm->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}


		virtual RectangularAnimation &CreateMR() const override {
			if(mr)
				return mr->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}


		virtual RectangularAnimation &CreateBL() const override {
			if(bl)
				return bl->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}


		virtual RectangularAnimation &CreateBM() const override {
			if(bm)
				return bm->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}


		virtual RectangularAnimation &CreateBR() const override {
			if(br)
				return br->CreateAnimation(false);
			else
				return EmptyImage::Instance();
		}

		/// Returns TL provider, may return nullptr.
		A_ *GetTL() const {
            return tl;
        }

		/// Returns TM provider, may return nullptr.
		A_ *GetTM() const {
            return tm;
        }

		/// Returns TR provider, may return nullptr.
		A_ *GetTR() const {
            return tr;
        }

		/// Returns ML provider, may return nullptr.
		A_ *GetML() const {
            return ml;
        }

		/// Returns MM provider, may return nullptr.
		A_ *GetMM() const {
            return mm;
        }

		/// Returns MR provider, may return nullptr.
		A_ *GetMR() const {
            return mr;
        }

		/// Returns BL provider, may return nullptr.
		A_ *GetBL() const {
            return bl;
        }

		/// Returns BM provider, may return nullptr.
		A_ *GetBM() const {
            return bm;
        }

		/// Returns BR provider, may return nullptr.
		A_ *GetBR() const {
            return br;
        }
		/// Changes the TL animation, ownership semantics will not change
		void SetTL(A_ *value) {
            if(own)
                delete tl;
            
            tl = value;
        }

		/// Changes the TM animation, ownership semantics will not change
		void SetTM(A_ *value) {
            if(own)
                delete tm;
            
            tm = value;
        }

		/// Changes the TR animation, ownership semantics will not change
		void SetTR(A_ *value) {
            if(own)
                delete tr;
            
            tr = value;
        }

		/// Changes the ML animation, ownership semantics will not change
		void SetML(A_ *value) {
            if(own)
                delete ml;
            
            ml = value;
        }

		/// Changes the MM animation, ownership semantics will not change
		void SetMM(A_ *value) {
            if(own)
                delete mm;
            
            mm = value;
        }

		/// Changes the MR animation, ownership semantics will not change
		void SetMR(A_ *value) {
            if(own)
                delete mr;
            
            mr = value;
        }

		/// Changes the BL animation, ownership semantics will not change
		void SetBL(A_ *value) {
            if(own)
                delete bl;
            
            bl = value;
        }

		/// Changes the BM animation, ownership semantics will not change
		void SetBM(A_ *value) {
            if(own)
                delete bm;
            
            bm = value;
        }

		/// Changes the BR animation, ownership semantics will not change
		void SetBR(A_ *value) {
            if(own)
                delete br;
            
            br = value;
        }

		/// Prepares all animation providers if the they support Prepare function.
		void Prepare() {
			if(tl)
				tl->Prepare();
			if(tm)
				tm->Prepare();
			if(tr)
				tr->Prepare();

			if(ml)
				ml->Prepare();
			if(mm)
				mm->Prepare();
			if(mr)
				mr->Prepare();

			if(bl)
				bl->Prepare();
			if(bm)
				bm->Prepare();
			if(br)
				br->Prepare();
		}

		/// Issuing this function will make this rectangle to own its providers,
		/// destroying them along with itself.
		void OwnProviders() {
			own = true;
		}

		Geometry::Size GetSize() const override {
			int maxt = std::max(std::max(
				tl ? tl->GetHeight() : 0 , 
				tm ? tm->GetHeight() : 0), 
				tr ? tr->GetHeight() : 0);

			int maxb = std::max(std::max(
				bl ? bl->GetHeight() : 0 , 
				bm ? bm->GetHeight() : 0), 
				br ? br->GetHeight() : 0);

			int maxl = std::max(std::max(
				tl ? tl->GetWidth() : 0 , 
				ml ? ml->GetWidth() : 0), 
				bl ? bl->GetWidth() : 0);

			int maxr = std::max(std::max(
				tr ? tr->GetWidth() : 0 , 
				mr ? mr->GetWidth() : 0), 
				br ? br->GetWidth() : 0);

			return{maxl+maxr+(mm ? mm->GetWidth() : 0), maxt+maxb+(mm ? mm->GetHeight() : 0)};
		}
		
        
        

	private:
		A_ *tl = nullptr;
		A_ *tm = nullptr;
		A_ *tr = nullptr;

		A_ *ml = nullptr;
		A_ *mm = nullptr;
		A_ *mr = nullptr;

		A_ *bl = nullptr;
		A_ *bm = nullptr;
		A_ *br = nullptr;

		bool own = false;
	};
    
    using RectangleProvider = basic_RectangleProvider<RectangularAnimationProvider>;
    using TextureRectangleProvider = basic_RectangleProvider<TextureProvider>;
	using BitmapRectangleProvider = basic_RectangleProvider<Bitmap>;
	using AnimatedBitmapRectangleProvider = basic_RectangleProvider<BitmapAnimationProvider>;

    

    /// Horizontally slices the given image. t and b slices the image to 3 parts then each part is further sliced by 
    /// X offset pairs (tl, tr), (l, r), and (bl, br). Currently this works only with bitmaps. This function will create
    /// an atlas out of the given image, thus, the source should be kept alive. Currently atlas functionality does not
    /// work.
    BitmapRectangleProvider SliceHorizontal(const Bitmap &source, int t, int b, int tl, int tr, int l, int r, int bl, int br);
    
    /// Vertically slices the given image. l and r slices the image to 3 parts then each part is further sliced by
    /// Y offset pairs (tl, bl), (t, b), and (tr, br). Currently this works only with bitmaps. This function will create 
    /// an atlas out of the given image, thus, the source should be kept alive. Currently atlas functionality does not
    /// work.
    BitmapRectangleProvider SliceVertical(const Bitmap &source, int l, int r, int tl, int bl, int t, int b, int tr, int br);
    
    /// Slices an image to create a rectangle. Imagine a pound sign (#) slicing the image, center parameter is the central region of the
    /// pound sign. Currently this works only with bitmaps. This function will create an atlas out of the given image, thus, the source should
    /// be kept alive. Currently atlas functionality does not work.
    inline BitmapRectangleProvider Slice(const Bitmap &source, Geometry::Bounds center) {
        return SliceHorizontal(source, center.Top, center.Bottom, center.Left, center.Right, center.Left, center.Right, center.Left, center.Right);
    }
    
}
