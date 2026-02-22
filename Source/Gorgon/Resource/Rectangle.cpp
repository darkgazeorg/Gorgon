#include "Rectangle.h"
#include "Animation.h"
#include "Image.h"
#include "File.h"
#include "Reader.h"
#include "Null.h"
#include "AnimationServices.h"

namespace Gorgon :: Resource {

	
	Rectangle::Rectangle(Graphics::BitmapRectangleProvider &prov) : prov(&prov)
	{ }
	
	Rectangle::Rectangle(Graphics::AnimatedBitmapRectangleProvider &prov) : prov(&prov)
	{ }
	
	Rectangle::Rectangle(Graphics::RectangleProvider &prov) : prov(&prov)
	{ }

	template<class T_>
	static Graphics::basic_RectangleProvider<T_> &fillfrom(Containers::Collection<Base> &children) {
		T_ *tl = nullptr, *tm = nullptr, *tr = nullptr;
		T_ *bl = nullptr, *bm = nullptr, *br = nullptr;
		T_ *ml = nullptr, *mm = nullptr, *mr = nullptr;

		if(children.GetCount() == 1) {
			mm = dynamic_cast<T_*>(children.First().CurrentPtr());
		}
		else if(children.GetCount() == 4 || children.GetCount() == 5) {
			int n=0;
			tm = dynamic_cast<T_*>(&children[n++]);
			ml = dynamic_cast<T_*>(&children[n++]);
			if(children.GetCount() == 5)
				mm = dynamic_cast<T_*>(&children[n++]);

			mr = dynamic_cast<T_*>(&children[n++]);
			bm = dynamic_cast<T_*>(&children[n++]);
		}
		else if(children.GetCount() >= 8) {
			int n=0;
			if(children[n].GetGID() != GID::Null)
				tl = dynamic_cast<T_*>(&children[n]);
			n++;

			if(children[n].GetGID() != GID::Null)
				tm = dynamic_cast<T_*>(&children[n]);
			n++;

			if(children[n].GetGID() != GID::Null)
				tr = dynamic_cast<T_*>(&children[n]);
			n++;

			if(children[n].GetGID() != GID::Null)
				ml = dynamic_cast<T_*>(&children[n]);
			n++;

			if(children.GetCount() == 9) {
				if(children[n].GetGID() != GID::Null)
					mm = dynamic_cast<T_*>(&children[n]);
				n++;
			}
			if(children[n].GetGID() != GID::Null)
				mr = dynamic_cast<T_*>(&children[n]);
			n++;

			if(children[n].GetGID() != GID::Null)
				bl = dynamic_cast<T_*>(&children[n]);
			n++;

			if(children[n].GetGID() != GID::Null)
				bm = dynamic_cast<T_*>(&children[n]);
			n++;

			if(children[n].GetGID() != GID::Null)
				br = dynamic_cast<T_*>(&children[n]);
			n++;
		}

		return *new Graphics::basic_RectangleProvider<T_>(tl, tm, tr, ml, mm, mr, bl, bm, br);
	}

	Rectangle *Rectangle::LoadResource(std::weak_ptr<File> f, std::shared_ptr<Reader> reader, unsigned long totalsize) {
		auto target = reader->Target(totalsize);

		auto file = f.lock();

		auto rectangle = new Rectangle();
		bool ctile = false, stile = false;
		enum {
			unknown, img, anim, mixed
		} type = unknown;
		int c = 0;

		while(!target) {
			auto gid = reader->ReadGID();
			auto size= reader->ReadChunkSize();

			if(gid == GID::Rectangle_Props) {
				reader->ReadGuid();
				ctile = reader->ReadBool();
                reader->ReadBool(); //unused
                stile = reader->ReadBool();
                reader->ReadBool();
                reader->ReadBool();
                reader->ReadBool();
			}
			else if(gid == GID::Rectangle_Props_II) {
				ctile = reader->ReadBool();
				stile = reader->ReadBool();
            }
			else {
				auto resource = file->LoadChunk(*rectangle, gid, size, false);

				if(resource) {
					if(resource->GetGID() == GID::Image) {
						if(type == unknown)
							type = img;
						else if(type == anim)
							type = mixed;

					}
					else if(resource->GetGID() == GID::Animation) {
						if(type == unknown)
							type = anim;
						else if(type == img)
							type = mixed;
					}
					else if(resource->GetGID() == GID::Null) {
						//null is allowed
					}
					else if(IsAnimation(resource->GetGID())) {
						type = mixed;
					}
					else {
						throw std::runtime_error("Rectangle can only contain images or animations");
					}

					if(++c > 9) {
						throw std::runtime_error("Rectangle cannot have more than 9 parts");
					}

					rectangle->children.Add(resource);
				}
			}
		}

		if(type == anim) {
			rectangle->SetProvider(
				fillfrom<Graphics::BitmapAnimationProvider>(rectangle->children)
			);
		}
		else if(type == img) {
			rectangle->SetProvider(
				fillfrom<Graphics::Bitmap>(rectangle->children)
			);
		}
		else if(type == mixed) {
			rectangle->SetProvider(
				fillfrom<Graphics::RectangularAnimationProvider>(rectangle->children)
			);
		}
		//else empty rectangle

		if(type != unknown) {
			rectangle->SetCenterTiling(ctile);
			rectangle->SetSideTiling(stile);
        }

		rectangle->own = true;
		return rectangle;
	}

