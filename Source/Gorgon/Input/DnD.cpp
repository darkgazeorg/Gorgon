#include "DnD.h"
#include "../Window.h"

namespace Gorgon :: Input {

	bool needsclip(Input::Mouse::EventType event);

	void DragInfo::AddTextData(const std::string &text) {
		AssumeData(*new TextData(text));
	}

	void DragInfo::AddFileData(const std::string &file) {
		AssumeData(*new FileData(file));
	}

	void DragInfo::AddData(ExchangeData &value) {
		data.Add(value);
	}

	void DragInfo::AssumeData(ExchangeData &value) {
		data.Add(value);
		destroylist.Add(value);
	}


	bool DragInfo::HasData(Resource::GID::Type type) const {
		for(auto &d : data) {
			if(d.Type() == type)
				return true;
		}

		return false;
	}

	ExchangeData &DragInfo::GetData(Resource::GID::Type type) const {
		for(auto &d : data) {
			if(d.Type() == type)
				return d;
		}

		throw std::runtime_error("Cannot find data of type "+String::From(type));
	}
	
	void initdrag() {
        
    }

	void startdrag() {
		ASSERT(IsDragPrepared(), "Drag is not prepared.");

		dragstarted = true;
		DragStarted(*DragOperation);
	}

	void begindrag() {
		DragOperation = new DragInfo();
        
        initdrag();
	}

	void begindrag(DragSource &source) {
		DragOperation = new DragInfo(source);
        
        initdrag();
	}

	bool DropTarget::propagate_mouseevent(Input::Mouse::EventType event, Geometry::Point location, Input::Mouse::Button button, float amount, MouseHandler &handlers) {
		if(!IsDragging()) return false;
        
        auto &op = GetDragOperation();

		if(event == Input::Mouse::EventType::HitCheck) {
			dotransformandclip(true);

			mytransform = Transform;

			reverttransformandclip();
		}

		auto curlocation = mytransform * location;


		if(needsclip(event)) {
			dotransformandclip(true);

			bool out = false;
			if(
				curlocation.X < 0 ||
				curlocation.Y < 0 ||
				curlocation.X >= Clip.Width() ||
				curlocation.Y >= Clip.Height()
				)
				out = true;

			reverttransformandclip();

			if(out) return false;
		}

		if(hitcheck && !hitcheck(*this, curlocation))
			return false;

		if(event == Input::Mouse::EventType::Over) {
			bool ret = true;
			if(over) {
				 ret = over(*this, op);
            }

			if(ret && op.HasSource() && op.GetSource().over) {
				op.GetSource().over(op.GetSource(), op);
			}

			op.SetTarget(*this);

			return true;
		}
		else if(event == Input::Mouse::EventType::Out) {
			if(out)
				out(*this, op);

			if(op.HasSource() && op.GetSource().out) {
				op.GetSource().out(op.GetSource(), op);
			}

			op.RemoveTarget();

			return true;
		}
		else if(event == Input::Mouse::EventType::Move) {
			bool ret = true;

			if(move)
				ret = move(*this, op, curlocation);

			if(ret) {
				if(op.HasSource() && op.GetSource().move) {
					op.GetSource().move(op.GetSource(), op, curlocation);
				}
			}
			else { //if false, out event should occur
				propagate_mouseevent(Mouse::EventType::Out, location, button, amount, handlers);
			}

			return true;
		}
		else if(event == Input::Mouse::EventType::Up) {
			Drop(curlocation);

			return true;
		}
		else if(event == Input::Mouse::EventType::Down) {
			return true;
		}

		else if(event == Input::Mouse::EventType::HitCheck) {
			//don't call, window will decide which layers to call
			handlers.Add(this);

			return true;
		}

		//no other event can occur during drag
		return false;
	}

	void finishdrag(bool success) {
		if(!IsDragging()) return;

		auto &op = GetDragOperation();

		dragstarted = false;

		DragEnded(op, success);

		delete DragOperation;
		DragOperation = nullptr;
	}

	void Drop(Geometry::Point location) {
		if(!IsDragging()) return;

		auto &op = GetDragOperation();

		if(!op.HasTarget()) {
			CancelDrag();
			return;
		}

		bool ret = true;

		if(op.GetTarget().drop) {
			ret = op.GetTarget().drop(op.GetTarget(), op, location);
		}

		if(ret) {
			if(op.HasSource() && op.GetSource().accept) {
				op.GetSource().accept(op.GetSource(), op);
			}

			finishdrag(true);
		}
		else {
			CancelDrag();
		}
	}

	void CancelDrag() {
		if(!IsDragging()) return;

		auto &op = GetDragOperation();

		if(op.HasTarget() && op.GetTarget().cancel)
			op.GetTarget().cancel(op.GetTarget(), op);

		if(op.HasSource() && op.GetSource().cancel)
			op.GetSource().cancel(op.GetSource(), op);

		finishdrag(false);
	}

	DragInfo *DragOperation = nullptr;

	bool dragstarted = false;
    
	Event<void, DragInfo &> DragStarted;
    
	Event<void, DragInfo &, bool> DragEnded;
}
