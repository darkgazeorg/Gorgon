#pragma once

#include "Base.h"
#include "../Graphics/Animations.h"

namespace Gorgon :: Resource {

	class Writer;

	/// This class denotes the resource is an image animation storage. Unlike graphics based storages,
	/// this interface only allows a rectangular animation storage to be moved out.
	class AnimationStorage : public virtual Base {
	public:
		/// Moves this animation out as a generic value type animation
		Graphics::RectangularAnimationStorage MoveOut() {
			return animmoveout();
		}

	protected:
		virtual Graphics::RectangularAnimationStorage animmoveout() = 0;
	};

	//these functions are defined in Resource.cpp

	/// Saves a given generic rectangular animation as resource. Only known animation types can
	/// be saved.
	void SaveAnimation(Writer &writer, const Graphics::RectangularAnimationProvider &object);

	/// Saves a given generic rectangular animation as resource. Only known animation types can
	/// be saved. In this version, if the object is null, it will be saved as Null object.
	void SaveAnimation(Writer &writer, const Graphics::RectangularAnimationProvider *object);

}