	template <class T_>
	static void savethis(Writer &writer, const Graphics::basic_RectangleProvider<T_> &provider) {
		writer.WriteChunkHeader(GID::Rectangle_Props_II, 2 * 4);
		writer.WriteBool(provider.GetCenterTiling());
		writer.WriteBool(provider.GetSideTiling());

		auto tl = provider.GetTL();
		auto tm = provider.GetTM();
		auto tr = provider.GetTR();
		auto ml = provider.GetML();
		auto mm = provider.GetMM();
		auto mr = provider.GetMR();
		auto bl = provider.GetBL();
		auto bm = provider.GetBM();
		auto br = provider.GetBR();

		if(tm && ml && mr && bm && !tl && !tr && !bl && !br) {
			SaveAnimation(writer, tm);
			SaveAnimation(writer, ml);
			if(mm)
				SaveAnimation(writer, mm);
			SaveAnimation(writer, mr);
			SaveAnimation(writer, bm);
		}
		else if(tm && ml && mr && bm && tl && tr && bl && br) {
			SaveAnimation(writer, tl);
			SaveAnimation(writer, tm);
			SaveAnimation(writer, tr);
			SaveAnimation(writer, ml);
			if(mm)
				SaveAnimation(writer, mm);
			SaveAnimation(writer, mr);
			SaveAnimation(writer, bl);
			SaveAnimation(writer, bm);
			SaveAnimation(writer, br);
		}
		else if(!tr && !tm && !tr && !ml && !mr && !bl && !bm && !br) {
			if(mm)
				SaveAnimation(writer, mm);
		}
		else {
			//non standard

			SaveAnimation(writer, tl);
			SaveAnimation(writer, tm);
			SaveAnimation(writer, tr);

			SaveAnimation(writer, ml);
			SaveAnimation(writer, mm);
			SaveAnimation(writer, mr);

			SaveAnimation(writer, bl);
			SaveAnimation(writer, bm);
			SaveAnimation(writer, br);
		}
	}

	void Rectangle::save(Writer &writer) const {
		auto start = writer.WriteObjectStart(this);

		if(dynamic_cast<Graphics::BitmapRectangleProvider*>(prov)) {
			savethis(writer, *dynamic_cast<Graphics::BitmapRectangleProvider*>(prov));
		}
		else if(dynamic_cast<Graphics::AnimatedBitmapRectangleProvider*>(prov)) {
			savethis(writer, *dynamic_cast<Graphics::AnimatedBitmapRectangleProvider*>(prov));
		}
		else if(dynamic_cast<Graphics::RectangleProvider*>(prov)) {
			savethis(writer, *dynamic_cast<Graphics::RectangleProvider*>(prov));
		}
		else if(prov != nullptr) {
			throw std::runtime_error("Unknown line provider");
		}

		writer.WriteEnd(start);
	}

