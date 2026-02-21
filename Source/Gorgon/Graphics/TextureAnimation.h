#pragma once

#include "Animations.h"
#include "Texture.h"
#include "Bitmap.h"
#include "../Containers/Collection.h"

namespace Gorgon :: Graphics {
    
    template<class T_>
    class basic_TextureAnimationInjection {
    public:
        virtual ~basic_TextureAnimationInjection() { }
    };
    
    
    template<class T_, template<class, class, class> class A_, class F_>
    class basic_TextureAnimationProvider;
    
    template<class T_>
    class basic_AnimationFrame : public Gorgon::Animation::Frame {
		template<class T__, template<class, class, class> class A_, class F_>
		friend class basic_TextureAnimationProvider;
    public:
        basic_AnimationFrame(T_ &image, unsigned duration = 42, unsigned start = 0) :
        image(&image), duration(duration), start(start) { }
        
        /// Returns the duration of this frame
        unsigned GetDuration() const override {
            return duration;
        }
        
        /// Returns the starting time of this frame
        unsigned GetStart() const override {
            return start;
        }
        
        /// Returns the ending time of this frame
        unsigned GetEnd() const override {
            return start + duration;
        }
        
        /// Returns if the given time is within this frame
        bool IsIn(unsigned time) const override {
            return time >= start && time < start + duration;
        }
        
        /// Returns the image for this frame
        T_ &GetImage() const {
            return *image;
        }
        
    private:
        T_ *image;
        unsigned duration, start;
    };
    
    template<class T_, class P_, class F_>
    class basic_TextureAnimation : public virtual Image, public virtual DiscreteAnimation {
    public:
        using ParentType = P_;
        using FrameType  = F_;        

		/// Creates a new image animation from the given parent
		basic_TextureAnimation(const ParentType &parent, Gorgon::Animation::ControllerBase &controller) :
		Gorgon::Animation::Base(controller), parent(&parent) { }

		/// Creates a new image animation from the given parent
        basic_TextureAnimation(const ParentType &parent, bool create) :
		Gorgon::Animation::Base(create), parent(&parent) { }

		/// Deletes this animation object
		virtual void DeleteAnimation() const override {
			delete this;
		}
        
		virtual bool Progress(unsigned &leftover) override;
                		
		int GetDuration() const override {
            return parent->GetDuration();
        }

        virtual GL::Texture GetID() const override {
			if(current < (unsigned)parent->GetCount())
				return (*parent)[current].GetID();
            else if(parent->GetCount()) 
                return (*parent)[0].GetID();

            Utils::ASSERT_FALSE("Animation contains no frames.");
            
            return 0;
        }
        
        virtual Geometry::Size GetImageSize() const override {
			if(current < (unsigned)parent->GetCount())
				return (*parent)[current].GetImageSize();
            else if(parent->GetCount()) 
                return (*parent)[0].GetImageSize();

            Utils::ASSERT_FALSE("Animation contains no frames.");
            
            return {0, 0};
        }
        
        virtual ColorMode GetMode() const override {
			if(current < (unsigned)parent->GetCount())
				return (*parent)[current].GetMode();
            else if(parent->GetCount()) 
                return (*parent)[0].GetMode();

            Utils::ASSERT_FALSE("Animation contains no frames.");
            
            return ColorMode::RGBA;
        }
        
        virtual const Geometry::Pointf *GetCoordinates() const override {
            if(current < (unsigned)parent->GetCount())
                return (*parent)[current].GetCoordinates();
            else if(parent->GetCount()) 
                return (*parent)[0].GetCoordinates();

            Utils::ASSERT_FALSE("Animation contains no frames.");
            
            return nullptr;
        }

		unsigned CurrentFrame() const override {
			return current;
		}
        
    private:
        const ParentType *parent = nullptr;
        unsigned current = NoFrame;
    };
    
