#include "Color.h"
#include "../String.h"

namespace Gorgon :: Graphics {
    
    RGBA::operator std::string() const {
        std::stringstream str;
        str<<std::fixed<<std::setw(8)<<std::setfill('0')<<std::hex<<((const uint32_t)(*this));

        return str.str();
    }
    
    int hexchartoint(char c) {
        if(c >= '0' && c <= '9')
            return c-'0';
        else if(c >= 'A' && c <= 'F')
            return c-'A'+10;
        else if(c >= 'a' && c <= 'f')
            return c-'a'+10;
        
        return 0;
    }
    
    RGBA::RGBA(const std::string &c) {
        auto color = String::Trim(c);
        
        if(color.empty()) {
            (*this) = 0x0;
            return;
        }
        
        if(color[0] == '#') { //html color
            A = 255;

            switch(color.length()-1) {
            case 0:
                R = 0;
                G = 0;
                B = 0;
                break;
            case 1:
                R = hexchartoint(color[1]) * 17;
                G = hexchartoint(color[1]) * 17;
                B = hexchartoint(color[1]) * 17;
                break;           
            case 2:              
                R = hexchartoint(color[1]) * 16 + hexchartoint(color[2]);
                G = hexchartoint(color[1]) * 16 + hexchartoint(color[2]);
                B = hexchartoint(color[1]) * 16 + hexchartoint(color[2]);
                break;           
            case 3:              
                R = hexchartoint(color[1]) * 17;
                G = hexchartoint(color[2]) * 17;
                B = hexchartoint(color[3]) * 17;
                break;
            case 4:              
                R = hexchartoint(color[1]) * 16 + hexchartoint(color[2]);
                G = hexchartoint(color[3]) * 16 + hexchartoint(color[4]);
                B = 0;           
                break;
            case 5:              
                R = hexchartoint(color[1]) * 16 + hexchartoint(color[2]);
                G = hexchartoint(color[3]) * 16 + hexchartoint(color[4]);
                B = hexchartoint(color[5]) * 17;
                break;
            default:
                R = hexchartoint(color[1]) * 16 + hexchartoint(color[2]);
                G = hexchartoint(color[3]) * 16 + hexchartoint(color[4]);
                B = hexchartoint(color[5]) * 16 + hexchartoint(color[6]);
                break;
            case 7:
                R = hexchartoint(color[1]) * 16 + hexchartoint(color[2]);
                G = hexchartoint(color[3]) * 16 + hexchartoint(color[4]);
                B = hexchartoint(color[5]) * 16 + hexchartoint(color[6]);
                A = hexchartoint(color[7]) * 17;
                break;
            case 8:
                R = hexchartoint(color[1]) * 16 + hexchartoint(color[2]);
                G = hexchartoint(color[3]) * 16 + hexchartoint(color[4]);
                B = hexchartoint(color[5]) * 16 + hexchartoint(color[6]);
                A = hexchartoint(color[7]) * 16 + hexchartoint(color[8]);
                break;
            }
            
            return;
        }
        
        if(color[0] == '0') {
            if(color.length() > 1 && color[1] == 'x') {
                (*this) = String::HexTo<uint32_t>(color);
                return;
            }
        }
        
        //TODO parse r, g, b
        
        if(color.find_first_not_of("0123456789abcdefABCDEF") != color.npos) {
            (*this) = Color::GetNamedColor(String::ToLower(color));
            return;
        }
        
        try {
            (*this) = String::HexTo<uint32_t>(color);
        }
        catch(...) {
            (*this) = 0x0;
        }
        
        if(color.length() < 7)
            A = 255;
    }
    
    RGBAf::RGBAf(const std::string &c) {
        auto color = String::Trim(c);
        
        if(color.empty()) {
            (*this) = RGBA(0x0);
            return;
        }
        
        if(color[0] == '#') {
            (*this) = RGBA(c);
            return;
        }
        
        if(color[0] == '0') {
            if(color.length() > 1 && color[1] == 'x') {
                (*this) = RGBA(color);
                return;
            }
        }
        
        if(color[0] == '(') {
            color = color.substr(1);
        }
        
        if(color.find_first_of('.') != color.npos) { //float values
            if(color.find_first_of(',') == color.npos) { //single float value: grayscale
                try {
                    (*this) = String::To<float>(color);
                }
                catch(...) {
                    (*this) = RGBA(0x0);
                }
                
                return;
            }
            else {
                std::stringstream ss(color);
                
                R = 0;
                G = 0;
                B = 0;
                A = 1;
                
                try {
                    std::string str;
                    
                    if(std::getline(ss, str, ','))
                        R = String::To<float>(str);
                    if(std::getline(ss, str, ','))
                        G = String::To<float>(str);
                    if(std::getline(ss, str, ','))
                        B = String::To<float>(str);
                    if(std::getline(ss, str, ','))
                        A = String::To<float>(str);
                }
                catch(...) {
                    (*this) = RGBA(0x0);
                }
                
                return;
            }
        }
        
        (*this) = RGBA(color);
    }