	void Rectangle::SaveThis(Writer &writer, const Graphics::IRectangleProvider &provider) {
		auto start = writer.WriteChunkStart(GID::Rectangle);
		auto prov = &provider;

		if(dynamic_cast<const Graphics::BitmapRectangleProvider*>(prov)) {
			savethis(writer, *dynamic_cast<const Graphics::BitmapRectangleProvider*>(prov));
		}
		else if(dynamic_cast<const Graphics::AnimatedBitmapRectangleProvider*>(prov)) {
			savethis(writer, *dynamic_cast<const Graphics::AnimatedBitmapRectangleProvider*>(prov));
		}
		else if(dynamic_cast<const Graphics::RectangleProvider*>(prov)) {
			savethis(writer, *dynamic_cast<const Graphics::RectangleProvider*>(prov));
		}
		else {
			throw std::runtime_error("Unknown line provider");
		}

		writer.WriteEnd(start);
	}

	template<class T_, class F_>
	static void setthis(F_, Graphics::basic_RectangleProvider<T_> *, T_ *) {
		Utils::ASSERT_FALSE("Should not run");
	}

	template<class F_>
	static void setthis(F_ f, Graphics::BitmapRectangleProvider *provider, Graphics::Bitmap *o) {
		CallBitmapAnimationSetter(f, provider, o);
	}

	template<class F_>
	static void setthis(F_ f, Graphics::AnimatedBitmapRectangleProvider *provider, Graphics::BitmapAnimationProvider *o) {
		CallBitmapAnimationAnimationSetter(f, provider, o);
	}

	template<class F_>
	static void setthis(F_ f, Graphics::RectangleProvider *provider, Graphics::RectangularAnimationProvider *o) {
		CallGenericAnimationSetter(f, provider, o);
	}

	template<class T_>
	static void moveout(Graphics::basic_RectangleProvider<T_> *provider, Graphics::IRectangleProvider *&p) {
		auto bp = new Graphics::basic_RectangleProvider<T_>;
		p = bp;
		bp->SetCenterTiling(provider->GetCenterTiling());
		bp->SetSideTiling(provider->GetSideTiling());

		auto tl = provider->GetTL();
		auto tm = provider->GetTM();
		auto tr = provider->GetTR();
		auto ml = provider->GetML();
		auto mm = provider->GetMM();
		auto mr = provider->GetMR();
		auto bl = provider->GetBL();
		auto bm = provider->GetBM();
		auto br = provider->GetBR();

		setthis(&Graphics::basic_RectangleProvider<T_>::SetTL, bp, tl);
		setthis(&Graphics::basic_RectangleProvider<T_>::SetTM, bp, tm);
		setthis(&Graphics::basic_RectangleProvider<T_>::SetTR, bp, tr);
		setthis(&Graphics::basic_RectangleProvider<T_>::SetML, bp, ml);
		setthis(&Graphics::basic_RectangleProvider<T_>::SetMM, bp, mm);
		setthis(&Graphics::basic_RectangleProvider<T_>::SetMR, bp, mr);
		setthis(&Graphics::basic_RectangleProvider<T_>::SetBL, bp, bl);
		setthis(&Graphics::basic_RectangleProvider<T_>::SetBM, bp, bm);
		setthis(&Graphics::basic_RectangleProvider<T_>::SetBR, bp, br);

		bp->OwnProviders();
	}

    Graphics::RectangularAnimationStorage Rectangle::animmoveout() {
        return Graphics::RectangularAnimationStorage(MoveOutProvider(), true);
    }
    
    Graphics::IRectangleProvider &Rectangle::MoveOutProvider() {
		if(!prov)
			throw std::runtime_error("Provider is not set");

		IRectangleProvider *p = nullptr;


		if(dynamic_cast<Graphics::BitmapRectangleProvider*>(prov)) {
			auto provider = dynamic_cast<Graphics::BitmapRectangleProvider*>(prov);
			moveout(provider, p);
		}
		else if(dynamic_cast<Graphics::AnimatedBitmapRectangleProvider*>(prov)) {
			auto provider = dynamic_cast<Graphics::AnimatedBitmapRectangleProvider*>(prov);
			moveout(provider, p);
		}
		else if(dynamic_cast<Graphics::RectangleProvider*>(prov)) {
			auto provider = dynamic_cast<Graphics::RectangleProvider*>(prov);
			moveout(provider, p);
		}

		if(!p)
			throw std::runtime_error("Provider is not set");

		for(auto &child : children) {
			child.DeleteResource();
		}

		children.Clear();

		if(own)
			delete prov;

		prov = nullptr;


		if(!p)
			throw std::runtime_error("Provider is not set");

		return *p;
	}

}
