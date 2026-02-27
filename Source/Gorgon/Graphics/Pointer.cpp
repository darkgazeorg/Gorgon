#include "Pointer.h"

namespace Gorgon :: Graphics {

    void PointerStack::Add(PointerType type, const Pointer &pointer) {
        ASSERT((int)type>(int)PointerType::None && (int)type<=(int)PointerType::Drag, "Invalid pointer type");
        
        if(pointers[(int)type].ptr && pointers[(int)type].owned) {
            delete pointers[(int)type].ptr;
        }
        
        pointers[(int)type].ptr   = &pointer;
        pointers[(int)type].owned = false;
    }
    
    void PointerStack::Assume(PointerType type, const Pointer &pointer) {
        Add(type, pointer);
        pointers[(int)type].owned = true;
    }
    
    void PointerStack::Add(PointerType type, const Drawable &image, Geometry::Point hotspot) {
        auto ptr = new DrawablePointer(image, hotspot);
        ptr->SetType(type);
        
        Add(type, *ptr);
        pointers[(int)type].owned = true;
    }

    PointerStack::Token PointerStack::Set(PointerType type) {
        ASSERT((int)type>(int)PointerType::None && (int)type<(int)PointerType::Max, "Invalid pointer type");
        
        stack.insert({lastind, {type, pointers[(int)type].ptr}});
        
        PointerChanged();
        
        return Token(this, lastind++);
    }
    
    PointerStack::Token PointerStack::Set(const Pointer &pointer) {
        stack.insert({lastind, {pointer.GetType(), &pointer}});
        
        PointerChanged();
        
        return Token(this, lastind++);
    }
    
    void PointerStack::Reset(Token &token) {
        if(token.parent != this) return;
        
        Graphics::PointerType curid = PointerType::None;
        
        if(!stack.empty()) {
            curid = stack.rbegin()->second.first;
        }
        
        stack.erase(token.ind);
        
        if(stack.empty() || stack.rbegin()->second.first != curid) {
            PointerChanged();
        }
        
        token.parent = nullptr;
        token.ind = 0;
    }
    
    const Pointer &PointerStack::Current() const {
        if(!stack.empty()) {
            return *stack.rbegin()->second.second;
        }
        else {
            for(int i=(int)PointerType::Arrow; i<(int)PointerType::Max; i++) {
                if(pointers[i].ptr)
                    return *pointers[i].ptr;
            }
        }
        
        throw std::runtime_error("No suitable pointer found.");
    }
    
    bool PointerStack::IsValid() const {
        if(!stack.empty()) {
            return true;
        }
        else {
            for(int i=(int)PointerType::Arrow; i<(int)PointerType::Max; i++) {
                if(pointers[i].ptr)
                    return true;
            }
        }
        
        return false;
    }
    
    PointerType PointerStack::GetCurrentType() const {
        if(!stack.empty()) {
            return stack.rbegin()->second.first;
        }
        else {
            return PointerType::None;
        }
    }
    
}
