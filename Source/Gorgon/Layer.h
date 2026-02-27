#pragma once

#include "Containers/Collection.h"
#include "Geometry/Point.h"
#include "Geometry/Bounds.h"
#include "Geometry/Transform3D.h"
#include "Input/Mouse.h"

namespace Gorgon {
    class Layer;

    /// Current layer transformation, only for render and mouse 
    extern Geometry::Transform3D Transform;

    /// Current clipping size, for mouse and clipping events
    extern Geometry::Bounds        Clip;

    /// Current layer offset from the top left of the window
    extern Geometry::Point         Offset;
    
    class MouseHandler {
    public:
        MouseHandler(MouseHandler &&other) {
            layers.Swap(other.layers);
        }
        
        MouseHandler &operator =(MouseHandler &&other) {
            layers.Swap(other.layers);
            
            return *this;
        }
        
        explicit MouseHandler(Layer *layer = nullptr) {
            if(layer) layers.Add(layer);
        }
        
        operator Layer *() const {
            if(layers.GetSize())
                return &layers[0];
            else
                return nullptr;
        }
        
        operator bool() const {
            return layers.GetCount() > 0;
        }
        
        void Clear() {
            layers.Clear();
        }
        
        void Add(Layer *l) {
            layers.Add(l);
        }
        
        void Swap(MouseHandler &other) {
            layers.Swap(other.layers);
        }
        
        Containers::Collection<Layer> layers;
    };

    /// This should be called by the windows to reset transformation stack.
    inline void ResetTransform(const Geometry::Size &size) {
        Transform={};
        Transform.Translate({-1.0f, 1.0f, 0});
        Transform.Scale(2.0f / size.Width, -2.0f / size.Height, 1.0f);
    }

    /** 
    * This class is the base class for all layer types.
    * All common methods are implemented with a common way.
    * This class always implements basic processing functions.
    * Derived classes can override them. Unless overridden these
    * basic functions only propagate the call. Copying a layer is
    * dangerous and therefore disabled. @nosubgrouping
    */
    class Layer {
    public:
        
        /// Initializing constructor
        Layer(const Geometry::Bounds &bounds) :
            Children(children),
            bounds(bounds)
        { }

        /// Constructor that sets the layer to cover entire parent, no matter how big it is. The
        /// location of the layer is set to be the origin
        Layer() : Layer(EntireRegion) { }

        /// Constructor that places the layer to the given location
        Layer(const Geometry::Point &location) :
        Layer({location, Geometry::Size::Max()})
        { }
        
        /// Copy constructor is disabled.
        Layer(const Layer&) = delete;
        
        /// Move constructor
        Layer(Layer &&other) : Children(children) {
            Swap(other);
        }
        
        /// Copy assignment is deleted
        Layer &operator =(const Layer &)=delete;
        
        /// Move assignment
        Layer &operator =(Layer &&other);

        /// Destructor
        virtual ~Layer();
        
        /// Swaps two layers, mostly used for move semantics
        void Swap(Layer &other);

        /// @name Children related functions
        /// @{
        /// These functions deals with child layers.
        
        /// Adds the given layer as a child. Notice that the newly added
        /// layer is drawn on top of others.
        virtual void Add(Layer &layer);

        /// Adds the given layer as a child. Notice that the newly added
        /// layer is drawn on top of others.
        void Add(Layer *layer) {
            if(!layer) return;
            
            Add(*layer);
        }

        /// For debugging
        void setname(std::string value) {
#ifndef NDEBUG
            name = value;
#endif
        }
        
        /// Inserts the given layer before the given index. The newly inserted
        /// layer will be drawn *under* the layer in the given index
        virtual void Insert(Layer &layer, long under);
        
        /// Inserts the given layer before the given index. The newly inserted
        /// layer will be drawn *under* the layer in the given index
        void Insert(Layer *layer, long under) {
            if(!layer) return;
            
            Insert(*layer, under);
        }

        /// Removes the given layer
        virtual void Remove(Layer &layer) {
            children.Remove(layer);
            
            removed(layer);
            layer.located(nullptr);
            layer.parent = nullptr;
        }
        
        /// Removes the given layer
        void Remove(Layer *layer) {
            if(!layer) return;
            
            Remove(*layer);
        }
        
        /// Returns whether this layer has a parent
        virtual bool HasParent() const {
            return parent!=nullptr;
        }
        
        /// Returns the parent of this layer. Throws if no parent is set. Use HasParent
        /// to make sure this layer has a parent
        /// @throw std::runtime_error if no parent is set
        virtual Layer &GetParent() const {
            if(!parent) throw std::runtime_error("No parent is set");
            
            return *parent;
        }

        /// Returns the top level layer that contains this layer.
        virtual Layer &GetTopLevel() {
            if(!parent) 
                return *this;

            return parent->GetTopLevel();
        }
        
        /// Returns the top level layer that contains this layer.
        virtual Layer &GetTopLevel() const {
            if(!parent) 
                return const_cast<Layer &>(*this);

            return parent->GetTopLevel();
        }
        
        /// Translates the given location to the top level
        virtual Geometry::Point TranslateToTopLevel(Geometry::Point location = {0, 0}) const;

        /// @}
        
        /// @name Iterator related functions
        /// @{
        /// These functions return an iterator that allows children to be iterated.
        
        /// An iterator pointing to the start of the children
        Containers::Collection<Layer>::ConstIterator begin() const {
            return children.begin();
        }
        
        /// An iterator pointing to the end of the children
        Containers::Collection<Layer>::ConstIterator end() const {
            return children.end();
        }
        
