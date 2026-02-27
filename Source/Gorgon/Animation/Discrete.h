#pragma once

#include "../Animation.h"

namespace Gorgon :: Animation {

	// Represents an instance of a discrete animation made out of frames
	class DiscreteAnimation : public virtual Base {
	public:

		/// Returns the current frame. This function might return NoFrame to denote that the
		/// animation does not contain a frame.
		virtual unsigned CurrentFrame() const = 0;

		/// This variable denotes that this animation has no frame at the moment.
		static const unsigned NoFrame = (unsigned)-1;
	};

	/// This is the base class for a single frame in a discreet animation
	class Frame {
	public:
        virtual ~Frame() { }
        
		/// Returns the duration of this frame
		virtual unsigned GetDuration() const = 0;

		/// Returns the starting time of this frame
		virtual unsigned GetStart() const = 0;

		/// Returns the ending time of this frame
		virtual unsigned GetEnd() const {
			return GetStart() + GetDuration();
		}

		/// Returns if the given time is within this frame
		virtual bool IsIn(unsigned time) const {
			return time >= GetStart() && time < GetEnd();
		}
	};

	/**
	* Provides a discreet animation that is made out of frames. This is an interface and should be
	* derived in order to be used.
	*/
	class DiscreteProvider : public virtual Provider {
	public:
		/// Adds a frame to the end of the animation. Frames are designed to be copied, thus they should
		/// be lightweight object.
		virtual void Add(const Frame &frame) = 0;

		/// Adds a frame to the specified point the animation. Frames are designed to be copied, thus they 
		/// should be lightweight object. Before is the index of the frame that this frame will be placed
		/// before. If it is greater or equal to the number of frames, this function will act like add.
		/// Additionally, before could be negative, denoting it will start from the end. -1 is before the
		/// last item.
		virtual void Insert(const Frame &frame, int before) = 0;

		/// Moves the frame with the given index to the specified point the animation. Before is the index 
		/// of the frame that this frame will be placed before. If it is greater or equal to the number of 
		/// frames, this function will move the item to the end. Additionally, before could be negative, 
		/// denoting it will start from the end. -1 is before the last item.
		virtual void MoveBefore(unsigned index, int before) = 0;

		/// Removes the given frame.
		virtual void Remove(unsigned index) = 0;

		/// Returns the frame at specific point
		virtual const Frame &FrameAt(int index) const = 0;

		/// Returns the frame at specific point
		const Frame &operator[](int index) const { return FrameAt(index); }

		/// Returns the starting time of the given frame
		virtual unsigned StartOf(unsigned frame) const = 0;

		/// Returns the duration of the animation
		virtual unsigned GetDuration() const = 0;

		/// Returns the duration of the given frame
		virtual unsigned GetDuration(unsigned frame) const = 0;

		/// Clears all frames from this animation
		virtual void Clear() = 0;

		/// Returns number of frames
		virtual int GetCount() const = 0;

	protected:
	};
}
