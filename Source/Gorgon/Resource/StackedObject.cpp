#include "StackedObject.h"
#include "Animation.h"
#include "Image.h"
#include "File.h"
#include "Reader.h"
#include "Null.h"
#include "AnimationServices.h"

namespace Gorgon :: Resource {


    StackedObject::StackedObject(Graphics::StackedBitmapProvider &prov) : prov(&prov)
    { }

    StackedObject::StackedObject(Graphics::StackedBitmapAnimationProvider &prov) : prov(&prov)
    { }

    StackedObject::StackedObject(Graphics::StackedObjectProvider &prov) : prov(&prov)
    { }

    template<class T_>
    static Graphics::basic_StackedObjectProvider<T_> &fillfrom(Containers::Collection<Base> &children, Geometry::Point offset) {
        T_ *t = nullptr, *b = nullptr;

        if(children.GetCount() == 1) {
            t = dynamic_cast<T_*>(&children[0]);
        }
        else if(children.GetCount() == 2) {
            t = dynamic_cast<T_*>(&children[0]);
            b = dynamic_cast<T_*>(&children[1]);
        }

        return *new Graphics::basic_StackedObjectProvider<T_>(b, t, offset);
    }

    StackedObject *StackedObject::LoadResource(std::weak_ptr<File> f, std::shared_ptr<Reader> reader, unsigned long totalsize) {
        auto target = reader->Target(totalsize);

        auto file = f.lock();

        auto stacked = new StackedObject();

        enum {
            unknown, img, anim, mixed
        } type = unknown;
        int c = 0;
        
        Geometry::Point offset;

        while(!target) {
            auto gid = reader->ReadGID();
            auto size= reader->ReadChunkSize();


            if(gid == GID::StackedObject_Props) {
                offset = reader->ReadPoint();
            }
            else {
                auto resource = file->LoadChunk(*stacked, gid, size, false);

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
                        throw std::runtime_error("StackedObject can only contain images or animations");
                    }

                    if(++c > 2) {
                        throw std::runtime_error("StackedObject cannot have more than 2 parts");
                    }

                    stacked->children.Add(resource);
                }
            }
        }

        if(type == anim) {
            stacked->SetProvider(
                fillfrom<Graphics::BitmapAnimationProvider>(stacked->children, offset)
            );
        }
        else if(type == img) {
            stacked->SetProvider(
                fillfrom<Graphics::Bitmap>(stacked->children, offset)
            );
        }
        else if(type == mixed) {
            stacked->SetProvider(
                fillfrom<Graphics::RectangularAnimationProvider>(stacked->children, offset)
            );
        }
        //else empty stacked object

        stacked->own = true;
        return stacked;
    }

    template <class T_>
    static void savethis(Writer &writer, const Graphics::basic_StackedObjectProvider<T_> &provider) {
        auto t = provider.GetTop();
        auto b = provider.GetBottom();
        
        writer.WriteChunkHeader(GID::StackedObject_Props, 2 * 4);
        writer.WritePoint(provider.GetOffset());

        if(!t) {
            SaveAnimation(writer, t);
        }
        else {
            SaveAnimation(writer, t);
            SaveAnimation(writer, b);
        }
    }

    void StackedObject::save(Writer &writer) const {
        auto start = writer.WriteObjectStart(this);

        if(dynamic_cast<Graphics::StackedBitmapProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::StackedBitmapProvider*>(prov));
        }
        else if(dynamic_cast<Graphics::StackedBitmapAnimationProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::StackedBitmapAnimationProvider*>(prov));
        }
        else if(dynamic_cast<Graphics::StackedObjectProvider*>(prov)) {
            savethis(writer, *dynamic_cast<Graphics::StackedObjectProvider*>(prov));
        }
        else if(prov != nullptr) {
            throw std::runtime_error("Unknown stacked provider");
        }

        writer.WriteEnd(start);
    }

    void StackedObject::SaveThis(Writer &writer, const Graphics::IStackedObjectProvider &provider) {
        auto start = writer.WriteChunkStart(GID::StackedObject);
        auto prov = &provider;

        if(dynamic_cast<const Graphics::StackedBitmapProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::StackedBitmapProvider*>(prov));
        }
        else if(dynamic_cast<const Graphics::StackedBitmapAnimationProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::StackedBitmapAnimationProvider*>(prov));
        }
        else if(dynamic_cast<const Graphics::StackedObjectProvider*>(prov)) {
            savethis(writer, *dynamic_cast<const Graphics::StackedObjectProvider*>(prov));
        }
        else {
            throw std::runtime_error("Unknown stacked provider");
        }

        writer.WriteEnd(start);
    }

    template<class T_, class F_>
    static void setthis(F_ f, Graphics::basic_StackedObjectProvider<T_> *provider, T_ *o) {
        Utils::ASSERT_FALSE("Should not run");
    }

    template<class F_>
    static void setthis(F_ f, Graphics::StackedBitmapProvider *provider, Graphics::Bitmap *o) {
		CallBitmapAnimationSetter(f, provider, o);
    }

    template<class F_>
    static void setthis(F_ f, Graphics::StackedBitmapAnimationProvider *provider, Graphics::BitmapAnimationProvider *o) {
        CallBitmapAnimationAnimationSetter(f, provider, o);
    }

    template<class F_>
    static void setthis(F_ f, Graphics::StackedObjectProvider *provider, Graphics::RectangularAnimationProvider *o) {
		CallGenericAnimationSetter(f, provider, o);
    }

    template<class T_>
    static void moveout(Graphics::basic_StackedObjectProvider<T_> *provider, Graphics::IStackedObjectProvider *&p) {
        auto bp = new Graphics::basic_StackedObjectProvider<T_>();
        p = bp;

        auto t = provider->GetTop();
        auto b = provider->GetBottom();

        setthis(&Graphics::basic_StackedObjectProvider<T_>::SetTop, bp, t);
        setthis(&Graphics::basic_StackedObjectProvider<T_>::SetBottom, bp, b);
        
        bp->SetOffset(provider->GetOffset());

        bp->OwnProviders();
    }

    Graphics::RectangularAnimationStorage StackedObject::animmoveout() {
        return Graphics::RectangularAnimationStorage(MoveOutProvider(), true);
    }
    
    Graphics::IStackedObjectProvider &StackedObject::MoveOutProvider() {
        if(!prov)
            throw std::runtime_error("Provider is not set");

        IStackedObjectProvider *p = nullptr;


        if(dynamic_cast<Graphics::StackedBitmapProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::StackedBitmapProvider*>(prov);
            moveout(provider, p);
        }
        else if(dynamic_cast<Graphics::StackedBitmapAnimationProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::StackedBitmapAnimationProvider*>(prov);
            moveout(provider, p);
        }
        else if(dynamic_cast<Graphics::StackedObjectProvider*>(prov)) {
            auto provider = dynamic_cast<Graphics::StackedObjectProvider*>(prov);
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
