#include "../Resource.h"
#include "DataItems.h"
#include "AnimationStorage.h"

#include "Image.h"
#include "Animation.h"
#include "Line.h"
#include "Rectangle.h"
#include "MaskedObject.h"
#include "TintedObject.h"
#include "ScalableObject.h"
#include "StackedObject.h"
#include "Null.h"


namespace Gorgon :: Resource {
	
	void Initialize() {
		DataItem::InitializeLoaders();
	}

	void SaveAnimation(Writer &writer, const Graphics::RectangularAnimationProvider &object) {
		auto obj = &object;

		if(dynamic_cast<const Graphics::Bitmap*>(obj)) {
			Image::SaveThis(writer, dynamic_cast<const Graphics::Bitmap&>(object));
		}
		else if(dynamic_cast<const Graphics::BitmapAnimationProvider*>(obj)) {
			Animation::SaveThis(writer, dynamic_cast<const Graphics::BitmapAnimationProvider&>(object));
		}
		else if(dynamic_cast<const Graphics::ILineProvider*>(obj)) {
			Line::SaveThis(writer, dynamic_cast<const Graphics::ILineProvider&>(object));
		}
		else if(dynamic_cast<const Graphics::IRectangleProvider*>(obj)) {
			Rectangle::SaveThis(writer, dynamic_cast<const Graphics::IRectangleProvider&>(object));
		}
		else if(dynamic_cast<const Graphics::IMaskedObjectProvider*>(obj)) {
			MaskedObject::SaveThis(writer, dynamic_cast<const Graphics::IMaskedObjectProvider&>(object));
		}
		else if(dynamic_cast<const Graphics::ITintedObjectProvider*>(obj)) {
			TintedObject::SaveThis(writer, dynamic_cast<const Graphics::ITintedObjectProvider&>(object));
		}
		else if(dynamic_cast<const Graphics::IScalableObjectProvider*>(obj)) {
			ScalableObject::SaveThis(writer, dynamic_cast<const Graphics::IScalableObjectProvider&>(object));
		}
		else if(dynamic_cast<const Graphics::IStackedObjectProvider*>(obj)) {
			StackedObject::SaveThis(writer, dynamic_cast<const Graphics::IStackedObjectProvider&>(object));
		}
		else {
			throw std::runtime_error("Cannot save animation: Unsupported animation type.");
		}
	}

	void SaveAnimation(Writer &writer, const Graphics::RectangularAnimationProvider *object) {
		if(!object) {
			Null::SaveThis(writer);
		}
		else {
			SaveAnimation(writer, *object);
		}
	}
	
}
