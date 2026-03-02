/// @file Event.h contains event distribution mechanism
#pragma once

#include <type_traits>
#include <functional>
#include <mutex>

#include <atomic>

#include "Containers/Collection.h"
#include "Utils/Assert.h"
#include "TMP.h"


namespace Gorgon {

	/// @cond INTERNAL
    namespace internal :: event {
            template<class Source_, typename... Params_>
            struct HandlerBase {
                virtual void Fire(std::mutex &locker, Source_ *, Params_...) = 0;
                
                virtual ~HandlerBase() {}
            };

            template<class Source_, typename... Params_>
            struct EmptyHandlerFn : public HandlerBase<Source_, Params_...> {
                EmptyHandlerFn(std::function<void()> fn) : fn(fn) { }

                virtual void Fire(std::mutex &locker, Source_ *, Params_...) {
                    auto f=fn;
                    locker.unlock();
                    f();
                }

                std::function<void()> fn;
            };

            template<class Source_, typename... Params_>
            struct ArgsHandlerFn : public HandlerBase<Source_, Params_...> {
                ArgsHandlerFn(std::function<void(Params_...)> fn) : fn(fn) { }

                virtual void Fire(std::mutex &locker, Source_ *, Params_... args) {
                    auto f=fn;
                    locker.unlock();
                    f(std::forward<Params_>(args)...);
                }

                std::function<void(Params_...)> fn;
            };

            template<class Source_, typename... Params_>
            struct FullHandlerFn : public HandlerBase<Source_, Params_...> {
                static_assert(!std::is_same<Source_, void>::value, "No source class exists for this event (void cannot be passed around)");
                
                FullHandlerFn(std::function<void(Source_&, Params_...)> fn) : fn(fn) { }

                virtual void Fire(std::mutex &locker, Source_ *source, Params_... args) {
                    auto f=fn;
                    locker.unlock();
                    f(*source, std::forward<Params_>(args)...);
                }

                std::function<void(Source_&, Params_...)> fn;
            };

            template<class Source_, class... Params_>
            static HandlerBase<Source_, Params_...>& createhandlerfn(void(*fn)()) {
                return *new EmptyHandlerFn<Source_, Params_...>(fn);
            }

            template<class Source_, class... Params_>
            static HandlerBase<Source_, Params_...>& createhandlerfn(void(*fn)(Params_...)) {
                return *new ArgsHandlerFn<Source_, Params_...>(fn);
            }

            template<class Source_, class... Params_>
            static HandlerBase<Source_, Params_...>& createhandlerfn(void(*fn)(Source_ &, Params_...)) {
                return *new FullHandlerFn<Source_, Params_...>(fn);
            }

            template<class F_, class Source_, int N, class... Params_>
            static typename std::enable_if<TMP::FunctionTraits<F_>::Arity==0, HandlerBase<Source_, Params_...>&>::type 
            createhandler_internal(F_ fn) {
                return *new EmptyHandlerFn<Source_, Params_...>(fn);
            }

            template<class F_, class Source_, int N, typename... Params_>
            static typename std::enable_if<TMP::FunctionTraits<F_>::Arity!=0 && TMP::FunctionTraits<F_>::Arity==N, HandlerBase<Source_, Params_...>&>::type
            createhandler_internal(F_ fn) {
                return *new ArgsHandlerFn<Source_, Params_...>(fn);
            }

            template<class F_, class Source_, int N, class... Params_>
            static typename std::enable_if<TMP::FunctionTraits<F_>::Arity==N+1, HandlerBase<Source_, Params_...>&>::type
            createhandler_internal(F_ fn) {
                return *new FullHandlerFn<Source_, Params_...>(fn);
            }

            template<class F_, class Source_, class... Params_>
            typename std::enable_if<TMP::HasParanthesisOperator<F_>::value, HandlerBase<Source_, Params_...>&>::type 
            create_handler(F_ fn) {
                return createhandler_internal<F_, Source_, sizeof...(Params_), Params_...>(fn);
            }

            template<class F_, class Source_, class... Params_, typename N=void>
            typename std::enable_if<!TMP::HasParanthesisOperator<F_>::value, HandlerBase<Source_, Params_...>&>::type 
            create_handler(F_ fn) {
                return createhandlerfn<Source_, Params_...>(fn);
            }
        }

	/// @endcond
	
	
	/// Generic type to store event tokens.
    typedef intptr_t EventToken;
	
	/// This class provides event mechanism. Different function signatures are allowed to
	/// as event handlers. These are:
	///
	/// * <b>`void fn()`</b> neither event source nor event parameters are supplied. 
	/// * <b>`void fn(Params_... params)`</b> parameters will be passed
	/// * <b>`void fn(Source_ &source, Params_... params)`</b> the source and parameters
	///   will be passed
	/// 
	/// Class members or lambda functions can also be used as event handlers. An
	/// event handler can be registered using Register function.
	template<class Source_=void, typename... Params_>
	class Event {
	public:

		/// Data type for tokens
		typedef intptr_t Token;
		
		/// Constructor for empty source
		Event() : source(nullptr) {
			fire.clear();
			static_assert( std::is_same<Source_, void>::value , "Empty constructor cannot be used." );
		}
		
		/// Constructor for class specific source
		template <class S_ = Source_>
		explicit Event(typename std::enable_if<!std::is_same<S_, void>::value, S_>::type &source) : source(&source) {
			fire.clear();
			static_assert(!std::is_same<Source_, void>::value, "Filling constructor is not required, use the default.");
		}
		
