#pragma once

#include <string>
#include <vector>

namespace Gorgon :: String {
   
    /**
     * Represents a link in markdown text
     */
    struct MarkdownLink {
        int Index;
        std::string Text;
        std::string ID;
        std::string Destination;
        std::string Title;
    };
        
    /**
     * This function parses markdown formatted text to advanced text formating. Advanced text format
     * can be printed on the screen using AdvancedPrinter which can be provided by the widget 
     * registry: `Gorgon::Widgets::Registry::Active().Printer()`. This function is unicode aware. 
     * Not all markdown spec is supported. This is a very quick parser, performing a single pass
     * except for a few cases which require a single look forward. The following markdown features
     * are currently supported:
     * * Space and new line folding
     * * Block code segments using spacing (no fence support)
     * * Inline code
     * * ATX headers (up to level 4, level 4+ is considered as 4)
     * * Bullets using *, +, and -. Up to 4 levels is supported.
     * * Italic with a single * , bold with double **. Note: Unlike standard markdown, you can have
     * multiline bold and italic statements, terminating them at the end of lines require multiple
     * passes.
     * * Strike using ~~
     * * Line break without paragraph using double space or \ at the end
     * * Links: both inline and split links are supported. Titles are also supported but they must
     * be on a single line. Additionally, autolinks are supported using <>. To place literal < add 
     * a space after it.
     * 
     * Returns the advanced string with the links that are extracted. Link order is the same as
     * link regions that will be returned by AdvancedPrint function, allowing a clicked region to
     * be converted to link without much of a work.
     * 
     * Bullets do not work perfectly with every font, requiring tab stops.
     */
    std::pair<std::string, std::vector<MarkdownLink>> ParseMarkdown(const std::string &markdown, bool useinfofont = false);
    
}
