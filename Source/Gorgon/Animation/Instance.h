#pragma once

#include <type_traits>
#include <utility>

#include "../Animation.h"

namespace Gorgon :: Animation {
    
    /**
     * Specializing this class allows code injection to animation instances
     */
    template<class T_>
    class basic_InstanceInjection {
    };
    
    /**
     * This class allows storing an animation instance regarless of its underlying type as a value.
     * The contained animation can be destroyed along with the instance itself. Destruction process
     * uses DeleteAnimation so that shared animations will not be destroyed.
     */
    template<class A_>
    class basic_Instance : public virtual Base, public basic_InstanceInjection<A_> {
    public:
        
        /// Empty constructor
        basic_Instance() = default;
        
        /// Filling constructor
        basic_Instance(A_ &instance, bool owner = true) : instance(&instance), isowned(owner) { }

		/// Copy constructor is disabled for ownership reasons
		basic_Instance(const basic_Instance &) = delete;

		/// Move constructor
		basic_Instance(basic_Instance &&other) : instance(other.instance), isowned(other.isowned) {
			other.isowned = false;
			other.instance  = nullptr;
		}
        
		/// Copy assignment
		basic_Instance &operator =(const basic_Instance &) = delete;

		/// Move assignment
		basic_Instance &operator =(basic_Instance &&other) {
			RemoveAnimation();
			isowned = other.isowned;
			instance = other.instance;
			other.isowned = false;
			other.instance = nullptr;

			return *this;
		}

		/// Destructor
		virtual ~basic_Instance() {
			RemoveAnimation();
		}

		/// Move assignment, owns the assigned object as CreateAnimation returns
		/// objects that needs to be owned
		basic_Instance &operator =(A_ &instance) {
			RemoveAnimation();
			isowned = true;
			this->instance = &instance;

            return *this;
		}

		/// Move constructor for value-semantic animations or temporaries (rvalues).
		/// Transfers ownership to a heap-allocated copy managed by this instance.
		template<class T_, typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<T_>, basic_Instance> &&
            !std::is_lvalue_reference_v<T_> &&
			std::is_base_of_v<A_, std::decay_t<T_>> &&
			!std::is_abstract_v<std::decay_t<T_>>
		>>
		basic_Instance(T_ &&val) : instance(new std::decay_t<T_>(std::forward<T_>(val))), isowned(true) { }

		/// Move assignment for value-semantic animations or temporaries (rvalues).
		/// Transfers ownership to a heap-allocated copy managed by this instance.
		template<class T_, typename = std::enable_if_t<
			!std::is_same_v<std::decay_t<T_>, basic_Instance> &&
			std::is_base_of_v<A_, std::decay_t<T_>> &&
			!std::is_abstract_v<std::decay_t<T_>>
		>>
		basic_Instance &operator =(T_ &&val) {
			RemoveAnimation();
			instance = new std::decay_t<T_>(std::forward<T_>(val));
			isowned = true;

			return *this;
		}

		/// Check if this instance has an animation
		bool HasAnimation() const {
			return instance != nullptr;
		}
		
		/// Returns the animation stored in the object. If
		/// there is no animation provider stored, it will throw std::runtime_error
		A_ &GetAnimation() const {
			if(instance)
				return *instance;
			else
				throw std::runtime_error("Instance contains no animation");
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
		void SetAnimation(A_ &value, bool owner = true) {
			RemoveAnimation();

			instance = &value;
			this->isowned = owner;
		}

		/// Sets the animation stored in this container from value-semantic animations or temporaries (rvalues).
		template<class T_, typename = std::enable_if_t<
			!std::is_same_v<std::decay_t<T_>, basic_Instance> &&
			std::is_base_of_v<A_, std::decay_t<T_>> &&
			!std::is_abstract_v<std::decay_t<T_>>
		>>
		void SetAnimation(T_ &&val) {
			RemoveAnimation();

			instance = new std::decay_t<T_>(std::forward<T_>(val));
			this->isowned = true;
		}
		
		/// Removes the animation stored in the container, if the container owns
		/// the animation, it will be destroyed. Use Release to release resource
		/// without destroying it
		void RemoveAnimation() {
			if(isowned)
				instance->DeleteAnimation();

			isowned = false;
			instance = nullptr;
		}
        
		/// Removes the animation from the storage without destroying it.
		A_ *ReleaseAnimation() {
			auto temp = instance;
			
			isowned = false;

			RemoveAnimation();

			return temp;
		}
        
		/// Whether the stored animation is owned by this container
		bool IsOwner() const {
			return isowned;
		}

        
    private:
        A_ *instance = nullptr;
        bool isowned = false;
    };
    

	/// Moves one type of animation into another.
	template<class Target_, class Original_>
	basic_Instance<Target_> AnimationCast(basic_Instance<Original_> &&original) {
		basic_Instance<Target_> target;

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
	using Instance = basic_Instance<const Base>;
    
}
