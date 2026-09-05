#include "String.h"
#include <stdexcept>

namespace Gorgon :: String {


std::string Extract_UseQuotes(std::string &original, char marker, QuoteType quotetype) {
    int inquotes=0;
    std::size_t pos=0;
    
    for(auto &c : original) {
        if(inquotes==1) {
            if(c=='\'') {
                inquotes=0;
            }
        }
        else if(inquotes==2) {
            if(c=='"') {
                inquotes=0;
            }
        }
        else if(c==marker) {
            std::string temp=original.substr(0, pos);
            original=original.substr(pos+1);
            
            return temp;
        }
        else if(c=='\'' && (quotetype==QuoteType::Single || quotetype==QuoteType::Both)) {
            inquotes=1;
        }
        else if(c=='"' && (quotetype==QuoteType::Double || quotetype==QuoteType::Both)) {
            inquotes=2;
        }
        
        pos++;
    }
    
    std::string temp;
    std::swap(temp, original);
    
    return temp;
}

/// This function will extract the part of the string until a given marker. Specified 
/// marker will be removed from the original string. If the marker is not found, entire
/// string will be extracted. This function will skip parentheses and quoted sections of 
/// the string. Multiple types of parentheses can be specified. Close and open should be
/// matched. Unbalanced parentheses will be silently ignored. Quotes will not be removed.
std::string Extract_UseParentheses(std::string &original, char marker, std::string open, std::string close, QuoteType quotetype) {
    if (open.size() != close.size()) {
        throw std::runtime_error("Extract_UseParentheses: open and close delimiters must have equal length");
    }

    int inquotes = 0;
    std::vector<char> parentheses;

    std::size_t pos = 0;

    for (auto &c : original) {
        if (inquotes == 1) {
            if (c == '\'') {
                inquotes = 0;
            }
        }
        else if (inquotes == 2) {
            if (c == '"') {
                inquotes = 0;
            }
        }
        else if (!parentheses.empty() && c == parentheses.back()) {
            parentheses.pop_back();
        }
        else if (open.find(c) != open.npos) {
            parentheses.push_back(close[open.find(c)]);
        }
        else if (c == marker && parentheses.empty()) {
            std::string temp = original.substr(0, pos);
            original = original.substr(pos + 1);

            return temp;
        }
        else if (c == '\'' && (quotetype == QuoteType::Single || quotetype == QuoteType::Both)) {
            inquotes = 1;
        }
        else if (c == '"' && (quotetype == QuoteType::Double || quotetype == QuoteType::Both)) {
            inquotes = 2;
        }

        pos++;
    }

    std::string temp;
    std::swap(temp, original);

    return temp;
}

}