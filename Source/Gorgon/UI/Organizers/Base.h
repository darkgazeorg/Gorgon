#pragma once

#include <stdexcept>


namespace Gorgon :: UI {
    
    class WidgetContainer;

    class Widget;
    
    /**
     * This namespace contains organizers that manage the location of the widgets
     * in a container automatically.
     */
    namespace Organizers {
        /**
        * Provides the basis for the automatic UI organizers
        */
        class Base {
        public:
            
            /// Ensuring correct destruction
            virtual ~Base() { }
            
            /// Attaches this organizer to a container
            void AttachTo(WidgetContainer &container);
            
            /// Removes the organizer from the container
            void RemoveFrom();
            

            /// Returns if this organizer is attached to a container
            bool IsAttached() const {
                return attachedto != nullptr;
            }
            
            /// Returns the container that this organizer is attached to. If
            /// organizer is not attached, this function will throw.
            WidgetContainer &GetAttached() const {
                if(!attachedto)
                    throw std::runtime_error("Organizer is not attached to any container");
                
                return *attachedto;
            }
            
            /// Stops reorganizing. Even manual calls to reorganize will be ignored until 
            /// StartReorganize is issued which will immediately reorganize everything.
            void PauseReorganize();
            
            /// Starts reorganizing. If paused, this will also call Reorganize.
            void StartReorganize();
            
            /// Reorganizes the widgets that are organized by this organizer
            void Reorganize();
            
            /// Adds the given widget to the attached container.
            virtual Base &Add (Widget &widget);
            
            /// Adds the given text as a label to the attached container
            virtual Base &Add (const std::string &title);
            
            /// Returns if the organizer is currently working
            bool IsReorganizing() const {
                return organizing;
            }
        
        protected:
            /// Called when the attachment of the organizer is changed
            virtual void attachmentchanged() { }
            
            /// Should reorganize the contents of the organizer. This will only be called if the
            /// organizer is attached.
            virtual void reorganize() = 0;
            
        private:
            WidgetContainer *attachedto = nullptr;
            bool organizing = false;
            bool paused = false;
        };
    }
}
