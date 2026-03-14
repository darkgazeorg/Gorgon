#pragma once

#include "AnimationStorage.h"
#include "../Graphics/StackedObject.h"
#include <memory>


namespace Gorgon :: Resource {
	class File;
	class Reader;

    /**
     * This is a stacked object resource. It stores a Graphics::StackedObjectProvider or compatible types:
     * Graphics::StackedBitmapProvider and Graphics::StackedBitmapAnimationProvider.
     */
	class StackedObject : public Graphics::IStackedObjectProvider, public AnimationStorage {
	public:
        /// Creates a new stacked object using another stacked object provider.
		explicit StackedObject(Graphics::StackedBitmapProvider &prov);

        /// Creates a new stacked object using another stacked object provider.
		explicit StackedObject(Graphics::StackedBitmapAnimationProvider &prov);

        /// Creates a new stacked object using another stacked object provider.
		explicit StackedObject(Graphics::StackedObjectProvider &prov);
        
        /// Creates a new stacked object using another stacked object provider.
		explicit StackedObject(Graphics::StackedBitmapProvider &&prov) : StackedObject(*new Graphics::StackedBitmapProvider(std::move(prov))) {
            own = true;
        }

        /// Creates a new stacked object using another stacked object provider.
		explicit StackedObject(Graphics::StackedBitmapAnimationProvider &&prov) : StackedObject(*new Graphics::StackedBitmapAnimationProvider(std::move(prov))) {
            own = true;
        }

        /// Creates a new stacked object using another stacked object provider.
		explicit StackedObject(Graphics::StackedObjectProvider &&prov) : StackedObject(*new Graphics::StackedObjectProvider(std::move(prov))) {
            own = true;
        }
        
        /// Creates a new empty stacked object
		StackedObject() { }
		
		IStackedObjectProvider &MoveOutProvider() override;

		GID::Type GetGID() const override {
			return GID::StackedObject;
		}

		/// Changes the provider stored in this stacked object, ownership will not be transferred
		void SetProvider(Graphics::StackedBitmapProvider &value) {
            RemoveProvider();
			prov = &value;
		}

		/// Changes the provider stored in this stacked object, ownership will not be transferred
		void SetProvider(Graphics::StackedBitmapAnimationProvider &value) {
            RemoveProvider();
			prov = &value;
		}

		/// Changes the provider stored in this stacked object, ownership will not be transferred
		void SetProvider(Graphics::StackedObjectProvider &value) {
            RemoveProvider();
			prov = &value;
		}
		
		/// Changes the provider stored in this stacked object, ownership will be transferred
		void AssumeProvider(Graphics::StackedBitmapProvider &value) {
            RemoveProvider();
			prov = &value;
            own = true;
		}

		/// Changes the provider stored in this stacked object, ownership will be transferred
		void AssumeProvider(Graphics::StackedBitmapAnimationProvider &value) {
            RemoveProvider();
			prov = &value;
            own = true;
		}

		/// Changes the provider stored in this stacked object, ownership will be transferred
		void AssumeProvider(Graphics::StackedObjectProvider &value) {
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

		virtual Graphics::RectangularAnimation &CreateTop() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateTop();
		}

		virtual Graphics::RectangularAnimation &CreateBottom() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateBottom();
		}
		
		virtual Geometry::Point GetOffset() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->GetOffset();
        }
        
		virtual void SetOffset(const Geometry::Point &value) override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->SetOffset(value);
        }

		virtual Graphics::StackedObject &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return dynamic_cast<Gorgon::Graphics::StackedObject &>(prov->CreateAnimation(timer));
		}
		
		virtual Graphics::StackedObject &CreateAnimation(bool create=true) const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return dynamic_cast<Gorgon::Graphics::StackedObject &>(prov->CreateAnimation(create));
		}

		virtual Geometry::Size GetSize() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->GetSize();
		}

		/// This function loads a stacked object resource from the file
		static StackedObject *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);
        
		static void SaveThis(Writer &writer, const Graphics::IStackedObjectProvider &provider);

	protected:
		void save(Writer &writer) const override;

		virtual Graphics::RectangularAnimationStorage animmoveout() override;

	private:
		virtual ~StackedObject() { 
			if(own) 
				delete prov;
		}

		IStackedObjectProvider *prov = nullptr;
		
		bool own = false;
    };
}