        /// An iterator pointing to the start of the children
        Containers::Collection<Layer>::ConstIterator First() const {
            return children.First();
        }
        
        /// An iterator pointing to the last item of the children
        Containers::Collection<Layer>::ConstIterator Last() const {
            return children.Last();
        }
        /// @}

        
        /// @name Ordering functions
        /// @{
        /// This layer should be a in another layer for these methods to work.

        /// Places this layer before the given index. This layer will be drawn
        /// above the given index.
        void PlaceBefore(int before) {
            if(parent) {
                if(before==parent->children.GetCount()) before --;
                
                parent->children.MoveBefore(*this, before++);
            }
        }

        /// Places this layer to the top of the layer stack its in. This makes
        /// sure that its not being covered by other layers apart from its own
        /// children
        void PlaceToTop() {
            if(parent)
                parent->children.MoveBefore(*this, parent->children.GetCount());
        }

        /// Places this layer to the bottom of the layer stack.
        void PlaceToBottom() {
            if(parent)
                parent->children.MoveBefore(*this, 0);
        }

        /// Gets the current order of the stack. 0 means the layer is the bottommost
        /// layer in the stack
        int GetOrder() const { 
            if(parent)
                return parent->children.FindLocation(this);
            else
                return -1;
        }
        /// @}

        /// @name Geometry related functions
        /// @{
        /// These functions modify or return the boundaries of this layer

        /// Moves this layer to the given location
        virtual void Move(const Geometry::Point &location) {
            bounds.Move(location);
        }

        /// Moves this layer to the given location
        virtual void Move(int x, int y) {
            bounds.Move({x, y});
        }

        /// Resizes the layer to the given size
        virtual void Resize(const Geometry::Size &size) {
            bounds.Resize(size);
        }

        /// Resizes the layer to the given size
        void Resize(int width, int height) {
            Resize({width, height});
        }

        /// Resizes the layer to the given size
        void SetWidth(int width) {
            bounds.SetWidth(width);
        }

        /// Resizes the layer to the given size
        void SetHeight(int height) {
            bounds.SetHeight(height);
        }

        /// Sets the boundaries of this layer.
        virtual void SetBounds(const Geometry::Bounds &bounds) {
            this->bounds=bounds;
        }
        
        /// Returns the size of the layer
        Geometry::Size GetSize() const {
            return GetBounds().GetSize();
        }
        
        /// Returns the size of the layer. If any dimension size is
        /// 0, it means the layer will span its parent, this function
        /// will perform necessary calculation to get parent size.
        Geometry::Size GetCalculatedSize() const;
        
        /// Returns the width of the layer
        int GetWidth() const {
            return GetBounds().Width();
        }

        /// Returns the height of the layer
        int GetHeight() const {
            return GetBounds().Height();
        }

        /// Returns the current location of the layer
        Geometry::Point GetLocation() const {
            return GetBounds().TopLeft();
        }

        /// Returns the current location of the layer
        int GetLeft() const {
            return GetBounds().Left;
        }

        /// Returns the current location of the layer
        int GetTop() const {
            return GetBounds().Top;
        }
        
        /// Returns the boundaries of the layer
        virtual Geometry::Bounds GetBounds() const {
            return bounds;
        }

        /// Returns the effective boundaries of the layer
        Geometry::Bounds GetEffectiveBounds() const;
        /// @}

        /// Displays this layer
        virtual void Show() { isvisible=true; }

        /// Hides this layer
        virtual void Hide() { isvisible=false; }
        
        /// Returns whether this layer is effectively visible
        virtual bool IsVisible() const { 
            if(parent && !parent->isvisible) return false;
            
            return isvisible;
        }

        /// Propagates a mouse event. Some events will be called directly. Do not use this function. Direct calls should never touch handlers. 
        /// Input layers should cache perform transformations on direct calls. Direct calls should not be clipped. button is set only for
        /// Click/Down/Up events, amount will be set for Scroll/Zoom events. Mouse event system is not thread safe.
        virtual bool propagate_mouseevent(Input::Mouse::EventType evet, Geometry::Point location, Input::Mouse::Button button, float amount, MouseHandler &handlers);
        
        /// Renders the current layer, default handling is to pass the request to the sub-layers. Rendering is not thread safe.
        virtual void Render();
        
        /// Sub-layers that this layer holds, all the sub-layers are
        /// considered to be above current layer
        const Containers::Collection<Layer> &Children;
        
        /// When used as layer bounds, represents the entire region its placed in.
        static const Geometry::Bounds EntireRegion;

    protected:

        /// Will be called when a layer is added. This function will even be called
        /// when the given layer was already in the children.
        virtual void added(Layer &layer) { }
        
        /// Will be called when a layer is removed. This function will be called even
        /// if the given layer is not a child of this layer.
        virtual void removed(Layer &layer) { }
        
        /// Will be called when this layer is added to another.
        virtual void located(Layer *parent) { }

        /// Performs transformation and clipping. Use inverse for reverse mapping for mouse events
        void dotransformandclip(bool inverse = false);

        /// Reverts previously done transformation
        void reverttransformandclip();
        
        /// Child layers that this layer holds, all child layers are
        /// considered to be above current layer
        Containers::Collection<Layer> children;
        
        /// Parent layer, could be nullptr
        Layer *parent = nullptr;

        /// Bounds of this layer.
        Geometry::Bounds bounds;

        /// Whether this layer is visible, invisible layers will not
        /// be drawn or receive any events
        bool isvisible =true; 

#ifndef NDEBUG
        /// For debugging
        std::string name;
#endif

    };

}
