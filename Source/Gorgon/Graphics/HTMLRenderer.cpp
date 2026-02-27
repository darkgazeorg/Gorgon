#include "HTMLRenderer.h"

#include <utility>
#include <vector>
#include <string>

namespace Gorgon :: Graphics {

    namespace HTMLRendererInternal {
    Utils::Logger Logger;
    }

    std::unordered_map<unsigned int, bool> HTMLRenderer::attsupportmap;
    std::unordered_map<HTMLRenderer::Tag, std::string, HTMLRenderer::HashType> HTMLRenderer::emptytagmap;
    bool HTMLRenderer::initialized = false;
        
    void HTMLRenderer::parseandprint(TextureTarget &target, const std::string &str, int x, int y) {
        this->target = &target;
        
        // clear previous state
        drawunderlined = false;
        drawstriked = false;
        underlinedstart = 0;
        strikedstart = 0;
        baselineoffset = 0;
        orgx = xx = x;
        orgy = yy = y;
        
        unsigned int offset = 0;
        bool intag = false;
        bool remove = false;
        bool inquote = false;
        bool parseattrbts = false;
        char current;
        std::string text, tag, attribute, attval;
        std::vector<std::pair<std::string, std::string>> attributes;
        Tag tagenum;
        
        for(std::size_t i = 0; i < str.length(); i++) {
            current = str[i];

            if(current == '<') {
                // if not inside a tag currently, and got some text accumulated, print them
                if(!intag && !text.empty()) {
                    HR_LOG_NOTICE("printing text: \"" + text + "\"");
                    print(target, text, xx, yy);
                    xx += offset;
                    offset = 0;
                    text.clear();
                }
                intag = true;
                continue;
            }
            else if(current == '>') {
                tagenum = string2tag(tag);
                // if it's an empty tag, like <br> or </br>, just apply and move one
                if(emptytagmap.count(tagenum)) {
                    HR_LOG_NOTICE("about to apply empty tag");
                    applytag(tagenum);
                    remove = false; // to handle empty tags with closing character
                }
                else if(remove) {
                    remove = false;
                    removetag(tagenum);
                    clearattributes(tagenum);
                }
                else {
                    parseattrbts = false;
                    if(!attribute.empty()) {
                        HR_LOG_NOTICE("extracting accumulated attribute string before style application");
                        attributes.emplace_back(std::pair<std::string, std::string>(attribute, attval));
                        attribute.clear();
                        attval.clear();
                    }
                    HR_LOG_NOTICE("attribute count: " + std::to_string(attributes.size()));
                    applytag(tagenum);
                    applyattributes(tagenum, attributes);
                    attributes.clear();
                }
                tag.clear();
                intag = false;
                continue;
            }
            else if(current == '/') {
                remove = true;
                continue;
            }
            else if(current == '\"' || current == '\'') {
                if(inquote) {
                    inquote = false;
                }
                else {
                    inquote = true;
                }
                continue;
            }
            
            if(intag) {
                // start parsing attributes once we encounter a space while parsing tag
                if(internal::isspace(current) && !parseattrbts) {
                    parseattrbts = true;
                    continue;
                }
                if(parseattrbts) {
                    // ignore assignment character
                    if(current == '=') {
                        continue;
                    }
                    // keep filling attribute vector as we encounter spaces while parsing attributes
                    else if(internal::isspace(current)) {
                        attributes.emplace_back(std::pair<std::string, std::string>(attribute, attval));
                        attribute.clear();
                        attval.clear();
                    }
                    // if we are inside quote, accumulate into attribute value string
                    // otherwise, use attirubte string
                    else {
                        if(inquote) {
                            attval += current;
                        }
                        else {
                            attribute += current;
                        }
                    }
                }
                else {
                    tag += current;
                }
            }
            else {
                auto start = str.begin() + i;
                int width = (int)std::round(renderer.GetGlyphRenderer().GetCursorAdvance(internal::decode(start, str.end())));
                offset += width;
                text += current;
            }
        }
        
        if(!text.empty()) {
            HR_LOG_NOTICE("about to print text");
            print(target, text, xx, yy);
        }
    }
        
} // end of namespace Graphics
 // end of namespace Gorgon