    std::istream &operator>> (std::istream &in, RGBA &color) {
        while(isspace(in.peek()))
            in.ignore();
        
        std::string str;
        in >> str;
        
        color = RGBA{str};

        return in;
    }

    std::ostream &operator<< (std::ostream &stream, const RGBAf &color) {
        stream << "(" 
               << std::round(color.R*10000)/10000 << (std::round(color.R*10000) == 0 || std::round(color.R*10000) == 10000 ? ".0" : "") << ", " 
               << std::round(color.G*10000)/10000 << (std::round(color.G*10000) == 0 || std::round(color.G*10000) == 10000 ? ".0" : "") << ", " 
               << std::round(color.B*10000)/10000 << (std::round(color.B*10000) == 0 || std::round(color.B*10000) == 10000 ? ".0" : "") << ", " 
               << std::round(color.A*10000)/10000 << (std::round(color.A*10000) == 0 || std::round(color.A*10000) == 10000 ? ".0" : "") << ")";

        return stream;
    }

    std::string RGBA::HTMLColor()const {
        std::stringstream str;

        str << "#" << std::fixed << std::hex << std::setfill('0') 
            << std::setw(2) << (int)R << std::setw(2) << (int)G 
            << std::setw(2) << (int)B;
            
        if(A != 255)
            str << std::setw(2) << (int)A;

        return str.str();
    }

