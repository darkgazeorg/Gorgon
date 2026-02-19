#include "ScalableObject.h"
#include "Animation.h"
#include "Image.h"
#include "File.h"
#include "Reader.h"
#include "Null.h"
#include "AnimationServices.h"

namespace Gorgon :: Resource {

    ScalableObject::ScalableObject(Graphics::ScalableBitmapProvider &prov) : prov(&prov)
    { }

    ScalableObject::ScalableObject(Graphics::ScalableBitmapAnimationProvider &prov) : prov(&prov)
    { }

    ScalableObject::ScalableObject(Graphics::ScalableObjectProvider &prov) : prov(&prov)
    { }

    template<class T_>
    static Graphics::basic_ScalableObjectProvider<T_> &fillfrom(Containers::Collection<Base> &children) {
        T_ *b = nullptr;

        if(children.GetCount() == 1) {
            b = dynamic_cast<T_*>(&children[0]);
        }

        if(b)
            return *new Graphics::basic_ScalableObjectProvider<T_>(*b);
        else
            return *new Graphics::basic_ScalableObjectProvider<T_>();
    }

    ScalableObject *ScalableObject::LoadResource(std::weak_ptr<File> f, std::shared_ptr<Reader> reader, unsigned long totalsize) {
        auto target = reader->Target(totalsize);

        auto file = f.lock();

        auto tinted = new ScalableObject();

        enum {
            unknown, img, anim, mixed
        } type = unknown;
        int c = 0;
        
        Graphics::SizeController s;

        while(!target) {
            auto gid = reader->ReadGID();
            auto size= reader->ReadChunkSize();

            if(gid == GID::ScalableObject_Props) {
                s.Horizontal = reader->ReadEnum32<Graphics::SizeController::Tiling>();
                s.Vertical   = reader->ReadEnum32<Graphics::SizeController::Tiling>();
                s.Place      = reader->ReadEnum32<Graphics::Placement>();
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
                        throw std::runtime_error("ScalableObject can only contain images or animations");
                    }

                    if(++c > 1) {
                        throw std::runtime_error("ScalableObject cannot have more than 1 part");
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
            tinted->SetController(s);

        tinted->own = true;
        return tinted;
    }

    template <class T_>
    static void savethis(Writer &writer, const Graphics::basic_ScalableObjectProvider<T_> &provider) {
        auto b = provider.GetBase();

        writer.WriteChunkHeader(GID::ScalableObject_Props, 3 * 4);
        
        auto s = provider.GetController();
        writer.WriteEnum32(s.Horizontal);
        writer.WriteEnum32(s.Vertical);
        writer.WriteEnum32(s.Place);

        if(b) {
            SaveAnimation(writer, b);
        }
    }

    void ScalableObject::save(Writer &writer) const {
        auto start = writer.WriteObjectStart(this);

        if(dynamic_cast<Graphics::ScalableBitmapProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::ScalableBitmapProvider*>(prov));
        }
        else if(dynamic_cast<Graphics::ScalableBitmapAnimationProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::ScalableBitmapAnimationProvider*>(prov));
        }
        else if(dynamic_cast<Graphics::ScalableObjectProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::ScalableObjectProvider*>(prov));
        }
        else if(prov != nullptr) {
            throw std::runtime_error("Unknown tinted provider");
        }

        writer.WriteEnd(start);
    }

    void ScalableObject::SaveThis(Writer &writer, const Graphics::IScalableObjectProvider &provider) {
        auto start = writer.WriteChunkStart(GID::ScalableObject);
        auto prov = &provider;

        if(dynamic_cast<const Graphics::ScalableBitmapProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::ScalableBitmapProvider*>(prov));
        }
        else if(dynamic_cast<const Graphics::ScalableBitmapAnimationProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::ScalableBitmapAnimationProvider*>(prov));
        }
        else if(dynamic_cast<const Graphics::ScalableObjectProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::ScalableObjectProvider*>(prov));
        }
        else {
            throw std::runtime_error("Unknown tinted provider");
        }

        writer.WriteEnd(start);
    }

    template<class T_, class F_>
    static void setthis(F_, Graphics::basic_ScalableObjectProvider<T_> *, T_ *) {
        Utils::ASSERT_FALSE("Should not run");
    }

    template<class F_>
    static void setthis(F_ f, Graphics::ScalableBitmapProvider *provider, Graphics::Bitmap *o) {
		CallBitmapAnimationSetter(f, provider, o);
    }

    template<class F_>
    static void setthis(F_ f, Graphics::ScalableBitmapAnimationProvider *provider, Graphics::BitmapAnimationProvider *o) {
		CallBitmapAnimationAnimationSetter(f, provider, o);
    }

    template<class F_>
    static void setthis(F_ f, Graphics::ScalableObjectProvider *provider, Graphics::RectangularAnimationProvider *o) {
		CallGenericAnimationSetter(f, provider, o);
    }

    template<class T_>
    static void moveout(Graphics::basic_ScalableObjectProvider<T_> *provider, Graphics::IScalableObjectProvider *&p) {
        auto bp = new Graphics::basic_ScalableObjectProvider<T_>();
        p = bp;

        auto b = provider->GetBase();

        setthis(&Graphics::basic_ScalableObjectProvider<T_>::SetBase, bp, b);
        bp->SetController(provider->GetController());

        bp->OwnProvider();
    }

    Graphics::RectangularAnimationStorage ScalableObject::animmoveout() {
        return Graphics::RectangularAnimationStorage(MoveOutProvider(), true);
    }
    
    Graphics::IScalableObjectProvider &ScalableObject::MoveOutProvider() {
        if(!prov)
            throw std::runtime_error("Provider is not set");

        IScalableObjectProvider *p = nullptr;


        if(dynamic_cast<Graphics::ScalableBitmapProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::ScalableBitmapProvider*>(prov);
            moveout(provider, p);
        }
        else if(dynamic_cast<Graphics::ScalableBitmapAnimationProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::ScalableBitmapAnimationProvider*>(prov);
            moveout(provider, p);
        }
        else if(dynamic_cast<Graphics::ScalableObjectProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::ScalableObjectProvider*>(prov);
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
