#pragma once

#include "AnimationStorage.h"
#include "../Graphics/ScalableObject.h"


namespace Gorgon :: Resource {
		class File;
		class Reader;

		/**
		* This is a tinted object resource. It stores a Graphics::ScalableObjectProvider or compatible types:
		* Graphics::ScalableBitmapProvider and Graphics::ScalableBitmapAnimationProvider.
		*/
		class ScalableObject : public Graphics::IScalableObjectProvider, public AnimationStorage {
		public:
			/// Creates a new tinted object using another tinted object provider.
			explicit ScalableObject(Graphics::ScalableBitmapProvider &prov);

			/// Creates a new tinted object using another tinted object provider.
			explicit ScalableObject(Graphics::ScalableBitmapAnimationProvider &prov);

			/// Creates a new tinted object using another tinted object provider.
			explicit ScalableObject(Graphics::ScalableObjectProvider &prov);

			/// Creates a new tinted object using another tinted object provider.
			explicit ScalableObject(Graphics::ScalableBitmapProvider &&prov) : ScalableObject(*new Graphics::ScalableBitmapProvider(std::move(prov))) {
				own = true;
			}

			/// Creates a new tinted object using another tinted object provider.
			explicit ScalableObject(Graphics::ScalableBitmapAnimationProvider &&prov) : ScalableObject(*new Graphics::ScalableBitmapAnimationProvider(std::move(prov))) {
				own = true;
			}

			/// Creates a new tinted object using another tinted object provider.
			explicit ScalableObject(Graphics::ScalableObjectProvider &&prov) : ScalableObject(*new Graphics::ScalableObjectProvider(std::move(prov))) {
				own = true;
			}

			/// Creates a new empty tinted object
			ScalableObject() {}

			IScalableObjectProvider &MoveOutProvider() override;

			GID::Type GetGID() const override {
				return GID::ScalableObject;
			}

			/// Changes the provider stored in this tinted object, ownership will not be transferred
			void SetProvider(Graphics::ScalableBitmapProvider &value) {
				RemoveProvider();
				prov = &value;
			}

			/// Changes the provider stored in this tinted object, ownership will not be transferred
			void SetProvider(Graphics::ScalableBitmapAnimationProvider &value) {
				RemoveProvider();
				prov = &value;
			}

			/// Changes the provider stored in this tinted object, ownership will not be transferred
			void SetProvider(Graphics::ScalableObjectProvider &value) {
				RemoveProvider();
				prov = &value;
			}

			/// Changes the provider stored in this tinted object, ownership will be transferred
			void AssumeProvider(Graphics::ScalableBitmapProvider &value) {
				RemoveProvider();
				prov = &value;
				own = true;
			}

			/// Changes the provider stored in this tinted object, ownership will be transferred
			void AssumeProvider(Graphics::ScalableBitmapAnimationProvider &value) {
				RemoveProvider();
				prov = &value;
				own = true;
			}

			/// Changes the provider stored in this tinted object, ownership will be transferred
			void AssumeProvider(Graphics::ScalableObjectProvider &value) {
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

			virtual Graphics::SizeController GetController() const override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return prov->GetController();
			}
			virtual void SetController(const Graphics::SizeController &value) override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return prov->SetController(value);
			}

			virtual Graphics::ScalableObject &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return dynamic_cast<Gorgon::Graphics::ScalableObject &>(prov->CreateAnimation(timer));
			}

			virtual Graphics::ScalableObject &CreateAnimation(bool create=true) const override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return dynamic_cast<Gorgon::Graphics::ScalableObject &>(prov->CreateAnimation(create));
			}

			virtual Geometry::Size GetSize() const override {
				if(!prov)
					throw std::runtime_error("Provider is not set.");

				return prov->GetSize();
			}

			/// This function loads a tinted object resource from the file
			static ScalableObject *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);

			static void SaveThis(Writer &writer, const Graphics::IScalableObjectProvider &provider);

		protected:
			void save(Writer &writer) const override;

			virtual Graphics::RectangularAnimationStorage animmoveout() override;

		private:
			virtual ~ScalableObject() {
				if(own)
					delete prov;
			}

			IScalableObjectProvider *prov = nullptr;

			bool own = false;
		};
	}