    template<class T_, template<class, class, class> class A_, class F_>
    class basic_TextureAnimationProvider : public virtual DiscreteAnimationProvider, public basic_TextureAnimationInjection<T_> {
    public:
        using Iterator = typename std::vector<basic_AnimationFrame<T_>>::iterator;
        using ConstIterator = typename std::vector<basic_AnimationFrame<T_>>::const_iterator;
        using AnimationType = basic_TextureAnimation<T_, basic_TextureAnimationProvider<T_, A_, F_>, F_>;
        using FrameType = F_;
        
        basic_TextureAnimationProvider() = default;
        
        basic_TextureAnimationProvider(const basic_TextureAnimationProvider &) = delete;
        
		template<class C_>
        basic_TextureAnimationProvider(C_ &&other) { 
			swapout(other);
		}
		
        //types are derived not to type the same code for every class
		virtual auto MoveOutProvider() -> decltype(*this) override {
            auto ret = new typename std::remove_reference<decltype(*this)>::type(std::move(*this));
            
            return *ret;
        }

		basic_TextureAnimationProvider &operator =(const basic_TextureAnimationProvider &) = delete;

		template<class C_>
		basic_TextureAnimationProvider &operator =(C_ &&other) {
			frames.clear();
			duration = 0;

			swapout(other);
            
            return *this;
		}
		
		~basic_TextureAnimationProvider() {
            destroylist.Destroy();
        }

		basic_TextureAnimationProvider Duplicate() const {
			basic_TextureAnimationProvider p;
			p.frames = frames;
			p.duration = duration;

			return p;
		}

		/// Returns the size of the first image
		int GetWidth() const {
			if(frames.size()>0)
				return frames[0].GetImage().GetWidth();
            
			return 0;
		}

		/// Returns the size of the first image
		int GetHeight() const {
			if(frames.size()>0)
				return frames[0].GetImage().GetHeight();
            
			return 0;
		}

		/// Returns the size of the first image
		Geometry::Size GetSize() const override {
			if(frames.size()>0)
				return frames[0].GetImage().GetSize();
            
			return {0, 0};
		}

		/// Returns number of frames
		int GetCount() const override {
			return (int)frames.size();
		}

		/// Creates a new animation from this resource
		virtual AnimationType &CreateAnimation(Gorgon::Animation::ControllerBase &controller) const override {
			return *new AnimationType(*this, controller);
		}

		/// Creates a new animation from this resource
		virtual AnimationType &CreateAnimation(bool create=true) const override {
			return *new AnimationType(*this, create);
		}

		/// Returns the image that is to be shown at the given time. If the given time is larger
		/// than the animation duration, animation is assumed to be looping.
		T_ &ImageAt(unsigned time) const {
            ASSERT(frames.size() || duration==0, "Animation is empty");
            
			time=time%GetDuration();

			return frames[FrameAt(time)].GetImage();
		}

		/// Returns the image at the given frame
		T_ &operator [](int frame) const {
            ASSERT(frame>=0 && frame<(int)frames.size(), "Index out of bounds");
            
			return frames[frame].GetImage();
		}

		/// Returns which frame is at the given time. If the given time is larger than the animation
		/// duration, last frame is returned.
		unsigned FrameIndexAt(unsigned time) const {
            ASSERT(frames.size(), "Animation is empty");
            
            int i = 0;
            for(auto f : frames) {
                if(f.GetEnd() >= time)
                    return i;
                
                i++;
            }
            
            return i-1;
        }
        
        /// Returns the frame at specific point
        const F_ &FrameAt(int index) const override {
            ASSERT(index>=0 && index<(int)frames.size(), "Index out of bounds");
            
            return frames[index];
        }

		/// Returns the starting time of the given frame
		unsigned StartOf(unsigned frame) const override {
			return frames[frame].GetStart();
		}

		/// Returns the duration of the animation
		unsigned GetDuration() const override {
			return duration;
		}