    RGBAf::operator std::string()const {
        std::stringstream ss;

        ss << "(" 
           << std::round(R*10000)/10000 << (std::round(R*10000) == 0 || std::round(R*10000) == 10000 ? ".0" : "") << ", " 
           << std::round(G*10000)/10000 << (std::round(G*10000) == 0 || std::round(G*10000) == 10000 ? ".0" : "") << ", " 
           << std::round(B*10000)/10000 << (std::round(B*10000) == 0 || std::round(B*10000) == 10000 ? ".0" : "") << ", " 
           << std::round(A*10000)/10000 << (std::round(A*10000) == 0 || std::round(A*10000) == 10000 ? ".0" : "") << ")";

        return ss.str();
    }



namespace Color {
    const std::vector<std::pair<std::string, Gorgon::Graphics::RGBA>> &Names() {
        static std::vector<std::pair<std::string, RGBA>> names = {
            {"Transparent", Transparent},
            {"White", White},
            {"Red", Red},
            {"Green", Green},
            {"Blue", Blue},
            {"Yellow", Yellow},
            {"Purple", Purple},
            {"Orange", Orange},
            {"Pink", Pink},
            {"Black", Black},
            {"Grey", Grey},
            {"Brown", Brown},
            {"Light Blue", LightBlue},
            {"Teal", Teal},
            {"Light Green", LightGreen},
            {"Magenta", Magenta},
            {"Sky Blue", SkyBlue},
            {"Semi Dark Grey", SemiDarkGrey},
            {"Lime Green", LimeGreen},
            {"Light Purple", LightPurple},
            {"Violet", Violet},
            {"Dark Green", DarkGreen},
            {"Turquoise", Turquoise},
            {"Lavender", Lavender},
            {"Dark Blue", DarkBlue},
            {"Tan", Tan},
            {"Cyan", Cyan},
            {"Aqua", Aqua},
            {"Forest Green", ForestGreen},
            {"Mauve", Mauve},
            {"Dark Purple", DarkPurple},
            {"Bright Green", BrightGreen},
            {"Maroon", Maroon},
            {"Olive", Olive},
            {"Salmon", Salmon},
            {"Beige", Beige},
            {"Royal Blue", RoyalBlue},
            {"Navy Blue", NavyBlue},
            {"Lilac", Lilac},
            {"Hot Pink", HotPink},
            {"Light Brown", LightBrown},
            {"Pale Green", PaleGreen},
            {"Peach", Peach},
            {"Olive Green", OliveGreen},
            {"Dark Pink", DarkPink},
            {"Periwinkle", Periwinkle},
            {"Sea Green", SeaGreen},
            {"Lime", Lime},
            {"Indigo", Indigo},
            {"Mustard", Mustard},
            {"Light Pink", LightPink},
            {"Rose", Rose},
            {"Bright Blue", BrightBlue},
            {"Neon Green", NeonGreen},
            {"Burnt Orange", BurntOrange},
            {"Aquamarine", Aquamarine},
            {"Navy", Navy},
            {"Grass Green", GrassGreen},
            {"Pale Blue", PaleBlue},
            {"Dark Red", DarkRed},
            {"Bright Purple", BrightPurple},
            {"Yellow Green", YellowGreen},
            {"Baby Blue", BabyBlue},
            {"Gold", Gold},
            {"Mint Green", MintGreen},
            {"Plum", Plum},
            {"Royal Purple", RoyalPurple},
            {"Brick Red", BrickRed},
            {"Dark Teal", DarkTeal},
            {"Burgundy", Burgundy},
            {"Khaki", Khaki},
            {"Blue Green", BlueGreen},
            {"Seafoam Green", SeafoamGreen},
            {"Pea Green", PeaGreen},
            {"Taupe", Taupe},
            {"Dark Brown", DarkBrown},
            {"Deep Purple", DeepPurple},
            {"Chartreuse", Chartreuse},
            {"Bright Pink", BrightPink},
            {"Light Orange", LightOrange},
            {"Mint", Mint},
            {"Pastel Green", PastelGreen},
            {"Sand", Sand},
            {"Dark Orange", DarkOrange},
            {"Spring Green", SpringGreen},
            {"Puce", Puce},
            {"Seafoam", Seafoam},
            {"Grey Blue", GreyBlue},
            {"Army Green", ArmyGreen},
            {"Dark Grey", DarkGrey},
            {"Dark Yellow", DarkYellow},
            {"Goldenrod", Goldenrod},
            {"Slate", Slate},
            {"Light Teal", LightTeal},
            {"Rust", Rust},
            {"Deep Blue", DeepBlue},
            {"Pale Pink", PalePink},
            {"Cerulean", Cerulean},
            {"Light Red", LightRed},
            {"Mustard Yellow", MustardYellow},
            {"Ochre", Ochre},
            {"Pale Yellow", PaleYellow},
            {"Crimson", Crimson},
            {"Fuchsia", Fuchsia},
            {"Hunter Green", HunterGreen},
            {"Blue Grey", BlueGrey},
            {"Slate Blue", SlateBlue},
            {"Pale Purple", PalePurple},
            {"Sea Blue", SeaBlue},
            {"Pinkish Purple", PinkishPurple},
            {"Light Grey", LightGrey},
            {"Leaf Green", LeafGreen},
            {"Light Yellow", LightYellow},
            {"Eggplant", Eggplant},
            {"Steel Blue", SteelBlue},
            {"Moss Green", MossGreen},
            {"Grey Green", GreyGreen},
            {"Sage", Sage},
            {"Brick", Brick},
            {"Burnt Sienna", BurntSienna},
            {"Reddish Brown", ReddishBrown},
            {"Cream", Cream},
            {"Coral", Coral},
            {"Ocean Blue", OceanBlue},
            {"Greenish", Greenish},
            {"Dark Magenta", DarkMagenta},
            {"Red Orange", RedOrange},
            {"Bluish Purple", BluishPurple},
            {"Midnight Blue", MidnightBlue},
            {"Light Violet", LightViolet},
            {"Dusty Rose", DustyRose},
            {"Greenish Yellow", GreenishYellow},
            {"Yellowish Green", YellowishGreen},
            {"Purplish Blue", PurplishBlue},
            {"Greyish Blue", GreyishBlue},
            {"Grape", Grape},
            {"Light Olive", LightOlive},
            {"Cornflower Blue", CornflowerBlue},
            {"Pinkish Red", PinkishRed},
            {"Bright Red", BrightRed},
            {"Azure", Azure},
            {"Blue Purple", BluePurple},
            {"Dark Turquoise", DarkTurquoise},
            {"Electric Blue", ElectricBlue},
            {"Off White", OffWhite},
            {"Powder Blue", PowderBlue},
            {"Wine", Wine},
            {"Dull Green", DullGreen},
            {"Apple Green", AppleGreen},
            {"Light Turquoise", LightTurquoise},
            {"Neon Purple", NeonPurple},
            {"Cobalt", Cobalt},
            {"Pinkish", Pinkish},
            {"Olive Drab", OliveDrab},
            {"Dark Cyan", DarkCyan},
            {"Purple Blue", PurpleBlue},
            {"Dark Violet", DarkViolet},
            {"Dark Lavender", DarkLavender},
            {"Forrest Green", ForrestGreen},
            {"Pale Orange", PaleOrange},
            {"Greenish Blue", GreenishBlue},
            {"Dark Tan", DarkTan},
            {"Green Blue", GreenBlue},
            {"Bluish Green", BluishGreen},
            {"Pastel Blue", PastelBlue},
            {"Moss", Moss},
            {"Grass", Grass},
            {"Deep Pink", DeepPink},
            {"Blood Red", BloodRed},
            {"Sage Green", SageGreen},
            {"Aqua Blue", AquaBlue},
            {"Terracotta", Terracotta},
            {"Pastel Purple", PastelPurple},
            {"Sienna", Sienna},
            {"Dark Olive", DarkOlive},
            {"Green Yellow", GreenYellow},
            {"Scarlet", Scarlet},
            {"Greyish Green", GreyishGreen},
            {"Chocolate", Chocolate},
            {"Blue Violet", BlueViolet},
            {"Baby Pink", BabyPink},
            {"Charcoal", Charcoal},
            {"Pine Green", PineGreen},
            {"Pumpkin", Pumpkin},
            {"Greenish Brown", GreenishBrown},
            {"Red Brown", RedBrown},
            {"Brownish Green", BrownishGreen},
            {"Tangerine", Tangerine},
            {"Salmon Pink", SalmonPink},
            {"Aqua Green", AquaGreen},
            {"Raspberry", Raspberry},
            {"Greyish Purple", GreyishPurple},
            {"Rose Pink", RosePink},
            {"Neon Pink", NeonPink},
            {"Cobalt Blue", CobaltBlue},
            {"Orange Brown", OrangeBrown},
            {"Deep Red", DeepRed},
            {"Orange Red", OrangeRed},
            {"Dirty Yellow", DirtyYellow},
            {"Orchid", Orchid},
            {"Reddish Pink", ReddishPink},
            {"Reddish Purple", ReddishPurple},
            {"Yellow Orange", YellowOrange},
            {"Light Cyan", LightCyan},
            {"Sky", Sky},
            {"Light Magenta", LightMagenta},
            {"Pale Red", PaleRed},
            {"Emerald", Emerald},
            {"Dark Beige", DarkBeige},
            {"Jade", Jade},
            {"Greenish Grey", GreenishGrey},
            {"Dark Salmon", DarkSalmon},
            {"Purplish Pink", PurplishPink},
            {"Dark Aqua", DarkAqua},
            {"Brownish Orange", BrownishOrange},
            {"Light Olive Green", LightOliveGreen},
            {"Light Aqua", LightAqua},
            {"Clay", Clay},
            {"Burnt Umber", BurntUmber},
            {"Dull Blue", DullBlue},
            {"Pale Brown", PaleBrown},
            {"Emerald Green", EmeraldGreen},
            {"Brownish", Brownish},
            {"Mud", Mud},
            {"Dark Rose", DarkRose},
            {"Brownish Red", BrownishRed},
            {"Pink Purple", PinkPurple},
            {"Pinky Purple", PinkyPurple},
            {"Camo Green", CamoGreen},
            {"Faded Green", FadedGreen},
            {"Dusty Pink", DustyPink},
            {"Purple Pink", PurplePink},
            {"Deep Green", DeepGreen},
            {"Reddish Orange", ReddishOrange},
            {"Mahogany", Mahogany},
            {"Aubergine", Aubergine},
            {"Dull Pink", DullPink},
            {"Evergreen", Evergreen},
            {"Dark Sky Blue", DarkSkyBlue},
            {"Ice Blue", IceBlue},
            {"Light Tan", LightTan},
            {"Dirty Green", DirtyGreen},
            {"Neon Blue", NeonBlue},
            {"Denim", Denim},
            {"Eggshell", Eggshell},
            {"Jungle Green", JungleGreen},
            {"Dark Peach", DarkPeach},
            {"Umber", Umber},
            {"Bright Yellow", BrightYellow},
            {"Dusty Blue", DustyBlue},
            {"Electric Green", ElectricGreen},
            {"Lighter Green", LighterGreen},
            {"Slate Grey", SlateGrey},
            {"Teal Green", TealGreen},
            {"Marine Blue", MarineBlue},
            {"Avocado", Avocado},
            {"Forest", Forest},
            {"Pea Soup", PeaSoup},
            {"Lemon", Lemon},
            {"Muddy Green", MuddyGreen},
            {"Marigold", Marigold},
            {"Ocean", Ocean},
            {"Light Mauve", LightMauve},
            {"Bordeaux", Bordeaux},
            {"Pistachio", Pistachio},
            {"Lemon Yellow", LemonYellow},
            {"Red Violet", RedViolet},
            {"Dusky Pink", DuskyPink},
            {"Dirt", Dirt},
            {"Pine", Pine},
            {"Vermillion", Vermillion},
            {"Amber", Amber},
            {"Silver", Silver},
            {"Coffee", Coffee},
            {"Sepia", Sepia},
            {"Faded Red", FadedRed},
            {"Canary Yellow", CanaryYellow},
            {"Cherry Red", CherryRed},
            {"Ocre", Ocre},
            {"Ivory", Ivory},
            {"Copper", Copper},
            {"Dark Lime", DarkLime},
            {"Strawberry", Strawberry},
            {"Dark Navy", DarkNavy},
            {"Cinnamon", Cinnamon},
            {"Cloudy Blue", CloudyBlue},
        };

        return names;
    }
    
