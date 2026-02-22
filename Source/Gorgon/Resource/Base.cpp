#include "Base.h"
#include "Writer.h"


namespace Gorgon :: Resource {
	void Base::Resolve(File &file) {
		for(auto it=children.First(); it.IsValid(); ) {

			Base &r=*it;
			it.Next();

			r.Resolve(file);
		}
	}

	void Base::Prepare() {
		for(auto &child : *this) {
			child.Prepare();
		}
	}

	void Base::Discard() {
		for(auto &child : *this) {
			child.Discard();
		}
	}

	Base::Base() : Children(children) { }

	Base::~Base() {
		destroychildren();
	}

	bool Base::DeleteResource() {
		--refcount;
		if(refcount) return false;

		delete this;

		return true;
	}

	void Base::destroychildren() {
		for(auto it=children.First(); it.IsValid(); it.Next()) {
			if(!it->DeleteResource()) {
				// making sure that we don't leave any objects in
				// undetermined state as we are no longer its parent
				if(it->parent==this) {
					it->parent=nullptr;
					it->root=nullptr;
				}
			}
		}

		children.Clear();
	}
	void Base::Save(Writer &writer) {
		ASSERT(writer.IsGood(), "Writer is not ready for saving the resource");
		if(guid.IsEmpty())
			guid.New();
		
		save(writer);
	}
}