		/// Returns the duration of the given frame
		unsigned GetDuration(unsigned frame) const override {
            ASSERT(frame<frames.size(), "Index out of bounds");
            
			return frames[frame].GetDuration();
		}
		
		/// Adds the given image to the end of the animation
		void Add(T_ &image, unsigned duration = 42, bool own = false) {
			frames.push_back({image, duration, this->duration});
			this->duration += duration;
            
            if(own)
                destroylist.Push(image);
		}
		
		/// Adds the given image to the end of the animation. This version owns the given image
		/// by moving it in the ownership list
		void Add(T_ &&image, unsigned duration = 42) {
            destroylist.Push(new T_(std::move(image)));
            T_ &img = *destroylist.Last();
			frames.push_back({img, duration, this->duration});
			this->duration += duration;
		}

		void Add(const F_ &frame) {
			frames.push_back(frame);
			frames.back().start = duration;
			duration += frame.duration;
		}

		void Add(const Gorgon::Animation::Frame &frame) override {
			if(dynamic_cast<const F_*>(&frame) == nullptr)
				throw std::runtime_error("Wrong frame type.");

			Add(dynamic_cast<const F_&>(frame));
		}
		
		/// Inserts the given image before the given frame
		void Insert(T_ &image, int before, unsigned duration = 42) {
			if(before < 0)
				before += frames.size();

			if(before > frames.size()) before = frames.size();

			frames.insert(frames.begin()+before, {image, duration, frames[before].GetStart()});
			
			for(unsigned i=before+1; i<frames.size(); i++) {
				frames[i].start=frames[i-1].GetEnd();
			}
			
			this->duration += duration;
		}
		
		/// Inserts the given image before the given frame
		void Insert(T_ &&img, int before, unsigned duration = 42) {
			if(before < 0)
				before += frames.size();

			if(before > frames.size()) before = frames.size();

            destroylist.Push(new T_(std::move(img)));
            T_ &image = *destroylist.Last();
            
			frames.insert(frames.begin()+before, {image, duration, frames[before].GetStart()});
			
			for(unsigned i=before+1; i<frames.size(); i++) {
				frames[i].start=frames[i-1].GetEnd();
			}
			
			this->duration += duration;
		}

		/// Inserts the given image before the given frame
		void Insert(const F_ &frm, int before) {
			auto frame = frm;

			if(before < 0)
				before += (int)frames.size();

			if((unsigned)before > frames.size()) before = (int)frames.size();

			if((unsigned)before == frames.size())
				frame.start = duration;
			else
				frame.start = frames[before].start;

			frames.insert(frames.begin()+before, frame);

			for(unsigned i=before+1; i<frames.size(); i++) {
				frames[i].start=frames[i-1].GetEnd();
			}

			this->duration += duration;
		}

		/// Inserts the given image before the given frame
		void Insert(const Gorgon::Animation::Frame &frame, int before) override {
			if(dynamic_cast<const F_*>(&frame) == nullptr)
				throw std::runtime_error("Wrong frame type.");

			Insert(dynamic_cast<const F_&>(frame), before);
		}

		/// Moves a frame that has the index before the given position.
		void MoveBefore(unsigned index, int before) override {
			if(before < 0)
				before += (int)frames.size();

			if((unsigned)before >= frames.size()) {
                auto tmp = frames[index];
                frames.erase(frames.begin()+index);
                frames.push_back(tmp);
            }
            else if(index < (unsigned)before) {
                std::rotate(frames.begin()+index, frames.begin()+index+1, frames.begin()+before);
            }
            else if((unsigned)before < index) {
                std::rotate(frames.rbegin()+(frames.size()-1-index), frames.rbegin()+(frames.size()-1-index)+1, frames.rbegin()+(frames.size()-1-before));
            }
        }
		
		/// Owns the given image so that it would be destroyed with this animation
		void Own(T_ &image) {
            destroylist.Add(image);
        }
		
