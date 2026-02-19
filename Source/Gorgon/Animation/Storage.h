#pragma once

#include <stdexcept>

#include "../Animation.h"

namespace Gorgon :: Animation {

    /**
     * Specializing this class allows code injection to animation storages
     */
    template<class T_>
    class basic_StorageInjection {
    };
    
	/**
	* This class stores animations as a part of itself so that it can be moved around as
	* a value rather than a reference.
	*/
	template<class A_>
	class basic_Storage : public virtual Provider, public basic_StorageInjection<A_> {
	public:

		/// Empty constructor
		basic_Storage() = default;

		/// Filling constructor
		basic_Storage(A_ &anim, bool owner = false) : anim(&anim), isowned(owner) { }

		/// Copy constructor is disabled for ownership reasons
		basic_Storage(const basic_Storage &) = delete;

		/// Move constructor
		basic_Storage(basic_Storage &&other) : anim(other.anim), isowned(other.isowned) {
			other.isowned = false;
			other.anim  = nullptr;
		}
		
        //types are derived not to type the same code for every class
		virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new basic_Storage(std::move(*this));
            
            return *ret;
        }

		/// Copy assignment
		basic_Storage &operator =(const basic_Storage &) = delete;

		/// Move assignment
		basic_Storage &operator =(basic_Storage &&other) {
			Remove();
			isowned = other.isowned;
			anim = other.anim;
			other.isowned = false;
			other.anim = nullptr;

			return *this;
		}

		/// Check if this storage has an animation
		bool HasAnimation() const {
			return anim != nullptr;
		}

		/// Returns the animation stored in the object. If
		/// there is no animation provider stored, it will throw std::runtime_error
		A_ &GetAnimation() const {
			if(anim)
				return *anim;
			else
				throw std::runtime_error("Storage contains no animation");
		}
		
		/// Alias for GetAnimation
		A_ &operator *() const {
            return GetAnimation();
        }
        
		/// Alias for GetAnimation
        A_ *operator ->() const {
            return &GetAnimation();
        }

		/// Sets the animation stored in this container
		void SetAnimation(A_ &value, bool owner = false) {
			Remove();

			anim = &value;
			this->isowned = owner;
		}

		/// Sets the animation stored in this container
		void SetAnimation(A_ &&value) {
			Remove();

			anim = &value.MoveOutProvider();
			this->isowned = true;
		}

		/// Removes the animation stored in the container, if the container owns
		/// the animation, it will be destroyed. Use Release to release resource
		/// without destroying it
		void Remove() {
			if(isowned)
				delete anim;

			isowned = false;
			anim = nullptr;
		}
        
		/// Removes the animation from the storage without destroying it.
		A_ *Release() {
			auto temp = anim;
			
			isowned = false;

			Remove();

			return temp;
		}
        
		/// Whether the stored animation is owned by this container
		bool IsOwner() const {
			return isowned;
		}

		/// This function creates a new animation from the stored animation provider. If
		/// there is no animation provider stored, it will throw std::runtime_error
		virtual typename A_::AnimationType &CreateAnimation(ControllerBase &timer) const override {
			if(anim)
				return dynamic_cast<typename A_::AnimationType &>(anim->CreateAnimation(timer));
			else
				throw std::runtime_error("Storage contains no animation");
		}

		/// This function creates a new animation from the stored animation provider. If
		/// there is no animation provider stored, it will throw std::runtime_error
		virtual typename A_::AnimationType &CreateAnimation(bool create=true) const override {
			if(anim)
				return dynamic_cast<typename A_::AnimationType &>(anim->CreateAnimation(create));
			else
				throw std::runtime_error("Storage contains no animation");
		}
        
	private:
		A_ *anim = nullptr;
		bool isowned = false;
	};

	/// Moves one type of animation into another.
	template<class Target_, class Original_>
	basic_Storage<Target_> AnimationCast(basic_Storage<Original_> &&original) {
		basic_Storage<Target_> target;

        if(!original.HasAnimation())
            return target;
        

		bool owned = original.IsOwner();
		Target_ *anim = dynamic_cast<Target_*>(original.Release());
        if(!anim)
            throw std::runtime_error("Animation types are not compatible");
        
		target.SetAnimation(*anim, owned);

		return target;
	}

	/// Basic animation storage, can store all types of animation and can be moved around as a value.
	using Storage = basic_Storage<const Provider>;

}
