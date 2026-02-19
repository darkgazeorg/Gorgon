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
    
}
