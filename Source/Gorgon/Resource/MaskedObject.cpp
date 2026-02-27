#include "MaskedObject.h"
#include "Animation.h"
#include "Image.h"
#include "File.h"
#include "Reader.h"
#include "Null.h"
#include "AnimationStorage.h"
#include "AnimationServices.h"

namespace Gorgon :: Resource {


    MaskedObject::MaskedObject(Graphics::MaskedBitmapProvider &prov) : prov(&prov)
    { }

    MaskedObject::MaskedObject(Graphics::MaskedBitmapAnimationProvider &prov) : prov(&prov)
    { }

    MaskedObject::MaskedObject(Graphics::MaskedObjectProvider &prov) : prov(&prov)
    { }

    template<class T_>
    static Graphics::basic_MaskedObjectProvider<T_> &fillfrom(Containers::Collection<Base> &children) {
        T_ *b = nullptr, *m = nullptr;

        if(children.GetCount() == 1) {
            b = dynamic_cast<T_*>(&children[0]);
        }
        else if(children.GetCount() == 2) {
            b = dynamic_cast<T_*>(&children[0]);
            m = dynamic_cast<T_*>(&children[1]);
        }

        return *new Graphics::basic_MaskedObjectProvider<T_>(b, m);
    }

    MaskedObject *MaskedObject::LoadResource(std::weak_ptr<File> f, std::shared_ptr<Reader> reader, unsigned long totalsize) {
        auto target = reader->Target(totalsize);

        auto file = f.lock();

        auto masked = new MaskedObject();

        enum {
            unknown, img, anim, mixed
        } type = unknown;
        int c = 0;

        while(!target) {
            auto gid = reader->ReadGID();
            auto size= reader->ReadChunkSize();

            auto resource = file->LoadChunk(*masked, gid, size, false);

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
                    throw std::runtime_error("MaskedObject can only contain images or animations");
                }

                if(++c > 2) {
                    throw std::runtime_error("MaskedObject cannot have more than 2 parts");
                }

                masked->children.Add(resource);
            }
        }

        if(type == anim) {
            masked->SetProvider(
                fillfrom<Graphics::BitmapAnimationProvider>(masked->children)
            );
        }
        else if(type == img) {
            masked->SetProvider(
                fillfrom<Graphics::Bitmap>(masked->children)
            );
        }
        else if(type == mixed) {
            masked->SetProvider(
                fillfrom<Graphics::RectangularAnimationProvider>(masked->children)
            );
        }
        //else empty masked object

        masked->own = true;
        return masked;
    }

    template <class T_>
    static void savethis(Writer &writer, const Graphics::basic_MaskedObjectProvider<T_> &provider) {
        auto b = provider.GetBase();
        auto m = provider.GetMask();

        if(!m) {
            SaveAnimation(writer, m);
        }
        else {
            SaveAnimation(writer, b);
            SaveAnimation(writer, m);
        }
    }

    void MaskedObject::save(Writer &writer) const {
        auto start = writer.WriteObjectStart(this);

        if(dynamic_cast<Graphics::MaskedBitmapProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::MaskedBitmapProvider*>(prov));
        }
        else if(dynamic_cast<Graphics::MaskedBitmapAnimationProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::MaskedBitmapAnimationProvider*>(prov));
        }
        else if(dynamic_cast<Graphics::MaskedObjectProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::MaskedObjectProvider*>(prov));
        }
        else if(prov != nullptr) {
            throw std::runtime_error("Unknown masked provider");
        }

        writer.WriteEnd(start);
    }

    void MaskedObject::SaveThis(Writer &writer, const Graphics::IMaskedObjectProvider &provider) {
        auto start = writer.WriteChunkStart(GID::MaskedObject);
        auto prov = &provider;

        if(dynamic_cast<const Graphics::MaskedBitmapProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::MaskedBitmapProvider*>(prov));
        }
        else if(dynamic_cast<const Graphics::MaskedBitmapAnimationProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::MaskedBitmapAnimationProvider*>(prov));
        }
        else if(dynamic_cast<const Graphics::MaskedObjectProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::MaskedObjectProvider*>(prov));
        }
        else {
            throw std::runtime_error("Unknown masked provider");
        }

        writer.WriteEnd(start);
    }

    template<class T_, class F_>
    static void setthis(F_, Graphics::basic_MaskedObjectProvider<T_> *, T_ *) {
        Utils::ASSERT_FALSE("Should not run");
    }

    template<class F_>
    static void setthis(F_ f, Graphics::MaskedBitmapProvider *provider, Graphics::Bitmap *o) {
        CallBitmapAnimationSetter(f, provider, o);
    }

    template<class F_>
    static void setthis(F_ f, Graphics::MaskedBitmapAnimationProvider *provider, Graphics::BitmapAnimationProvider *o) {
        CallBitmapAnimationAnimationSetter(f, provider, o);
    }

    template<class F_>
    static void setthis(F_ f, Graphics::MaskedObjectProvider *provider, Graphics::RectangularAnimationProvider *o) {
        CallGenericAnimationSetter(f, provider, o);
    }

    template<class T_>
    static void moveout(Graphics::basic_MaskedObjectProvider<T_> *provider, Graphics::IMaskedObjectProvider *&p) {
        auto bp = new Graphics::basic_MaskedObjectProvider<T_>();
        p = bp;

        auto b = provider->GetBase();
        auto m = provider->GetMask();

        setthis(&Graphics::basic_MaskedObjectProvider<T_>::SetBase, bp, b);
        setthis(&Graphics::basic_MaskedObjectProvider<T_>::SetMask, bp, m);

        bp->OwnProviders();
    }

    Graphics::RectangularAnimationStorage MaskedObject::animmoveout() {
        return Graphics::RectangularAnimationStorage(MoveOutProvider(), true);
    }
    
    Graphics::IMaskedObjectProvider &MaskedObject::MoveOutProvider() {
        if(!prov)
            throw std::runtime_error("Provider is not set");

        IMaskedObjectProvider *p = nullptr;


        if(dynamic_cast<Graphics::MaskedBitmapProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::MaskedBitmapProvider*>(prov);
            moveout(provider, p);
        }
        else if(dynamic_cast<Graphics::MaskedBitmapAnimationProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::MaskedBitmapAnimationProvider*>(prov);
            moveout(provider, p);
        }
        else if(dynamic_cast<Graphics::MaskedObjectProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::MaskedObjectProvider*>(prov);
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
