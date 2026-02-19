#include "TintedObject.h"
#include "Animation.h"
#include "Image.h"
#include "File.h"
#include "Reader.h"
#include "Null.h"
#include "AnimationServices.h"

namespace Gorgon :: Resource {


    TintedObject::TintedObject(Graphics::TintedBitmapProvider &prov) : prov(&prov)
    { }

    TintedObject::TintedObject(Graphics::TintedBitmapAnimationProvider &prov) : prov(&prov)
    { }

    TintedObject::TintedObject(Graphics::TintedObjectProvider &prov) : prov(&prov)
    { }

    template<class T_>
    static Graphics::basic_TintedObjectProvider<T_> &fillfrom(Containers::Collection<Base> &children) {
        T_ *b = nullptr;

        if(children.GetCount() == 1) {
            b = dynamic_cast<T_*>(&children[0]);
        }

        if(b)
            return *new Graphics::basic_TintedObjectProvider<T_>(*b);
        else
            return *new Graphics::basic_TintedObjectProvider<T_>();
    }

    TintedObject *TintedObject::LoadResource(std::weak_ptr<File> f, std::shared_ptr<Reader> reader, unsigned long totalsize) {
        auto target = reader->Target(totalsize);

        auto file = f.lock();

        auto tinted = new TintedObject();

        enum {
            unknown, img, anim, mixed
        } type = unknown;
        int c = 0;
        
        Graphics::RGBAf color = 1.f;

        while(!target) {
            auto gid = reader->ReadGID();
            auto size= reader->ReadChunkSize();

            if(gid == GID::TintedObject_Props) {
                color = reader->ReadRGBAf();
            }
            else {
                auto resource = file->LoadChunk(*tinted, gid, size, false);

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
                        throw std::runtime_error("TintedObject can only contain images or animations");
                    }

                    if(++c > 1) {
                        throw std::runtime_error("TintedObject cannot have more than 1 part");
                    }

                    tinted->children.Add(resource);
                }
            }
        }

        if(type == anim) {
            tinted->SetProvider(
                fillfrom<Graphics::BitmapAnimationProvider>(tinted->children)
            );
        }
        else if(type == img) {
            tinted->SetProvider(
                fillfrom<Graphics::Bitmap>(tinted->children)
            );
        }
        else if(type == mixed) {
            tinted->SetProvider(
                fillfrom<Graphics::RectangularAnimationProvider>(tinted->children)
            );
        }
        //else empty tinted object
        
        if(type != unknown)
            tinted->SetColor(color);

        tinted->own = true;
        return tinted;
    }

    template <class T_>
    static void savethis(Writer &writer, const Graphics::basic_TintedObjectProvider<T_> &provider) {
        auto b = provider.GetBase();

        writer.WriteChunkHeader(GID::TintedObject_Props, 4 * 4);
        writer.WriteRGBAf(provider.GetColor());

        if(b) {
            SaveAnimation(writer, b);
        }
    }

    void TintedObject::save(Writer &writer) const {
        auto start = writer.WriteObjectStart(this);

        if(dynamic_cast<Graphics::TintedBitmapProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::TintedBitmapProvider*>(prov));
        }
        else if(dynamic_cast<Graphics::TintedBitmapAnimationProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::TintedBitmapAnimationProvider*>(prov));
        }
        else if(dynamic_cast<Graphics::TintedObjectProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::TintedObjectProvider*>(prov));
        }
        else if(prov != nullptr) {
            throw std::runtime_error("Unknown tinted provider");
        }

        writer.WriteEnd(start);
    }

    void TintedObject::SaveThis(Writer &writer, const Graphics::ITintedObjectProvider &provider) {
        auto start = writer.WriteChunkStart(GID::TintedObject);
        auto prov = &provider;

        if(dynamic_cast<const Graphics::TintedBitmapProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::TintedBitmapProvider*>(prov));
        }
        else if(dynamic_cast<const Graphics::TintedBitmapAnimationProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::TintedBitmapAnimationProvider*>(prov));
        }
        else if(dynamic_cast<const Graphics::TintedObjectProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::TintedObjectProvider*>(prov));
        }
        else {
            throw std::runtime_error("Unknown tinted provider");
        }

        writer.WriteEnd(start);
    }

    template<class T_, class F_>
    static void setthis(F_, Graphics::basic_TintedObjectProvider<T_> *, T_ *) {
        Utils::ASSERT_FALSE("Should not run");
    }

    template<class F_>
    static void setthis(F_ f, Graphics::TintedBitmapProvider *provider, Graphics::Bitmap *o) {
		CallBitmapAnimationSetter(f, provider, o);
    }

    template<class F_>
    static void setthis(F_ f, Graphics::TintedBitmapAnimationProvider *provider, Graphics::BitmapAnimationProvider *o) {
		CallBitmapAnimationAnimationSetter(f, provider, o);
    }

    template<class F_>
    static void setthis(F_ f, Graphics::TintedObjectProvider *provider, Graphics::RectangularAnimationProvider *o) {
		CallGenericAnimationSetter(f, provider, o);
    }

    template<class T_>
    static void moveout(Graphics::basic_TintedObjectProvider<T_> *provider, Graphics::ITintedObjectProvider *&p) {
        auto bp = new Graphics::basic_TintedObjectProvider<T_>();
        p = bp;

        auto b = provider->GetBase();

        setthis(&Graphics::basic_TintedObjectProvider<T_>::SetBase, bp, b);
        bp->SetColor(provider->GetColor());

        bp->OwnProvider();
    }

    Graphics::RectangularAnimationStorage TintedObject::animmoveout() {
        return Graphics::RectangularAnimationStorage(MoveOutProvider(), true);
    }
    
    Graphics::ITintedObjectProvider &TintedObject::MoveOutProvider() {
        if(!prov)
            throw std::runtime_error("Provider is not set");

        ITintedObjectProvider *p = nullptr;


        if(dynamic_cast<Graphics::TintedBitmapProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::TintedBitmapProvider*>(prov);
            moveout(provider, p);
        }
        else if(dynamic_cast<Graphics::TintedBitmapAnimationProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::TintedBitmapAnimationProvider*>(prov);
            moveout(provider, p);
        }
        else if(dynamic_cast<Graphics::TintedObjectProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::TintedObjectProvider*>(prov);
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
