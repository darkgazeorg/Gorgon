#pragma once

#include "AnimationStorage.h"
#include "../Graphics/Rectangle.h"


namespace Gorgon :: Resource {
	class File;
	class Reader;

	class Rectangle : public Graphics::IRectangleProvider, public AnimationStorage {
	public:
        /// Creates a new rectangle using another rectangle provider
		explicit Rectangle(Graphics::BitmapRectangleProvider &prov);

		/// Creates a new rectangle using another rectangle provider
		explicit Rectangle(Graphics::AnimatedBitmapRectangleProvider &prov);

		/// Creates a new rectangle using another rectangle provider
		explicit Rectangle(Graphics::RectangleProvider &prov);

        /// Creates a new rectangle using another rectangle provider
		explicit Rectangle(Graphics::BitmapRectangleProvider &&prov) : Rectangle(*new Graphics::BitmapRectangleProvider(std::move(prov))) {
            own = true;
        }

		/// Creates a new rectangle using another rectangle provider
		explicit Rectangle(Graphics::AnimatedBitmapRectangleProvider &&prov) : Rectangle(*new Graphics::AnimatedBitmapRectangleProvider(std::move(prov))) {
			own = true;
		}
		/// Creates a new rectangle using another rectangle provider
		explicit Rectangle(Graphics::RectangleProvider &&prov) : Rectangle(*new Graphics::RectangleProvider(std::move(prov))) {
			own = true;
		}

        /// Creates an empty rectangle
		Rectangle() = default;
		
		Graphics::IRectangleProvider &MoveOutProvider() override;
        
		GID::Type GetGID() const override {
			return GID::Rectangle;
		}
        
        /// Changes provider to the given provider, ownership will not be transferred
		void SetProvider(Graphics::BitmapRectangleProvider &value) {
            RemoveProvider();
			prov = &value;
		}

		/// Changes provider to the given provider, ownership will not be transferred
		void SetProvider(Graphics::AnimatedBitmapRectangleProvider &value) {
			RemoveProvider();
			prov = &value;
		}

		/// Changes provider to the given provider, ownership will not be transferred
		void SetProvider(Graphics::RectangleProvider &value) {
			RemoveProvider();
			prov = &value;
		}

		/// Changes the provider stored in this line, ownership will be transferred
		void AssumeProvider(Graphics::BitmapRectangleProvider &value) {
            RemoveProvider();
			prov = &value;
            own = true;
		}

		/// Changes the provider stored in this line, ownership will be transferred
		void AssumeProvider(Graphics::AnimatedBitmapRectangleProvider &value) {
			RemoveProvider();
			prov = &value;
			own = true;
		}

		/// Changes the provider stored in this line, ownership will be transferred
		void AssumeProvider(Graphics::RectangleProvider &value) {
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

		virtual Graphics::RectangularAnimation &CreateTL() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateTL();
		}

		virtual Graphics::RectangularAnimation &CreateTM() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateTM();
		}

		virtual Graphics::RectangularAnimation &CreateTR() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateTR();
		}

		virtual Graphics::RectangularAnimation &CreateML() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateML();
		}

		virtual Graphics::RectangularAnimation &CreateMM() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateMM();
		}

		virtual Graphics::RectangularAnimation &CreateMR() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateMR();
		}

		virtual Graphics::RectangularAnimation &CreateBL() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateBL();
		}

		virtual Graphics::RectangularAnimation &CreateBM() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateBM();
		}

		virtual Graphics::RectangularAnimation &CreateBR() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateBR();
		}

		virtual void SetCenterTiling(bool value) override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			prov->SetCenterTiling(value);
		}

		virtual bool GetCenterTiling() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->GetCenterTiling();
		}

		virtual void SetSideTiling(bool value) override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			prov->SetSideTiling(value);
		}

		virtual bool GetSideTiling() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->GetSideTiling();
		}
		
		virtual Gorgon::Graphics::Rectangle &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return dynamic_cast<Gorgon::Graphics::Rectangle &>(prov->CreateAnimation(timer));
		}
		
		virtual Gorgon::Graphics::Rectangle &CreateAnimation(bool create=true) const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return dynamic_cast<Gorgon::Graphics::Rectangle &>(prov->CreateAnimation(create));
		}

		virtual Geometry::Size GetSize() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->GetSize();
		}

		/// This function loads a rectangle resource from the file
		static Rectangle *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);

		static void SaveThis(Writer &writer, const Graphics::IRectangleProvider &provider);


	protected:
		void save(Writer &writer) const override;

		virtual Graphics::RectangularAnimationStorage animmoveout() override;

	private:
		virtual ~Rectangle() { 
			if(own) 
				delete prov;
		}

		IRectangleProvider *prov = nullptr;
		
		bool own = false;
	};

}
