#include "../OS.h"

#include <iomanip>

namespace Gorgon :: OS {
   
    void DumpFontFamilies(std::ostream &file) {
        file << std::left;
        file << std::setw(40) << "Family name" << " | ";
        file << std::setw(25) << "Style" << " | " 
             << std::setw(6)  << "Weight" << " | " 
             << std::setw(6)  << "Italic" << " | " 
             << std::setw(4)  << "Mono" << " | " 
             << std::setw(5)  << "Width" << " | " 
             << "Filename"
             << std::endl;
             
        file << "-----------------------------------------+---------------------------+-"
             << "-------+--------+------+-------+----------------------------------------------------------------------------"
             << std::endl;

        for(auto &fam : GetFontFamilies()) {
            file << std::setw(40) << fam.Family << " | ";
            bool first = true;
            for(auto &face : fam.Faces) {
                if(!first)
                    file << std::setw(40) << " " << " | ";

                file << std::setw(25) << face.Style << " | " 
                    << std::setw(6)  << face.Weight << " | " 
                    << std::setw(6)  << (face.Italic ? "Italic" : "") << " | " 
                    << std::setw(4)  << (face.Monospaced ? "Mono" : "") << " | " 
                    << std::setw(5)  << face.Width << " | " 
                    << face.Filename 
                    << std::endl;

                first = false;
            }
        }
    }
    
    FontWeight Font::GetWeight() const {
        if(Weight <= 150) {
            return FontWeight::Thin;
        }
        else if(Weight <= 250) {
            return FontWeight::ExtraLight;
        }
        else if(Weight <= 350) {
            return FontWeight::Light;
        }
        else if(Weight > 850) {
            return FontWeight::Heavy;
        }
        else if(Weight > 750) {
            return FontWeight::ExtraBold;
        }
        else if(Weight > 650) {
            return FontWeight::Bold;
        }
        else if(Weight > 550) {
            return FontWeight::SemiBold;
        }
        else if(Weight > 450) {
            return FontWeight::Medium;
        }
        else {
            return FontWeight::Regular;
        }
    }
}
