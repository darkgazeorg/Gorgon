#pragma once

#include "AnimationStorage.h"
#include "../Graphics/Line.h"
#include <memory>


namespace Gorgon :: Resource {
	class File;
	class Reader;

    /**
     * This is a line resource, it stores a Graphics::LineProvider. This resource can work with
     * Graphics::LineProvider, Graphics::BitmapLineProvider and Graphics::AnimatedBitmapLineProvider.
     * @see Gorgon::Graphics::basic_LineProvider for details
     */
	class Line : public Graphics::ILineProvider, public AnimationStorage {
	public:
        /// Creates a new line using another line provider.
		explicit Line(Graphics::BitmapLineProvider &prov);

        /// Creates a new line using another line provider.
		explicit Line(Graphics::AnimatedBitmapLineProvider &prov);

        /// Creates a new line using another line provider.
		explicit Line(Graphics::LineProvider &prov);
        
        /// Creates a new line using another line provider.
		explicit Line(Graphics::BitmapLineProvider &&prov) : Line(*new Graphics::BitmapLineProvider(std::move(prov))) {
            own = true;
        }

        /// Creates a new line using another line provider.
		explicit Line(Graphics::AnimatedBitmapLineProvider &&prov) : Line(*new Graphics::AnimatedBitmapLineProvider(std::move(prov))) {
            own = true;
        }

        /// Creates a new line using another line provider.
		explicit Line(Graphics::LineProvider &&prov) : Line(*new Graphics::LineProvider(std::move(prov))) {
            own = true;
        }
        
        /// Creates a new empty line
		Line() : ILineProvider(Graphics::Orientation::Horizontal) { }
		
		ILineProvider &MoveOutProvider() override;

		GID::Type GetGID() const override {
			return GID::Line;
		}

		/// Changes the provider stored in this line, ownership will not be transferred
		void SetProvider(Graphics::BitmapLineProvider &value) {
            RemoveProvider();
			prov = &value;
		}

		/// Changes the provider stored in this line, ownership will not be transferred
		void SetProvider(Graphics::AnimatedBitmapLineProvider &value) {
            RemoveProvider();
			prov = &value;
		}

		/// Changes the provider stored in this line, ownership will not be transferred
		void SetProvider(Graphics::LineProvider &value) {
            RemoveProvider();
			prov = &value;
		}
		
		/// Changes the provider stored in this line, ownership will be transferred
		void AssumeProvider(Graphics::BitmapLineProvider &value) {
            RemoveProvider();
			prov = &value;
            own = true;
		}

		/// Changes the provider stored in this line, ownership will be transferred
		void AssumeProvider(Graphics::AnimatedBitmapLineProvider &value) {
            RemoveProvider();
			prov = &value;
            own = true;
		}

		/// Changes the provider stored in this line, ownership will be transferred
		void AssumeProvider(Graphics::LineProvider &value) {
            RemoveProvider();
			prov = &value;
            own = true;
		}
		
		/// Removes the provider, if it is owned by this resource it will be deleted.
		void RemoveProvider() {
            if(own)
                delete prov;
            
            own = false;
            
            prov = nullptr;
        }

		virtual Graphics::RectangularAnimation &CreateStart() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateStart();
		}

		virtual Graphics::RectangularAnimation &CreateMiddle() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateMiddle();
		}

		virtual Graphics::RectangularAnimation &CreateEnd() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->CreateEnd();
		}

		virtual void SetOrientation(Graphics::Orientation value) override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			prov->SetOrientation(value);
		}

		virtual Graphics::Orientation GetOrientation() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->GetOrientation();
		}

		virtual void SetTiling(bool value) override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			prov->SetTiling(value);
		}

		virtual bool GetTiling() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->GetTiling();
		}

		virtual Gorgon::Graphics::Line &CreateAnimation(Gorgon::Animation::ControllerBase &timer) const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return dynamic_cast<Gorgon::Graphics::Line &>(prov->CreateAnimation(timer));
		}
		
		virtual Gorgon::Graphics::Line &CreateAnimation(bool create=true) const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return dynamic_cast<Gorgon::Graphics::Line &>(prov->CreateAnimation(create));
		}

		virtual Geometry::Size GetSize() const override {
			if(!prov)
				throw std::runtime_error("Provider is not set.");

			return prov->GetSize();
		}

		/// This function loads a line resource from the file
		static Line *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);
        
		static void SaveThis(Writer &writer, const Graphics::ILineProvider &provider);

	protected:
		void save(Writer &writer) const override;

		virtual Graphics::RectangularAnimationStorage animmoveout() override;

	private:
		virtual ~Line() { 
			if(own) 
				delete prov;
		}

		ILineProvider *prov = nullptr;
		
		bool own = false;
	};

}
