#pragma once

#include <string>
#include "../String.h"

namespace Gorgon :: UI {
    
    /**
     * 
     * @page validators Validation
     * 
     * Validation in Gorgon UI is done over validator classes. These classes should have the
     * following members:
     * 
     * * Type: a type declaration of the type that is managed.
     * 
     * * bool IsValid(std::string): returns true if the given string is a valid value for the type
     * 
     * * bool AllowInsert(std::string start, std::string insert, std::string end): this function
     * is used when typing or pasting into the editor. If the `insert` is a valid string to place
     * between `start` and `end` this function should return true. The result of this function is
     * not IsValid(start + insert + end) as in this case, a partially correct data should be allowed.
     * Otherwise, it might be impossible to enter input. For instance, for Geometry::Point, entering
     * 5 when the inputbox is empty should be allowed, even though 5 is not a valid point.
     * 
     * * bool AllowErase(std::string before, int count, std::string after): this function is used 
     * check if it is possible to erase `count` characters from `before`. The text after the deletion
     * point is given in `after`. Having strong rules could cause difficulty in editing the data,
     * thus extensive use of this function is not recommended.
     * 
     * * Type From(std::string text): Converts the given text in to type. This function should not
     * throw even if the input is not valid. In this case it should return initial value for the
     * Type.
     * 
     * * std::string ToString(const Type &value): Converts the given value into a string. Input
     * validation requires a proper serialization as input systems may call ToString(From(text)) to
     * fix the text. It is advisable to have unit tests ensuring correct serialization.
     * 
     */
    
    /**
     * Accepts all input and tries to convert using in-library functions
     */
    template<class T_>
    class ConversionValidator {
    public:
        using Type = T_;
        
        /// Checks if the given string is valid
        bool IsValid(std::string) const {
            return true;
        }
        
        /// Checks if given string can be inserted between start and end
        bool AllowInsert(std::string /*start*/, std::string /*insert*/, std::string /*end*/) const {
            return true;
        }
        
        /// Checks if given number of characters can be erased from before.
        bool AllowErase(std::string /*before*/, int /*count*/, std::string /*after*/) const {
            return true;
        }
        
        /// Checks if given number of characters can be replace with insert at the end of before.
        bool AllowReplace(std::string /*before*/, int /*count*/, std::string /*insert*/, std::string /*after*/) const {
            return true;
        }
        
        /// Converts the given string to the type. If input is not valid, return initial value
        Type From(std::string text) const {
            return String::To<Type>(text);
        }
        
        /// Converts the given value to string.
        std::string ToString(const Type &value) const {
            return String::From(value);
        }
        
    };
    
    /**
     * This overload helps with displaying color
     */
    template<>
    class ConversionValidator<Graphics::RGBAf> {
    public:
        using Type = Graphics::RGBAf;
        
        enum DisplayType {
            Hex,
            RGBAf,
            HTML,
            //LCh
        };
        
        /// Checks if the given string is valid
        bool IsValid(std::string) const {
            return true;
        }
        
        /// Checks if given string can be inserted between start and end
        bool AllowInsert(std::string /*start*/, std::string /*insert*/, std::string /*end*/) const {
            return true;
        }
        
        /// Checks if given number of characters can be erased from before.
        bool AllowErase(std::string /*before*/, int /*count*/, std::string /*after*/) const {
            return true;
        }
        
        /// Checks if given number of characters can be replace with insert at the end of before.
        bool AllowReplace(std::string /*before*/, int /*count*/, std::string /*insert*/, std::string /*after*/) const {
            return true;
        }
        
        /// Converts the given string to the type. If input is not valid, return initial value
        Type From(std::string text) const {
            return String::To<Type>(text);
        }
        
        /// Converts the given value to string.
        std::string ToString(const Type &value) const {
            switch(Display) {
            case Hex:
                return String::From((Graphics::RGBA)value);
            case HTML:
                return Graphics::RGBA(value).HTMLColor();
            /*case LCh:
                return String::From(Graphics::LChAf(value));*/
            default:
                return String::From(value);
            }
        }
        
        DisplayType Display = HTML;
    };
}
