#pragma once

#include "Inputbox.h"

namespace Gorgon :: Widgets {
    
    /// This inputbox variant is designed to contain text. Unlike regular Textbox, this
    /// variant allows overriding the validator
    template<class V_ = UI::ConversionValidator<std::string>>
    using TextInputbox = Inputbox<std::string, V_, TextualProperty>;
    
    /// An inputbox variant designed to edit text.
    using Textbox = Inputbox<std::string, UI::ConversionValidator<std::string>, TextualProperty>;
    
}
