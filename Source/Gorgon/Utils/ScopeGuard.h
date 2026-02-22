#pragma once

#include <functional>

namespace Gorgon :: Utils {

	/**
	 * This class guards a scope and as soon as the object runs out of scope calls the given
	 * function.
	 */
	class ScopeGuard {
	public:
		/// Construct a scope guard with the given function
		template<class F_>
		ScopeGuard(F_ &&fn) : fn(std::forward<F_>(fn)) {}

		/// Construct an empty scope guard
		ScopeGuard() = default;

		/// Move constructor
		ScopeGuard(ScopeGuard &&other) : fn(std::move(other.fn)) {
		}

		/// No copying
		ScopeGuard(const ScopeGuard&) = delete;

		/// Move assignment
		ScopeGuard &operator = (ScopeGuard &&other) {
			fn = std::move(other.fn);

			return *this;
		}

		/// No copy assignment
		ScopeGuard &operator = (const ScopeGuard&) = delete;

		/// Disarm this scope guard, after this function it will not fire
		void Disarm() {
			fn = nullptr;
		}

		/// Arms the scope guard with the given function.
		template<class F_>
		void Arm(F_ &&value) {
			fn = std::forward<F_>(value);
		}

		/// Destructor
		~ScopeGuard() {
			if(fn)
				fn();
		}

	private:
		std::function<void()> fn;
	};

}