		/// Constructor for class specific source
		template <class S_ = Source_>
		explicit Event(typename std::enable_if<!std::is_same<S_, void>::value, S_>::type *source) : source(source) {
			ASSERT(source, "Source cannot be nullptr");
			fire.clear();
			static_assert(!std::is_same<Source_, void>::value, "Filling constructor is not required, use the default.");
		}
		
		/// Move constructor
		Event(Event &&event) : source(nullptr) {
			fire.clear();
			Swap(event);
#ifndef NDEBUG
			event.movedout = true;
#endif
		}
		

		/// Destructor
		~Event() {
			ASSERT(!fire.test_and_set(), "An event cannot be destroyed while its being fired.");
			std::lock_guard<std::mutex> g(access);
			
			handlers.Destroy();
		}
		
		/// Copy constructor is disabled
		Event(const Event &) = delete;
		
		/// Copy assignment is disabled
		Event &operator =(const Event &) = delete;

		/// Move assignment, should be called synchronized
		Event &operator =(Event &&other) {
			if(&other==this) return *this;

			std::lock_guard<std::mutex> g(access);

			ASSERT(!fire.test_and_set(), "An event cannot be moved into while its being fired.");

			handlers.Destroy();

			source=nullptr;
			fire.clear();
			Swap(other);
            
            return *this;
		}

		/// Swaps two Events, used for move semantics
		void Swap(Event &other) {
			if(&other==this) return;

			using std::swap;

			ASSERT(!fire.test_and_set() && !other.fire.test_and_set(), "An event cannot be swapped while its being fired.");

			swap(source, other.source);
			swap(handlers, other.handlers);

			fire.clear();
			other.fire.clear();
		}

		/// Registers a new function to be called when this event is triggered. This function can
		/// be called from event handler of the same event. The registered event handler will be
		/// called immediately in this case. If you register a class with a () operator, this class 
		/// will be copied. If you require call to be made to the same instance, instead of using
		/// `Register(a)` use `Register(a, &decltype(a)::operator())`
		template<class F_>
		Token Register(F_ fn) {

			ASSERT(!movedout, "This event is moved out of");

			auto &handler=internal::event::create_handler<F_, Source_, Params_...>(fn);

			std::lock_guard<std::mutex> g(access);
			handlers.Add(handler);

			return reinterpret_cast<Token>(&handler);
		}

		/// Registers a new function to be called when this event is triggered. This variant is 
		/// designed to be used with member functions. **Example:**
		/// @code
		/// A a;
		/// b.ClickEvent.Register(a, &A::f);
		/// @endcode
		///
		/// This function can be called from event handler of the same event. The registered 
		/// event handler will be called immediately in this case. 
		template<class C_, typename... A_>
		Token Register(C_ &c, void(C_::*fn)(A_...)) {

			ASSERT(!movedout, "This event is moved out of");

			std::function<void(A_...)> f=TMP::MakeFunctionFromMember(fn, &c);

			return Register(f);
		}

		/// Unregisters the given marked with the given token. This function performs no
		/// operation if the token is not contained within this event. A handler can be
		/// unregistered safely while the event is being fired. In this case, if the event that
		/// is being deleted is different from the current event handler, the deleted event
		/// handler might have already been fired. If not, it will not be fired.
		void Unregister(Token token) {

			ASSERT(!movedout, "This event is moved out of");
			
			std::lock_guard<std::mutex> g(access);

			auto item=reinterpret_cast<internal::event::HandlerBase<Source_, Params_...>*>(token);
			
			auto l=handlers.FindLocation(item);
			if(l==-1) return;
			
			if(iterator.IsValid() && iterator.CurrentPtr()==item) {
				handlers.Delete(l);
				
				//Collection iterator can point element -1
				iterator.Previous(); 
			}
			else {
				handlers.Delete(l);
			}
		}

		/// Fire this event. This function will never allow recursive firing from the same thread, 
		/// i.e. an event handler cannot cause this event to be fired again.
		void operator()(Params_... args) {
            //stops the request if it is received from a different thread.
            std::lock_guard<std::recursive_mutex> g(firemtx);
            
			ASSERT(!movedout, "This event is moved out of");

			try {
				for(iterator=handlers.begin(); iterator.IsValid(); iterator.Next()) {
					access.lock();
					// fire method will unlock access after it creates a local copy of the function
					// this allows the fired object to be safely deleted.
					iterator->Fire(access, source, args...);
				}
			}
			catch(...) {
				//unlock everything if something goes bad
				
				//just in case
				access.unlock();
				
				fire.clear();
				
				throw;
			}
			
			fire.clear();
		}
		
		/// Removes all registered handlers from this event
		void Clear() {
			std::lock_guard<std::recursive_mutex> g1(firemtx);
			std::lock_guard<std::mutex> g2(access);
            
#ifndef NDEBUG
			ASSERT(!fire.test_and_set(), "Recursion detected during event execution.");
#else
			//prevent recursion
			if(fire.test_and_set()) return;
#endif
			
			fire.clear();
			
			handlers.DeleteAll();
        }
		
		/// value for an empty token
		static const Token EmptyToken;
		
	private:

#ifndef NDEBUG
		bool movedout = false;
#endif


		std::mutex access;
		std::atomic_flag fire;
		Source_ *source;
		Containers::Collection<internal::event::HandlerBase<Source_, Params_...>> handlers;
		typename Containers::Collection<internal::event::HandlerBase<Source_, Params_...>>::Iterator iterator;
        std::recursive_mutex firemtx;
	};
    
    template<class C_, class ...P_>
    const typename Event<C_, P_...>::Token Event<C_, P_...>::EmptyToken = 0;
	
	/// Swaps two events
	template<class Source_, class... Args_>
	void swap(Event<Source_, Args_...> &l, Event<Source_, Args_...> &r) {
		l.Swap(r);
	}
}
