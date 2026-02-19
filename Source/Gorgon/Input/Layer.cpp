#include "Layer.h"

namespace Gorgon :: Input {

    bool needsclip(Input::Mouse::EventType event) {
        switch(event) {
            case Input::Mouse::EventType::Over:
            case Input::Mouse::EventType::Out:
			case Input::Mouse::EventType::Up:
			case Input::Mouse::EventType::MovePressed:
			case Input::Mouse::EventType::DownPressed:
                return false;
            default: 
                return true;
        }
    }
    
    bool needsforward(Input::Mouse::EventType event) {
        switch(event) {
            case Input::Mouse::EventType::Out:
			case Input::Mouse::EventType::Up:
			case Input::Mouse::EventType::MovePressed:
			case Input::Mouse::EventType::DownPressed:
                return false;
            default: 
                return true;
        }
    }
    
	bool Layer::propagate_mouseevent(Input::Mouse::EventType event, Geometry::Point location, Input::Mouse::Button button, float amount, MouseHandler &handlers) { 
        if(needsforward(event)) {
            if(Gorgon::Layer::propagate_mouseevent(event, location, button, amount, handlers))
                return true;
        }
        
        if(event == Input::Mouse::EventType::HitCheck) {
			dotransformandclip(true);
            
            mytransform = Transform;
            
			reverttransformandclip();
        }

		auto curlocation = mytransform * location;
        
        auto size = GetCalculatedSize();
        
        if(needsclip(event)) {
            bool out = false;
            if(
                curlocation.X < 0 || 
                curlocation.Y < 0 || 
                curlocation.X >= size.Width || 
                curlocation.Y >= size.Height
            )
                out = true;
            
            if(out) return false;
        }

        if(event == Input::Mouse::EventType::Over) {
            if(over)
                over(*this);
            
            return true;
        }
        else if(event == Input::Mouse::EventType::Out) {
            if(out)
                out(*this);
            
            return true;
        }
        else if(event == Input::Mouse::EventType::Up) {
            if(up)
                up(*this, curlocation, button);
            
            return true;
        }
        else if(event == Input::Mouse::EventType::MovePressed) {
            if(move)
                move(*this, curlocation);
            
            return true;
        }
		else if(event == Input::Mouse::EventType::DownPressed) {
			if(down) {
				down(*this, curlocation, button);
				handlers.Add(this);
			}

			if(down || click || up) {
				return true;
			}
		}
		else if(event == Input::Mouse::EventType::HitCheck) {
            Gorgon::Layer::propagate_mouseevent(event, curlocation, button, amount, handlers);
        }
        else { //click/scroll/move/down
            if(Gorgon::Layer::propagate_mouseevent(event, curlocation, button, amount, handlers))
                return true;
        }

		if(hitcheck && !hitcheck(*this, curlocation)) 
			return false;
        
		if(event == Input::Mouse::EventType::Click && click) {
			click(*this, curlocation, button);
            
            handlers.Add(this);

			return true;
		}
		if(event == Input::Mouse::EventType::Down) {
            if(down) {
                down(*this, curlocation, button);
				handlers.Add(this);
            }
            
            if(down || click || up) {
                return true;
            }
		}
		else if(event == Input::Mouse::EventType::Scroll_Vert && vscroll) {
			if(vscroll(*this, curlocation, amount)) {
                handlers.Add(this);

                return true;
            }
		}
		else if(event == Input::Mouse::EventType::Scroll_Hor && hscroll) {
			if(hscroll(*this, curlocation, amount)) {
                handlers.Add(this);

                return true;
            }
		}
		else if(event == Input::Mouse::EventType::Zoom && zoom) {
			if(zoom(*this, curlocation, amount)) {
                handlers.Add(this);

                return true;
            }
		}
		else if(event == Input::Mouse::EventType::Rotate && rotate) {
			if(rotate(*this, curlocation, amount)) {
                handlers.Add(this);

                return true;
            }
		}
		else if(event == Input::Mouse::EventType::Move && move) {
			move(*this, curlocation);

			handlers.Add(this);

			return true;
		}
		else if(event == Input::Mouse::EventType::HitCheck && over) {
            //don't call, window will decide which layers to call
            handlers.Add(this);

			return true;
		}

		return false;
	}

}
