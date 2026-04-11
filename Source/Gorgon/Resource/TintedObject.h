#pragma once

#include "AnimationStorage.h"
#include "../Graphics/TintedObject.h"
#include <memory>


namespace Gorgon :: Resource {
		class File;
		class Reader;

		/**
		* This is a tinted object resource. It stores a Graphics::TintedObjectProvider or compatible types:
		* Graphics::TintedBitmapProvider and Graphics::TintedBitmapAnimationProvider.
		*/
		class TintedObject : public Graphics::ITintedObjectProvider, public AnimationStorage {
		public:
			/// Creates a new tinted object using another tinted object provider.
			explicit TintedObject(Graphics::TintedBitmapProvider &prov);

			/// Creates a new tinted object using another tinted object provider.
			explicit TintedObject(Graphics::TintedBitmapAnimationProvider &prov);

			/// Creates a new tinted object using another tinted object provider.
			explicit TintedObject(Graphics::TintedObjectProvider &prov);

			/// Creates a new tinted object using another tinted object provider.
			explicit TintedObject(Graphics::TintedBitmapProvider &&prov) : TintedObject(*new Graphics::TintedBitmapProvider(std::move(prov))) {
				own = true;
			}

			/// Creates a new tinted object using another tinted object provider.
			explicit TintedObject(Graphics::TintedBitmapAnimationProvider &&prov) : TintedObject(*new Graphics::TintedBitmapAnimationProvider(std::move(prov))) {
				own = true;
			}

			/// Creates a new tinted object using another tinted object provider.
			explicit TintedObject(Graphics::TintedObjectProvider &&prov) : TintedObject(*new Graphics::TintedObjectProvider(std::move(prov))) {
				own = true;
			}

			/// Creates a new empty tinted object
			TintedObject() {}

			ITintedObjectProvider &MoveOutProvider() override;

			GID::Type GetGID() const override {
				return GID::TintedObject;
			}

			/// Changes the provider stored in this tinted object, ownership will not be transferred
			void SetProvider(Graphics::TintedBitmapProvider &value) {
				RemoveProvider();
				prov = &value;
			}

			/// Changes the provider stored in this tinted object, ownership will not be transferred
			void SetProvider(Graphics::TintedBitmapAnimationProvider &value) {
				RemoveProvider();
				prov = &value;
			}

			/// Changes the provider stored in this tinted object, ownership will not be transferred
			void SetProvider(Graphics::TintedObjectProvider &value) {
				RemoveProvider();
				prov = &value;
			}

			/// Changes the provider stored in this tinted object, ownership will be transferred
			void AssumeProvider(Graphics::TintedBitmapProvider &value) {
				RemoveProvider();
				prov = &value;
				own = true;
			}

			/// Changes the provider stored in this tinted object, ownership will be transferred
			void AssumeProvider(Graphics::TintedBitmapAnimationProvider &value) {
				RemoveProvider();
				prov = &value;
				own = true;
			}

			/// Changes the provider stored in this tinted object, ownership will be transferred
			void AssumeProvider(Graphics::TintedObjectProvider &value) {
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

			virtual Graphics::RGBAf GetColor() const override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return prov->GetColor();
			}
			virtual void SetColor(const Graphics::RGBAf &value) override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return prov->SetColor(value);
			}

			virtual Graphics::TintedObject &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return dynamic_cast<Gorgon::Graphics::TintedObject &>(prov->CreateAnimation(timer));
			}

			virtual Graphics::TintedObject &CreateAnimation(bool create=true) const override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return dynamic_cast<Gorgon::Graphics::TintedObject &>(prov->CreateAnimation(create));
			}

			virtual Geometry::Size GetSize() const override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return prov->GetSize();
			}

			/// This function loads a tinted object resource from the file
			static TintedObject *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);

			static void SaveThis(Writer &writer, const Graphics::ITintedObjectProvider &provider);

		protected:
			void save(Writer &writer) const override;

			virtual Graphics::RectangularAnimationStorage animmoveout() override;

		private:
			virtual ~TintedObject() {
				if(own)
					delete prov;
			}

			ITintedObjectProvider *prov = nullptr;

			bool own = false;
		};
	}
