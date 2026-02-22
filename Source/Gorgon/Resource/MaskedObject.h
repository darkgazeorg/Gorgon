#pragma once

#include "AnimationStorage.h"
#include "../Graphics/MaskedObject.h"


namespace Gorgon :: Resource {
	class File;
	class Reader;

    /**
     * This is a masked object resource. It stores a Graphics::MaskedObjectProvider or compatible types:
     * Graphics::MaskedBitmapProvider and Graphics::MaskedBitmapAnimationProvider.
     */
	class MaskedObject : public Graphics::IMaskedObjectProvider, public AnimationStorage {
	public:
        /// Creates a new masked object using another masked object provider.
		explicit MaskedObject(Graphics::MaskedBitmapProvider &prov);

        /// Creates a new masked object using another masked object provider.
		explicit MaskedObject(Graphics::MaskedBitmapAnimationProvider &prov);

        /// Creates a new masked object using another masked object provider.
		explicit MaskedObject(Graphics::MaskedObjectProvider &prov);
        
        /// Creates a new masked object using another masked object provider.
		explicit MaskedObject(Graphics::MaskedBitmapProvider &&prov) : MaskedObject(*new Graphics::MaskedBitmapProvider(std::move(prov))) {
            own = true;
        }

        /// Creates a new masked object using another masked object provider.
		explicit MaskedObject(Graphics::MaskedBitmapAnimationProvider &&prov) : MaskedObject(*new Graphics::MaskedBitmapAnimationProvider(std::move(prov))) {
            own = true;
        }

        /// Creates a new masked object using another masked object provider.
		explicit MaskedObject(Graphics::MaskedObjectProvider &&prov) : MaskedObject(*new Graphics::MaskedObjectProvider(std::move(prov))) {
            own = true;
        }
        
        /// Creates a new empty masked object
		MaskedObject() { }
		
		IMaskedObjectProvider &MoveOutProvider() override;

		GID::Type GetGID() const override {
			return GID::MaskedObject;
		}

		/// Changes the provider stored in this masked object, ownership will not be transferred
		void SetProvider(Graphics::MaskedBitmapProvider &value) {
            RemoveProvider();
			prov = &value;
		}

		/// Changes the provider stored in this masked object, ownership will not be transferred
		void SetProvider(Graphics::MaskedBitmapAnimationProvider &value) {
            RemoveProvider();
			prov = &value;
		}

		/// Changes the provider stored in this masked object, ownership will not be transferred
		void SetProvider(Graphics::MaskedObjectProvider &value) {
            RemoveProvider();
			prov = &value;
		}
		
		/// Changes the provider stored in this masked object, ownership will be transferred
		void AssumeProvider(Graphics::MaskedBitmapProvider &value) {
            RemoveProvider();
			prov = &value;
            own = true;
		}

		/// Changes the provider stored in this masked object, ownership will be transferred
		void AssumeProvider(Graphics::MaskedBitmapAnimationProvider &value) {
            RemoveProvider();
			prov = &value;
            own = true;
		}

		/// Changes the provider stored in this masked object, ownership will be transferred
		void AssumeProvider(Graphics::MaskedObjectProvider &value) {
            RemoveProvider();
			prov = &value;
            own = true;
		}
		
		/// Removes the provider, if it is own by this resource it will be deleted.
		void RemoveProvider() {
            if(own)
                delete prov;
            
            own = false;
            
            prov = nullptr;
        }

		virtual Graphics::RectangularAnimation &CreateBase() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateBase();
		}

		virtual Graphics::RectangularAnimation &CreateMask() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateMask();
		}

		virtual Graphics::MaskedObject &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return dynamic_cast<Gorgon::Graphics::MaskedObject &>(prov->CreateAnimation(timer));
		}
		
		virtual Graphics::MaskedObject &CreateAnimation(bool create=true) const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return dynamic_cast<Gorgon::Graphics::MaskedObject &>(prov->CreateAnimation(create));
		}

		virtual Geometry::Size GetSize() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->GetSize();
		}

		/// This function loads a masked object resource from the file
		static MaskedObject *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);
        
		static void SaveThis(Writer &writer, const Graphics::IMaskedObjectProvider &provider);

	protected:
		void save(Writer &writer) const override;

		virtual Graphics::RectangularAnimationStorage animmoveout() override;

	private:
		virtual ~MaskedObject() { 
			if(own) 
				delete prov;
		}

		IMaskedObjectProvider *prov = nullptr;
		
		bool own = false;
    };
}
