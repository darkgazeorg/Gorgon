#pragma once

namespace Gorgon :: UI {
   
    namespace internal {
        template<class I_, class T_>
        struct prophelper {
            prophelper(I_ *obj) : obj(obj) {}

            ~prophelper() { 
                
            }
            
            void set_(const T_&v) {
                obj->set(v);
            }
            T_ get_() const {
                return obj->get();
            }

            I_ *obj;
        };
        
        inline std::map<ComponentTemplate::Tag, std::function<Widget *(const Template &)>> mergegenerators(const std::map<ComponentTemplate::Tag, std::function<Widget *(const Template &)>> &l, const std::map<ComponentTemplate::Tag, std::function<Widget *(const Template &)>> &r) {
            std::map<ComponentTemplate::Tag, std::function<Widget *(const Template &)>> gens;
            
            for(auto p : l) {
                gens.insert(p);
            }
            
            for(auto p : r) {
                gens[p.first] = p.second;
            }
            
            return gens;
        }
    }
    
}