    Gorgon::Graphics::RGBA GetNamedColor(std::string name) {
        static std::map<std::string, RGBA> named = {
            {"transparent", Transparent},
            {"white", White},
            {"red", Red},
            {"green", Green},
            {"blue", Blue},
            {"yellow", Yellow},
            {"purple", Purple},
            {"orange", Orange},
            {"pink", Pink},
            {"black", Black},
            {"grey", Grey},
            {"brown", Brown},
            {"lightblue", LightBlue},
            {"teal", Teal},
            {"lightgreen", LightGreen},
            {"magenta", Magenta},
            {"skyblue", SkyBlue},
            {"semidarkgrey", SemiDarkGrey},
            {"limegreen", LimeGreen},
            {"lightpurple", LightPurple},
            {"violet", Violet},
            {"darkgreen", DarkGreen},
            {"turquoise", Turquoise},
            {"lavender", Lavender},
            {"darkblue", DarkBlue},
            {"tan", Tan},
            {"cyan", Cyan},
            {"aqua", Aqua},
            {"forestfreen", ForestGreen},
            {"mauve", Mauve},
            {"darkpurple", DarkPurple},
            {"brightgreen", BrightGreen},
            {"maroon", Maroon},
            {"olive", Olive},
            {"salmon", Salmon},
            {"beige", Beige},
            {"royalblue", RoyalBlue},
            {"navyblue", NavyBlue},
            {"lilac", Lilac},
            {"hotpink", HotPink},
            {"lightbrown", LightBrown},
            {"palegreen", PaleGreen},
            {"peach", Peach},
            {"olivegreen", OliveGreen},
            {"darkpink", DarkPink},
            {"periwinkle", Periwinkle},
            {"seagreen", SeaGreen},
            {"lime", Lime},
            {"indigo", Indigo},
            {"mustard", Mustard},
            {"lightpink", LightPink},
            {"rose", Rose},
            {"brightblue", BrightBlue},
            {"neongreen", NeonGreen},
            {"burntorange", BurntOrange},
            {"aquamarine", Aquamarine},
            {"navy", Navy},
            {"grassgreen", GrassGreen},
            {"paleblue", PaleBlue},
            {"darkred", DarkRed},
            {"brightpurple", BrightPurple},
            {"yellowgreen", YellowGreen},
            {"babyblue", BabyBlue},
            {"gold", Gold},
            {"mintgreen", MintGreen},
            {"plum", Plum},
            {"royalpurple", RoyalPurple},
            {"brickred", BrickRed},
            {"darkteal", DarkTeal},
            {"burgundy", Burgundy},
            {"khaki", Khaki},
            {"bluegreen", BlueGreen},
            {"seafoamgreen", SeafoamGreen},
            {"peagreen", PeaGreen},
            {"taupe", Taupe},
            {"darkbrown", DarkBrown},
            {"deeppurple", DeepPurple},
            {"chartreuse", Chartreuse},
            {"brightpink", BrightPink},
            {"lightorange", LightOrange},
            {"mint", Mint},
            {"pastelgreen", PastelGreen},
            {"sand", Sand},
            {"darkorange", DarkOrange},
            {"springgreen", SpringGreen},
            {"puce", Puce},
            {"seafoam", Seafoam},
            {"greyblue", GreyBlue},
            {"armygreen", ArmyGreen},
            {"darkgrey", DarkGrey},
            {"darkyellow", DarkYellow},
            {"goldenrod", Goldenrod},
            {"slate", Slate},
            {"lightteal", LightTeal},
            {"rust", Rust},
            {"deepblue", DeepBlue},
            {"palepink", PalePink},
            {"cerulean", Cerulean},
            {"lightred", LightRed},
            {"mustardyellow", MustardYellow},
            {"ochre", Ochre},
            {"paleyellow", PaleYellow},
            {"crimson", Crimson},
            {"fuchsia", Fuchsia},
            {"huntergreen", HunterGreen},
            {"bluegrey", BlueGrey},
            {"slateblue", SlateBlue},
            {"palepurple", PalePurple},
            {"seablue", SeaBlue},
            {"pinkishpurple", PinkishPurple},
            {"lightgrey", LightGrey},
            {"leafgreen", LeafGreen},
            {"lightyellow", LightYellow},
            {"eggplant", Eggplant},
            {"steelblue", SteelBlue},
            {"mossgreen", MossGreen},
            {"greygreen", GreyGreen},
            {"sage", Sage},
            {"brick", Brick},
            {"burntsienna", BurntSienna},
            {"reddishbrown", ReddishBrown},
            {"cream", Cream},
            {"coral", Coral},
            {"oceanblue", OceanBlue},
            {"greenish", Greenish},
            {"darkmagenta", DarkMagenta},
            {"redorange", RedOrange},
            {"bluishpurple", BluishPurple},
            {"midnightblue", MidnightBlue},
            {"lightviolet", LightViolet},
            {"dustyrose", DustyRose},
            {"greenishyellow", GreenishYellow},
            {"yellowishgreen", YellowishGreen},
            {"purplishblue", PurplishBlue},
            {"greyishblue", GreyishBlue},
            {"grape", Grape},
            {"lightolive", LightOlive},
            {"cornflowerblue", CornflowerBlue},
            {"pinkishred", PinkishRed},
            {"brightred", BrightRed},
            {"azure", Azure},
            {"bluepurple", BluePurple},
            {"darkturquoise", DarkTurquoise},
            {"electricblue", ElectricBlue},
            {"offwhite", OffWhite},
            {"powderblue", PowderBlue},
            {"wine", Wine},
            {"dullgreen", DullGreen},
            {"applegreen", AppleGreen},
            {"lightturquoise", LightTurquoise},
            {"neonpurple", NeonPurple},
            {"cobalt", Cobalt},
            {"pinkish", Pinkish},
            {"olivedrab", OliveDrab},
            {"darkcyan", DarkCyan},
            {"purpleblue", PurpleBlue},
            {"darkviolet", DarkViolet},
            {"darklavender", DarkLavender},
            {"forrestgreen", ForrestGreen},
            {"paleorange", PaleOrange},
            {"greenishblue", GreenishBlue},
            {"darktan", DarkTan},
            {"greenblue", GreenBlue},
            {"bluishgreen", BluishGreen},
            {"pastelblue", PastelBlue},
            {"moss", Moss},
            {"grass", Grass},
            {"deeppink", DeepPink},
            {"bloodred", BloodRed},
            {"sagegreen", SageGreen},
            {"aquablue", AquaBlue},
            {"terracotta", Terracotta},
            {"pastelpurple", PastelPurple},
            {"sienna", Sienna},
            {"darkolive", DarkOlive},
            {"greenyellow", GreenYellow},
            {"scarlet", Scarlet},
            {"greyishgreen", GreyishGreen},
            {"chocolate", Chocolate},
            {"blueviolet", BlueViolet},
            {"babypink", BabyPink},
            {"charcoal", Charcoal},
            {"pinegreen", PineGreen},
            {"pumpkin", Pumpkin},
            {"greenishbrown", GreenishBrown},
            {"redbrown", RedBrown},
            {"brownishgreen", BrownishGreen},
            {"tangerine", Tangerine},
            {"salmonpink", SalmonPink},
            {"aquagreen", AquaGreen},
            {"raspberry", Raspberry},
            {"greyishpurple", GreyishPurple},
            {"rosepink", RosePink},
            {"neonpink", NeonPink},
            {"cobaltblue", CobaltBlue},
            {"orangebrown", OrangeBrown},
            {"deepred", DeepRed},
            {"orangered", OrangeRed},
            {"dirtyyellow", DirtyYellow},
            {"orchid", Orchid},
            {"reddishpink", ReddishPink},
            {"reddishpurple", ReddishPurple},
            {"yelloworange", YellowOrange},
            {"lightcyan", LightCyan},
            {"sky", Sky},
            {"lightmagenta", LightMagenta},
            {"palered", PaleRed},
            {"emerald", Emerald},
            {"darkbeige", DarkBeige},
            {"jade", Jade},
            {"greenishgrey", GreenishGrey},
            {"darksalmon", DarkSalmon},
            {"purplishpink", PurplishPink},
            {"darkaqua", DarkAqua},
            {"brownishorange", BrownishOrange},
            {"lightolivegreen", LightOliveGreen},
            {"lightaqua", LightAqua},
            {"clay", Clay},
            {"burntumber", BurntUmber},
            {"dullblue", DullBlue},
            {"palebrown", PaleBrown},
            {"emeraldgreen", EmeraldGreen},
            {"brownish", Brownish},
            {"mud", Mud},
            {"darkrose", DarkRose},
            {"brownishred", BrownishRed},
            {"pinkpurple", PinkPurple},
            {"pinkypurple", PinkyPurple},
            {"camogreen", CamoGreen},
            {"fadedgreen", FadedGreen},
            {"dustypink", DustyPink},
            {"purplepink", PurplePink},
            {"deepgreen", DeepGreen},
            {"reddishorange", ReddishOrange},
            {"mahogany", Mahogany},
            {"aubergine", Aubergine},
            {"dullpink", DullPink},
            {"evergreen", Evergreen},
            {"darkskyblue", DarkSkyBlue},
            {"iceblue", IceBlue},
            {"lighttan", LightTan},
            {"dirtygreen", DirtyGreen},
            {"neonblue", NeonBlue},
            {"denim", Denim},
            {"eggshell", Eggshell},
            {"junglegreen", JungleGreen},
            {"darkpeach", DarkPeach},
            {"umber", Umber},
            {"brightyellow", BrightYellow},
            {"dustyblue", DustyBlue},
            {"electricgreen", ElectricGreen},
            {"lightergreen", LighterGreen},
            {"slategrey", SlateGrey},
            {"tealgreen", TealGreen},
            {"marineblue", MarineBlue},
            {"avocado", Avocado},
            {"forest", Forest},
            {"peasoup", PeaSoup},
            {"lemon", Lemon},
            {"muddygreen", MuddyGreen},
            {"marigold", Marigold},
            {"ocean", Ocean},
            {"lightmauve", LightMauve},
            {"bordeaux", Bordeaux},
            {"pistachio", Pistachio},
            {"lemonyellow", LemonYellow},
            {"redviolet", RedViolet},
            {"duskypink", DuskyPink},
            {"dirt", Dirt},
            {"pine", Pine},
            {"vermillion", Vermillion},
            {"amber", Amber},
            {"silver", Silver},
            {"coffee", Coffee},
            {"sepia", Sepia},
            {"fadedred", FadedRed},
            {"canaryyellow", CanaryYellow},
            {"cherryred", CherryRed},
            {"ocre", Ocre},
            {"ivory", Ivory},
            {"copper", Copper},
            {"darklime", DarkLime},
            {"strawberry", Strawberry},
            {"darknavy", DarkNavy},
            {"cinnamon", Cinnamon},
            {"cloudyblue", CloudyBlue},
        };
        
        name.erase(
            std::remove(name.begin(), name.end(), ' '), 
            name.end()
        );

        return named.count(name) ? named[name] : RGBA(0x0);
    }
}


}