		/// Removes an image from the animation
		void Remove(unsigned frame) override {
            ASSERT(frame < frames.size(), "Index out of bounds");
            
			duration -= (frames.begin() + frame)->GetDuration();
            frames.erase(frames.begin() + frame);
        }

        /// Removes all images from the animation
		void Clear() override {
			frames.clear();
			duration = 0;
		}
		
		/// Releases ownership of all images in the animation without destroying them
		void ReleaseAll() {
            destroylist.Clear();
        }
        
        /// Removes an image from the animation
        void Remove(ConstIterator it) {
            frames(it);
        }
		
		/// Returns an iterator to the beginning of the animation frames
		Iterator begin() {
			return frames.begin();
		}
		
		/// Returns an iterator to the beginning of the animation frames
		ConstIterator begin() const {
			return frames.begin();
		}
		
		/// Returns an iterator to the end of the animation frames
		Iterator end() {
			return frames.end();
		}
		
		/// Returns an iterator to the end of the animation frames
		ConstIterator end() const {
			return frames.end();
		}
        
    protected:
        std::vector<basic_AnimationFrame<T_>> frames;
        unsigned duration = 0;
        Containers::Collection<T_> destroylist;

	private:

		void swapout(basic_TextureAnimationProvider &other) {
			using std::swap;

			swap(frames, other.frames);
			swap(duration, other.duration);
            swap(destroylist, other.destroylist);
		}

		template<class N_, class R_ = T_>
		typename std::enable_if<std::is_const<R_>::value, void>::type
		swapout(basic_TextureAnimationProvider<typename std::remove_const<T_>::type, A_, N_> &other) {
			duration = 0;
			
			for(auto &frame : other) {
				Add(frame.GetImage(), frame.GetDuration());
                Own(frame.GetImage());
            }

            other.ReleaseAll();
			other.Clear();
		}

    };
    
    template<class T_, class P_, class F_>
    bool basic_TextureAnimation<T_, P_, F_>::Progress(unsigned &leftover) {
		if(!controller) return false;

		if(parent->GetCount()==0) return false;
        

		unsigned progress=controller->GetProgress();


        if(controller->IsControlled()) {
            if(progress>parent->GetDuration()) {
                current=parent->GetCount()-1;
                leftover=progress-parent->GetDuration();
                
                return false;
            }
            else {
                current=parent->FrameIndexAt(progress);
                
                return true;
            }
        }
        else {
            current = parent->FrameIndexAt(progress % parent->GetDuration());
            
            return true;
        }
    }
    
    template<>
    class basic_TextureAnimationInjection<Bitmap> {
    public:
        virtual ~basic_TextureAnimationInjection() { }
        
        void Prepare() {
            auto &me = dynamic_cast<DiscreteAnimationProvider&>(*this);
            int c = me.GetCount();
            for(int i=0; i<c; i++) {
                auto img = dynamic_cast<const basic_AnimationFrame<Bitmap>&>(me[i]);
                img.GetImage().Prepare();
            }
        }
    };

	using BitmapAnimationProvider = basic_TextureAnimationProvider<Bitmap, basic_TextureAnimation, basic_AnimationFrame<Bitmap>>;

	using BitmapAnimation = BitmapAnimationProvider::AnimationType;

	using ConstBitmapAnimationProvider = basic_TextureAnimationProvider<const Bitmap, basic_TextureAnimation, basic_AnimationFrame<const Bitmap>>;

	using ConstBitmapAnimation = ConstBitmapAnimationProvider::AnimationType;

	using ImageAnimationProvider = basic_TextureAnimationProvider<Image, basic_TextureAnimation, basic_AnimationFrame<Image>>;

	using ImageAnimation = ImageAnimationProvider::AnimationType;

	using ConstImageAnimationProvider = basic_TextureAnimationProvider<const Image, basic_TextureAnimation, basic_AnimationFrame<const Image>>;

	using ConstImageAnimation = ConstImageAnimationProvider::AnimationType;

}
